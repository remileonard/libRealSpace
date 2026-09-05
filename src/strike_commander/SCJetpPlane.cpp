//
//  SCJetpPlane.cpp
//  libRealSpace
//
//  Implementation de l'algorithme de vol reconstruit depuis STRIKE.EXE.
//  Chaque bloc renvoie a la section correspondante de
//  strike_commander_re/analysis/DATA_MODEL.md (§6.2) et PHYSICS.md.
//
//  Reperes :
//   - Monde  : X/Z horizontal, Y = altitude (convention deja etablie par SCPlane/RSArea).
//   - Corps (local, vx/vy/vz) : x = envergure (ASM c0), y = vertical/portance (ASM c2),
//     z = -nez (ASM c1) ; forward = {0,0,-1} donc "avancer" = vz < 0.
//
#include "precomp.h"
#include "../engine/gametimer.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr float G_SI = 9.797f;          // strike.asm:144603, dword_6FFD7 = -2508 (24.8) = -9.797 m/s^2
constexpr float RAD2DEG_57_29 = 57.29578f; // dword_707B2 = 0x394B = 180/pi ; angles d'ecoulement en degres
constexpr float HREF_M = 11000.0f;      // altitude de reference du lapse de poussee

// Zone morte du servo d'attitude : l'ASM compare l'erreur (en 24.8) contre 56 brut,
// soit 56/256 = 0.21875 degre. Valeur physique reelle en degres.
constexpr float ATTITUDE_DEAD_ZONE_DEG = 0.21875f;

// Coefficient du servo Aero_ComputeForcesMain (dword_70454, defaut seg112, = 1/dt_asm a 25 Hz).
// Sert de gain proportionnel fixe qui borne la loi racine pres de zero (target <= K*|err|).
constexpr float SERVO_K = 25.0f;
}

SCJetpPlane::SCJetpPlane() {
}

SCJetpPlane::SCJetpPlane(float LmaxDEF, float LminDEF, float Fmax, float Smax, float ELEVF_CSTE, float ROLLFF_CSTE,
                          float s, float W, float fuel_weight, float Mthrust, float b, float ie_pi_AR,
                          int MIN_LIFT_SPEED, RSArea *area, float x, float y, float z)
    : SCPlane(LmaxDEF, LminDEF, Fmax, Smax, ELEVF_CSTE, ROLLFF_CSTE, s, W, fuel_weight, Mthrust, b, ie_pi_AR,
              MIN_LIFT_SPEED, area, x, y, z) {
    // Valeurs de repli tant que object->entity->jdyn n'est pas encore attache par l'appelant
    // (loadFromEntity() les remplace par les vraies donnees JDYN/DYNM/THRS/STBL des que possible).
    this->mass_kg = W;
    this->thrust_max_n = Mthrust;
    this->fuel_capacity_kg = fuel_weight;
    this->fuel_kg = fuel_weight;
    this->fuel = (int)this->fuel_kg;
    this->inverse_mass = (this->mass_kg > 0.0f) ? (1.0f / this->mass_kg) : 0.0f;
    // position/velocity sont l'etat maitre depuis updatePosition() : il faut les amorcer avec les
    // coordonnees de spawn, sinon ils restent a (0,0,0) (sous le sol -> crash immediat au chargement).
    this->position = Vector3D(x, y, z);
    this->velocity = Vector3D(0.0f, 0.0f, 0.0f);
    this->ptw.Clear();
    this->incremental.Clear();
}

SCJetpPlane::~SCJetpPlane() {
}

