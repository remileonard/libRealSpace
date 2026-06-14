#pragma once
#include "../engine/timer.h"
#include <openxr/openxr.h>
#include <chrono>
#include <thread>

#define TARGET_FPS 90.0f
class VRTimer : public Timer {
public:
    VRTimer() {
        m_lastXrTime = 0;
        m_lastFPSUpdate = 0;
        m_targetFrameTime = 1.0f / TARGET_FPS; // 30 FPS = 33.33ms par frame
        m_smoothedDeltaTime = m_targetFrameTime;
        m_framesSinceLastUpdate = 0;
        m_expectedFPS =TARGET_FPS;
        m_hadSpikeThisSecond = false;
        m_lastFrameStart = std::chrono::high_resolution_clock::now();
    }
    
    void updateWithXrTime(XrTime predictedDisplayTime) {
        // Mesure réelle pour le debug uniquement (pas de sleep : xrWaitFrame pace déjà)
        auto now = std::chrono::high_resolution_clock::now();
        m_realDeltaTime = std::chrono::duration<float>(now - m_lastFrameStart).count();
        m_lastFrameStart = now;

        // Première frame : initialisation
        if (m_lastXrTime == 0) {
            m_lastXrTime = predictedDisplayTime;
            m_lastFPSUpdate = predictedDisplayTime;
            m_deltaTime = m_targetFrameTime;
            m_smoothedDeltaTime = m_targetFrameTime;
            m_framesSinceLastUpdate = 0;
            return;
        }

        // Source de vérité : delta XR prédit par le runtime
        float rawDeltaTime = (float)(predictedDisplayTime - m_lastXrTime) / 1e9f;

        // Limiter les spikes : cap à 2 frames (couvre le cas ASW/reprojection)
        // En dessous d'un minimum aussi (cas de reset de session)
        const float maxDeltaTime = m_targetFrameTime * 2.5f;
        const float minDeltaTime = 0.001f;
        if (rawDeltaTime > maxDeltaTime || rawDeltaTime < minDeltaTime) {
            m_hadSpikeThisSecond = true;
            rawDeltaTime = m_targetFrameTime;
        }

        m_lastXrTime = predictedDisplayTime;
        m_framesSinceLastUpdate++;

        // Calcul FPS lissé sur 1 seconde (pour debug)
        float elapsedTimeSec = (float)(predictedDisplayTime - m_lastFPSUpdate) / 1e9f;
        if (elapsedTimeSec >= 1.0f) {
            float measuredFPS = (float)m_framesSinceLastUpdate / elapsedTimeSec;
            m_smoothedDeltaTime = (!m_hadSpikeThisSecond && measuredFPS > 15.0f)
                ? 1.0f / measuredFPS
                : m_targetFrameTime;
            m_lastFPSUpdate = predictedDisplayTime;
            m_framesSinceLastUpdate = 0;
            m_hadSpikeThisSecond = false;
        }

        m_deltaTime = rawDeltaTime;
        m_totalTime += m_deltaTime;
        m_accumulator += m_deltaTime;
        m_frameCount++;
    }
    
    void update() override {
        // Pas utilisé en mode VR
    }
    
    bool shouldSimulate() override {
        if (m_accumulator >= m_fixedDeltaTime) {
            m_accumulator -= m_fixedDeltaTime;
            return true;
        }
        return false;
    }
    
    float getDeltaTime() const override { return m_deltaTime; }
    double getTotalTime() const override { return m_totalTime; }
    uint32_t getFrameCount() const override { return m_frameCount; }
    float getFixedDeltaTime() const override { return m_fixedDeltaTime; }
    
    XrTime getLastXrTime() const { return m_lastXrTime; }
    float getSmoothedFPS() const { return 1.0f / m_smoothedDeltaTime; }
    float getTargetFPS() const { return m_expectedFPS; }
    float m_realDeltaTime;
private:
    XrTime m_lastXrTime;
    XrTime m_lastFPSUpdate;
    uint32_t m_framesSinceLastUpdate;
    float m_targetFrameTime;
    float m_smoothedDeltaTime;
    float m_expectedFPS;
    
    bool m_hadSpikeThisSecond;
    std::chrono::high_resolution_clock::time_point m_lastFrameStart;
};