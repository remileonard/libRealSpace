#pragma once
#include "DebugConvPlayer.h"
#include "../../pacific_strike/PSConvplayer.h"

class DebugPSConvPlayer : public DebugConvPlayer, public PSConvPlayer {
public:
    DebugPSConvPlayer();
    ~DebugPSConvPlayer();
    virtual void init() override { PSConvPlayer::init();}
    virtual void renderMenu() override { DebugConvPlayer::renderMenu(); }
    virtual void renderUI() override { DebugConvPlayer::renderUI(); }
    virtual void runFrame(void) override;
};