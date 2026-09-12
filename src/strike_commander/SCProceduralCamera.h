#pragma once
#include "../realspace/RSWorld.h"   // RSCameraType

class SCPlane;

//
// Interface d'une caméra procédurale du registre CAMR (CHASE, bientôt
// TARGET, ROTA...). Miroir du slot de vtable ASM qui calcule la position et
// l'orientation par frame pour un type de caméra donné — cf.
// CAMERA_SYSTEM.md §3.3/§4bis (chaque type d'entrée CAMR a sa propre
// vtable/son propre calcul dans le jeu d'origine).
//
// Une instance par type, enregistrée une fois dans
// SCCameraDirector::procedural_cameras. Ajouter une caméra = écrire une
// nouvelle sous-classe + l'enregistrer ; SCCameraDirector::tick() et
// activateProceduralCamera() ne changent plus jamais.
//
class SCProceduralCamera {
public:
    virtual ~SCProceduralCamera() {}

    // RSCameraType géré par cette instance (RSCAM_CHAS, RSCAM_TARG...).
    virtual RSCameraType typeCode() const = 0;

    // Appelé UNE SEULE FOIS, au moment où l'on bascule SUR cette caméra
    // depuis une autre vue (pas à chaque frame) : pose l'état initial
    // (position de départ, distance, sujet suivi...).
    virtual void activate(SCPlane *subject) = 0;

    // Appelé à chaque frame tant que cette caméra est active ; écrit la
    // pose caméra dans les sorties du directeur.
    virtual void tick(float dt, Vector3D &out_pos, Vector3D &out_aim, Vector3D &out_up) = 0;
};