void SCJetpPlane::loadFromEntity() {
    if (this->entity_loaded)
        return;
    if (this->object == nullptr || this->object->entity == nullptr)
        return;

    RSEntity *entity = this->object->entity;

    if (entity->weight_in_kg > 0)
        this->mass_kg = (float)entity->weight_in_kg;               // chunk DYNM
    if (entity->thrust_in_newton > 0)
        this->thrust_max_n = (float)entity->thrust_in_newton;      // chunk THRS
    if (entity->thrust_mil_fraction > 0.0f)
        this->thrust_mil_fraction = entity->thrust_mil_fraction;
    if (entity->thrust_ref_alt_fraction > 0.0f)
        this->thrust_ref_alt_fraction = entity->thrust_ref_alt_fraction;
    if (entity->thrust_cutoff_alt_raw > 0)
        this->thrust_cutoff_alt_m = entity->thrust_cutoff_alt_raw * 100.0f;
    if (entity->stability_gain > 0.0f)
        this->stability_gain = entity->stability_gain;             // chunk STBL

    JDYN *jdyn = entity->jdyn;
    if (jdyn != nullptr) {
        // Correspondance champ-a-champ verifiee cette session (28 champs, meme ordre disque) :
        // cf. DATA_MODEL.md §6.2 "Layout du chunk JDYN".
        this->fuel_capacity_kg = (float)jdyn->FUEL;
        this->sfc = jdyn->field_new_02;
        this->drag_airbrake = jdyn->boost_modifier_a;
        this->drag_gear = jdyn->boost_modifier_b;
        this->ground_moment_1 = jdyn->drag_modifier_a;
        this->ground_moment_2 = jdyn->drag_modifier_b;
        this->rate_limit_dps = jdyn->inertia_like;
        this->max_turn_rate_dps = jdyn->pitch_rate_gain;
        this->stall_alpha_deg = (float)jdyn->rate_threshold;        // PAS jdyn->MAX_G (role different, cf. PHYSICS.md)
        this->wing_incidence_deg = (float)jdyn->ctrl_modifier_a;
        this->flap_lift_increment_deg = (float)jdyn->ctrl_modifier_b;
        this->pitch_rate_limit_dps = (float)jdyn->envelope_pitch_limit; // champ 15, jamais charge avant
        this->ground_effect_ceiling_m = jdyn->min_airspeed;
        this->induced_drag_k = jdyn->drag_coefficient;
        this->lift_gain = jdyn->airframe_response_scale;
        this->yaw_authority = (float)jdyn->gouverne; // "gouverne" (champ 21) = gain palonnier->consigne lacet,
                                                      // cf. Aero_ResetAccumulatorFlags75Bit5, decode cette session
        this->pitch_stick_gain = (float)jdyn->aileron;   // champ 20 (jdyn[0x65]) = borne finale consigne tangage
        this->pitch_load_gain = (float)jdyn->MAX_G;      // champ 22 (jdyn[0x67]) = gain manche -> demande de charge
                                                          // cf. Aero_ComputeControlFlags75Bit5B, decode cette session
    }

    this->fuel_kg = this->fuel_capacity_kg;
    this->fuel = (int)this->fuel_kg;
    this->W = this->mass_kg;
    this->Mthrust = this->thrust_max_n;
    this->inverse_mass = (this->mass_kg > 0.0f) ? (1.0f / this->mass_kg) : 0.0f;
    this->entity_loaded = true;

    // Table de densite de l'air : AssetManager fournit le tableau d'octets, RSAirdens le parse.
    if (!this->airdens.isLoaded()) {
        TreEntry *airdensEntry = AssetManager::getInstance().GetEntryByName("..\\..\\DATA\\AIRDENS.TBL");
        if (airdensEntry != nullptr)
            this->airdens.initFromRam(airdensEntry->data, airdensEntry->size);
        printf("  airdens: %s (%zu bandes de 256 m)\n", this->airdens.isLoaded() ? "chargee" : "ABSENTE -> repli exp",
               (airdensEntry != nullptr) ? (size_t)(airdensEntry->size / 4) : (size_t)0);
    }

    // Log unique au chargement : verifie que les chunks DYNM/THRS/STBL/JDYN se sont bien charges
    // (sinon tous les gains ci-dessous restent a leurs valeurs par defaut du .h, souvent 0.0 - ce
    // qui effondre silencieusement l'autorite de tangage/portance a zero).
    printf("SCJetpPlane::loadFromEntity jdyn=%p mass_kg=%.2f thrust_max_n=%.2f stability_gain=%.4f\n",
           (void *)(this->object ? this->object->entity->jdyn : nullptr), this->mass_kg, this->thrust_max_n,
           this->stability_gain);
    printf("  wing_incidence_deg=%.2f flap_lift_increment_deg=%.2f stall_alpha_deg=%.2f\n",
           this->wing_incidence_deg, this->flap_lift_increment_deg, this->stall_alpha_deg);
    printf("  lift_gain=%.4f induced_drag_k=%.4f yaw_authority=%.2f\n", this->lift_gain, this->induced_drag_k,
           this->yaw_authority);
    printf("  pitch_stick_gain(jdyn65)=%.2f pitch_load_gain(jdyn67)=%.2f rate_limit_dps=%.2f max_turn_rate_dps=%.2f "
           "pitch_rate_limit_dps(champ15)=%.2f\n",
           this->pitch_stick_gain, this->pitch_load_gain, this->rate_limit_dps, this->max_turn_rate_dps,
           this->pitch_rate_limit_dps);
}

float SCJetpPlane::airDensity(float altitude_m) const {
    // Table DATA\AIRDENS.TBL (une bande par 256 m). RSAirdens fait le repli exponentiel
    // si le fichier n'a pas ete trouve. Cf. PHYSICS.md §6.
    return this->airdens.densityAt(altitude_m);
}

float SCJetpPlane::throttleNotchToThrustFraction(float notch) const {
    // Aero_ThrottleThrustCurve : cran 0-10 (MIL 0-5 / PC 5-10).
    if (notch <= 0.0f)
        return 0.0f;
    if (notch <= 5.0f)
        return (notch / 5.0f) * this->thrust_mil_fraction;
    return this->thrust_mil_fraction + (1.0f - this->thrust_mil_fraction) * (notch - 5.0f) / 5.0f;
}

float SCJetpPlane::altitudeLapseFraction(float altitude_m) const {
    if (altitude_m <= HREF_M)
        return 1.0f - (altitude_m / HREF_M) * (1.0f - this->thrust_ref_alt_fraction);
    float denom = this->thrust_cutoff_alt_m - HREF_M;
    if (denom <= 0.0f)
        return 0.0f;
    float f = this->thrust_ref_alt_fraction - this->thrust_ref_alt_fraction * (altitude_m - HREF_M) / denom;
    return f > 0.0f ? f : 0.0f;
}

void SCJetpPlane::computeGravity() {
    this->gravity = G_SI;
    this->gravity_force = this->mass_kg * G_SI;
}

void SCJetpPlane::computeThrust() {
    // this->thrust est le regime moteur expose par SCPlane (0-100). Conversion vers le
    // cran ASM 0-10 : correspondance non tracee dans l'assembleur, hypothese simple.
    float notch = this->thrust / 10.0f;
    float fraction = this->throttleNotchToThrustFraction(notch) * this->altitudeLapseFraction(this->y);
    this->thrust_force = this->thrust_max_n * fraction;
    if (this->fuel_kg <= 0.0f)
        this->thrust_force = 0.0f; // flameout, cf. jdyn[0x6D] <= 0 dans PhysicsTicks
    // Poussee purement longitudinale, axe corps -z (ASM c1, cf. en-tete de fichier).
    this->thrust_vector = Vector3D(0.0f, 0.0f, -this->thrust_force);
}

