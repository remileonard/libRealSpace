#include "precomp.h"
#include "SCOrbitCamera.h"

// ASM (CAMERA_SYSTEM.md §4bis) : Camera_OrbitReset_85878 pose la distance
// d'orbite depuis la taille du sujet (recul = taille*2, bornes taille*1..*4,
// matrice d'orbite = identité) ; Camera_OrbitTrackCompute_1493A (loc_1493A)
// fait ensuite, par frame : cible = sujet.pos - distance*(orbite*axe) ;
// pos_cam += (cible - pos_cam) / 4 ; la caméra regarde le sujet. Le "recul"
// n'est pas une constante fichier : il n'existe aucune valeur de ce type
// dans les chunks CAMR CHAS/ROTA/TARG (confirmé sur les dumps
// data/ALASKA.WRLD.CAMR.{CHAS,ROTA,TARG}.DAT, slot look-at à zéro) ni dans
// REAL/OBJT/INFO (confirmé vide par Rémi) — c'est une géométrie calculée
// depuis le modèle, d'où boundingSize() ci-dessous. fov/farClip/nearClip/
// viewport SONT en revanche des données fichier réelles : def porte le
// RSCameraDef complet (cf. definition()) — à consommer côté rendu via le
// même chemin de mise à l'échelle résolution-indépendant que
// Renderer::bindCameraProjectionAndViewViewport (SCStrike.cpp), PAS en
// mappant viewX/Y/W/H (relatifs à la résolution d'origine 320x200)
// directement sur des pixels d'écran.
//
// Le sujet est n'importe quel SCMissionActors (avion, bateau, bâtiment —
// une cible verrouillée F7 peut être n'importe quel RSEntity, pas
// seulement un SCPlane). Position/forward/bounding-box sont résolus
// génériquement depuis l'acteur ci-dessous : `object` (MISN_PART*, position
// + azymuth/pitch/roll + entity) existe pour TOUT acteur, `plane` seulement
// pour un avion — quand il est présent on l'utilise (position/forward déjà
// intégrés par la physique, plus précis), sinon on retombe sur `object`.
//
// Cette classe ne connaît QUE le sujet (recul basé sur sa taille, lookAt sur
// lui-même) : aimPosition()/backwardAxis() sont les deux points d'extension
// pour une caméra qui vise/recule différemment (cf. SCTargetCamera) — pas de
// branche `if` ici, la classe de base reste fermée.

static float boundingSize(RSEntity *entity) {
    if (entity == nullptr) {
        return 30.0f;
    }
    BoudingBox *bb = entity->GetBoudingBpx();
    if (bb == nullptr) {
        return 30.0f;
    }
    float sizeX = bb->max.x - bb->min.x;
    float sizeY = bb->max.y - bb->min.y;
    float sizeZ = bb->max.z - bb->min.z;
    return sqrt(sizeX * sizeX + sizeY * sizeY + sizeZ * sizeZ);
}

RSEntity *SCOrbitCamera::actorEntity(SCMissionActors *actor) {
    if (actor == nullptr || actor->object == nullptr) {
        return nullptr;
    }
    return actor->object->entity;
}

Vector3D SCOrbitCamera::actorPosition(SCMissionActors *actor) {
    if (actor->plane != nullptr) {
        return Vector3D(actor->plane->x, actor->plane->y, actor->plane->z);
    }
    if (actor->object != nullptr) {
        return actor->object->position;
    }
    return Vector3D(0.0f, 0.0f, 0.0f);
}

// Vecteur avant générique : `plane->forward` (intégré par la physique)
// quand l'acteur est un avion, sinon reconstruit depuis l'orientation
// placée du MISN_PART (azymuth/pitch/roll, en degrés), avec la même
// convention d'axes que SCCameraSequence::buildOrientation/forwardFromAngles
// (angles fichier : yaw puis pitch puis roll, base (0,0,-1)).
Vector3D SCOrbitCamera::actorForward(SCMissionActors *actor) {
    if (actor->plane != nullptr) {
        return actor->plane->forward;
    }
    if (actor->object == nullptr) {
        return Vector3D(0.0f, 0.0f, -1.0f);
    }
    Matrix orientation;
    orientation.Identity();
    orientation.rotateM(degreeToRad((float)actor->object->azymuth), 0.0f, 1.0f, 0.0f);
    orientation.rotateM(degreeToRad((float)actor->object->pitch), 1.0f, 0.0f, 0.0f);
    orientation.rotateM(degreeToRad((float)actor->object->roll), 0.0f, 0.0f, 1.0f);
    Vector3D base(0.0f, 0.0f, -1.0f);
    return base.transformPoint(orientation);
}

SCOrbitCamera::SCOrbitCamera(const RSCameraDef *def) : def(def) {
}

RSCameraType SCOrbitCamera::typeCode() const {
    return this->def->typeCode;
}

const std::string &SCOrbitCamera::name() const {
    return this->def->name;
}

const std::string &SCOrbitCamera::subjectName() const {
    return this->def->subject;
}

float SCOrbitCamera::fov() const {
    return this->def->fov;
}

const RSCameraDef &SCOrbitCamera::definition() const {
    return *this->def;
}

Vector3D SCOrbitCamera::aimPosition(const Vector3D &subjectPos) const {
    return subjectPos;
}

