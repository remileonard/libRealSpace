#pragma once
#include "SCProceduralCamera.h"
#include "../realspace/RSWorld.h"   // RSCameraDef

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
//
// Une instance par RSCameraDef réellement présent dans le monde chargé
// (cf. SCCameraDirector::init()) : `def` porte le nom, le typeCode, le
// sujet déclaré, le fov, les plans de clip et le rect de viewport TELS QUE
// LUS DU FICHIER — ce n'est plus le directeur qui va piocher un champ à la
// fois, l'objet caméra EST sa définition fichier. typeCode()/name() sont
// dérivés de `def`, pas dupliqués en dur par sous-classe.
//
class SCOrbitCamera : public SCProceduralCamera {
public:
    explicit SCOrbitCamera(const RSCameraDef *def);

    RSCameraType              typeCode() const override;
    const std::string        &name() const override;         // def->name ("CHASECAM"/"ROTATCAM"/"AUTOTRAC")
    const std::string        &subjectName() const override;  // def->subject — résolu par le directeur
    float                     fov() const override;           // def->fov (degrés) — utilisable tel quel
    const RSCameraDef        &definition() const;             // accès complet (fov, clips, viewport, params...)

    void activate(SCPlane *subject) override;
    void tick(float dt, Vector3D &out_pos, Vector3D &out_aim, Vector3D &out_up) override;

protected:
    // Préfixe des traces ("[CHASE]", "[TARGET]", "[ROTA]") et flag de debug
    // de la sous-classe — chaque caméra a son propre static bool s_debug
    // pour pouvoir les activer/désactiver indépendamment.
    virtual const char *debugLabel() const = 0;
    virtual bool debugEnabled() const = 0;

    const RSCameraDef *def;   // dans world->cameras ; durée de vie = le monde

    SCPlane  *subject{nullptr};
    Matrix    orbit;                       // identité par défaut ; ROTA la tourne
    float     dist{0.0f};
    Vector3D  cam_pos{0.0f, 0.0f, 0.0f};
};
