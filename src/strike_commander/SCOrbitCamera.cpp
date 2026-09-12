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
// depuis le modèle, d'où boundingSize() ci-dessous.

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

static float clampf(float value, float minValue, float maxValue) {
    if (value < minValue) {
        return minValue;
    }
    if (value > maxValue) {
        return maxValue;
    }
    return value;
}

static RSEntity *planeEntity(SCPlane *subject) {
    if (subject == nullptr || subject->object == nullptr) {
        return nullptr;
    }
    return subject->object->entity;
}

void SCOrbitCamera::activate(SCPlane *subject) {
    this->subject = subject;
    this->orbit.Identity();
    if (subject == nullptr) {
        if (this->debugEnabled()) {
            printf("%s activate subject=NULL -> subject reste NULL, rien d'autre initialisé\n", this->debugLabel());
        }
        return;
    }
    float size = boundingSize(planeEntity(subject));
    this->dist = size * 2.0f;
    Vector3D subjectPos(subject->x, subject->y, subject->z);
    Vector3D axis = subject->forward.transformPoint(this->orbit);
    // Pas de recul initial "à vide" (pas de swoop-in à l'activation) : on
    // démarre déjà sur la position idéale, le lissage ne joue qu'ensuite.
    this->cam_pos = subjectPos - axis * this->dist;
    if (this->debugEnabled()) {
        printf("%s activate subject=%p entity=%p size=%.3f dist=%.3f subjPos=(%.3f,%.3f,%.3f) axis=(%.3f,%.3f,%.3f) camPos0=(%.3f,%.3f,%.3f)\n",
               this->debugLabel(), (void *)subject, (void *)planeEntity(subject), size, this->dist,
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
    Vector3D subjectPos(this->subject->x, this->subject->y, this->subject->z);

    float size    = boundingSize(planeEntity(this->subject));
    float distMin = size * 1.0f;
    float distMax = size * 4.0f;
    if (this->dist <= 0.0f) {
        this->dist = size * 2.0f;
    }
    this->dist = clampf(this->dist, distMin, distMax);

    // `orbit` = identité pour CHASE/TARGET ; tournée par ROTA avant l'appel
    // à cette méthode (cf. SCRotaCamera::tick).
    Vector3D axis   = this->subject->forward.transformPoint(this->orbit);
    Vector3D target = subjectPos - axis * this->dist;

    // Lissage ASM : pos += (cible - pos) / 4 PAR FRAME. Mis à l'échelle du dt
    // réel (référence 25 fps, cf. CAMERA_SYSTEM.md §6.3) pour rester correct
    // à tout framerate plutôt que de dépendre du taux de MissionUpdateEvent.
    float alpha = clampf((dt * 25.0f) / 4.0f, 0.0f, 1.0f);
    Vector3D camPosBefore = this->cam_pos;
    this->cam_pos = this->cam_pos + (target - this->cam_pos) * alpha;

    Vector3D lookDir = subjectPos - this->cam_pos;
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
        printf("%s f=%ld dt=%.5f alpha=%.5f subjPos=(%.3f,%.3f,%.3f) axis=(%.3f,%.3f,%.3f) dist=%.3f target=(%.3f,%.3f,%.3f) camPos %.3f,%.3f,%.3f -> %.3f,%.3f,%.3f out_aim=(%.3f,%.3f,%.3f)\n",
               this->debugLabel(), frame, dt, alpha,
               subjectPos.x, subjectPos.y, subjectPos.z,
               axis.x, axis.y, axis.z,
               this->dist,
               target.x, target.y, target.z,
               camPosBefore.x, camPosBefore.y, camPosBefore.z,
               this->cam_pos.x, this->cam_pos.y, this->cam_pos.z,
               out_aim.x, out_aim.y, out_aim.z);
    }
}