void SCJetpPlane::computeLift() {
    float V = sqrtf(this->vx * this->vx + this->vy * this->vy + this->vz * this->vz);
    if (V < 0.5f) {
        this->alpha_deg = this->beta_deg = this->alpha_eff_deg = 0.0f;
        this->ae = 0.0f;
        this->dynamic_pressure = 0.0f;
        this->lift_force = 0.0f;
        this->lift_vector = Vector3D(0.0f, 0.0f, 0.0f);
        this->side_vector = Vector3D(0.0f, 0.0f, 0.0f);
        this->wing_stall = 0;
        return;
    }

    // Angles d'ecoulement, repere corps. Aero_FlowAngle_AoA/Sideslip (DATA_MODEL.md §6.2).
    this->alpha_deg = -RAD2DEG_57_29 * (this->vy / V);
    this->beta_deg = -RAD2DEG_57_29 * (this->vx / V);

    // Incidence effective : calage d'aile toujours ajoute, volets si sortis (Aero_ComputeAoAWithTrim).
    this->alpha_eff_deg = this->alpha_deg + this->wing_incidence_deg;
    if (this->flaps > 0)
        this->alpha_eff_deg += this->flap_lift_increment_deg;

    if (this->alpha_eff_deg > this->stall_alpha_deg) {
        this->alpha_eff_deg = this->stall_alpha_deg;
        this->wing_stall = 1;
    } else if (this->alpha_eff_deg < -this->stall_alpha_deg) {
        this->alpha_eff_deg = -this->stall_alpha_deg;
        this->wing_stall = 1;
    } else {
        this->wing_stall = 0;
    }
    this->ae = this->alpha_eff_deg;

    this->dynamic_pressure = 0.5f * this->airDensity(this->y) * V * V;

    // Portance + force laterale : Aero_ComputeLiftAndSideForce (DATA_MODEL.md §6.2).
    float k_lift = this->lift_gain * this->dynamic_pressure;
    this->lift_force = k_lift * this->alpha_eff_deg;
    Vector3D dirLift(0.0f, -this->vz, this->vy); // perpendiculaire a la vitesse, plan vertical corps
    dirLift.Normalize();
    this->lift_vector = dirLift * this->lift_force;

    float k_side = 0.25f * this->lift_gain * this->dynamic_pressure;
    float sideForce = k_side * this->beta_deg;
    Vector3D dirSide(-this->vz, 0.0f, this->vx); // perpendiculaire a la vitesse, plan horizontal corps
    dirSide.Normalize();
    this->side_vector = dirSide * sideForce;

    this->Lmax = this->stall_alpha_deg;
    this->Lmin = -this->stall_alpha_deg;
}

void SCJetpPlane::computeDrag() {
    bool airbrakeOut = this->spoilers > 0;
    bool gearOut = this->wheels != 0;

    // Traînée : Aero_ComputeDragWithFeedback (DATA_MODEL.md §6.2).
    this->Cd = this->induced_drag_k * this->alpha_eff_deg * this->alpha_eff_deg +
               (this->induced_drag_k * 0.125f) * this->beta_deg * this->beta_deg +
               (1.0f + (airbrakeOut ? this->drag_airbrake : 0.0f) + (gearOut ? this->drag_gear : 0.0f));

    if (this->on_ground) {
        float groundTerm = this->ground_moment_1 + (airbrakeOut ? this->ground_moment_2 : 0.0f);
        float forwardSpeed = -this->vz; // ASM c1 (nez) = -vz local, cf. DATA_MODEL.md §6.2
        if (fabsf(forwardSpeed) < 1.0f)
            groundTerm *= forwardSpeed;
        this->Cd += groundTerm;
    }

    this->drag_force = this->dynamic_pressure * this->Cd;

    float V = sqrtf(this->vx * this->vx + this->vy * this->vy + this->vz * this->vz);
    if (V > 0.01f) {
        Vector3D dirDrag(this->vx / V, this->vy / V, this->vz / V);
        this->drag_vector = dirDrag * (-this->drag_force);
    } else {
        this->drag_vector = Vector3D(0.0f, 0.0f, 0.0f);
    }
}

void SCJetpPlane::updateForces() {
    this->acceleration.x = this->lift_vector.x + this->side_vector.x + this->drag_vector.x + this->thrust_vector.x;
    this->acceleration.y = this->lift_vector.y + this->side_vector.y + this->drag_vector.y + this->thrust_vector.y;
    this->acceleration.z = this->lift_vector.z + this->side_vector.z + this->drag_vector.z + this->thrust_vector.z;
}

void SCJetpPlane::updateAcceleration() {
    this->acceleration.x *= this->inverse_mass;
    this->acceleration.y *= this->inverse_mass;
    this->acceleration.z *= this->inverse_mass;

    // Gravite (monde) projetee en repere corps via ptw (deja reconstruite pour ce tic par
    // updatePosition(), qui s'execute avant dans Simulate() - simple lecture immediate, pas un
    // etat qu'on relit d'un tic sur l'autre), ajoutee APRES la division par la masse
    // (Aero_SumLinearForces_48639).
    this->gravity_vector = Vector3D(-this->ptw.v[0][1] * this->gravity, -this->ptw.v[1][1] * this->gravity,
                                     -this->ptw.v[2][1] * this->gravity);
    this->acceleration.x += this->gravity_vector.x;
    this->acceleration.y += this->gravity_vector.y;
    this->acceleration.z += this->gravity_vector.z;
}

