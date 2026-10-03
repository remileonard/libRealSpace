#include "precomp.h"
#include "SCCameraSequence.h"

//
// Modèle de temps. L'original avance par tick de dword_70458/256 s (~25 fps) :
// champ += taux * dword_70458 / 256 par tick. Ramené "par seconde" (× nombre de
// ticks/s = 256/dword_70458), le facteur se simplifie à 1. Donc ici, une maj par
// frame : champ += taux * dt. Idem pour les compteurs (elapsed, a0) : ils sont
// exprimés en secondes (opérandes divisés par 256 au chargement), donc a0 -= dt,
// elapsed += dt. Aucun facteur 24.8 au runtime.
//

SCCameraSequence::SCCameraSequence() {
    this->view_reset();
}

void SCCameraSequence::view_reset() {
    this->position = Vector3D(0.0f, 0.0f, 0.0f);
    this->angles = Vector3D(0.0f, 0.0f, 0.0f);
    this->target_angles = Vector3D(0.0f, 0.0f, 0.0f);
    this->aim_point = Vector3D(0.0f, 0.0f, 0.0f);
    this->anchor_offset = Vector3D(0.0f, 0.0f, 0.0f);
    this->rel_position = Vector3D(0.0f, 0.0f, 0.0f);
    this->velocity = Vector3D(0.0f, 0.0f, 0.0f);
    this->accel = Vector3D(0.0f, 0.0f, 0.0f);
    this->rot_rate = Vector3D(0.0f, 0.0f, 0.0f);
    this->target_vec = Vector3D(0.0f, 0.0f, 0.0f);
    this->out_aim = Vector3D(0.0f, 0.0f, -1.0f);
    this->out_up = Vector3D(0.0f, 1.0f, 0.0f);
    this->a0 = 0.0f;
    this->elapsed = 0.0f;
    this->dist = 0.0f;
    this->div_a4 = 0.0f;
    this->div_a8 = 0.0f;
    this->div_ac = 0.0f;
    this->approach_p = 0.0f;
    this->approach_rem = 0.0f;
    this->ip = 0;
    this->mode = 0xFF;
    this->flag_recompute_anchor = false;
    this->flag_anchor_path = false;
    this->flag_startcam = false;
    this->aiming = false;
    this->handoff_view.clear();
    this->step_scale = 0.0f;
}

void SCCameraSequence::start(const RSCameraSequence *sequence, SCPlane *target) {
    this->view_reset();
    this->seq = sequence;
    this->bound_target = target;
    if (this->seq == nullptr) {
        this->status = Finished;
        return;
    }
    this->status = Running;
    if (s_debug) {
        SCCameraSequence::dumpProgram(*this->seq);
    }
    this->runOpcodesUntilArmed();
    this->refreshOutputs();
    this->dump("start");
}

// ---------------------------------------------------------------------------
//  Dump d'un programme décodé (statique : sert aussi à l'inventaire au
//  chargement, avant qu'une séquence ne démarre)
// ---------------------------------------------------------------------------

void SCCameraSequence::dumpProgram(const RSCameraSequence &seq) {
    printf("[COMP program] %s  (%zu instr, handoff='%s')\n", seq.name.c_str(), seq.program.size(), seq.handoffView.c_str());
    for (size_t k = 0; k < seq.program.size(); k++) {
        const COMPInstr &pi = seq.program[k];
        printf("    %2zu: %-26s argc=%d  fichier[%.3f, %.3f, %.3f, %.3f]  name='%s'\n",
               k, compOpName(pi.op).c_str(), pi.argc,
               pi.args[0], pi.args[1], pi.args[2], pi.args[3], pi.name.c_str());
        // Résolution repère : opérande fichier (a0,a1,a2) -> RealSpace
        // (x, y=altitude, z) = {a0, a2, -a1} ; angles = {pitch a0, yaw a2, roll -a1}.
        // argc==4 -> le premier opérande est un compteur, les 3 suivants le vecteur.
        if (pi.argc == 3) {
            printf("        -> realspace vec (x=%.1f, y_alt=%.1f, z=%.1f)   ou angles (pitch=%.1f, yaw=%.1f, roll=%.1f)\n",
                   pi.args[0], pi.args[2], -pi.args[1], pi.args[0], pi.args[2], -pi.args[1]);
        } else if (pi.argc == 4) {
            printf("        -> scalaire=%.1f  realspace vec (x=%.1f, y_alt=%.1f, z=%.1f)\n",
                   pi.args[0], pi.args[1], pi.args[3], -pi.args[2]);
        }
    }
}

