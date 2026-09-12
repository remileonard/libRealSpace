#include "precomp.h"
#include "SCCameraDirector.h"
#include "SCChaseCamera.h"
#include "SCTargetCamera.h"
#include "SCRotaCamera.h"
#include "SCSequenceCamera.h"
#include "../realspace/RSWorld.h"
#include <functional>

//
// Caméra "vide" — repli quand aucune SCProceduralCamera ne correspond à la
// vue demandée (vues brutes FRONT/LEFT/RIGHT/REAR..., sans backing fichier :
// le placement reste à la charge de SCStrike). Existe uniquement pour que
// active_camera ne soit jamais nullptr, afin que tick() reste un simple
// appel polymorphe sans branche.
//
class SCNullCamera : public SCProceduralCamera {
public:
    RSCameraType typeCode() const override { return RSCAM_NONE; }
    void activate(SCMissionActors *, SCMissionActors *) override {}
    void tick(float, Vector3D &, Vector3D &, Vector3D &) override {}
};

SCCameraDirector::SCCameraDirector() {
    this->out_pos = Vector3D(0.0f, 0.0f, 0.0f);
    this->out_aim = Vector3D(0.0f, 0.0f, -1.0f);
    this->out_up = Vector3D(0.0f, 1.0f, 0.0f);

    this->null_camera = new SCNullCamera();
    this->active_camera = this->null_camera;

    // Les caméras procédurales (CHASE/TARGET/ROTA/...) et les séquences
    // COMP sont créées dans init(), une fois le monde chargé — cf.
    // createProceduralCamera() : leur existence est pilotée par les
    // RSCameraDef/RSCameraSequence réellement présents dans le fichier
    // WRLD, jamais construites en dur ici.

    this->subscription_id =
        MessageBus::getInstance().subscribeEvent(std::bind(&SCCameraDirector::onEvent, this, std::placeholders::_1));
}

SCCameraDirector::~SCCameraDirector() {
    if (this->subscription_id != -1) {
        MessageBus::getInstance().unsubscribe(this->subscription_id);
        this->subscription_id = -1;
    }
    for (size_t i = 0; i < this->procedural_cameras.size(); i++) {
        delete this->procedural_cameras[i];
    }
    this->procedural_cameras.clear();
    delete this->null_camera;
    this->null_camera = nullptr;
}

void SCCameraDirector::init(RSWorld *w, SCMissionActors *player) {
    this->world = w;
    this->player_entity = player;

    if (this->world == nullptr) {
        return;
    }

    // Une SCProceduralCamera par RSCameraDef réellement chargé (CAMR :
    // CHASE/TARGET/ROTA/...) — miroir du registre ASM 0x59CD côté
    // Kneeboard_SelectByStateCode. Un type non câblé côté C++ (CKPT/VICT/
    // WEAP/CONT) est silencieusement ignoré (createProceduralCamera ->
    // nullptr) plutôt que de fabriquer une caméra sans données.
    for (size_t i = 0; i < this->world->cameras.size(); i++) {
        SCProceduralCamera *cam = this->createProceduralCamera(&this->world->cameras[i]);
        if (cam != nullptr) {
            this->procedural_cameras.push_back(cam);
        }
    }

    // Une SCSequenceCamera par script COMP du monde (STARTCAM/TAKEOFF/...) —
    // miroir du registre ASM où chaque script a sa propre entrée. Le dump de
    // debug (inchangé) se fait au même moment, sur la même liste.
    for (size_t i = 0; i < this->world->cameraSequences.size(); i++) {
        if (SCCameraSequence::s_debug) {
            SCCameraSequence::dumpProgram(this->world->cameraSequences[i]);
        }
        this->procedural_cameras.push_back(new SCSequenceCamera(&this->world->cameraSequences[i]));
    }
}

SCProceduralCamera *SCCameraDirector::createProceduralCamera(const RSCameraDef *def) const {
    switch (def->typeCode) {
        case RSCAM_CHAS: {
            return new SCChaseCamera(def);
        }
        case RSCAM_TARG: {
            return new SCTargetCamera(def);
        }
        case RSCAM_ROTA: {
            return new SCRotaCamera(def);
        }
        default: {
            return nullptr;
        }
    }
}

// ---------------------------------------------------------------------------
//  Bus
// ---------------------------------------------------------------------------

void SCCameraDirector::onEvent(const EventMessage &event) {
    if (auto eventData = dynamic_cast<const CameraViewRequest*>(&event)) {
        this->onViewRequest(*eventData);
        return;
    }
    if (auto eventData = dynamic_cast<const MissionUpdateEvent*>(&event)) {
        this->tick(eventData->delta_time);
        return;
    }
}

void SCCameraDirector::onViewRequest(const CameraViewRequest &request) {
    // Recherche par nom (séquence COMP) — miroir de Kneeboard_SelectByID /
    // Kneeboard_FindByID.
    if (!request.sequence_name.empty()) {
        SCProceduralCamera *cam = this->findCameraByName(request.sequence_name);
        if (cam == nullptr) {
            return;
        }
        SCMissionActors *subject = request.subject;
        if (subject == nullptr) {
            subject = this->resolveEntity("PLAYER");
        }
        this->activateCamera(cam, subject, request.target);
        return;
    }

    // Recherche par code de type (CHASE/TARGET/ROTA...) — miroir de
    // Kneeboard_SelectByStateCode. Le sujet par défaut vient du fichier
    // (cam->subjectName(), lu depuis RSCameraDef::subject à la
    // construction), pas d'un nom en dur : c'est la seule source fidèle
    // quand l'appelant ne fournit pas explicitement request.subject.
    if (request.camera_type != RSCAM_NONE) {
        SCProceduralCamera *cam = this->findCameraByType(request.camera_type);
        if (cam != nullptr) {
            SCMissionActors *subject = request.subject;
            if (subject == nullptr) {
                subject = this->resolveEntity(cam->subjectName());
            }
            this->activateCamera(cam, subject, request.target);
            return;
        }
    }

    this->activateSimpleView(request.view);
}

