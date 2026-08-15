#pragma once
#include "precomp.h"

class SCMission;

class MissionUpdateEvent: public EventMessage {
public:
    std::string message;
    std::string objective;
    uint8_t area_id{0};
    uint8_t mission_update_id{0};
    SCMission *mission{nullptr};
};
class MissionEventActorHit: public EventMessage {
public:
    SCMissionActors *attacker{nullptr};
    SCMissionActors *target{nullptr};
    SCSimulatedObject *weapon{nullptr};
    SCMission *mission{nullptr};
};
class MissionEventSceneActivated: public EventMessage {
public:
    MISN_SCEN *scene{nullptr};
    SCMission *mission{nullptr};
};