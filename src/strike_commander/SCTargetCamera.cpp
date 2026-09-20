#include "precomp.h"
#include "SCTargetCamera.h"

bool SCTargetCamera::s_debug = false;

void SCTargetCamera::activate(SCMissionActors *subject, SCMissionActors *target) {
    this->aimTarget = target;
    SCOrbitCamera::activate(subject, target);
}

Vector3D SCTargetCamera::aimPosition(const Vector3D &subjectPos) const {
    if (this->aimTarget == nullptr) {
        return subjectPos;
    }
    return SCOrbitCamera::actorPosition(this->aimTarget);
}

// Recule le long de l'axe visée->sujet (SCOrbitCamera::tick fait
// desiredPos = subjectPos - axis*dist, donc axis doit pointer DE la visée
// VERS le sujet pour que soustraire l'éloigne de la visée, de l'autre côté
// du sujet) plutôt que du nez du sujet : caméra, sujet (la cible verrouillée)
// et visée (le joueur) restent alignés, donc le joueur reste dans le cadre
// derrière/au-delà de la cible — reculer selon le nez de la cible pourrait
// la placer du mauvais côté et masquer le joueur avec la cible elle-même.
Vector3D SCTargetCamera::backwardAxis(const Vector3D &subjectPos) const {
    if (this->aimTarget == nullptr) {
        return SCOrbitCamera::backwardAxis(subjectPos);
    }
    Vector3D dir = subjectPos - SCOrbitCamera::actorPosition(this->aimTarget);
    if (dir.Length() < 0.001f) {
        return SCOrbitCamera::backwardAxis(subjectPos);
    }
    dir.Normalize();
    Vector3D axis = dir.transformPoint(this->orbit) * -1.0f;

    // desiredPos = subjectPos - axis*dist (SCOrbitCamera::tick), donc
    // desiredPos.y = subjectPos.y - axis.y*dist : un axis.y POSITIF fait
    // DESCENDRE la caméra sous l'altitude du sujet. Si la visée (le joueur)
    // est nettement plus haute que le sujet (la cible au sol), c'est
    // exactement ce qui arrive — la caméra s'enfonce sous le sol. On plafonne
    // axis.y à une petite valeur négative (pas 0) : la caméra reste toujours
    // un peu AU-DESSUS de l'altitude du sujet plutôt que pile dessus, ce qui
    // évite de raser visuellement le sol quand la cible y est posée. Le
    // plafond est une fraction de dist (déjà basé sur la taille du sujet),
    // pas une constante arbitraire, pour rester cohérent avec le reste.
    const float minUpwardAxisY = -0.1f;
    if (axis.y > minUpwardAxisY) {
        axis.y = minUpwardAxisY;
        axis.Normalize();
    }
    return axis;
}

const char *SCTargetCamera::debugLabel() const {
    return "[TARGET]";
}

bool SCTargetCamera::debugEnabled() const {
    return SCTargetCamera::s_debug;
}
