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
