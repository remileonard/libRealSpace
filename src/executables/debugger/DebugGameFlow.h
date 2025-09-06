#pragma once
#include "../../strike_commander/precomp.h"

class DebugGameFlow : public SCGameFlow {
protected:
    bool pause{false};
    void flyMission() override;
    void playConv(uint8_t convId) override;
    void renderGameState();
    void renderMissionInfos();
public:
    DebugGameFlow();
    ~DebugGameFlow();

    void renderMenu() override;
    void renderUI() override;
};
