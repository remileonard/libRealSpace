//
//  R4Timer.cpp
//  libRealSpace
//
#include "R4Timer.h"

namespace {

// a longer frame (a loading, a pause of the window) counts as this time, so the simulation does not jump
constexpr float kMaxDelta = 0.1f;
constexpr float kFirstDelta = 1.0f / 60.0f;

} // namespace

void R4Timer::update() {
    const Uint64 now = SDL_GetPerformanceCounter();
    float delta = kFirstDelta;
    if (last_ != 0) {
        delta = (float)((double)(now - last_) / (double)SDL_GetPerformanceFrequency());
    }
    last_ = now;
    m_deltaTime = delta > kMaxDelta ? kMaxDelta : delta;
    m_totalTime += m_deltaTime;
    m_accumulator += m_deltaTime;
    m_frameCount++;
}

float R4Timer::getDeltaTime() const {
    return m_deltaTime;
}

double R4Timer::getTotalTime() const {
    return m_totalTime;
}

uint32_t R4Timer::getFrameCount() const {
    return m_frameCount;
}

bool R4Timer::shouldSimulate() {
    if (m_accumulator >= m_fixedDeltaTime) {
        m_accumulator -= m_fixedDeltaTime;
        return true;
    }
    return false;
}

float R4Timer::getFixedDeltaTime() const {
    return m_fixedDeltaTime;
}
