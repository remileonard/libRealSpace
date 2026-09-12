#include "precomp.h"
#include "SCCameraDirector.h"
#include "SCChaseCamera.h"
#include "SCTargetCamera.h"
#include "SCRotaCamera.h"
#include "../realspace/RSWorld.h"
#include <functional>

//
// Nom de vue de reprise (OP_IA_END du bytecode COMP) -> View de libRealSpace.
// Style identique à weapon_names.
//
static const std::unordered_map<std::string, View> comp_handoff_views = {
    {"COCKPIT", View::FRONT},
    {"CHASE",   View::FOLLOW},
    {"TARGET",  View::TARGET},
};

SCCameraDirector::SCCameraDirector() {
    this->out_pos = Vector3D(0.0f, 0.0f, 0.0f);
    this->out_aim = Vector3D(0.0f, 0.0f, -1.0f);
    this->out_up = Vector3D(0.0f, 1.0f, 0.0f);

    // Une SCProceduralCamera par RSCameraType câblé. Pour ajouter une
    // nouvelle caméra : écrire la sous-classe puis une ligne ici — rien
    // d'autre dans ce fichier ne change (cf. commentaire de classe dans
    // SCCameraDirector.h).
    this->procedural_cameras.push_back(new SCChaseCamera());
    this->procedural_cameras.push_back(new SCTargetCamera());
    this->procedural_cameras.push_back(new SCRotaCamera());

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
}

