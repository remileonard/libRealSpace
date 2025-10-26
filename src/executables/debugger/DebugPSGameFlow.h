#pragma once
#include "../../strike_commander/precomp.h"
#include "DebugGameFlow.h"

class DebugPSGameFlow : public DebugGameFlow {
protected:
    void playConv(uint8_t convId) override;
    void renderConvPlayer() override;
public:
    DebugPSGameFlow();
    ~DebugPSGameFlow();
};
