#include "DebugControllerActivity.h"
#include <algorithm>
#include <chrono>
#include "imgui.h"

using Clock = std::chrono::steady_clock;

using Clock = std::chrono::steady_clock;

DebugControllerActivity::DebugControllerActivity() {}

DebugControllerActivity::~DebugControllerActivity() {}

void DebugControllerActivity::init(void) {
        if (Game == nullptr) {
        Game = &GameEngine::getInstance();
    }
    this->m_keyboard = Game->getKeyboard();
    for (int sc = 0; sc < SDL_NUM_SCANCODES; ++sc) {
        m_keyboard->bindKeyToAction(CreateAction(InputAction::KEYBOARD_TEST,sc), static_cast<SDL_Scancode>(sc));
    }
    // Editeur de texte via Keyboard
    m_textEditor = m_keyboard->createTextEditor();
    m_textEditor->setText("");
    m_textInputEnabled = false;
    m_textEditor->setActive(false, nullptr);

    // Axes/boutons manette -> actions Keyboard
    setupGamepadDebugBindings();

    // Init timer
    m_lastTimeSec = std::chrono::duration<double>(Clock::now().time_since_epoch()).count();

    // Position initiale du carré au centre
    m_boxX = 160.0f;
    m_boxY = 100.0f;
}

void DebugControllerActivity::runFrame(void) {
    // Delta time
    double nowSec = std::chrono::duration<double>(Clock::now().time_since_epoch()).count();
    float dt = static_cast<float>(nowSec - m_lastTimeSec);
    m_lastTimeSec = nowSec;


    // Souris absolue
    m_keyboard->getMouseAbsolutePosition(m_mouseX, m_mouseY);

    // Collecte des touches "just pressed" (toutes les scancodes SDL)
    for (int sc = 0; sc < SDL_NUM_SCANCODES; ++sc) {
        if (m_keyboard->isActionJustPressed(CreateAction(InputAction::KEYBOARD_TEST,sc))) {
            m_recentKeys.push_back({ static_cast<SDL_Scancode>(sc), 1.0f }); // visible ~1s
        }
    }
    for (auto& k : m_recentKeys) k.ttl -= dt;
    m_recentKeys.erase(
        std::remove_if(m_recentKeys.begin(), m_recentKeys.end(),
                       [](const KeyHit& k) { return k.ttl <= 0.0f; }),
        m_recentKeys.end());

    // Mise à jour des axes/boutons manette via actions
    for (auto& a : m_axes) {
        auto act = static_cast<InputAction>(static_cast<int>(a.act));
        float v = m_keyboard->getActionValue(act); // -1..1 sticks, 0..1 triggers
        a.value = v;
    }
    for (auto& b : m_buttons) {
        auto act = static_cast<InputAction>(static_cast<int>(b.act));
        b.pressed = m_keyboard->isActionPressed(act);
    }

    // ===== Déplacement du carré avec le stick gauche =====
    float lx = m_keyboard->getActionValue(static_cast<InputAction>(static_cast<int>(DebugAct::GP_LX))); // [-1..1]
    float ly = m_keyboard->getActionValue(static_cast<InputAction>(static_cast<int>(DebugAct::GP_LY))); // [-1..1]

    float rx = m_keyboard->getActionValue(static_cast<InputAction>(static_cast<int>(DebugAct::GP_RX))); // [-1..1]
    float ry = m_keyboard->getActionValue(static_cast<InputAction>(static_cast<int>(DebugAct::GP_RY))); // [-1..1]

    // Inversion de Y pour correspondre aux coordonnées écran (haut = -)
    m_boxX += lx * m_boxSpeed * dt;
    m_boxY -= ly * m_boxSpeed * dt;

    r_boxX += rx * m_boxSpeed * dt;
    r_boxY -= ry * m_boxSpeed * dt;

    // Clamp dans l’écran 320x200 (ajuste si ton FrameBuffer a d’autres dimensions)
    if (m_boxX-m_boxSize/2 < 0) m_boxX = 0+m_boxSize/2;
    if (m_boxY-m_boxSize/2 < 0) m_boxY = 0+m_boxSize/2;
    if (m_boxX+m_boxSize/2 > 320) m_boxX = 320.0f-m_boxSize/2;
    if (m_boxY+m_boxSize/2 > 200) m_boxY = 200.0f-m_boxSize/2;

    if (r_boxX-m_boxSize/2 < 0) r_boxX = 0+m_boxSize/2;
    if (r_boxY-m_boxSize/2 < 0) r_boxY = 0+m_boxSize/2;
    if (r_boxX+m_boxSize/2 > 320) r_boxX = 320.0f-m_boxSize/2;
    if (r_boxY+m_boxSize/2 > 200) r_boxY = 200.0f-m_boxSize/2;

    VGA.activate();
    VGA.getFrameBuffer()->fillWithColor(0);

    VGA.getFrameBuffer()->rect_slow((int)m_boxX-m_boxSize/2, (int)m_boxY-m_boxSize/2, (int)m_boxX+m_boxSize/2,  (int)m_boxY+m_boxSize/2, m_boxColor);
    VGA.getFrameBuffer()->circle_slow((int)r_boxX, (int)r_boxY, m_boxSize/2, 14);
    VGA.vSync();
}

void DebugControllerActivity::renderMenu(void) {}

