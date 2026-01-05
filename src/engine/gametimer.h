#pragma once

#include "timer.h"
#include <memory>

class GameTimer {
public:
    static GameTimer& getInstance() {
        static GameTimer instance;
        return instance;
    }
    
    void setTimer(std::unique_ptr<Timer> timer) {
        m_timer = std::move(timer);
    }
    
    Timer* getTimer() const {
        return m_timer.get();
    }
    
    // Raccourcis pratiques
    void update() {
        if (m_timer) m_timer->update();
    }
    
    float getDeltaTime() const {
        return m_timer ? m_timer->getDeltaTime() : 0.0f;
    }
    
    bool shouldSimulate() {
        return m_timer ? m_timer->shouldSimulate() : false;
    }
    
    float getFixedDeltaTime() const {
        return m_timer ? m_timer->getFixedDeltaTime() : 1.0f / 30.0f;
    }
    
private:
    GameTimer() = default;
    std::unique_ptr<Timer> m_timer;
};
