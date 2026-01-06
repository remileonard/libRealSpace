#pragma once
#include "../engine/timer.h"
#include <openxr/openxr.h>
#include <chrono>
#include <thread>

class VRTimer : public Timer {
public:
    VRTimer() {
        m_lastXrTime = 0;
        m_lastFPSUpdate = 0;
        m_targetFrameTime = 1.0f / 30.0f; // 30 FPS = 33.33ms par frame
        m_smoothedDeltaTime = m_targetFrameTime;
        m_framesSinceLastUpdate = 0;
        m_expectedFPS = 30.0f;
        m_hadSpikeThisSecond = false;
        m_lastFrameStart = std::chrono::high_resolution_clock::now();
    }
    
    void updateWithXrTime(XrTime predictedDisplayTime) {
        // Attendre pour atteindre 30 FPS avec précision
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = now - m_lastFrameStart;
        float elapsedSeconds = elapsed.count();
        
        // Si on est en avance, on attend
        if (elapsedSeconds < m_targetFrameTime) {
            float remainingTime = m_targetFrameTime - elapsedSeconds;
            
            // Sleep pour la majorité du temps (moins précis mais économe en CPU)
            if (remainingTime > 0.002f) { // 2ms de marge
                std::this_thread::sleep_for(std::chrono::duration<float>(remainingTime - 0.002f));
            }
            
            // Spin-wait pour les dernières millisecondes (précis mais utilise CPU)
            do {
                now = std::chrono::high_resolution_clock::now();
                elapsed = now - m_lastFrameStart;
                elapsedSeconds = elapsed.count();
            } while (elapsedSeconds < m_targetFrameTime);
        }
        
        // Calculer le vrai deltaTime mesuré
        now = std::chrono::high_resolution_clock::now();
        elapsed = now - m_lastFrameStart;
        float rawDeltaTime = elapsed.count();
        
        // Clamp pour éviter les valeurs aberrantes
        const float minFrameTime = 0.001f; // 1ms min
        const float maxFrameTime = 0.1f;   // 100ms max
        if (rawDeltaTime < minFrameTime || rawDeltaTime > maxFrameTime) {
            rawDeltaTime = m_targetFrameTime;
        }
        
        m_lastFrameStart = now;
        
        // Initialisation
        if (m_lastXrTime == 0) {
            m_lastXrTime = predictedDisplayTime;
            m_lastFPSUpdate = predictedDisplayTime;
            m_deltaTime = m_targetFrameTime; // Utiliser la valeur cible
            m_smoothedDeltaTime = m_targetFrameTime;
            m_framesSinceLastUpdate = 0;
            return;
        }
        
        // Détecter les spikes : plus de 50% au-dessus du temps cible
        const float maxReasonableDeltaTime = m_targetFrameTime * 1.5f;
        if (rawDeltaTime > maxReasonableDeltaTime) {
            m_hadSpikeThisSecond = true;
            rawDeltaTime = m_targetFrameTime;
        }
        
        m_lastXrTime = predictedDisplayTime;
        m_framesSinceLastUpdate++;
        
        // Lissage sur une seconde
        XrTime elapsed_ns = predictedDisplayTime - m_lastFPSUpdate;
        float elapsedTimeSec = (float)elapsed_ns / 1e9f;
        
        if (elapsedTimeSec >= 1.0f) {
            // Calcul du FPS réel mesuré sur 1 seconde
            float measuredFPS = (float)m_framesSinceLastUpdate / elapsedTimeSec;
            
            // Si on a eu un spike cette seconde, on garde l'ancien smoothedDeltaTime
            if (!m_hadSpikeThisSecond && measuredFPS > 15.0f && measuredFPS < 60.0f) {
                // Seulement si le FPS mesuré est raisonnable
                m_smoothedDeltaTime = 1.0f / measuredFPS;
            } else {
                // Sinon on reste proche de la cible
                m_smoothedDeltaTime = m_targetFrameTime;
            }
            
            m_lastFPSUpdate = predictedDisplayTime;
            m_framesSinceLastUpdate = 0;
            m_hadSpikeThisSecond = false;
        }
        
        // Utiliser directement le targetFrameTime pour un deltaTime constant
        m_deltaTime = m_targetFrameTime;
        
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