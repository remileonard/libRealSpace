#pragma once
#include "../engine/timer.h"
#include <openxr/openxr.h>

class VRTimer : public Timer {
public:
    VRTimer() {
        m_lastXrTime = 0;
        m_lastFPSUpdate = 0;
        m_smoothedDeltaTime = 1.0f / 90.0f; // VR est généralement à 90 ou 120 FPS
        m_framesSinceLastUpdate = 0;
        m_expectedFPS = 90.0f;
        m_hadSpikeThisSecond = false;
    }
    
    void updateWithXrTime(XrTime predictedDisplayTime) {
        if (m_lastXrTime == 0) {
            m_lastXrTime = predictedDisplayTime;
            m_lastFPSUpdate = predictedDisplayTime;
            m_deltaTime = m_smoothedDeltaTime;
            return;
        }
        
        // XrTime est en nanosecondes
        float rawDeltaTime = (float)(predictedDisplayTime - m_lastXrTime) / 1e9f;
        
        // Détecter les spikes : plus de 3x le temps normal attendu
        const float maxReasonableDeltaTime = m_smoothedDeltaTime * 3.0f;
        if (rawDeltaTime > maxReasonableDeltaTime) {
            // Frame anormale détectée
            m_hadSpikeThisSecond = true;
            rawDeltaTime = m_smoothedDeltaTime;
        }
        
        const float maxDeltaTime = 0.25f;
        if (rawDeltaTime > maxDeltaTime) {
            rawDeltaTime = maxDeltaTime;
        }
        
        m_lastXrTime = predictedDisplayTime;
        m_framesSinceLastUpdate++;
        
        // Recalculer le deltaTime moyenné toutes les secondes (1e9 nanosecondes)
        uint64_t timeSinceLastFPSUpdate = predictedDisplayTime - m_lastFPSUpdate;
        if (timeSinceLastFPSUpdate >= 1000000000) { // 1 seconde en nanosecondes
            // Calculer le FPS moyen sur la dernière seconde
            float avgFPS = (float)m_framesSinceLastUpdate / ((float)timeSinceLastFPSUpdate / 1e9f);
            
            // Ne mettre à jour que si :
            // 1. Pas de spike pendant cette période
            // 2. FPS raisonnable (pas trop bas, pas trop haut)
            // 3. Pas trop loin du FPS attendu (tolérance de 50%)
            bool isReasonable = avgFPS >= 30.0f && avgFPS <= 200.0f;
            bool isCloseToExpected = avgFPS > (m_expectedFPS * 0.5f);
            
            if (!m_hadSpikeThisSecond && isReasonable && isCloseToExpected) {
                m_smoothedDeltaTime = 1.0f / avgFPS;
                m_expectedFPS = avgFPS;
            }
            // Sinon on garde l'ancien smoothedDeltaTime
            
            m_lastFPSUpdate = predictedDisplayTime;
            m_framesSinceLastUpdate = 0;
            m_hadSpikeThisSecond = false;
        }
        
        m_deltaTime = m_smoothedDeltaTime;
        m_totalTime += m_deltaTime;
        m_accumulator += m_deltaTime;
        m_frameCount++;
    }
    
    void update() override {
        // Pas utilisé en mode VR, on utilise updateWithXrTime
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
    
private:
    XrTime m_lastXrTime;
    XrTime m_lastFPSUpdate;
    uint32_t m_framesSinceLastUpdate;
    float m_smoothedDeltaTime;
    float m_expectedFPS;
    bool m_hadSpikeThisSecond;
};