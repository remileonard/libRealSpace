#include "../strike_commander/precomp.h"
#include "EnhancedMission.h"
#include "EnhancedEntity.h"

EnhancedMission::EnhancedMission(const std::string &name, std::unordered_map<std::string, RSEntity *> *objCache)
    : SCMission() {
    // loadMission() creates entities through createEntity(), so it has to run here and not in the SCMission constructor
    this->mission_name = name;
    this->obj_cache = objCache;
    this->loadMission();
    this->subscription_id = this->messageBus.subscribeEvent(std::bind(&EnhancedMission::onEvent, this, std::placeholders::_1));
}

RSEntity *EnhancedMission::createEntity() {
    return new EnhancedEntity();
}
