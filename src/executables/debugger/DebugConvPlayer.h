#pragma once
#include "../../strike_commander/precomp.h"

class DebugConvPlayer : public virtual SCConvPlayer {
protected:
    bool paused{false};
    void CheckFrameExpired(void) override;
public:
    DebugConvPlayer();
    ~DebugConvPlayer();

    void renderMenu() override;
    void renderUI() override;
    void pauseToggle() { this->paused = !this->paused; };
};
