#pragma once
#include <string>
#include "SCenums.h"
#include "../engine/EventMessage.h"

class SCPlane;

//
// Demande d'activation d'une vue caméra. Publié par : couche input (touches de
// vue), autopilote, VM de script mission, détection décollage/atterrissage.
// Consommé par SCCameraDirector.
//
// `sequence_name` est renseigné pour les vues scriptées COMP
// ("STARTCAM" / "TAKEOFF" / "LANDING" / "AUTOPILT") ; sinon vide.
//
class CameraViewRequest : public EventMessage {
public:
    View        view{View::FRONT};
    std::string sequence_name;
    SCPlane    *subject{nullptr};
};

//
// Le directeur annonce que la vue active a changé. Émis notamment quand une
// séquence COMP se termine (OP_IA_END -> vue de reprise, "COCKPIT").
// Consommé par SCStrike / SCCockpit / SCHud pour reconfigurer le rendu.
//
class CameraViewChanged : public EventMessage {
public:
    View        view{View::FRONT};
    std::string reason;
};