void SCJetpPlane::updateVelocity() {
    float dt = GameTimer::getInstance().getDeltaTime();
    if (dt <= 0.0f)
        dt = 1.0f / 30.0f;

    // this->acceleration est en repere CORPS a ce stade. Contrainte sol appliquee dans ce repere,
    // comme dans PhysicsTicks (projection de la vitesse hors du plan sol).
    if (this->on_ground && this->status > MEXPLODE) {
        this->acceleration.x = 0.0f;
        float sinPitch = sinf(tenthOfDegreeToRad(this->pitch));
        float cosPitch = cosf(tenthOfDegreeToRad(this->pitch));
        if (cosPitch == 0.0f)
            cosPitch = 0.0001f;
        float floorVy = this->vz * sinPitch / cosPitch;
        if (this->vy + this->acceleration.y * dt < floorVy)
            this->acceleration.y = (floorVy - this->vy) / dt;
    }

    // Transform corps->monde pour la partie LINEAIRE (Physics_IntegratePosition, seg101).
    // IMPORTANT : on utilise EXACTEMENT la meme matrice `ptw` que updatePosition() pour le
    // transform inverse monde->corps. Reconstruire une matrice depuis les angles d'Euler
    // re-derives ferait diverger les deux transforms (la re-derivation n'est pas l'inverse exact
    // de la composition incrementale) -> l'aller-retour corps<->monde ne serait plus l'identite
    // -> vitesse qui s'emballe, traînee et G delirants (bug constate le 2026-09-05).
    Matrix rot = this->ptw;
    rot.v[3][0] = rot.v[3][1] = rot.v[3][2] = 0.0f; // partie rotation seule
    Vector3D worldAccel = this->acceleration.transformPoint(rot);
    this->velocity += worldAccel * dt;
}

void SCJetpPlane::updatePosition() {
    // ORIENTATION = matrice persistante tournee par petits increments (mecanisme ASM decode le
    // 2026-09-05, voir DATA_MODEL.md §PhysicsTicks / known_functions.json) :
    //   WorldObject_ComposeOrientation3Angles_3CAE3 (methode vtable des objets du monde, famille
    //   ~18 vtables) -> Matrix_BuildFullOrientation_575B2(objet+0x2C, &aX, &aY, &aZ)
    //     -> Matrix_BuildAxisX/Y/Z_56EC3 : chacun applique une rotation d'axe INCREMENTALE EN PLACE
    //        sur les lignes de la matrice d'orientation PERSISTANTE (row1' = row1*sin + row2*cos ;
    //        row2' = row2*sin - row1*cos), sans reconstruction from-scratch, no-op si |angle| <
    //        0x38 brut = 56/256 = 0.21875 deg.
    // Les 3 angles sont les INCREMENTS = vitesses angulaires physique * dt. Il n'y a donc PAS
    // d'accumulateur d'angle d'Euler cote jeu d'origine : l'etat d'orientation vit uniquement dans
    // la matrice. `pitch`/`yaw`/`roll` (SCPlane) sont ici RE-DERIVES de la matrice a chaque tic pour
    // le reste du moteur (HUD, rendu) ; ils ne sont l'entree que lorsqu'ils sont ecrits de
    // l'exterieur (heading de spawn) - detecte plus bas.
    float dt = GameTimer::getInstance().getDeltaTime();
    if (dt <= 0.0f)
        dt = 1.0f / 30.0f;

    this->last_px = this->x;
    this->last_py = this->y;
    this->last_pz = this->z;

    // --- Re-seed de la position si x/y/z ont ete ecrits de l'exterieur (autopilote, teleport,
    //     "set altitude", spawn...) ou au 1er tic. `position` (Vector3D) est l'etat maitre de
    //     l'integration ; on la resynchronise depuis x/y/z quand ils divergent. ---
    if (!this->position_seeded || this->x != this->m_seed_x || this->y != this->m_seed_y ||
        this->z != this->m_seed_z) {
        this->position = Vector3D(this->x, this->y, this->z);
        this->position_seeded = true;
    }

    // --- Integration lineaire (Physics_IntegratePosition) ---
    this->position += this->velocity * dt;

    // --- Re-seed de la matrice si un angle a ete ecrit de l'exterieur (spawn : heading) ou au 1er tic ---
    bool eulerDirty = !this->orientation_seeded || this->pitch != this->m_seed_pitch ||
                      this->yaw != this->m_seed_yaw || this->roll != this->m_seed_roll;
    if (eulerDirty) {
        Matrix seed;
        seed.Identity();
        seed.rotateM(tenthOfDegreeToRad(this->yaw), 0, 1, 0);
        seed.rotateM(tenthOfDegreeToRad(this->pitch), 1, 0, 0);
        seed.rotateM(tenthOfDegreeToRad(this->roll), 0, 0, 1);
        this->ptw = seed;
        this->orientation_seeded = true;
    }

    // --- Composition INCREMENTALE en repere avion (Matrix_BuildFullOrientation_575B2 : X puis Y puis Z) ---
    // pitch_speed/yaw_speed/roll_speed sont en deg/s (issus du servo aero, processInput). L'increment
    // de rotation de ce tic = vitesse_angulaire * dt, converti en radians. PAS de *10 : ce facteur
    // venait de l'ancien accumulateur d'Euler en dixiemes de degre, il n'a plus lieu d'etre ici.
    float dPitch = degreeToRad(this->pitch_speed * dt);
    float dYaw = degreeToRad(this->yaw_speed * dt);
    float dRoll = degreeToRad(this->roll_speed * dt);
    // NB : le seuil "0x38" (0.21875 deg) de Matrix_BuildAxis*_56EC3 est une micro-optim ASM
    // "skip si negligeable", calibree pour le tick DOS a taux fixe (~15-30 Hz). A framerate
    // variable/eleve, l'increment par tic passe sous ce seuil et l'avion ne tournerait jamais :
    // on ne le reproduit PAS ici.
    if (dPitch != 0.0f)
        this->ptw.rotateM(dPitch, 1, 0, 0);
    if (dYaw != 0.0f)
        this->ptw.rotateM(dYaw, 0, 1, 0);
    if (dRoll != 0.0f)
        this->ptw.rotateM(dRoll, 0, 0, 1);

    // --- Re-derivation des angles d'Euler depuis la matrice, pour le reste du moteur ---
    // Convention de build : R = Rz(roll) * Rx(pitch) * Ry(yaw) (ordre des rotateM ci-dessus).
    // Lignes de ptw = axes locaux X/Y/Z exprimes en monde (convention vecteur-ligne de Vector3D).
    Matrix &m = this->ptw;
    float sinPitch = std::clamp(-m.v[2][1], -1.0f, 1.0f);
    float newPitchDeg = radToDegree(asinf(sinPitch));
    float newYawDeg = radToDegree(atan2f(m.v[2][0], m.v[2][2]));
    float newRollDeg = radToDegree(atan2f(m.v[0][1], m.v[1][1]));

    this->m_old_pitch = this->pitch;
    this->m_old_yaw = this->yaw;
    this->pitch = newPitchDeg * 10.0f;
    this->yaw = norm3600(newYawDeg * 10.0f);
    this->roll = norm3600(newRollDeg * 10.0f);
    this->m_pitch_var = this->pitch - this->m_old_pitch;
    this->m_yaw_var = this->yaw - this->m_old_yaw;
    this->m_seed_pitch = this->pitch;
    this->m_seed_yaw = this->yaw;
    this->m_seed_roll = this->roll;

    // --- Sorties derivees pour le reste du moteur ---
    this->x = this->position.x;
    this->y = this->position.y;
    this->z = this->position.z;
    this->m_seed_x = this->x;   // memorise pour detecter une ecriture externe au prochain tic
    this->m_seed_y = this->y;
    this->m_seed_z = this->z;
    this->groundlevel = this->area->getY(this->x, this->z);

    this->ptw.SetTranslation(this->x, this->y, this->z);
    this->forward = Vector3D(0.0f, 0.0f, -1.0f).transformPoint(this->ptw);

    Matrix worldToBody = this->ptw;
    worldToBody.v[3][0] = worldToBody.v[3][1] = worldToBody.v[3][2] = 0.0f;
    worldToBody.Transpose();
    Vector3D bodyVelocity = this->velocity.transformPoint(worldToBody);
    this->vx = bodyVelocity.x;
    this->vy = bodyVelocity.y;
    this->vz = bodyVelocity.z;

    this->angular_velocity = Vector3D(-degreeToRad(this->pitch_speed), -degreeToRad(this->yaw_speed),
                                       -degreeToRad((float)this->roll_speed));
}

