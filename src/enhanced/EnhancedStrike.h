#pragma once
#include "../strike_commander/precomp.h"
#include "../strike_commander/SCStrike.h"

class EnhancedStrike : public SCStrike {
public:
    EnhancedStrike() = default;

protected:
    SCMission *createMission(char const *missionName) override;
};
