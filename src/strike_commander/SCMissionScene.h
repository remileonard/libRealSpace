#pragma once

#include "precomp.h"

class SCMission;

class SCMissionScene {
private:
    MessageBus::SubscriptionId subscription_id{ -1 };
    void onEvent(const EventMessage &event);
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