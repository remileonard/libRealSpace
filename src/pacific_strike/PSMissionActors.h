#pragma once
#include "../strike_commander/precomp.h"

class PSMissionActors : public SCMissionActors {
public:
    bool respondToRadioMessage(int message_id, SCMission *mission, SCMissionActors *sender = nullptr) override;
    bool setObjective(prog_op command, uint8_t arg) override;

protected:
    void forcedMessage(uint8_t message);
    bool last_in_progress{ false };
};

// Tourelle portée par un avion ou un navire (enfant SWPN du chunk CHLD) : un SWPN qui suit son porteur. Elle n'est pas dans la liste
// des acteurs de la mission : ni visée, ni touchée, ni comptée par les scripts.
class PSMissionActorsTurret : public SCMissionActorsSwpn {
public:
    SCMissionActors *parent{ nullptr };
    // Enregistrement CHLD du porteur : décalage (nez = z, haut = y, droite = x) ; lacet positif vers la droite, tangage
    // positif vers le haut.
    RSEntity::CHLD *mount{ nullptr };
    void onMissionUpdate(const MissionUpdateEvent &event) override;

protected:
    bool inFiringArc(const Vector3D &to_target) override;

private:
    bool parentActive();
    Vector3D parentToWorld(Vector3D model);
    void giveShotsToParent();
};
