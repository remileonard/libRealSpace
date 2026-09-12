#pragma once
#include "SCOrbitCamera.h"

//
// Vue CHASE — caméra orbitale à orbite figée (identité) derrière l'avion
// suivi. Cf. SCOrbitCamera pour la formule complète.
//
class SCChaseCamera : public SCOrbitCamera {
public:
    static bool s_debug;   // true = trace CHASE (activation + par frame) au stdout

    RSCameraType typeCode() const override;

protected:
    const char *debugLabel() const override;
    bool debugEnabled() const override;
};
