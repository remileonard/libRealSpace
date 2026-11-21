//
//  R4Timer.h
//  libRealSpace
//
//  The timer of neosc_r4: the time of each frame as the high resolution counter measures it, at any frame rate.
//  DesktopTimer snaps its rate to 60 at most, and keeps it when the rate goes above 200: the game time then runs faster than the real time.
//
#pragma once

#include "engine/timer.h"

#include <SDL.h>

class R4Timer : public Timer {
public:
    void update() override;
    float getDeltaTime() const override;
    double getTotalTime() const override;
    uint32_t getFrameCount() const override;
    bool shouldSimulate() override;
    float getFixedDeltaTime() const override;

private:
    Uint64 last_ = 0;
};
