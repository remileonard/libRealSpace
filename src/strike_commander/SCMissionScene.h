#pragma once

#include "precomp.h"

class SCMission;

class SCMissionScene {
private:
    MessageBus::SubscriptionId subscription_id;
    void onEvent(const EventMessage &event);
    void onMissionUpdate(const MissionUpdateEvent &event);
    void onSceneActivated(const MissionEventSceneActivated &event);
public:
    SCMissionScene(SCMission *mission, MISN_SCEN *scene);
    ~SCMissionScene();
    SCMission *mission{nullptr};
    MISN_SCEN *scene{nullptr};
};