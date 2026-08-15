#pragma once
#include "precomp.h"

struct SCAiPlane {
    SCPlane *plane{nullptr};
    SCPilot *pilot{nullptr};
    MISN_PART *object{nullptr};
    RSProf *ai{nullptr};
    std::string name;
};

struct SCMissionWaypoint {
    SPOT *spot{nullptr};
    std::string *message{nullptr};
    std::string *objective{nullptr};
};

struct RadioMessages {
    std::string message;
    MemSound *sound{nullptr};
};


class SCProg;

class SCMission {
public:
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
protected:
    std::string mission_name;
    std::unordered_map<std::string, RSEntity *> *obj_cache{nullptr};
    RSEntity * LoadEntity(std::string name);
    RSProf * LoadProfile(std::string name);
    uint32_t last_time{0};
    uint32_t last_tick{0};
    uint32_t tick_counter{0};
    uint32_t tps{0};
    SCState &GameState = SCState::getInstance();
    AssetManager &Assets = AssetManager::getInstance();
    SCRenderer &Renderer = SCRenderer::getInstance();
    RSMixer &Mixer = RSMixer::getInstance();
    MessageBus &messageBus = MessageBus::getInstance();
    void onMissionUpdate(const EventMessage &event);
public:    
    std::vector<SCMissionActors *> actors;
    std::vector<SCMissionActors *> enemies;
    std::vector<SCMissionActors *> friendlies;
    std::vector<SCMissionWaypoint *> waypoints;
    std::vector<SCExplosion *> explosions;
    std::unordered_map<uint16_t, int16_t> gameflow_registers;
    std::unordered_map<uint8_t, std::vector<uint8_t>> progs_traces;
    bool success{false};
    bool failure{false};
    bool mission_ended{false};
    uint8_t current_area_id{0};
    SCMissionActors *player{nullptr};
    RSArea *area{nullptr};
    RSMission *mission{nullptr};
    RSWorld *world{nullptr};
    RSSound &sound = RSSound::getInstance();
    bool in_combat{false};
    std::vector<RadioMessages*> radio_messages;
    bool mission_over{false};
    bool mission_won{false};
    SCMission();
    SCMission(std::string mission_name, std::unordered_map<std::string, RSEntity *> *objCache);
    ~SCMission();
    virtual void loadMission();
    void cleanup();
    virtual void update();
    void executeProg(std::vector<PROG> *prog);
    uint8_t getAreaID(Vector3D position);
};


