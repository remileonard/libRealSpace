#pragma once

#include "precomp.h"

class SCMission;

class SCMissionScene {
private:
    void runProg(int16_t prog_id);
public:
    SCMissionScene(SCMission *mission, MISN_SCEN *scene);
    ~SCMissionScene();
    void activate();
    void runMissionUpdate();
    void runLeaving();
    SCMission *mission{nullptr};
    MISN_SCEN *scene{nullptr};
};