#pragma once
#include "../strike_commander/SCGameFlow.h"

class EnhancedGameFlow : public SCGameFlow {
public:
    EnhancedGameFlow() = default;
    void flyMission(uint8_t missionid) override;
};
