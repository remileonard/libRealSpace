#pragma once
#include "DebugStrike.h"
#include "../../pacific_strike/PSStrike.h"

class DebugPacificStrikeMISN : public DebugStrike, public PSStrike {
protected:
    void renderMissionLoadingDialog(bool &show_mission) override;
public:
    DebugPacificStrikeMISN();
    ~DebugPacificStrikeMISN();
    void setMission(char const *missionName) override;
    
    virtual void init() override { DebugStrike::init(); this->verticalOffset = 0.0f; this->eye_y = 0;}
    virtual void renderMenu() override { DebugStrike::renderMenu(); }
    virtual void renderUI() override { DebugStrike::renderUI(); }
    virtual void runFrame(void) override { DebugStrike::runFrame(); }
    void firePrimary() override;
    void selectNextWeapon() override;
};  
