#pragma once
#include "SCProceduralCamera.h"
#include "SCCameraSequence.h"

//
// Vue COMP scriptée (STARTCAM/TAKEOFF/LANDING/AUTOPILT...). Une instance par
// RSCameraSequence du monde, créée une fois dans SCCameraDirector::init()
// — miroir du registre ASM 0x59CD, où chaque script COMP a sa propre
// entrée, cherchée par nom (Kneeboard_SelectByID/FindByID). typeCode()
// renvoie RSCAM_COMP pour toutes (tag ASM 'COMP' = 0x13) ; seul name() les
// distingue.
//
// Pas de couplage au directeur : à la fin de la séquence, publie
// elle-même sur le bus la CameraViewRequest de la vue de reprise
// (OP_IA_END -> "COCKPIT" etc.), exactement comme n'importe quel autre
// déclencheur de vue (touche F1, VM de script...).
//
class SCSequenceCamera : public SCProceduralCamera {
public:
    explicit SCSequenceCamera(const RSCameraSequence *def);

    RSCameraType       typeCode() const override;
    const std::string &name() const override;
    void                activate(SCMissionActors *subject, SCMissionActors *target) override;
    void                tick(float dt, Vector3D &out_pos, Vector3D &out_aim, Vector3D &out_up) override;

private:
    const RSCameraSequence *def;       // dans world->cameraSequences ; durée de vie = le monde
    SCCameraSequence         sequence; // état d'exécution
};
