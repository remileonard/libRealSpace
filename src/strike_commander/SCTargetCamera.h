#pragma once
#include "SCOrbitCamera.h"

//
// Vue TARGET — même formule de recul/lissage que CHASE (cf. SCOrbitCamera :
// recul basé sur la taille du sujet, pas une distance fixe — sinon une
// grosse cible ne rentrerait pas dans le cadre), mais avec un point de
// visée distinct : `subject` = la cible verrouillée (position/recul de la
// caméra), `aimTarget` = le joueur (ce qu'elle regarde). Le code ASM exact
// de TARGET (F7) reste non identifié (CAMERA_SYSTEM.md §9 point 7) — ce
// modèle est un choix de conception délibéré (Rémi, 2026-09-12), pas un
// portage direct.
//
// Toute la logique propre à TARGET vit ICI (aimPosition()/backwardAxis()),
// pas dans SCOrbitCamera qui reste une classe de base fermée, sans branche
// spécifique à une caméra particulière.
//
// L'appelant (SCStrike, F7) peut inverser subject/target d'un appui à
// l'autre — rien à changer ici, activate() retient juste ce qu'on lui donne.
//
// `subjectName()` (= RSCameraDef::subject du chunk CAMR TARG) vaut toujours
// "PLAYER" dans les fichiers livrés (dump data/ALASKA.WRLD.CAMR.TARG.DAT) :
// simple repli fichier si la requête ne fournit pas de sujet explicite —
// dans ce cas `aimTarget` reste nullptr et la caméra se rabat sur un plan
// chase autour du joueur (dégradé mais pas cassé).
//
class SCTargetCamera : public SCOrbitCamera {
public:
    static bool s_debug;   // true = trace TARGET (activation + par frame) au stdout

    explicit SCTargetCamera(const RSCameraDef *def) : SCOrbitCamera(def) {}

    void activate(SCMissionActors *subject, SCMissionActors *target) override;

protected:
    const char *debugLabel() const override;
    bool debugEnabled() const override;

    Vector3D aimPosition(const Vector3D &subjectPos) const override;
    Vector3D backwardAxis(const Vector3D &subjectPos) const override;

private:
    // Le joueur (ou tout acteur visé), séparé du sujet autour duquel la
    // caméra recule — propre à cette classe, SCOrbitCamera n'en sait rien.
    SCMissionActors *aimTarget{nullptr};
};
