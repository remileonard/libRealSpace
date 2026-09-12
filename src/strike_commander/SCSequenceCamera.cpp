#include "precomp.h"
#include "SCSequenceCamera.h"
#include "SCCameraEvent.h"

//
// Nom de vue de reprise (OP_IA_END du bytecode COMP) -> View de libRealSpace.
// Style identique à weapon_names.
//
static const std::unordered_map<std::string, View> comp_handoff_views = {
    {"COCKPIT", View::FRONT},
    {"CHASE",   View::FOLLOW},
    {"TARGET",  View::TARGET},
};

SCSequenceCamera::SCSequenceCamera(const RSCameraSequence *def) : def(def) {
}

RSCameraType SCSequenceCamera::typeCode() const {
    return RSCAM_COMP;
}

const std::string &SCSequenceCamera::name() const {
    return this->def->name;
}

void SCSequenceCamera::activate(SCPlane *subject) {
    this->sequence.start(this->def, subject);
}

void SCSequenceCamera::tick(float dt, Vector3D &out_pos, Vector3D &out_aim, Vector3D &out_up) {
    SCCameraSequence::Status status = this->sequence.tick(dt);
    out_pos = this->sequence.getPosition();
    out_aim = this->sequence.getAimPoint();
    out_up  = this->sequence.getUp();

    if (status != SCCameraSequence::Finished) {
        return;
    }

    // Fin de séquence : demande la vue de reprise via le même mécanisme que
    // n'importe quelle autre requête de vue (F1/F2/STARTCAM...) — le
    // directeur n'a besoin d'aucune connaissance spécifique aux séquences.
    View next = View::REAL;
    std::string handoff = this->sequence.getHandoffView();
    std::unordered_map<std::string, View>::const_iterator it = comp_handoff_views.find(handoff);
    if (it != comp_handoff_views.end()) {
        next = it->second;
    }
    CameraViewRequest request;
    request.view = next;
    MessageBus::getInstance().publish(std::make_unique<CameraViewRequest>(request));
}
