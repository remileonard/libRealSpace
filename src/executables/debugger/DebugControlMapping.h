#pragma once
#include "../../strike_commander/precomp.h"
#include <SDL.h>
#include <string>
#include <vector>

// Représente une action mappable affichée dans l'UI
struct MappableAction {
    const char*  label;       // Nom affiché
    InputAction  action;      // Action cible
    std::string  currentBinding; // Description du binding actuel (ex: "Joy0 Axis2")
};

// État de la capture en cours
struct CaptureState {
    bool         active      = false;
    int          actionIndex = -1;   // index dans m_actions
    float        timeout     = 5.0f; // secondes restantes
};

class DebugControlMapping : public IActivity {
public:
    DebugControlMapping();
    ~DebugControlMapping();

    void init() override;
    void runFrame() override;
    void renderUI() override;
    void renderMenu() override {}

private:
    Keyboard*                   m_keyboard   = nullptr;
    std::vector<SDL_Joystick*>  m_joysticks;           // joysticks ouverts
    std::vector<std::string>    m_joystickNames;
    std::vector<std::string>    m_joystickGUIDs;

    std::vector<MappableAction> m_actions;
    CaptureState                m_capture;

    // Snapshot des axes au début de la capture (pour détecter le mouvement)
    std::vector<std::vector<Sint16>> m_axisBaseline;

    double m_lastTimeSec = 0.0;

    void openJoysticks();
    void closeJoysticks();
    void buildActionList();
    void startCapture(int actionIndex);
    void updateCapture(float dt);
    void applyBinding(int actionIndex, int joyIndex, int axisOrBtn, bool isAxis, float scale);
    std::string describeBinding(InputAction action) const;

    // Seuil de détection d'un mouvement d'axe
    static constexpr Sint16 AXIS_THRESHOLD = 8000;
};