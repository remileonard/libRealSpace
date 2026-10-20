#include "precomp.h"
#include "SCFactory.h"
#include "SCGameFlow.h"
#include "SCStrike.h"
#include "SCMission.h"
#include "SCCockpit.h"

std::function<SCGameFlow *()> SCFactory::gameFlow;
std::function<SCStrike *()> SCFactory::strike;
std::function<SCMission *(const std::string &, std::unordered_map<std::string, RSEntity *> *)> SCFactory::mission;
std::function<SCCockpit *()> SCFactory::cockpit;

SCGameFlow *SCFactory::createGameFlow() { return gameFlow ? gameFlow() : new SCGameFlow(); }
SCStrike *SCFactory::createStrike() { return strike ? strike() : new SCStrike(); }
SCMission *SCFactory::createMission(const std::string &name, std::unordered_map<std::string, RSEntity *> *objCache) {
    return mission ? mission(name, objCache) : new SCMission(name, objCache);
}
SCCockpit *SCFactory::createCockpit() { return cockpit ? cockpit() : new SCCockpit(); }
