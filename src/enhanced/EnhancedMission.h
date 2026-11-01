#pragma once
#include "../strike_commander/precomp.h"
#include <string>
#include <unordered_map>
#include "../strike_commander/SCMission.h"

class EnhancedMission : public SCMission {
public:
    EnhancedMission(const std::string &name, std::unordered_map<std::string, RSEntity *> *objCache);

protected:
    RSEntity *createEntity() override;
};