SCProceduralCamera *SCCameraDirector::findCameraByType(RSCameraType type_code) const {
    for (size_t i = 0; i < this->procedural_cameras.size(); i++) {
        if (this->procedural_cameras[i]->typeCode() == type_code) {
            return this->procedural_cameras[i];
        }
    }
    return nullptr;
}

SCProceduralCamera *SCCameraDirector::findCameraByName(const std::string &name) const {
    for (size_t i = 0; i < this->procedural_cameras.size(); i++) {
        if (this->procedural_cameras[i]->name() == name) {
            return this->procedural_cameras[i];
        }
    }
    return nullptr;
}

void SCCameraDirector::activateCamera(SCProceduralCamera *cam, SCMissionActors *subject, SCMissionActors *target) {
    // activate() à la transition VERS cette caméra (pas à chaque appui
    // répété sur la même touche, sinon on perdrait le lissage en repartant
    // de zéro à chaque frame où la même vue est redemandée) — MAIS AUSSI
    // quand le sujet ou la cible changent alors que la caméra reste la
    // même instance (TARGET : une seule SCProceduralCamera pour tout le
    // typeCode ; reverrouiller une autre cible, ou inverser subject/target
    // d'un appui F7 à l'autre, doit réinitialiser cam_pos/dist, pas être
    // ignoré parce que c'est "la même caméra").
    if (this->active_camera != cam || subject != this->active_subject || target != this->active_target) {
        if (SCCameraDirector::s_debug) {
            printf("[DIRECTOR] activateCamera typeCode=%d name='%s' subject %p -> %p target %p -> %p (camChanged=%d subjectChanged=%d targetChanged=%d)\n",
                   (int)cam->typeCode(), cam->name().c_str(),
                   (void *)this->active_subject, (void *)subject,
                   (void *)this->active_target, (void *)target,
                   this->active_camera != cam, subject != this->active_subject, target != this->active_target);
        }
        cam->activate(subject, target);
    }
    this->active_camera = cam;
    this->active_subject = subject;
    this->active_target = target;

    this->view_desc = CameraViewDesc();
    this->view_desc.view = View::CAM_DIRECTOR;
    this->view_desc.scripted = (cam->typeCode() == RSCAM_COMP);
    this->view_desc.first_person = false;
    this->view_desc.renders_cockpit = false;

    this->current_view = View::CAM_DIRECTOR;
    this->publishViewChanged(View::CAM_DIRECTOR, "camera-start");
}

void SCCameraDirector::activateSimpleView(View view) {
    // Vue non scriptée, sans backing fichier : le placement reste à la
    // charge de SCStrike.
    this->active_camera = this->null_camera;
    this->current_view = view;
    this->view_desc = CameraViewDesc();
    this->view_desc.view = view;
    this->view_desc.scripted = false;
}

// ---------------------------------------------------------------------------
//  Tick
// ---------------------------------------------------------------------------

void SCCameraDirector::tick(float dt) {
    // Point de dispatch unique, fermé pour toujours : ajouter une caméra ne
    // touche jamais cette fonction. active_camera n'est jamais nullptr
    // (SCNullCamera par défaut).
    this->active_camera->tick(dt, this->out_pos, this->out_aim, this->out_up);
}

// ---------------------------------------------------------------------------
//  Résolution
// ---------------------------------------------------------------------------

SCMissionActors *SCCameraDirector::resolveEntity(const std::string &name) const {
    if (name == "PLAYER") {
        return this->player_entity;
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
//  Vue courante
// ---------------------------------------------------------------------------

void SCCameraDirector::publishViewChanged(View view, const std::string &reason) {
    CameraViewChanged changed;
    changed.view = view;
    changed.reason = reason;
    MessageBus::getInstance().publish(std::make_unique<CameraViewChanged>(changed));
}

// ---------------------------------------------------------------------------
//  Accesseurs
// ---------------------------------------------------------------------------

const Vector3D &SCCameraDirector::position() const {
    return this->out_pos;
}

const Vector3D &SCCameraDirector::aimPoint() const {
    return this->out_aim;
}

const Vector3D &SCCameraDirector::up() const {
    return this->out_up;
}

bool SCCameraDirector::s_debug = true;

float SCCameraDirector::fov() const {
    float value = this->active_camera->fov();
    if (SCCameraDirector::s_debug) {
        printf("[DIRECTOR] fov() typeCode=%d name='%s' -> %.3f\n",
               (int)this->active_camera->typeCode(), this->active_camera->name().c_str(), value);
    }
    return value;
}

const CameraViewDesc &SCCameraDirector::viewDesc() const {
    return this->view_desc;
}

View SCCameraDirector::currentView() const {
    return this->current_view;
}

RSCameraType SCCameraDirector::activeCameraType() const {
    return this->active_camera->typeCode();
}

SCMissionActors *SCCameraDirector::activeSubject() const {
    return this->active_subject;
}
