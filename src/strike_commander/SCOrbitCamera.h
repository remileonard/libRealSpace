#pragma once
#include "SCProceduralCamera.h"

//
// Base commune aux caméras orbitales du registre CAMR — CHASE, TARGET, et
// ROTA. Une seule et même formule ASM pour les trois
// (Camera_OrbitReset_85878 / Camera_OrbitTrackCompute_1493A, cf.
// CAMERA_SYSTEM.md §4bis) : distance de recul dérivée de la taille du
// sujet (défaut ×2, bornes ×1..×4), position lissée (+= (cible-pos)/4 par
// frame), orientation qui regarde le sujet. Seule la matrice d'orbite
// diffère d'une caméra à l'autre :
//   - CHASE / TARGET : identité (pas de rotation joueur)
//   - ROTA           : tournée par les entrées joueur (souris/joystick),
//                       cf. SCRotaCamera — pas encore câblé.
// Les sous-classes n'ont donc à fournir que typeCode() + les deux hooks de
// debug ; ROTA en plus surcharge tick() pour tourner `orbit` avant d'appeler
// la version de base.
//
class SCOrbitCamera : public SCProceduralCamera {
public:
    void activate(SCPlane *subject) override;
    void tick(float dt, Vector3D &out_pos, Vector3D &out_aim, Vector3D &out_up) override;

protected:
    // Préfixe des traces ("[CHASE]", "[TARGET]", "[ROTA]") et flag de debug
    // de la sous-classe — chaque caméra a son propre static bool s_debug
    // pour pouvoir les activer/désactiver indépendamment.
    virtual const char *debugLabel() const = 0;
    virtual bool debugEnabled() const = 0;

    SCPlane  *subject{nullptr};
    Matrix    orbit;                       // identité par défaut ; ROTA la tourne
    float     dist{0.0f};
    Vector3D  cam_pos{0.0f, 0.0f, 0.0f};
};
