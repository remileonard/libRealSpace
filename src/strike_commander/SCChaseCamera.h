#pragma once
#include "SCOrbitCamera.h"

//
// Vue CHASE — caméra orbitale à orbite figée (identité) derrière l'avion
// suivi. Cf. SCOrbitCamera pour la formule complète. Une instance par
// RSCameraDef de typeCode RSCAM_CHAS réellement présent dans le monde
// (cf. SCCameraDirector::init()).
//
class SCChaseCamera : public SCOrbitCamera {
public:
    static bool s_debug;;   // true = trace CHASE (activation + par frame) au stdout

    explicit SCChaseCamera(const RSCameraDef *def) : SCOrbitCamera(def) {}

protected:
    const char *debugLabel() const override;
    bool debugEnabled() const override;
};