void SCJetpPlane::processInput() {
    float dt = GameTimer::getInstance().getDeltaTime();
    if (dt <= 0.0f)
        dt = 1.0f / 30.0f;

    // control_stick_x/y = offset en pixels, pleine deflexion = hauteur/largeur d'ecran / 2.5
    // (souris, SCStrike.cpp L1496/1500) ou +/-150 / +/-200 (clavier). L'ASM travaille avec un
    // manche normalise +/-1.0 (24.8 : [ctrl+0x1F] pleine butee +/-16.0, puis /16). On normalise
    // donc sur la MEME reference que la souris (hauteur/2.5 ~ hauteur/2 cote Remi) et on borne
    // +/-1 pour que le clavier (150) reste utilisable meme s'il sature un peu tot.
    float refY = 150.0f, refX = 200.0f;
    if (RSScreen::hasInstance()) {
        RSScreen &scr = RSScreen::instance();
        if (scr.height > 2) refY = scr.height / 2.5f;   // meme reference que la souris (SCStrike L1500)
        if (scr.width > 2) refX = scr.width / 2.5f;
    }
    // Signe conserve de l'implementation validee par Remi ("comportement des axes bon") :
    // elevator NEGATIF quand on tire le manche.
    this->elevator = std::clamp(-(this->control_stick_y / refY), -1.0f, 1.0f);
    this->rollers = std::clamp(-(this->control_stick_x / refX), -1.0f, 1.0f);

    float V = sqrtf(this->vx * this->vx + this->vy * this->vy + this->vz * this->vz);

    // q = pression dynamique (Aero_DynamicPressure). q' = q * stability_gain / 100 (servo).
    float q = this->dynamic_pressure;
    float qServo = q * this->stability_gain / 100.0f;
    if (qServo < 0.0f) qServo = 0.0f;

    // TANGAGE et LACET : l'ASM (Aero_ComputeControlFlags75Bit5B) ne gate PAS sur on_ground -
    // seulement sur flags_75.bit5 ou q trop faible (pas d'air). Il FAUT l'autorite de tangage au
    // sol pour cabrer au decollage. Le seul gate est "assez de pression dynamique".
    bool ctrlLive = (q > 1.0f);
    // ROULIS : Aero_ComputeControlFlags75Bit5C gate sur vitesse (V <= 0x2800/256 = 40 -> roulis nul)
    // ET pas d'autorite au sol.
    bool rollLive = (!this->on_ground && V > 40.0f);

    // ===================================================================================
    // TANGAGE : Aero_ComputeControlFlags75Bit5B (consigne d'alpha) + Aero_ComputeForcesMain (servo)
    // Transcription fidele du seg103 L1155-1529 + seg102.
    // Echelle manche : l'ASM fait `loadDemand = MAX_G * [ctrl+0x1F] / 16`. `[ctrl+0x1F]` est un
    // 24.8 fixe de pleine butee +/-16.0 (le code lit sa partie entiere par `sar eax,8`). Le `/16`
    // ramene donc le manche a +/-1.0 -> avec `elevator` deja normalise +/-1 ici, le `/16` ET
    // l'echelle manche s'annulent : loadDemand = MAX_G * elevator.
    // Signe : `elevator` < 0 quand on tire le manche -> demande de charge > 0 -> on utilise -elevator.
    // ===================================================================================
    float loadGain = this->pitch_load_gain;          // MAX_G (jdyn[0x67], octet brut = G)
    if (this->elevator > 0.0f)                        // pousser (nez bas) : autorite /3 (ASM : var_E < 0)
        loadGain /= 3.0f;
    float loadDemand = -this->elevator * loadGain;    // -elevator : tirer -> demande positive

    // var_30 = sin(angle(vecteur avant, Z monde)) ~ cos(tangage) ; signe inverse si sur le dos.
    float bankTerm = cosf(tenthOfDegreeToRad(this->pitch));
    if (this->ptw.v[1][1] < 0.0f)
        bankTerm = -bankTerm;

    float var38 = loadDemand + bankTerm;

    // incidencePerG = ((mass / q) / lift_gain) * 1.5 * g   (ASM : *0x180=1.5, *dword_6FFD7=g, puis neg)
    float incidencePerG = 0.0f;
    if (q > 0.0f && this->lift_gain != 0.0f)
        incidencePerG = ((this->mass_kg / q) / this->lift_gain) * 1.5f * G_SI;

    float baseline = -this->wing_incidence_deg;
    if (this->flaps > 0)
        baseline -= this->flap_lift_increment_deg;

    float boundA = 0.0f, boundB = 0.0f;
    if (var38 != 0.0f) {
        boundA = var38 * incidencePerG;      // ASM : var_2C = var_38 * incidencePerG (si var_38 != 0)
        boundB = bankTerm * incidencePerG;   // ASM : var_28 = var_30 * incidencePerG
    }
    boundA += baseline;                       // ASM loc_48B36 : var_2C += var_22 ; var_28 += var_22
    boundB += baseline;

    // ASM loc_48967..loc_48B42 (seg103 L1284-1518), retrace ligne par ligne.
    // var_1A NE part PAS de 0 : quand flags_75.bit4 est pose (defaut de A5620 = bit4|bit7) et
    // que l'avion est en vol, var_1A = alpha courant (scale par un terme geometrique
    // AI_ComputeGeometrySolution_57C67 si alpha<0 - non decode, approxime a 1 ici). C'est ca qui
    // rend le manche neutre STABLE : boundA==boundB en neutre -> le clamp laisse var_1A = alpha
    // -> err = alpha - alpha = 0 -> aucune action. (Avec var_1A=0 comme avant, err = -alpha ->
    // le servo poussait alpha vers 0 -> alpha_eff = calage_aile -> ~2.2G sans manche.)
    float pitchCommandDeg = this->alpha_deg;
    // Clamp loc_48B42 : ramene var_1A vers boundA SAUF si var_1A est deja dans l'intervalle
    // [0..boundB] (borne = boundA sinon boundB selon les tests jle/jg/jge exacts).
    {
        float v = pitchCommandDeg;
        bool keep;
        if (v <= boundB)
            keep = (boundA >= v) && (boundA <= boundB);
        else
            keep = (boundA <= v) && (boundA >= boundB);
        if (!keep)
            pitchCommandDeg = boundA;
    }
    // clamp final +/- pitch_stick_gain (jdyn[0x65])
    pitchCommandDeg = std::clamp(pitchCommandDeg, -this->pitch_stick_gain, this->pitch_stick_gain);

    // --- Servo Aero_ComputeForcesMain (seg102) + Physics_IntegrateSecondaryPosition ---
    // Relu octet-pres (seg102 L2440-2652, seg112 L999-1045) :
    //   target = +/- min( 2*sqrt(q'*|err|) , K*|err| )         K = dword_70454
    //   accel  = clamp( (target - rate) * K , +/- 3*q' )       [deg/s^2]
    //   rate  += accel * dt                                    dt = dword_70458
    // dword_70458 = (0x100<<8)/dword_70454 = 1.0 / dword_70454  ->  K = 1/dt_asm.
    //   Defaut dword_70454 = 0x1900 = 25.0 -> dt_asm = 1/25 s ; clampe [2.0, 25.0].
    // Comme K = 1/dt_asm, le terme (target-rate)*K*dt est "deadbeat" : sans le clamp la cible
    // serait atteinte en 1 tick. Le seul vrai limiteur est accel <= +/- 3*q'. Net et
    // framerate-independant :  rate += clamp( target - rate , +/- 3*q'*dt ).
    // Le K=25 reste dans le clamp de `target` (gain proportionnel fixe qui borne la loi sqrt
    // pres de zero) ; il est calibre pour le tick ASM 25 Hz, on le garde fixe (pas 1/dt_port).

    float errPitch = pitchCommandDeg - this->alpha_deg;
    float targetPitchRate = 0.0f;
    if (ctrlLive && fabsf(errPitch) >= ATTITUDE_DEAD_ZONE_DEG) {
        float sqrtLaw = 2.0f * sqrtf(qServo * fabsf(errPitch));
        targetPitchRate = copysignf(std::min(sqrtLaw, SERVO_K * fabsf(errPitch)), errPitch);
    }
    float maxPitchAccel = 3.0f * qServo * dt;   // 3*q' [deg/s^2] integre sur dt
    this->pitch_speed += std::clamp(targetPitchRate - this->pitch_speed, -maxPitchAccel, maxPitchAccel);
    this->pitch_speed = std::clamp(this->pitch_speed, -this->max_turn_rate_dps, this->max_turn_rate_dps);

    {
        float horizontalSpeed = sqrtf(this->velocity.x * this->velocity.x + this->velocity.z * this->velocity.z);
        float gammaDeg = RAD2DEG_57_29 * atan2f(this->velocity.y, horizontalSpeed);
        printf("[pitch2] og=%d aero=%d V=%.1f gl=%.2f y=%.2f x=%.1f z=%.1f status=%u mrate=%.2f target=%.3f "
               "boundA=%.2f boundB=%.2f\n",
               (int)this->on_ground, (int)ctrlLive, V, this->groundlevel, this->y, this->x, this->z, this->status,
               this->max_turn_rate_dps, targetPitchRate, boundA, boundB);
        printf("[pitch] elev=%.2f loadDem=%.2f iPG=%.3f var38=%.2f cmd=%.3f alpha=%.3f err=%.3f "
               "q=%.1f qS=%.3f rate=%.3f | pDeg=%.2f gDeg=%.2f alt=%.0f\n",
               this->elevator, loadDemand, incidencePerG, var38, pitchCommandDeg, this->alpha_deg, errPitch,
               q, qServo, this->pitch_speed, this->pitch / 10.0f, gammaDeg, this->y);
    }

    // ===================================================================================
    // LACET : servo fidele Aero_ComputeForcesMain, err = consigne_palonnier - beta.
    //   - consigne (Aero_ResetAccumulatorFlags75Bit5) = palonnier + coeff avion, PAS de beta.
    //   - beta (Aero_FlowAngle_Sideslip_46AB5) = -Ca * v_corps.c0 / |v| = -Ca * vx / V. C'EST
    //     dans l'ASM : c'est la stabilite de girouette (ramene le derapage vers 0).
    // Signe : le monde ASM est Z-up, le port Y-up avec c1/c2 echanges -> le sens de la rotation
    // de lacet est inverse. On NEGATE la sortie du servo pour retablir la chiralite (sinon
    // beta<0 -> nez tourne du mauvais cote -> derapage amplifie -> vrille divergente).
    // ===================================================================================
    float yawCommandDeg = this->rudder * this->yaw_authority;
    float errYaw = yawCommandDeg - this->beta_deg;
    float targetYawRate = 0.0f;
    if (ctrlLive && fabsf(errYaw) >= ATTITUDE_DEAD_ZONE_DEG) {
        float sqrtLaw = 2.0f * sqrtf(qServo * fabsf(errYaw));
        targetYawRate = copysignf(std::min(sqrtLaw, SERVO_K * fabsf(errYaw)), errYaw);
    }
    targetYawRate = -targetYawRate;   // chiralite Z-up (ASM) -> Y-up (port)
    float maxYawAccel = 3.0f * qServo * dt;
    this->yaw_speed += std::clamp(targetYawRate - this->yaw_speed, -maxYawAccel, maxYawAccel);
    this->yaw_speed = std::clamp(this->yaw_speed, -this->max_turn_rate_dps, this->max_turn_rate_dps);
    if (this->on_ground)
        this->yaw_speed = 0.0f;

    // ===================================================================================
    // ROULIS : Aero_ComputeControlFlags75Bit5C - loi directe manche, pas de couplage.
    // ===================================================================================
    float rollRateTarget = 0.0f;
    if (rollLive)
        rollRateTarget = this->rollers * this->max_turn_rate_dps;
    rollRateTarget = std::clamp(rollRateTarget, -this->max_turn_rate_dps, this->max_turn_rate_dps);

    float maxRollDelta = this->rate_limit_dps * dt;
    this->roll_speed += std::clamp(rollRateTarget - (float)this->roll_speed, -maxRollDelta, maxRollDelta);

    if (this->on_ground)
        this->roll_speed = 0;

    this->elevation_speedf = this->pitch_speed;
    this->azimuth_speedf = this->yaw_speed;
}

