#pragma once
#include "../strike_commander/precomp.h"

// Acteur de Pacific Strike : réponse aux ordres radio du joueur (AI_PlayerRadioCommand_E3CA).
class PSMissionActors : public SCMissionActors {
public:
    bool respondToRadioMessage(int message_id, SCMission *mission, SCMissionActors *sender = nullptr) override;

protected:
    void forcedMessage(uint8_t message);
};