// ---------------------------------------------------------------------------
//  Exécution du bytecode
// ---------------------------------------------------------------------------

void SCCameraSequence::runOpcodesUntilArmed() {
    while (this->ip < this->seq->program.size()) {
        const COMPInstr &instr = this->seq->program[this->ip];
        this->ip++;
        this->applyInstruction(instr);
        if (this->status == Finished) {
            return;
        }
        if (this->mode != 0xFF) {
            return;
        }
    }
    this->status = Finished;
}

void SCCameraSequence::applyInstruction(const COMPInstr &instr) {
    switch (instr.op) {
    case OP_IA_ARM_HOLD: {
        this->a0 = instr.args[0];
        this->mode = 1;
        break;
    }
    case OP_IA_SET_POS_ABS: {
        this->position = this->compToVec3(instr.args[0], instr.args[1], instr.args[2]);
        break;
    }
    case OP_IA_REBUILD_MATRIX: {
        this->angles = Vector3D(0.0f, 0.0f, 0.0f);
        break;
    }
    case OP_IA_ARM_TURN: {
        this->a0 = instr.args[0];
        this->target_angles = this->compAngles(instr.args[1], instr.args[2], instr.args[3]);
        this->mode = 4;
        break;
    }
    case OP_IA_SET_ROT_RATE: {
        this->rot_rate = this->compAngles(instr.args[0], instr.args[1], instr.args[2]);
        break;
    }
    case OP_IA_SET_VELOCITY: {
        this->velocity = this->compToVec3(instr.args[0], instr.args[1], instr.args[2]);
        break;
    }
    case OP_IA_BIND_ENTITY: {
        // L'entité est résolue par le directeur et passée à start(). Rien ici.
        break;
    }
    case OP_IA_SET_A0_MODE8: {
        this->a0 = instr.args[0];
        this->mode = 8;
        break;
    }
    case OP_IA_SET_DIST: {
        this->dist = instr.args[0];
        // loc_78C43 : si une entité est liée, active le chemin d'ancrage + recul.
        if (this->bound_target != nullptr) {
            this->flag_recompute_anchor = true;
            this->flag_anchor_path = true;
        }
        break;
    }
    case OP_IA_SET_REL_POS: {
        this->rel_position = this->compToVec3(instr.args[0], instr.args[1], instr.args[2]);
        break;
    }
    case OP_IA_SET_A0_MODE_B: {
        this->a0 = instr.args[0];
        this->mode = 0xB;
        break;
    }
    case OP_IA_SET_DIST_C: {
        this->dist = instr.args[0];
        this->flag_recompute_anchor = false;
        this->flag_anchor_path = true;
        break;
    }
    case OP_IA_DIV_SETUP: {
        // loc_78D3C. Opérandes déjà en réel (÷256 au chargement) : div_a4 = budget
        // de rapprochement (u), a0 = durée (s), div_a8 = valeur initiale. La forme
        // 24.8 de l'ASM se simplifie exactement en réel (tous les 256 s'annulent) :
        //   flag 0  : div_a8 = div_a4 / a0                       ; div_ac = 0
        //   flag !=0 : div_ac = 2*(div_a4 - div_a8*a0) / a0^2
        this->div_a4 = instr.args[0];
        this->a0 = instr.args[1];
        this->div_a8 = instr.args[2];
        if (instr.divFlag != 0) {
            this->div_ac = 2.0f * (this->div_a4 - this->div_a8 * this->a0);
            float aa = this->a0 * this->a0;
            if (aa != 0.0f) {
                this->div_ac = this->div_ac / aa;
            }
        } else {
            if (this->a0 != 0.0f) {
                this->div_a8 = this->div_a4 / this->a0;
            }
            this->div_ac = 0.0f;
        }
        break;
    }
    case OP_IA_SET_POS_REL_ENTITY: {
        Vector3D off = this->compToVec3(instr.args[0], instr.args[1], instr.args[2]);
        this->position = this->entityPos() + this->rotateByEntity(off);
        break;
    }
    case OP_IA_ARM_APPROACH_ANGLE: {
        this->approach_p = instr.args[0];
        this->approach_rem = instr.args[1];
        break;
    }
    case OP_IA_COMPUTE_GEOM: {
        if (this->bound_target != nullptr) {
            this->angles = this->entityAngles();
        }
        break;
    }
    case OP_IA_SET_ORIENT_AXES: {
        // ASM case 0x11 : Matrix_BuildAxisZ/X/Y composent la rotation SUR la
        // matrice d'orientation courante (lecture-recombinaison-réécriture), ils
        // ne la remplacent pas. Cette matrice a été semée au cap de l'entité par
        // OP_IA_COMPUTE_GEOM. Les angles du script sont donc RELATIFS au cap de
        // l'avion, pas absolus -> on cumule.
        this->angles = this->angles + this->compAngles(instr.args[0], instr.args[1], instr.args[2]);
        break;
    }
    case OP_IA_SET_ANCHOR_OFS: {
        this->anchor_offset = this->compToVec3(instr.args[0], instr.args[1], instr.args[2]);
        break;
    }
    case OP_IA_FLAG_FF: {
        this->flag_recompute_anchor = true;
        this->flag_anchor_path = false;
        break;
    }
    case OP_IA_VEL_FROM_ORIENT_ROW: {
        this->velocity = this->forwardFromAngles() * instr.args[0];
        break;
    }
    case OP_IA_SET_VEC_CC: {
        this->accel = this->compToVec3(instr.args[0], instr.args[1], instr.args[2]);
        break;
    }
    case OP_IA_ACCEL_FROM_ORIENT_ROW: {
        this->accel = this->forwardFromAngles() * instr.args[0];
        break;
    }
    case OP_IA_VEL_DIV: {
        if (instr.args[0] != 0.0f) {
            this->velocity = this->velocity * (1.0f / instr.args[0]);
        }
        break;
    }
    case OP_IA_VEL_FROM_ENTITY: {
        if (this->bound_target != nullptr) {
            this->velocity = this->entityVelocity();
        }
        break;
    }
    case OP_IA_ROT_VEL_BY_ENTITY: {
        if (this->bound_target != nullptr) {
            this->velocity = this->rotateByEntity(this->velocity);
        }
        break;
    }
    case OP_IA_ROT_VEC_CC_BY_ENTITY: {
        if (this->bound_target != nullptr) {
            this->accel = this->rotateByEntity(this->accel);
        }
        break;
    }
    case OP_IA_START_FLAG: {
        this->flag_startcam = true;
        break;
    }
    case OP_IA_DIST_TO_ENTITY: {
        if (this->bound_target != nullptr) {
            Vector3D d = this->entityPos() - this->position;
            this->dist = d.Length();
            this->flag_recompute_anchor = true;
            this->flag_anchor_path = true;
        }
        break;
    }
    case OP_IA_ARM_MOVE_SEGMENT: {
        this->a0 = instr.args[0];
        this->target_vec = this->compToVec3(instr.args[1], instr.args[2], instr.args[3]);
        if (this->bound_target != nullptr) {
            this->rel_position = this->entityPos() - this->position;
        }
        this->mode = 0x1D;
        break;
    }
    case OP_IA_END: {
        this->handoff_view = instr.name;
        this->status = Finished;
        break;
    }
    default: {
        // opcode inconnu : ignoré (le décodeur a déjà avancé le curseur)
        break;
    }
    }
}

