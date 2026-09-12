#pragma once
#include "SCOrbitCamera.h"

//
// Vue TARGET — même formule que CHASE (cf. SCOrbitCamera). Une instance par
// RSCameraDef de typeCode RSCAM_TARG réellement présent dans le monde (cf.
// SCCameraDirector::init()).
//
// `subjectName()` (= RSCameraDef::subject du chunk CAMR TARG) vaut toujours
// "PLAYER" dans les fichiers livrés (dump data/ALASKA.WRLD.CAMR.TARG.DAT) :
// ce n'est PAS un sujet fixe comme pour CHASE, c'est juste le repli du
// fichier. C'est à l'émetteur de la CameraViewRequest (la logique de
// verrouillage de cible) de renseigner explicitement `request.subject` =
// l'avion cible courant ; à défaut, le directeur retombe sur
// resolveEntity(subjectName()) = "PLAYER" (comportement dégradé mais pas
// cassé : orbite autour du joueur).
//
class SCTargetCamera : public SCOrbitCamera {
public:
    static bool s_debug;   // true = trace TARGET (activation + par frame) au stdout

    explicit SCTargetCamera(const RSCameraDef *def) : SCOrbitCamera(def) {}

protected:
    const char *debugLabel() const override;
    bool debugEnabled() const override;
};