Vector3D SCOrbitCamera::backwardAxis(const Vector3D &subjectPos) const {
    (void)subjectPos;
    return SCOrbitCamera::actorForward(this->subject).transformPoint(this->orbit);
}

void SCOrbitCamera::activate(SCMissionActors *subject, SCMissionActors *target) {
    (void)target;   // ignoré par la classe de base — cf. commentaire de tête de fichier
    this->subject = subject;
    this->orbit.Identity();
    if (subject == nullptr) {
        if (this->debugEnabled()) {
            printf("%s activate name='%s' subject=NULL -> subject reste NULL, rien d'autre initialisé\n",
                   this->debugLabel(), this->def->name.c_str());
        }
        return;
    }
    float size = boundingSize(SCOrbitCamera::actorEntity(subject));
    this->dist = size * 2.0f;
    Vector3D subjectPos = SCOrbitCamera::actorPosition(subject);
    Vector3D axis = this->backwardAxis(subjectPos);
    // Pas de recul initial "à vide" (pas de swoop-in à l'activation) : on
    // démarre déjà sur la position idéale, le lissage ne joue qu'ensuite.
    this->cam_pos = subjectPos - axis * this->dist;
    if (this->debugEnabled()) {
        printf("%s activate name='%s' fov=%.3f view=(%u,%u,%u,%u) subject=%p entity=%p size=%.3f dist=%.3f subjPos=(%.3f,%.3f,%.3f) axis=(%.3f,%.3f,%.3f) camPos0=(%.3f,%.3f,%.3f)\n",
               this->debugLabel(), this->def->name.c_str(), this->def->fov,
               this->def->viewX, this->def->viewY, this->def->viewW, this->def->viewH,
               (void *)subject, (void *)SCOrbitCamera::actorEntity(subject), size, this->dist,
               subjectPos.x, subjectPos.y, subjectPos.z,
               axis.x, axis.y, axis.z,
               this->cam_pos.x, this->cam_pos.y, this->cam_pos.z);
    }
}

void SCOrbitCamera::tick(float dt, Vector3D &out_pos, Vector3D &out_aim, Vector3D &out_up) {
    if (this->subject == nullptr) {
        if (this->debugEnabled()) {
            printf("%s tick subject=NULL -> return sans toucher out_pos/out_aim\n", this->debugLabel());
        }
        return;
    }
    Vector3D subjectPos = SCOrbitCamera::actorPosition(this->subject);

    float size    = boundingSize(SCOrbitCamera::actorEntity(this->subject));
    float distMin = size * 1.0f;
    float distMax = size * 4.0f;
    if (this->dist <= 0.0f) {
        this->dist = size * 2.0f;
    }
    this->dist = std::clamp(this->dist, distMin, distMax);

    // `orbit` = identité pour CHASE/TARGET ; tournée par ROTA avant l'appel
    // à cette méthode (cf. SCRotaCamera::tick).
    Vector3D axis       = this->backwardAxis(subjectPos);
    Vector3D desiredPos = subjectPos - axis * this->dist;

    // Lissage ASM : pos += (cible - pos) / 4 PAR FRAME. Mis à l'échelle du dt
    // réel (référence 25 fps, cf. CAMERA_SYSTEM.md §6.3) pour rester correct
    // à tout framerate plutôt que de dépendre du taux de MissionUpdateEvent.
    float alpha = std::clamp((dt * 25.0f) / 4.0f, 0.0f, 1.0f);
    Vector3D camPosBefore = this->cam_pos;
    this->cam_pos = this->cam_pos + (desiredPos - this->cam_pos) * alpha;

    Vector3D aimPos = this->aimPosition(subjectPos);
    Vector3D lookDir = aimPos - this->cam_pos;
    if (lookDir.Length() < 0.001f) {
        lookDir = axis * -1.0f;
    }
    lookDir.Normalize();

    out_pos = this->cam_pos;
    // Point de visée projeté loin devant : évite l'annulation catastrophique
    // float32 de Camera::lookAt aux coordonnées monde ~1e5 (§6.4).
    out_aim = this->cam_pos + lookDir * 10000.0f;
    out_up  = Vector3D(0.0f, 1.0f, 0.0f);

    if (this->debugEnabled()) {
        static long frame = 0;
        frame = frame + 1;
        printf("%s f=%ld dt=%.5f alpha=%.5f subjPos=(%.3f,%.3f,%.3f) axis=(%.3f,%.3f,%.3f) dist=%.3f desiredPos=(%.3f,%.3f,%.3f) camPos %.3f,%.3f,%.3f -> %.3f,%.3f,%.3f aimPos=(%.3f,%.3f,%.3f) out_aim=(%.3f,%.3f,%.3f)\n",
               this->debugLabel(), frame, dt, alpha,
               subjectPos.x, subjectPos.y, subjectPos.z,
               axis.x, axis.y, axis.z,
               this->dist,
               desiredPos.x, desiredPos.y, desiredPos.z,
               camPosBefore.x, camPosBefore.y, camPosBefore.z,
               this->cam_pos.x, this->cam_pos.y, this->cam_pos.z,
               aimPos.x, aimPos.y, aimPos.z,
               out_aim.x, out_aim.y, out_aim.z);
    }
}