void SCCameraDirector::init(RSWorld *w, SCPlane *player) {
    this->world = w;
    this->player_entity = player;

    if (SCCameraSequence::s_debug && this->world != nullptr) {
        printf("[COMP] %zu sequences COMP dans le monde :\n", this->world->cameraSequences.size());
        for (size_t i = 0; i < this->world->cameraSequences.size(); i++) {
            SCCameraSequence::dumpProgram(this->world->cameraSequences[i]);
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
    if (!request.sequence_name.empty()) {
        this->activateSequence(request);
        return;
    }

    // Miroir de Kneeboard_SelectByStateCode : request.camera_type est le
    // code de type tel que porté par le fichier (RSCameraType) — pas une
    // traduction depuis `view`. C'est à l'émetteur de la requête (touche F2,
    // VM de script...) de le renseigner.
    if (request.camera_type != RSCAM_NONE) {
        const RSCameraDef *entry = this->findCameraDef(request.camera_type);
        if (entry != nullptr) {
            this->activateProceduralCamera(*entry, request);
            return;
        }
    }

    this->deactivateProceduralViews();
    this->activateSimpleView(request.view);
}

const RSCameraDef *SCCameraDirector::findCameraDef(RSCameraType type_code) const {
    if (this->world == nullptr) {
        return nullptr;
    }
    for (size_t i = 0; i < this->world->cameras.size(); i++) {
        if (this->world->cameras[i].typeCode == type_code) {
            return &this->world->cameras[i];
        }
    }
    return nullptr;
}

SCProceduralCamera *SCCameraDirector::findProceduralCamera(RSCameraType type_code) const {
    for (size_t i = 0; i < this->procedural_cameras.size(); i++) {
        if (this->procedural_cameras[i]->typeCode() == type_code) {
            return this->procedural_cameras[i];
        }
    }
    return nullptr;
}

void SCCameraDirector::deactivateProceduralViews() {
    this->active_procedural = nullptr;
}

void SCCameraDirector::activateProceduralCamera(const RSCameraDef &entry, const CameraViewRequest &request) {
    SCProceduralCamera *cam = this->findProceduralCamera(entry.typeCode);
    if (cam == nullptr) {
        // Type reconnu dans le fichier (ROTA/TARG/VICT/WEAP/CONT/CKPT) mais
        // pas encore câblé côté directeur : pas de caméra procédurale pour
        // l'instant, on retombe sur la vue brute demandée.
        this->deactivateProceduralViews();
        this->activateSimpleView(request.view);
        return;
    }

    SCPlane *subject = request.subject;
    if (subject == nullptr) {
        subject = this->resolveEntity(entry.subject);
    }

    // activate() ne s'appelle qu'à la transition VERS cette caméra (pas à
    // chaque appui répété sur la même touche) : sinon on perdrait le lissage
    // en repartant de zéro. Rester sur une caméra déjà active ne réinitialise
    // rien, comme resetChase() avant l'extraction.
    if (this->active_procedural != cam) {
        cam->activate(subject);
    }
    this->active_procedural = cam;

    if (this->sequence.getStatus() == SCCameraSequence::Running) {
        this->sequence.start(nullptr, nullptr);
    }

    this->view_desc = CameraViewDesc();
    this->view_desc.view = View::CAM_DIRECTOR;
    this->view_desc.scripted = false;
    this->view_desc.first_person = false;
    this->view_desc.renders_cockpit = false;

    this->current_view = View::CAM_DIRECTOR;
    this->publishViewChanged(View::CAM_DIRECTOR, "procedural-start");
}

void SCCameraDirector::activateSimpleView(View view) {
    // Vue non scriptée (ni procédurale ni COMP) : on coupe toute séquence en
    // cours ; le placement reste à la charge de SCStrike.
    this->current_view = view;
    if (this->sequence.getStatus() == SCCameraSequence::Running) {
        this->sequence.start(nullptr, nullptr);
    }
    this->view_desc = CameraViewDesc();
    this->view_desc.view = view;
    this->view_desc.scripted = false;
}

void SCCameraDirector::activateSequence(const CameraViewRequest &request) {
    this->current_view = request.view;

    const RSCameraSequence *seq = this->findSequence(request.sequence_name);
    if (seq == nullptr) {
        return;
    }

    this->deactivateProceduralViews();

    SCPlane *target = request.subject;
    if (target == nullptr) {
        target = this->resolveEntity("PLAYER");
    }
    this->sequence.start(seq, target);

    this->view_desc = CameraViewDesc();
    this->view_desc.view = View::CAM_DIRECTOR;
    this->view_desc.scripted = true;
    this->view_desc.first_person = false;
    this->view_desc.renders_cockpit = false;

    this->out_pos = this->sequence.getPosition();
    this->out_aim = this->sequence.getAimPoint();
    this->out_up = this->sequence.getUp();

    // SCStrike bascule camera_mode sur CAM_DIRECTOR.
    this->current_view = View::CAM_DIRECTOR;
    this->publishViewChanged(View::CAM_DIRECTOR, "sequence-start");
}

// ---------------------------------------------------------------------------
//  Tick
// ---------------------------------------------------------------------------

void SCCameraDirector::tick(float dt) {
    // Point de dispatch unique, fermé : ajouter une caméra procédurale ne
    // touche jamais cette fonction (elle s'enregistre dans le constructeur,
    // cf. procedural_cameras).
    if (this->active_procedural != nullptr) {
        this->active_procedural->tick(dt, this->out_pos, this->out_aim, this->out_up);
        return;
    }
    this->tickSequence(dt);
}

void SCCameraDirector::tickSequence(float dt) {
    if (this->sequence.getStatus() != SCCameraSequence::Running) {
        return;
    }

    SCCameraSequence::Status status = this->sequence.tick(dt);
    this->out_pos = this->sequence.getPosition();
    this->out_aim = this->sequence.getAimPoint();
    this->out_up = this->sequence.getUp();

    if (status != SCCameraSequence::Finished) {
        return;
    }

    // Fin de séquence : bascule sur la vue de reprise.
    View next = View::REAL;
    std::string handoff = this->sequence.getHandoffView();
    std::unordered_map<std::string, View>::const_iterator it = comp_handoff_views.find(handoff);
    if (it != comp_handoff_views.end()) {
        next = it->second;
    }
    this->setView(next, "sequence-end");
}

// ---------------------------------------------------------------------------
//  Résolution
// ---------------------------------------------------------------------------

const RSCameraSequence *SCCameraDirector::findSequence(const std::string &name) const {
    if (this->world == nullptr) {
        return nullptr;
    }
    for (size_t i = 0; i < this->world->cameraSequences.size(); i++) {
        if (this->world->cameraSequences[i].name == name) {
            return &this->world->cameraSequences[i];
        }
    }
    return nullptr;
}

SCPlane *SCCameraDirector::resolveEntity(const std::string &name) const {
    if (name == "PLAYER") {
        return this->player_entity;
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
//  Vue courante
// ---------------------------------------------------------------------------

void SCCameraDirector::setView(View view, const std::string &reason) {
    this->current_view = view;
    this->view_desc = CameraViewDesc();
    this->view_desc.view = view;
    this->view_desc.scripted = false;
    this->publishViewChanged(view, reason);
}

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

const CameraViewDesc &SCCameraDirector::viewDesc() const {
    return this->view_desc;
}

View SCCameraDirector::currentView() const {
    return this->current_view;
}

bool SCCameraDirector::isRunningSequence() const {
    return this->sequence.getStatus() == SCCameraSequence::Running;
}
