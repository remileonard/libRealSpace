#pragma once
#include <string>
#include <unordered_map>
#include "../strike_commander/SCMission.h"

// Extension point for the mission. Intentionally empty for now.
class EnhancedMission : public SCMission {
public:
    EnhancedMission(const std::string &name, std::unordered_map<std::string, RSEntity *> *objCache) : SCMission(name, objCache) {}
};
