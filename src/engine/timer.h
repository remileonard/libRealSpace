#ifndef TIMER_H
#define TIMER_H

#include <cstdint>

class Timer {
public:
    virtual ~Timer() = default;
    
    // Méthodes principales
    virtual void update() = 0;
    virtual float getDeltaTime() const = 0;
    virtual double getTotalTime() const = 0;
    virtual uint32_t getFrameCount() const = 0;
    
    // Pour la simulation à timestep fixe
    virtual bool shouldSimulate() = 0;
    virtual float getFixedDeltaTime() const = 0;
    static float snapToStandardFPS(float measured) {
        // Arrondi aux valeurs standard
        if (measured >= 50.0f) return 60.0f;
        if (measured >= 22.0f) return 30.0f;
        if (measured >= 17.0f) return 20.0f;
        return 15.0f;
    };
protected:
    float m_deltaTime = 0.0f;
    float m_fixedDeltaTime = 1.0f / 30.0f;
    float m_accumulator = 0.0f;
    double m_totalTime = 0.0;
    uint32_t m_frameCount = 0;
    int m_warmupFrames = 0;        // nouveau
    float m_warmupAccum = 0.0f;      // nouveau
    bool m_calibrated = false;
};

#endif // TIMER_H