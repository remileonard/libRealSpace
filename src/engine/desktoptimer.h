#pragma once
#include "timer.h"
#include <SDL.h>

class DesktopTimer : public Timer {
public:
    DesktopTimer() {
        m_lastTime = SDL_GetTicks64();
        m_lastFPSUpdate = SDL_GetTicks64();
        m_smoothedDeltaTime = 1.0f / 60.0f;
        m_framesSinceLastUpdate = 0;
        m_expectedFPS = 60.0f;
        m_hadSpikeThisSecond = false;
    }
    
    void update() override {
        uint64_t currentTime = SDL_GetTicks64();
        float rawDeltaTime = (float)(currentTime - m_lastTime) / 1000.0f;
        
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
        
        m_lastTime = currentTime;
        m_framesSinceLastUpdate++;
        
        // Recalculer le deltaTime moyenné toutes les secondes
        uint64_t timeSinceLastFPSUpdate = currentTime - m_lastFPSUpdate;
        if (timeSinceLastFPSUpdate >= 1000) {
            // Calculer le FPS moyen sur la dernière seconde
            float avgFPS = (float)m_framesSinceLastUpdate / ((float)timeSinceLastFPSUpdate / 1000.0f);
            
            // Ne mettre à jour que si :
            // 1. Pas de spike pendant cette période
            // 2. FPS raisonnable (pas trop bas, pas trop haut)
            // 3. Pas trop loin du FPS attendu (tolérance de 50%)
            bool isReasonable = avgFPS >= 10.0f && avgFPS <= 200.0f;
            bool isCloseToExpected = avgFPS > (m_expectedFPS * 0.5f);
            
            if (!m_hadSpikeThisSecond && isReasonable && isCloseToExpected) {
                m_smoothedDeltaTime = 1.0f / avgFPS;
                m_expectedFPS = avgFPS;
            }
            // Sinon on garde l'ancien smoothedDeltaTime
            
            m_lastFPSUpdate = currentTime;
            m_framesSinceLastUpdate = 0;
            m_hadSpikeThisSecond = false;
        }
        
        m_deltaTime = m_smoothedDeltaTime;
        m_totalTime += m_deltaTime;
        m_accumulator += m_deltaTime;
        m_frameCount++;
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
    
private:
    uint64_t m_lastTime;
    uint64_t m_lastFPSUpdate;
    uint32_t m_framesSinceLastUpdate;
    float m_smoothedDeltaTime;
    float m_expectedFPS;
    bool m_hadSpikeThisSecond;
};