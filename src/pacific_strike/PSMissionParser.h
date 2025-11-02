#pragma once
#include "../realspace/RSMission.h"

class PSMissionParser : public RSMission {
protected:
    void paseMissionScript(uint8_t *data, size_t size) override;
public:
    PSMissionParser();
    ~PSMissionParser();
};