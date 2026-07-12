#pragma once
#include "../engine/timer.h"
#include <openxr/openxr.h>
#include <chrono>

#define TARGET_FPS 90.0f

// Interface minimale implémentée par VRScreen pour piloter le cycle de frame XR.
// Permet au VRTimer de déclencher xrWaitFrame/xrBeginFrame sans dépendre du type concret.
class IVRFrameDriver {
public:
    virtual ~IVRFrameDriver() = default;
    // Démarre l'unique frame XR du tick (wait + begin).
    // Renvoie true si une frame a été démarrée. Remplit predictedDisplayTime/shouldRender.
    virtual bool xrBeginTickFrame(XrTime& predictedDisplayTime, bool& shouldRender) = 0;
    // True si une session XR est active (sinon le timer retombe sur l'horloge système).
    virtual bool xrSessionActive() const = 0;
};

class VRTimer : public Timer {
public:
    VRTimer() {
        m_lastXrTime = 0;
        m_lastFPSUpdate = 0;
        m_targetFrameTime = 1.0f / TARGET_FPS;
        m_smoothedDeltaTime = m_targetFrameTime;
        m_framesSinceLastUpdate = 0;
        m_expectedFPS = TARGET_FPS;
        m_hadSpikeThisSecond = false;
        m_lastFrameStart = std::chrono::high_resolution_clock::now();
    }

    void setFrameDriver(IVRFrameDriver* driver) { m_driver = driver; }

    // Accès pour VRScreen::refresh() : temps prédit + faut-il rendre.
    XrTime getPredictedDisplayTime() const { return m_predictedDisplayTime; }
    bool   frameStarted() const { return m_frameStarted; }
    bool   shouldRender() const { return m_shouldRender; }
    // VRScreen marque la frame comme présentée (après xrEndFrame).
    void   markFramePresented() { m_frameStarted = false; }

    // ─────────────────────────────────────────────────────────────────────────
    // POINT CENTRAL : appelé en tête de boucle par GameTimer::update().
    // C'est ICI que xrWaitFrame est invoqué, une seule fois par tick.
    // ─────────────────────────────────────────────────────────────────────────
    void update() override {
        // Mesure réelle (debug uniquement).
        auto now = std::chrono::high_resolution_clock::now();
        m_realDeltaTime = std::chrono::duration<float>(now - m_lastFrameStart).count();
        m_lastFrameStart = now;

        m_frameStarted = false;
        m_shouldRender = false;

        // Si pas de session XR active, on retombe sur un pacing temporel simple.
        if (!m_driver || !m_driver->xrSessionActive()) {
            advanceWithDelta(m_targetFrameTime);
            return;
        }

        // xrWaitFrame + xrBeginFrame (bloque jusqu'au créneau d'affichage).
        XrTime predicted = 0;
        bool shouldRender = false;
        if (!m_driver->xrBeginTickFrame(predicted, shouldRender)) {
            // Frame non démarrée (session en transition) : pacing de secours.
            advanceWithDelta(m_targetFrameTime);
            return;
        }

        m_frameStarted = true;
        m_shouldRender = shouldRender;
        m_predictedDisplayTime = predicted;

        // Cale la simulation sur le delta XR prédit.
        applyXrTime(predicted);
    }

    // Conservé pour compat avec le code existant (refresh appelait updateWithXrTime).
    // Désormais, le calcul de delta passe par applyXrTime() depuis update().
    void updateWithXrTime(XrTime predictedDisplayTime) {
        applyXrTime(predictedDisplayTime);
    }

    bool shouldSimulate() override {
        if (m_accumulator >= m_fixedDeltaTime) {
            m_accumulator -= m_fixedDeltaTime;
            return true;
        }
        return false;
    }

    float    getDeltaTime() const override { return m_deltaTime; }
    double   getTotalTime() const override { return m_totalTime; }
    uint32_t getFrameCount() const override { return m_frameCount; }
    float    getFixedDeltaTime() const override { return m_fixedDeltaTime; }

    XrTime getLastXrTime() const { return m_lastXrTime; }
    float  getSmoothedFPS() const { return 1.0f / m_smoothedDeltaTime; }
    float  getTargetFPS() const { return m_expectedFPS; }
    float  m_realDeltaTime;

private:
    // Avance la simulation à partir d'un delta déjà calculé (chemin non-XR).
    void advanceWithDelta(float dt) {
        m_deltaTime = dt;
        m_totalTime += dt;
        m_accumulator += dt;
        m_frameCount++;
    }

    // Calcule le delta à partir du temps XR prédit + lissage FPS (debug).
    void applyXrTime(XrTime predictedDisplayTime) {
        if (m_lastXrTime == 0) {
            m_lastXrTime = predictedDisplayTime;
            m_lastFPSUpdate = predictedDisplayTime;
            m_deltaTime = m_targetFrameTime;
            m_smoothedDeltaTime = m_targetFrameTime;
            m_framesSinceLastUpdate = 0;
            m_totalTime += m_deltaTime;
            m_accumulator += m_deltaTime;
            m_frameCount++;
            return;
        }

        float rawDeltaTime = (float)(predictedDisplayTime - m_lastXrTime) / 1e9f;

        // Cap des spikes (ASW/reprojection) et plancher (reset session).
        const float maxDeltaTime = m_targetFrameTime * 2.5f;
        const float minDeltaTime = 0.001f;
        if (rawDeltaTime > maxDeltaTime || rawDeltaTime < minDeltaTime) {
            m_hadSpikeThisSecond = true;
            rawDeltaTime = m_targetFrameTime;
        }

        m_lastXrTime = predictedDisplayTime;
        m_framesSinceLastUpdate++;

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

    // État du cycle de frame piloté par le timer.
    IVRFrameDriver* m_driver{nullptr};
    XrTime m_predictedDisplayTime{0};
    bool   m_frameStarted{false};
    bool   m_shouldRender{false};

    XrTime m_lastXrTime;
    XrTime m_lastFPSUpdate;
    uint32_t m_framesSinceLastUpdate;
    float m_targetFrameTime;
    float m_smoothedDeltaTime;
    float m_expectedFPS;

    bool m_hadSpikeThisSecond;
    std::chrono::high_resolution_clock::time_point m_lastFrameStart;
};