void SCJetpPlane::updateSpeedOfSound() {
    // Aucune correspondance ASM trouvee pour Mach/vitesse du son (PHYSICS.md §8) :
    // approximation standard, decorrelee du modele de force decode.
    float sos = 340.3f - 0.0038f * this->y;
    if (sos < 295.0f)
        sos = 295.0f;
    this->sos = sos;
    float V = sqrtf(this->vx * this->vx + this->vy * this->vy + this->vz * this->vz);
    this->mach = (this->sos > 0.0f) ? (V / this->sos) : 0.0f;
}

void SCJetpPlane::checkStatus() {
    if (this->y > 20000.0f)
        this->thrust_force = 0.0f;

    if (this->y > this->groundlevel + 1.0f) {
        if (!this->takeoff) {
            this->takeoff = true;
            this->landed = false;
        }
        this->on_ground = FALSE;
    } else if (this->y <= this->groundlevel + 0.5f) {
        if (this->object->alive == 0)
            this->status = MEXPLODE;

        if (this->isOnRunWay()) {
            if (!this->on_ground) {
                int rating = report_card(-this->climbspeed, this->roll, (int)this->vx,
                                          (int)(-this->vz * 1.944f), this->wheels);
                if (this->nocrash) {
                    if (rating == -1) {
                        this->status = MEXPLODE;
                    } else {
                        this->fuel_kg = std::min(this->fuel_kg + (float)rating, this->fuel_capacity_kg);
                        this->fuel = (int)this->fuel_kg;
                    }
                }
            } else if (this->nocrash == 0) {
                this->status = MEXPLODE;
            }
            if (fabsf(this->vz) < 5.0f && this->thrust < 20) {
                this->thrust = 0;
                this->vx = this->vy = this->vz = 0.0f;
                if (this->takeoff)
                    this->landed = true;
            }
        } else {
            this->velocity = Vector3D(0.0f, 0.0f, 0.0f);
            this->vx = this->vy = this->vz = 0.0f;
            this->crached = true;
            this->object->alive = 0;
        }
        this->on_ground = TRUE;
        if (this->status > MEXPLODE) {
            if (this->pitch < 0)
                this->pitch = 0;
            if (this->roll != 0)
                this->roll = 0;
        }
    }

    if (this->fuel_kg <= 0.0f) {
        this->fuel_kg = 0.0f;
        this->fuel = 0;
        this->thrust = 0;
    }
}