// ---------------------------------------------------------------------------
//  Tick
// ---------------------------------------------------------------------------

bool SCCameraSequence::s_debug = false;

void SCCameraSequence::dump(const char *tag) const {
    if (!s_debug) {
        return;
    }
    Vector3D plane(0.0f, 0.0f, 0.0f);
    if (this->bound_target != nullptr) {
        plane = this->bound_target->position;
    }
    Vector3D off = this->position - plane;
    Vector3D copy = off;
    printf("[COMP %-10s] mode=%02X elapsed=%.2f a0=%.2f dist=%.2f\n"
           "    plane  = (%.3f, %.3f, %.3f)\n"
           "    angles = (%.3f, %.3f, %.3f)   (pitch,yaw,roll deg)\n"
           "    rel    = (%.3f, %.3f, %.3f)\n"
           "    campos = (%.3f, %.3f, %.3f)\n"
           "    aim    = (%.3f, %.3f, %.3f)\n"
           "    offset = (%.2f, %.2f, %.2f)   |offset|=%.2f\n",
           tag, this->mode, this->elapsed, this->a0, this->dist,
           plane.x, plane.y, plane.z,
           this->angles.x, this->angles.y, this->angles.z,
           this->rel_position.x, this->rel_position.y, this->rel_position.z,
           this->position.x, this->position.y, this->position.z,
           this->out_aim.x, this->out_aim.y, this->out_aim.z,
           off.x, off.y, off.z, copy.Length());
}

