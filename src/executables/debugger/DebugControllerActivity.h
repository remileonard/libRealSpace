#pragma once
#include <memory>
#include <vector>
#include <utility>
#include <SDL.h>
#include "../../strike_commander/precomp.h"

class DebugControllerActivity : public IActivity {
public:
    DebugControllerActivity();
    ~DebugControllerActivity();
    void init(void) override;
    void runFrame(void) override;
    void renderMenu(void) override;
    void renderUI(void) override;

private:
    Keyboard* m_keyboard = nullptr;

    // Saisie de texte via Keyboard
    std::shared_ptr<Keyboard::TextEditor> m_textEditor;
    bool m_textInputEnabled = false;

    // Historique des touches "just pressed"
    struct KeyHit { SDL_Scancode sc; float ttl; };
    std::vector<KeyHit> m_recentKeys;

    // Timing
    double m_lastTimeSec = 0.0;

    // Souris (absolu)
    int m_mouseX = 0;
    int m_mouseY = 0;

    // ===== Debug manette via actions Keyboard =====
    enum class DebugAct : int {
        GP_LX = 10000, GP_LY, GP_RX, GP_RY, GP_LT, GP_RT,
        GP_A, GP_B, GP_X, GP_Y, GP_LB, GP_RB, GP_BACK, GP_START, GP_LS, GP_RS,
        GP_DU, GP_DD, GP_DL, GP_DR
    };

    struct AxisBinding {
        const char* label;
        DebugAct act;
        SDL_GameControllerAxis axis;
        float scale;
        bool zeroToOne;
        float value = 0.0f;
    };
    struct ButtonBinding {
        const char* label;
        DebugAct act;
        SDL_GameControllerButton btn;
        bool pressed = false;
    };

    int m_padIndex = 0;
    std::vector<AxisBinding> m_axes;
    std::vector<ButtonBinding> m_buttons;

    void setupGamepadDebugBindings();

    // ===== Carré déplacé par le joystick =====
    float m_boxX = 160.0f;   // centre initial (320x200)
    float m_boxY = 100.0f;
    float r_boxX = 160.0f;   // centre initial (320x200)
    float r_boxY = 100.0f;
    int   m_boxSize = 16;    // taille du carré
    float m_boxSpeed = 160.0f; // pixels/sec
    uint8_t m_boxColor = 12; // index palette
};