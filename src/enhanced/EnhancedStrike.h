#pragma once
#include "../strike_commander/SCStrike.h"

// Extension point for the flight activity (effects, debug aids). Intentionally empty for now: derived behaviour is added here
// instead of in the original SCStrike.
class EnhancedStrike : public SCStrike {
public:
    EnhancedStrike() = default;
};
