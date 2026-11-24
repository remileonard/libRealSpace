#pragma once
#include "../../strike_commander/precomp.h"

class DebugGameFlow : public virtual SCGameFlow {
protected:
    bool pause{false};
    bool conv_player{false};
    void renderGameState();
    void renderMissionInfos();
    virtual void renderConvPlayer();
public:
    DebugGameFlow();
    ~DebugGameFlow();

    void renderMenu() override;
    void renderUI() override;
};