void DebugControllerActivity::renderUI(void) {
    if (ImGui::BeginTabBar("Input Debug Tabs")) {
        if (ImGui::BeginTabItem("Inputs")) {
            // Souris (absolue)
            ImGui::Text("Souris (absolu): X=%d  Y=%d", m_mouseX, m_mouseY);

            // Touches récemment appuyées (via isKeyJustPressed)
            ImGui::SeparatorText("Touches (dernieres ~1s)");
            ImGui::BeginChild("recent_keys", ImVec2(0, 120), true);
            int col = 0;
            for (const auto& k : m_recentKeys) {
                const char* name = SDL_GetScancodeName(k.sc);
                if (name == nullptr || name[0] == '\0') name = "Unknown";
                ImGui::TextUnformatted(name);
                if (++col % 6 != 0) ImGui::SameLine(0.0f, 12.0f);
            }
            ImGui::EndChild();

            // Saisie de texte (Keyboard::TextEditor)
            ImGui::SeparatorText("Saisie de texte (Keyboard)");
            if (ImGui::Checkbox("Activer la saisie", &m_textInputEnabled)) {
                m_textEditor->setActive(m_textInputEnabled, nullptr);
            }
            ImGui::Text("Texte: %s", m_textEditor->getDisplayText().c_str());

            // Deadzone joystick (via Keyboard)
            float dz = m_keyboard->getJoystickDeadzone();
            if (ImGui::SliderFloat("Deadzone joystick", &dz, 0.0f, 1.0f, "%.2f")) {
                m_keyboard->setJoystickDeadzone(dz);
            }

            // Manette (axes et boutons) via actions Keyboard
            ImGui::SeparatorText("Manette - Axes");
            for (const auto& a : m_axes) {
                float norm = a.zeroToOne ? a.value : (a.value + 1.0f) * 0.5f; // [0..1]
                ImGui::Text("%s: %.2f", a.label, a.value);
                ImGui::SameLine();
                ImGui::ProgressBar(norm, ImVec2(200, 0));
            }

            ImGui::SeparatorText("Manette - Boutons (presses)");
            ImGui::BeginChild("pad_buttons", ImVec2(0, 100), true);
            int bcol = 0;
            for (const auto& b : m_buttons) {
                ImGui::Text("%s: %s", b.label, b.pressed ? "ON" : "OFF");
                if (++bcol % 4 != 0) ImGui::SameLine(0.0f, 16.0f);
            }
            ImGui::EndChild();
        ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

void DebugControllerActivity::setupGamepadDebugBindings() {
    // Définition des axes que l’on veut suivre
    m_axes = {
        { "LX", DebugAct::GP_LX, SDL_CONTROLLER_AXIS_LEFTX,  1.0f, false },
        { "LY", DebugAct::GP_LY, SDL_CONTROLLER_AXIS_LEFTY, -1.0f, false },
        { "RX", DebugAct::GP_RX, SDL_CONTROLLER_AXIS_RIGHTX, 1.0f, false },
        { "RY", DebugAct::GP_RY, SDL_CONTROLLER_AXIS_RIGHTY,-1.0f, false },
        { "LT", DebugAct::GP_LT, SDL_CONTROLLER_AXIS_TRIGGERLEFT,  1.0f, true },
        { "RT", DebugAct::GP_RT, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, 1.0f, true },
    };

    // Définition des boutons
    m_buttons = {
        { "A",     DebugAct::GP_A,     SDL_CONTROLLER_BUTTON_A,     false },
        { "B",     DebugAct::GP_B,     SDL_CONTROLLER_BUTTON_B,     false },
        { "X",     DebugAct::GP_X,     SDL_CONTROLLER_BUTTON_X,     false },
        { "Y",     DebugAct::GP_Y,     SDL_CONTROLLER_BUTTON_Y,     false },
        { "LB",    DebugAct::GP_LB,    SDL_CONTROLLER_BUTTON_LEFTSHOULDER,  false },
        { "RB",    DebugAct::GP_RB,    SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, false },
        { "Back",  DebugAct::GP_BACK,  SDL_CONTROLLER_BUTTON_BACK,  false },
        { "Start", DebugAct::GP_START, SDL_CONTROLLER_BUTTON_START, false },
        { "LS",    DebugAct::GP_LS,    SDL_CONTROLLER_BUTTON_LEFTSTICK,  false },
        { "RS",    DebugAct::GP_RS,    SDL_CONTROLLER_BUTTON_RIGHTSTICK, false },
        { "Up",    DebugAct::GP_DU,    SDL_CONTROLLER_BUTTON_DPAD_UP,    false },
        { "Down",  DebugAct::GP_DD,    SDL_CONTROLLER_BUTTON_DPAD_DOWN,  false },
        { "Left",  DebugAct::GP_DL,    SDL_CONTROLLER_BUTTON_DPAD_LEFT,  false },
        { "Right", DebugAct::GP_DR,    SDL_CONTROLLER_BUTTON_DPAD_RIGHT, false },
    };

    // Enregistrer et binder toutes les actions via Keyboard
    for (const auto& a : m_axes) {
        auto act = static_cast<InputAction>(static_cast<int>(a.act));
        m_keyboard->registerAction(act);
        m_keyboard->bindGamepadAxisToAction(act, m_padIndex, a.axis, a.scale);
    }
    for (const auto& b : m_buttons) {
        auto act = static_cast<InputAction>(static_cast<int>(b.act));
        m_keyboard->registerAction(act);
        m_keyboard->bindGamepadButtonToAction(act, m_padIndex, b.btn);
    }
}