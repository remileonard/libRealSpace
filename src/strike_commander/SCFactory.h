#pragma once
#include <functional>
#include <string>
#include <unordered_map>

class SCGameFlow;
class SCStrike;
class SCMission;
class SCCockpit;
class RSEntity;

// Extension points: where the game creates its main objects it asks this factory, which returns the plain
// original class unless a derived class was registered (see src/enhanced).
class SCFactory {
public:
    static std::function<SCGameFlow *()> gameFlow;
    static std::function<SCStrike *()> strike;
    static std::function<SCMission *(const std::string &, std::unordered_map<std::string, RSEntity *> *)> mission;
    static std::function<SCCockpit *()> cockpit;

    static SCGameFlow *createGameFlow();
    static SCStrike *createStrike();
    static SCMission *createMission(const std::string &name, std::unordered_map<std::string, RSEntity *> *objCache);
    static SCCockpit *createCockpit();
};
