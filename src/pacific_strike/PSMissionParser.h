#pragma once
#include "../realspace/RSMission.h"

class PSMissionParser : public RSMission {
protected:
    void paseMissionScript(uint8_t *data, size_t size) override;
    static bool isFlagInstruction(uint8_t opcode);

public:
    PSMissionParser();
    ~PSMissionParser();
};