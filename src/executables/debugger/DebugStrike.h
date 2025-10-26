#pragma once
#include "../../strike_commander/precomp.h"

class DebugStrike : public SCStrike {
protected:
    virtual void radar();
    virtual void simInfo();
    virtual void simConfig();
    virtual void loadPlane();
    virtual void showActorDetails(SCMissionActors* actor);
    virtual void showOffCamera();
    virtual void showTextures();
    virtual void renderMissionLoadingDialog(bool &show_mission);
    enum class DebugEntityMode { None, Actor, Scene, Area, Spot, Entity };
    DebugEntityMode debug_entity_mode{DebugEntityMode::Actor};
    RSEntity* debug_entity{nullptr};
    MISN_SCEN* debug_scene{nullptr};
    AREA* debug_area{nullptr};
    SPOT* debug_spot{nullptr};
    SCState &GameState = SCState::getInstance();
public:
    DebugStrike();
    ~DebugStrike();
    void init() override;
    virtual void setMission(std::string mission_name);
    virtual void renderMenu() override;
    virtual void renderUI() override;
    virtual void renderWorkingRegisters();
};