SCCameraSequence::Status SCCameraSequence::tick(float dt) {
    if (this->status != Running) {
        return this->status;
    }
    // Une maj par frame. step_scale = dt : facteur commun aux taux (deg/s, u/s) et
    // aux compteurs (déjà en secondes).
    this->step_scale = dt;
    uint8_t mode_before = this->mode;

    if (this->mode == 0xFF) {
        this->runOpcodesUntilArmed();
        this->refreshOutputs();
        return this->status;
    }

    if (this->mode == 1) {
        this->tickModeHold();
    } else if (this->mode == 4) {
        this->tickModeTurn();
    } else if (this->mode == 0xB) {
        this->tickModeAim();
    } else if (this->mode == 0x1D) {
        this->tickModeMoveSegment();
    }

    if (this->mode == 0xFF) {
        // le mode vient de se terminer : on reprend le flux d'opcodes cette frame
        this->runOpcodesUntilArmed();
        this->refreshOutputs();
        return this->status;
    }

    this->tickGenericIntegrator();

    if (this->a0 > 0.0f && this->mode != 1) {
        this->a0 = this->a0 - this->step_scale;
    }
    this->elapsed = this->elapsed + this->step_scale;

    this->refreshOutputs();

    if (s_debug) {
        static long f = 0;
        f = f + 1;
        printf("[COMP qui devrait pas] f=%ld dt=%.5f elapsed=%.4f yaw=%.4f dist=%.3f campos=(%.3f, %.3f, %.3f) aim=(%.3f, %.3f, %.3f)\n",
               f, dt, this->elapsed, this->angles.y, this->dist,
               this->position.x, this->position.y, this->position.z,
               this->out_aim.x, this->out_aim.y, this->out_aim.z);
        if (this->mode != mode_before) {
            printf("[COMP]   ^ mode %02X -> %02X\n", mode_before, this->mode);
        }
    }
    return this->status;
}

void SCCameraSequence::tickModeHold() {
    if (this->elapsed >= this->a0) {
        this->mode = 0xFF;
    }
}

void SCCameraSequence::tickModeTurn() {
    if (this->a0 <= 0.0f) {
        this->angles = this->target_angles;
        this->mode = 0xFF;
        return;
    }
    Vector3D diff = this->target_angles - this->angles;
    if (diff.Length() < 2.0f) {
        this->angles = this->target_angles;
        this->mode = 0xFF;
        return;
    }
    float t = this->step_scale / this->a0;
    this->angles = this->angles + diff * t;
}

