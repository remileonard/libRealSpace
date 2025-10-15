#pragma once
#include "DebugStrike.h"
#include "../../pacific_strike/PSStrike.h"

class DebugPacificStrikeMISN : public DebugStrike, public PSStrike {
protected:
    void renderMissionLoadingDialog(bool &show_mission) override;
public:
    DebugPacificStrikeMISN();
    ~DebugPacificStrikeMISN();
    virtual void setMission(std::string mission_name);
    
    virtual void init() override { DebugStrike::init(); }
    virtual void renderMenu() override { DebugStrike::renderMenu(); }
    virtual void renderUI() override { DebugStrike::renderUI(); }
    virtual void runFrame(void) override { DebugStrike::runFrame(); }
};  
