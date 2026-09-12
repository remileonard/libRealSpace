#pragma once
#include "SCOrbitCamera.h"

//
// Vue TARGET — même formule que CHASE (cf. SCOrbitCamera), mais le "sujet"
// est la cible verrouillée, pas l'avion joueur. RSCameraDef::subject du
// chunk CAMR TARG vaut toujours "PLAYER" dans les fichiers livrés (dump
// data/ALASKA.WRLD.CAMR.TARG.DAT) : ce n'est PAS un sujet fixe résolvable
// par nom comme pour CHASE. C'est à l'émetteur de la CameraViewRequest (la
// touche/logique qui gère le verrouillage de cible) de renseigner
// explicitement `request.subject` = l'avion cible courant ; à défaut,
// SCCameraDirector retombe sur resolveEntity("PLAYER") (comportement
// dégradé mais pas cassé : orbite autour du joueur).
//
class SCTargetCamera : public SCOrbitCamera {
public:
    static bool s_debug;   // true = trace TARGET (activation + par frame) au stdout

    RSCameraType typeCode() const override;

protected:
    const char *debugLabel() const override;
    bool debugEnabled() const override;
};