void SCCameraSequence::tickModeAim() {
    if (this->bound_target == nullptr || this->a0 <= 0.0f) {
        this->aiming = false;
        this->mode = 0xFF;
        return;
    }
    this->aiming = true;
    this->aim_point = this->rel_position;
}

void SCCameraSequence::tickModeMoveSegment() {
    if (this->bound_target == nullptr || this->a0 <= 0.0f) {
        this->position = this->entityPos() + this->rel_position;
        this->mode = 0xFF;
        return;
    }
    Vector3D target_world = this->rotateByEntity(this->target_vec);
    Vector3D delta = target_world - this->rel_position;
    Vector3D step = delta * (this->step_scale / this->a0);
    this->rel_position = this->rel_position + step;
    this->position = this->entityPos() + this->rel_position;
}

void SCCameraSequence::tickGenericIntegrator() {
    float s = this->step_scale;

    if (this->velocity.Length() > 0.0f) {
        this->position = this->position + this->velocity * s;
    }
    if (this->accel.Length() > 0.0f) {
        this->velocity = this->velocity + this->accel * s;
    }
    if (this->rot_rate.Length() > 0.0f) {
        this->angles = this->angles + this->rot_rate * s;
    }

    if (this->flag_anchor_path && this->bound_target != nullptr) {
        // loc_798E6 : ré-ancrage relatif à l'entité
        if (this->flag_recompute_anchor) {
            this->rel_position = this->entityPos() + this->rotateByEntity(this->anchor_offset);
        }
        // loc_7998E : décompte OP_IA_DIV_SETUP -> grignote div_a4 et dist
        if (this->div_a4 > 0.0f && this->a0 > 0.0f) {
            this->div_a8 = this->div_a8 + this->div_ac * s;
            float delta = this->div_a8 * s;
            this->div_a4 = this->div_a4 - delta;
            this->dist = this->dist - delta;
        }
        // loc_79A18 : consommation de l'angle d'approche OP_IA_ARM_APPROACH_ANGLE
        if (this->approach_rem > 0.0f) {
            float a = this->approach_p * s;
            float abs_a = a < 0.0f ? -a : a;
            this->approach_rem = this->approach_rem - abs_a;
            this->angles.y = this->angles.y + a;   // accum sur l'axe Z ASM = lacet libRealSpace
        }
        // loc_79ABB : recul le long de l'axe de visée (ASM : ligne 0x2C = axe Y
        // horizontal ; natif = direction forward de la caméra). La caméra se
        // retrouve à `dist` de l'ancre, en la regardant. `dist` et la famille
        // div_* sont stockés en 24.8 brut (formule 0D fidèle) -> /256 ici, au
        // seul point d'usage en unités monde.
        this->position = this->rel_position - this->forwardFromAngles() * this->dist;
    } else if (this->flag_recompute_anchor && this->bound_target != nullptr) {
        // loc_79BBE : viser l'entité
        this->aiming = true;
        this->aim_point = this->entityPos();
    }
}

Vector3D SCCameraSequence::orientRow(int row) const {
    Matrix m;
    this->buildOrientation(m, this->angles);
    Vector3D base(0.0f, 0.0f, 0.0f);
    if (row == 0) {
        base.x = 1.0f;
    } else if (row == 1) {
        base.y = 1.0f;
    } else {
        base.z = 1.0f;
    }
    return base.transformPoint(m);
}

// ---------------------------------------------------------------------------
//  Sorties / repère
// ---------------------------------------------------------------------------

void SCCameraSequence::refreshOutputs() {
    if (this->aiming) {
        this->out_aim = this->aim_point;
        this->out_up = Vector3D(0.0f, 1.0f, 0.0f);
    } else {
        // Point de visée projeté loin devant : Camera::lookAt reconstruit la
        // direction par (position - out_aim). Aux coordonnées monde (~1e5), l'ULP
        // float32 vaut ~0.015 ; avec un point de visée à 1 unité la soustraction
        // perd ~2 % de précision -> la direction tremble d'une frame à l'autre.
        // À 10000 unités l'erreur relative retombe à ~1e-6.
        this->out_aim = this->position + this->forwardFromAngles() * 10000.0f;
        this->out_up = this->upFromAngles();
    }
}

