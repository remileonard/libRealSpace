#pragma once
#include "SCOrbitCamera.h"

//
// Vue ROTA — même formule que CHASE/TARGET (cf. SCOrbitCamera), mais la
// matrice d'orbite `orbit` est tournée par les entrées joueur (souris /
// joystick) et la distance `dist` est zoomable, au lieu de rester figée à
// l'identité. ASM (CAMERA_SYSTEM.md §4bis) :
//   Matrix_BuildAxisZ_572BC(orbit, dx<<8) ; Matrix_BuildAxisX_56EC3(orbit, dy<<8)
//   zoom : dist += / -= dword_70448, borné [taille*1, taille*4]
// PAS ENCORE CÂBLÉ : la lecture des entrées joueur (dx/dy, zoom in/out)
// n'est pas branchée ici — tick() ne fait pour l'instant que déléguer à
// SCOrbitCamera::tick() avec `orbit` restée à l'identité, donc ROTA se
// comporte comme CHASE tant que ce n'est pas fait. Voir tick() ci-dessous
// pour le point d'insertion exact.
//
class SCRotaCamera : public SCOrbitCamera {
public:
    static bool s_debug;   // true = trace ROTA (activation + par frame) au stdout

    RSCameraType typeCode() const override;
    void tick(float dt, Vector3D &out_pos, Vector3D &out_aim, Vector3D &out_up) override;

protected:
    const char *debugLabel() const override;
    bool debugEnabled() const override;
};
