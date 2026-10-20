#pragma once
#include "../strike_commander/SCCockpit.h"

// Extension point for the cockpit. Intentionally empty for now: derived behaviour is added here
// instead of in the original SCCockpit.
class EnhancedCockpit : public SCCockpit {
public:
    EnhancedCockpit() = default;
};