Vector3D SCCameraSequence::compToVec3(float a0v, float a1v, float a2v) const {
    // Repère : opérande fichier (a0,a1,a2) -> libRealSpace (x, y=altitude, z).
    // 1:1, float, pas d'échelle. Signe de z à confirmer.
    Vector3D v;
    v.x = a0v;
    v.y = a2v;
    v.z = -a1v;
    return v;
}

Vector3D SCCameraSequence::compAngles(float o0, float o1, float o2) const {
    // Opérandes d'angle en degrés. Ordre ASM = (rot autour X, rot autour Y, rot
    // autour Z) ; axe Z de l'ASM = altitude -> lacet côté libRealSpace.
    // Même remap que compToVec3 : o0->pitch(x), o2->yaw(y), -o1->roll(z).
    Vector3D a;
    a.x = o0;   // pitch
    a.y = o2;   // yaw
    a.z = -o1;  // roll
    return a;
}

void SCCameraSequence::buildOrientation(Matrix &out, const Vector3D &angles_deg) const {
    // transformPoint = v.M, Multiply = pré-mult -> pour que le lacet soit la
    // rotation extérieure (tangage constant quel que soit le cap), appliquer
    // Y (yaw), puis X (pitch), puis Z (roll).
    out.Identity();
    out.rotateM(degreeToRad(angles_deg.y), 0.0f, 1.0f, 0.0f);
    out.rotateM(degreeToRad(angles_deg.x), 1.0f, 0.0f, 0.0f);
    out.rotateM(degreeToRad(angles_deg.z), 0.0f, 0.0f, 1.0f);
}

Vector3D SCCameraSequence::forwardFromAngles() const {
    Matrix r;
    this->buildOrientation(r, this->angles);
    Vector3D base(0.0f, 0.0f, -1.0f);
    return base.transformPoint(r);
}

Vector3D SCCameraSequence::upFromAngles() const {
    Matrix r;
    this->buildOrientation(r, this->angles);
    Vector3D base(0.0f, 1.0f, 0.0f);
    return base.transformPoint(r);
}

Vector3D SCCameraSequence::entityPos() const {
    if (this->bound_target == nullptr) {
        return Vector3D(0.0f, 0.0f, 0.0f);
    }
    return this->bound_target->position;
}

Vector3D SCCameraSequence::entityVelocity() const {
    if (this->bound_target == nullptr) {
        return Vector3D(0.0f, 0.0f, 0.0f);
    }
    return this->bound_target->velocity;
}

Vector3D SCCameraSequence::entityAngles() const {
    Vector3D deg(0.0f, 0.0f, 0.0f);
    if (this->bound_target != nullptr) {
        deg.x = this->bound_target->elevationf / 10.0f;
        deg.y = this->bound_target->azimuthf / 10.0f;
        deg.z = this->bound_target->twist / 10.0f;
    }
    return deg;
}

Vector3D SCCameraSequence::rotateByEntity(const Vector3D &v) const {
    if (this->bound_target == nullptr) {
        return v;
    }
    Matrix r;
    this->buildOrientation(r, this->entityAngles());
    Vector3D copy = v;
    return copy.transformPoint(r);
}

// ---------------------------------------------------------------------------
//  Accesseurs
// ---------------------------------------------------------------------------

SCCameraSequence::Status SCCameraSequence::getStatus() const {
    return this->status;
}

const Vector3D &SCCameraSequence::getPosition() const {
    return this->position;
}

const Vector3D &SCCameraSequence::getAimPoint() const {
    return this->out_aim;
}

const Vector3D &SCCameraSequence::getUp() const {
    return this->out_up;
}

const std::string &SCCameraSequence::getHandoffView() const {
    return this->handoff_view;
}
