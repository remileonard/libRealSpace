#pragma once
#include <string>
#include "SCenums.h"
#include "../engine/EventMessage.h"
#include "../realspace/RSWorld.h"   // RSCameraType

class SCPlane;

//
// Demande d'activation d'une vue caméra. Publié par : couche input (touches de
// vue), autopilote, VM de script mission, détection décollage/atterrissage.
// Consommé par SCCameraDirector.
//
// Trois façons mutuellement exclusives de désigner la vue, dans l'ordre où
// SCCameraDirector::onViewRequest() les regarde :
//   1. `sequence_name` : séquence scriptée COMP, par nom de fichier
//      ("STARTCAM" / "TAKEOFF" / "LANDING" / "AUTOPILT").
//   2. `camera_type`   : entrée du registre CAMR (RSWorld::cameras), par
//      RSCameraType (RSCAM_CHAS, RSCAM_TARG...) — c'est le code que le
//      fichier de mission porte réellement, pas une traduction depuis `view`.
//   3. sinon `view` sert tel quel (FRONT/LEFT/RIGHT/REAR...) : vue sans
//      backing fichier, le placement reste à la charge de SCStrike.
// `view` seul reste le vocabulaire de SCStrike (camera_mode, rendu) — ce
// n'est jamais lui qui pilote la résolution interne du directeur.
//
class CameraViewRequest : public EventMessage {
public:
    View         view{View::FRONT};
    std::string  sequence_name;
    RSCameraType camera_type{RSCameraType::RSCAM_NONE};
    SCPlane     *subject{nullptr};
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
