#pragma once
#include "precomp.h"


struct SCMissionActors {
    std::string actor_name;
    uint8_t actor_id;
    RSProf *profile;
    MISN_PART *object;
    SCPlane *plane;
    SCPilot *pilot;
};

class SCMission {
private:
    std::string mission_name;
    RSArea *area{nullptr};
    RSMission *mission{nullptr};
    std::vector<std::vector<SCMissionActors *>> teams;
    std::map<std::string, RSEntity *> *obj_cache;
    SCMissionActors *player;
public:    
    SCMission(std::string mission_name, std::map<std::string, RSEntity *> *objCache);
    ~SCMission();
    void LoadMission();
    void SCMission::cleanup()
};