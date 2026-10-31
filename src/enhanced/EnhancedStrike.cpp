#include "../strike_commander/precomp.h"
#include "EnhancedStrike.h"
#include "EnhancedMission.h"

SCMission *EnhancedStrike::createMission(char const *missionName) {
    return new EnhancedMission(missionName, &object_cache);
}
