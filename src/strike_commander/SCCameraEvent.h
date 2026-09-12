#pragma once
#include <string>
#include "SCenums.h"
#include "../engine/EventMessage.h"
#include "../realspace/RSWorld.h"   // RSCameraType

class SCMissionActors;

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
    // N'importe quel acteur de mission (avion, bateau, bâtiment) — pas
    // seulement un SCPlane : une cible verrouillée (F7) peut être n'importe
    // quel RSEntity. Voir SCOrbitCamera pour la résolution position/forward/
    // bounding box générique.
    //
    // `subject` = l'acteur autour duquel la caméra se positionne (recul basé
    // sur sa taille, comme CHASE). `target` = l'acteur visé (lookAt), utilisé
    // seulement par les caméras qui en ont besoin (TARGET) ; ignoré sinon
    // (reste nullptr). Explicitement séparés de SCMissionActors::target (le
    // verrou de tir propre à un acteur) pour permettre l'inversion
    // position/visée d'un appui à l'autre sans dépendre de qui verrouille
    // qui.
    SCMissionActors *subject{nullptr};
    SCMissionActors *target{nullptr};
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