void SCJetpPlane::updatePlaneStatus() {
    float V = sqrtf(this->vx * this->vx + this->vy * this->vy + this->vz * this->vz);
    this->airspeed = (int)(V * 1.944f); // m/s -> noeuds, pour les instruments existants
    this->climbspeed = (short)this->vy; // m/s
    this->g_load = (this->lift_force * this->inverse_mass) / G_SI;
    this->ax = this->acceleration.x;
    this->ay = this->acceleration.y;
    this->az = this->acceleration.z;
}

void SCJetpPlane::Simulate() {
    if (!this->entity_loaded)
        this->loadFromEntity();

    float dt = GameTimer::getInstance().getDeltaTime();
    if (dt <= 0.0f)
        dt = 1.0f / 30.0f;
    this->tps = (uint32_t)(1.0f / dt + 0.5f);
    this->fps_knots = this->tps * (3600.0f / 6082.0f);

    if (this->chaff_timer > 0)
        this->chaff_timer -= dt;
    if (this->flare_timer > 0)
        this->flare_timer -= dt;

    this->groundlevel = this->area->getY(this->x, this->z);

    this->computeGravity();
    this->processInput();
    this->updatePosition();
    this->updateSpeedOfSound();
    this->checkStatus();
    this->computeLift();
    this->computeThrust();
    this->computeDrag();
    this->updateForces();
    this->updateAcceleration();
    this->updateVelocity();
    this->updatePlaneStatus();

    // Consommation carburant : facteur MIL/PC x SFC x cran x dt (PhysicsTicks, DATA_MODEL.md §6.2).
    float notch = this->thrust / 10.0f;
    float burnFactor = (notch <= 5.0f) ? 0.2f : 0.3f;
    float burn = burnFactor * this->sfc * fabsf(notch) * dt;
    this->fuel_kg -= burn;
    if (this->fuel_kg < 0.0f)
        this->fuel_kg = 0.0f;
    this->fuel = (int)this->fuel_kg;

    if (this->wheels) {
        this->wheel_anim--;
        if (this->wheel_anim == 0) {
            this->wheel_index++;
            if (this->wheel_index > 5)
                this->wheel_index = 5;
            this->wheel_anim = 10;
        }
    } else if (this->wheel_index >= 1) {
        this->wheel_anim--;
        if (this->wheel_anim == 0) {
            this->wheel_index--;
            if (this->wheel_index < 1)
                this->wheel_index = 0;
            this->wheel_anim = 10;
        }
    } else {
        this->wheel_index = 0;
    }

    for (auto sim_obj : this->weaps_object) {
        sim_obj->Simulate(this->tps);
    }
    // Purge des objets d'armement detruits (boucle explicite, pas de lambda).
    size_t writeIndex = 0;
    for (size_t readIndex = 0; readIndex < this->weaps_object.size(); ++readIndex) {
        if (this->weaps_object[readIndex]->alive) {
            this->weaps_object[writeIndex] = this->weaps_object[readIndex];
            ++writeIndex;
        }
    }
    this->weaps_object.resize(writeIndex);

    this->object->entity->position.x = this->x;
    this->object->entity->position.y = this->y;
    this->object->entity->position.z = this->z;

    const float rise_speed = 1.0f;
    if (this->object->alive == false) {
        Vector3D smoke_position(this->x, this->y, this->z);
        this->smoke_positions.insert(this->smoke_positions.begin(), smoke_position);
        if (this->smoke_anim_counters.size() < this->smoke_set->smoke_textures.size() + 10)
            this->smoke_anim_counters.insert(this->smoke_anim_counters.begin(), 0);
        if (this->smoke_positions.size() > this->smoke_set->smoke_textures.size() + 10) {
            this->smoke_positions.pop_back();
            if (this->smoke_anim_counters.size() > this->smoke_set->smoke_textures.size() + 10)
                this->smoke_anim_counters.pop_back();
        }
        if (this->crached) {
            for (auto &pos : this->smoke_positions) {
                pos.y += rise_speed;
            }
        }
    }

    this->tick_counter++;
    this->azimuthf = this->yaw;
    this->elevationf = this->pitch;
    this->twist = this->roll;
}
