#pragma once
#include "../strike_commander/SCGameFlow.h"

// Extension point for the game flow. Intentionally empty for now: derived behaviour is added here
// instead of in the original SCGameFlow.
class EnhancedGameFlow : public SCGameFlow {
public:
    EnhancedGameFlow() = default;
};
