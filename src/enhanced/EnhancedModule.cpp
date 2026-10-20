#include "EnhancedModule.h"
#include "../strike_commander/SCFactory.h"
#include "EnhancedEntity.h"
#include "EnhancedGameFlow.h"
#include "EnhancedStrike.h"
#include "EnhancedMission.h"
#include "EnhancedCockpit.h"

void EnhancedModule::registerAll() {
    RSEntity::factory = []() -> RSEntity * { return new EnhancedEntity(); };
    SCFactory::gameFlow = []() -> SCGameFlow * { return new EnhancedGameFlow(); };
    SCFactory::strike = []() -> SCStrike * { return new EnhancedStrike(); };
    SCFactory::mission = [](const std::string &name, std::unordered_map<std::string, RSEntity *> *cache) -> SCMission * {
        return new EnhancedMission(name, cache);
    };
    SCFactory::cockpit = []() -> SCCockpit * { return new EnhancedCockpit(); };
}
