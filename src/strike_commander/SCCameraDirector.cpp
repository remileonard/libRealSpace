#include "precomp.h"
#include "SCCameraDirector.h"
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
    this->subscription_id =
        MessageBus::getInstance().subscribeEvent(std::bind(&SCCameraDirector::onEvent, this, std::placeholders::_1));
}

SCCameraDirector::~SCCameraDirector() {
    if (this->subscription_id != -1) {
        MessageBus::getInstance().unsubscribe(this->subscription_id);
        this->subscription_id = -1;
    }
}

void SCCameraDirector::init(RSWorld *w, SCPlane *player) {
    this->world = w;
    this->player_entity = player;
}

// ---------------------------------------------------------------------------
//  Bus
// ---------------------------------------------------------------------------

void SCCameraDirector::onEvent(const EventMessage &event) {
    const CameraViewRequest *request = dynamic_cast<const CameraViewRequest *>(&event);
    if (request == nullptr) {
        return;
    }
    this->onViewRequest(*request);
}

void SCCameraDirector::onViewRequest(const CameraViewRequest &request) {
    this->current_view = request.view;

    if (request.sequence_name.empty()) {
        // Vue non scriptée : on coupe toute séquence en cours ; le placement
        // reste à la charge de SCStrike.
        if (this->sequence.getStatus() == SCCameraSequence::Running) {
            this->sequence.start(nullptr, nullptr);
        }
        this->view_desc = CameraViewDesc();
        this->view_desc.view = request.view;
        this->view_desc.scripted = false;
        return;
    }

    const RSCameraSequence *seq = this->findSequence(request.sequence_name);
    if (seq == nullptr) {
        return;
    }

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
