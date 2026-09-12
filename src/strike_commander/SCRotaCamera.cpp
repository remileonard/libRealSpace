#include "precomp.h"
#include "SCRotaCamera.h"

bool SCRotaCamera::s_debug = true;

RSCameraType SCRotaCamera::typeCode() const {
    return RSCAM_ROTA;
}

void SCRotaCamera::tick(float dt, Vector3D &out_pos, Vector3D &out_aim, Vector3D &out_up) {
    // TODO(entrée joueur) : tourner `this->orbit` selon la souris/joystick
    // (Matrix_BuildAxisZ/X sur dx/dy, bornées par les limites d'angle du
    // fichier) et faire varier `this->dist` selon le zoom, avant l'appel
    // ci-dessous — cf. le commentaire de classe. Rien de plus à changer
    // dans SCOrbitCamera::tick() une fois l'entrée branchée.
    SCOrbitCamera::tick(dt, out_pos, out_aim, out_up);
}

const char *SCRotaCamera::debugLabel() const {
    return "[ROTA]";
}

bool SCRotaCamera::debugEnabled() const {
    return SCRotaCamera::s_debug;
}
