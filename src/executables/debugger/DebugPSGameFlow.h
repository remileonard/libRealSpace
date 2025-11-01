#pragma once
#include "../../strike_commander/precomp.h"
#include "DebugGameFlow.h"

class DebugPSGameFlow : public DebugGameFlow {
protected:
    void playConv(uint8_t convId) override;
    void playShot(uint8_t shotId) override;
    void flyMission() override;
    void renderConvPlayer() override;
    void playMapShot(std::vector<MAP_POINT *> *points) override;
public:
    void init() override;
    DebugPSGameFlow();
    ~DebugPSGameFlow();
};
