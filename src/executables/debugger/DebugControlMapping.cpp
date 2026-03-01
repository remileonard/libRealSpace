#include "DebugControlMapping.h"
#include "imgui.h"
#include <chrono>
#include <algorithm>

using Clock = std::chrono::steady_clock;

// ------------------------------------------------------------------ helpers --
static std::string getGUIDString(int sdlIndex) {
    SDL_JoystickGUID guid = SDL_JoystickGetDeviceGUID(sdlIndex);
    char buf[33];
    SDL_JoystickGetGUIDString(guid, buf, sizeof(buf));
    return buf;
}

// ------------------------------------------------------------------ init ----
DebugControlMapping::DebugControlMapping() {}
DebugControlMapping::~DebugControlMapping() { closeJoysticks(); }

void DebugControlMapping::init() {
    if (Game == nullptr) Game = &GameEngine::getInstance();
    m_keyboard = Game->getKeyboard();

    openJoysticks();
    buildActionList();

    m_lastTimeSec = std::chrono::duration<double>(
        Clock::now().time_since_epoch()).count();
}

void DebugControlMapping::openJoysticks() {
    closeJoysticks();
    int n = SDL_NumJoysticks();
    for (int i = 0; i < n; i++) {
        SDL_Joystick* joy = SDL_JoystickOpen(i);
        m_joysticks.push_back(joy);
        m_joystickNames.push_back(joy ? SDL_JoystickName(joy) : "???");
        m_joystickGUIDs.push_back(getGUIDString(i));
    }
}

void DebugControlMapping::closeJoysticks() {
    for (auto* joy : m_joysticks)
        if (joy) SDL_JoystickClose(joy);
    m_joysticks.clear();
    m_joystickNames.clear();
    m_joystickGUIDs.clear();
}

void DebugControlMapping::buildActionList() {
    // Liste des actions du simulateur que l'on veut mapper
    // Ajoutez/retirez selon vos besoins
    using S = SimActionOfst;
    m_actions = {
        // --- Vol ---
        { "Pitch Up",               CreateAction(InputAction::SIM_START, S::PITCH_UP)           },
        { "Pitch Down",             CreateAction(InputAction::SIM_START, S::PITCH_DOWN)         },
        { "Roll Left",              CreateAction(InputAction::SIM_START, S::ROLL_LEFT)          },
        { "Roll Right",             CreateAction(InputAction::SIM_START, S::ROLL_RIGHT)         },
        { "Throttle Up",            CreateAction(InputAction::SIM_START, S::THROTTLE_UP)        },
        { "Throttle Down",          CreateAction(InputAction::SIM_START, S::THROTTLE_DOWN)      },
        { "Throttle 10%",           CreateAction(InputAction::SIM_START, S::THROTTLE_10)        },
        { "Throttle 20%",           CreateAction(InputAction::SIM_START, S::THROTTLE_20)        },
        { "Throttle 30%",           CreateAction(InputAction::SIM_START, S::THROTTLE_30)        },
        { "Throttle 40%",           CreateAction(InputAction::SIM_START, S::THROTTLE_40)        },
        { "Throttle 50%",           CreateAction(InputAction::SIM_START, S::THROTTLE_50)        },
        { "Throttle 60%",           CreateAction(InputAction::SIM_START, S::THROTTLE_60)        },
        { "Throttle 70%",           CreateAction(InputAction::SIM_START, S::THROTTLE_70)        },
        { "Throttle 80%",           CreateAction(InputAction::SIM_START, S::THROTTLE_80)        },
        { "Throttle 90%",           CreateAction(InputAction::SIM_START, S::THROTTLE_90)        },
        { "Throttle 100%",          CreateAction(InputAction::SIM_START, S::THROTTLE_100)       },
        { "Autopilot",              CreateAction(InputAction::SIM_START, S::AUTOPILOT)          },
        { "Landing Gear",           CreateAction(InputAction::SIM_START, S::LANDING_GEAR)       },
        { "Toggle Brakes",          CreateAction(InputAction::SIM_START, S::TOGGLE_BRAKES)      },
        { "Toggle Flaps",           CreateAction(InputAction::SIM_START, S::TOGGLE_FLAPS)       },
        { "Toggle Mouse",           CreateAction(InputAction::SIM_START, S::TOGGLE_MOUSE)       },

        // --- Axes souris/stick ---
        { "Mouse X",                CreateAction(InputAction::SIM_START, S::MOUSE_X)            },
        { "Mouse Y",                CreateAction(InputAction::SIM_START, S::MOUSE_Y)            },
        { "Stick Left X",           CreateAction(InputAction::SIM_START, S::CONTROLLER_STICK_LEFT_X)  },
        { "Stick Left Y",           CreateAction(InputAction::SIM_START, S::CONTROLLER_STICK_LEFT_Y)  },
        { "Stick Right X",          CreateAction(InputAction::SIM_START, S::CONTROLLER_STICK_RIGHT_X) },
        { "Stick Right Y",          CreateAction(InputAction::SIM_START, S::CONTROLLER_STICK_RIGHT_Y) },

        // --- Combat ---
        { "Fire Primary",           CreateAction(InputAction::SIM_START, S::FIRE_PRIMARY)       },
        { "Target Nearest",         CreateAction(InputAction::SIM_START, S::TARGET_NEAREST)     },
        { "Chaff",                  CreateAction(InputAction::SIM_START, S::CHAFF)              },
        { "Flare",                  CreateAction(InputAction::SIM_START, S::FLARE)              },
        { "Eyes On Target",         CreateAction(InputAction::SIM_START, S::EYES_ON_TARGET)     },
        { "Single Target Mode",     CreateAction(InputAction::SIM_START, S::SINGLE_TARGET_MODE) },
        { "Weapon Mode Toggle",     CreateAction(InputAction::SIM_START, S::WEAPON_MODE_TOGGLE) },
        { "Infinite Ammo Toggle",   CreateAction(InputAction::SIM_START, S::INFINIT_AMMO_TOGGLE)},

        // --- Vues ---
        { "Look Left",              CreateAction(InputAction::SIM_START, S::LOOK_LEFT)          },
        { "Look Right",             CreateAction(InputAction::SIM_START, S::LOOK_RIGHT)         },
        { "Look Behind",            CreateAction(InputAction::SIM_START, S::LOOK_BEHIND)        },
        { "Look Forward",           CreateAction(InputAction::SIM_START, S::LOOK_FORWARD)       },
        { "View Cockpit",           CreateAction(InputAction::SIM_START, S::VIEW_COCKPIT)       },
        { "View Target",            CreateAction(InputAction::SIM_START, S::VIEW_TARGET)        },
        { "View Behind",            CreateAction(InputAction::SIM_START, S::VIEW_BEHIND)        },
        { "View Weapons",           CreateAction(InputAction::SIM_START, S::VIEW_WEAPONS)       },

        // --- MFD / Radar ---
        { "MDFS Radar",             CreateAction(InputAction::SIM_START, S::MDFS_RADAR)         },
        { "MDFS Damage",            CreateAction(InputAction::SIM_START, S::MDFS_DAMAGE)        },
        { "MDFS Weapons",           CreateAction(InputAction::SIM_START, S::MDFS_WEAPONS)       },
        { "MDFS Target Camera",     CreateAction(InputAction::SIM_START, S::MDFS_TARGET_CAMERA) },
        { "Radar Zoom In",          CreateAction(InputAction::SIM_START, S::RADAR_ZOOM_IN)      },
        { "Radar Zoom Out",         CreateAction(InputAction::SIM_START, S::RADAR_ZOOM_OUT)     },
        { "Radar Mode Toggle",      CreateAction(InputAction::SIM_START, S::RADAR_MODE_TOGGLE)  },

        // --- Navigation / Système ---
        { "Show Navmap",            CreateAction(InputAction::SIM_START, S::SHOW_NAVMAP)        },
        { "Pause",                  CreateAction(InputAction::SIM_START, S::PAUSE)              },
        { "End Mission",            CreateAction(InputAction::SIM_START, S::END_MISSION)        },

        // --- Radio ---
        { "Comm Radio",             CreateAction(InputAction::SIM_START, S::COMM_RADIO)         },
        { "Comm Radio M1",          CreateAction(InputAction::SIM_START, S::COMM_RADIO_M1)      },
        { "Comm Radio M2",          CreateAction(InputAction::SIM_START, S::COMM_RADIO_M2)      },
        { "Comm Radio M3",          CreateAction(InputAction::SIM_START, S::COMM_RADIO_M3)      },
        { "Comm Radio M4",          CreateAction(InputAction::SIM_START, S::COMM_RADIO_M4)      },
        { "Comm Radio M5",          CreateAction(InputAction::SIM_START, S::COMM_RADIO_M5)      },
        { "Comm Radio M6",          CreateAction(InputAction::SIM_START, S::COMM_RADIO_M6)      },
        { "Comm Radio M7",          CreateAction(InputAction::SIM_START, S::COMM_RADIO_M7)      },
        { "Comm Radio M8",          CreateAction(InputAction::SIM_START, S::COMM_RADIO_M8)      },

        // --- Touches spéciales ---
        { "Spec Key 1 (F10)",       CreateAction(InputAction::SIM_START, S::SPEC_KEY_1)         },
        { "Spec Key 2 (F11)",       CreateAction(InputAction::SIM_START, S::SPEC_KEY_2)         },
    };

    // Remplir la description du binding actuel
    for (auto& a : m_actions)
        a.currentBinding = describeBinding(a.action);
}

// ----------------------------------------------------------- frame / update --
void DebugControlMapping::runFrame() {
    double nowSec = std::chrono::duration<double>(
        Clock::now().time_since_epoch()).count();
    float dt = static_cast<float>(nowSec - m_lastTimeSec);
    m_lastTimeSec = nowSec;

    if (m_capture.active)
        updateCapture(dt);
}

// ----------------------------------------------------------- capture logic ---
void DebugControlMapping::startCapture(int actionIndex) {
    m_capture.active      = true;
    m_capture.actionIndex = actionIndex;
    m_capture.timeout     = 5.0f;

    // Snapshot des axes actuels comme baseline
    m_axisBaseline.clear();
    for (auto* joy : m_joysticks) {
        std::vector<Sint16> axes;
        if (joy) {
            int n = SDL_JoystickNumAxes(joy);
            for (int i = 0; i < n; i++)
                axes.push_back(SDL_JoystickGetAxis(joy, i));
        }
        m_axisBaseline.push_back(axes);
    }
}

void DebugControlMapping::updateCapture(float dt) {
    m_capture.timeout -= dt;
    if (m_capture.timeout <= 0.f) {
        m_capture.active = false; // timeout → annulation
        return;
    }

    SDL_JoystickUpdate();

    for (int ji = 0; ji < (int)m_joysticks.size(); ji++) {
        SDL_Joystick* joy = m_joysticks[ji];
        if (!joy) continue;

        // --- Détection axe ---
        int numAxes = SDL_JoystickNumAxes(joy);
        for (int ai = 0; ai < numAxes; ai++) {
            Sint16 val  = SDL_JoystickGetAxis(joy, ai);
            Sint16 base = (ai < (int)m_axisBaseline[ji].size())
                          ? m_axisBaseline[ji][ai] : 0;

            if (std::abs(val - base) > AXIS_THRESHOLD) {
                float scale = (val > 0) ? 1.0f : -1.0f;
                applyBinding(m_capture.actionIndex, ji, ai, true, scale);
                m_capture.active = false;
                return;
            }
        }

        // --- Détection bouton ---
        int numButtons = SDL_JoystickNumButtons(joy);
        for (int bi = 0; bi < numButtons; bi++) {
            if (SDL_JoystickGetButton(joy, bi)) {
                applyBinding(m_capture.actionIndex, ji, bi, false, 1.0f);
                m_capture.active = false;
                return;
            }
        }
    }
}

void DebugControlMapping::applyBinding(
    int actionIndex, int joyIndex, int axisOrBtn, bool isAxis, float scale)
{
    auto& a = m_actions[actionIndex];

    m_keyboard->registerAction(a.action);
    if (isAxis)
        m_keyboard->bindJoystickAxisToAction(a.action, joyIndex, axisOrBtn, scale);
    else
        m_keyboard->bindJoystickButtonToAction(a.action, joyIndex, axisOrBtn);

    // Mettre à jour la description affichée
    a.currentBinding = describeBinding(a.action);
}

std::string DebugControlMapping::describeBinding(InputAction action) const {
    if (!m_keyboard) return "(non init)";

    const InputActionSystem* sys = m_keyboard->getInputSystem();
    if (!sys) return "(no system)";

    const std::vector<InputBinding>* bindings = sys->getBindings(action);
    if (!bindings || bindings->empty()) return "(non assigne)";

    std::string result;
    for (const auto& b : *bindings) {
        if (!result.empty()) result += " | ";
        switch (b.type) {
            case InputType::KEYBOARD:
                result += "Key:" + std::string(SDL_GetKeyName(SDL_GetKeyFromScancode((SDL_Scancode)b.code)));
                break;
            case InputType::JOYSTICK_AXIS:
                result += "Joy" + std::to_string(b.deviceId)
                       + " Axe" + std::to_string(b.code)
                       + (b.scale < 0.f ? "(-)" : "(+)");
                break;
            case InputType::JOYSTICK_BUTTON:
                result += "Joy" + std::to_string(b.deviceId)
                       + " Btn" + std::to_string(b.code);
                break;
            case InputType::GAMEPAD_AXIS:
                result += "Pad" + std::to_string(b.deviceId)
                       + " Axe" + std::to_string(b.code)
                       + (b.scale < 0.f ? "(-)" : "(+)");
                break;
            case InputType::GAMEPAD_BUTTON:
                result += "Pad" + std::to_string(b.deviceId)
                       + " Btn" + std::to_string(b.code);
                break;
            case InputType::MOUSE_BUTTON:
                result += "MouseBtn" + std::to_string(b.code);
                break;
            case InputType::MOUSE_AXIS:
                result += "MouseAxe" + std::to_string(b.code);
                break;
            default:
                result += "?";
                break;
        }
    }
    return result;
}

// ------------------------------------------------------------------ UI ------
void DebugControlMapping::renderUI() {
    ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_FirstUseEver);
    ImGui::Begin("Joystick Mapping");

    // --- Joysticks connectés ---
    ImGui::SeparatorText("Joysticks connectes");
    if (m_joysticks.empty()) {
        ImGui::TextColored(ImVec4(1,0.3f,0.3f,1), "Aucun joystick detecte.");
    }
    for (int i = 0; i < (int)m_joysticks.size(); i++) {
        ImGui::Text("[%d] %s", i, m_joystickNames[i].c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("GUID: %s", m_joystickGUIDs[i].c_str());
    }

    ImGui::Separator();

    // --- Capture en cours ---
    if (m_capture.active) {
        ImGui::TextColored(ImVec4(1,1,0,1),
            ">> Appuyez sur un bouton ou bougez un axe... (%.1fs)",
            m_capture.timeout);
        if (ImGui::Button("Annuler")) m_capture.active = false;
        ImGui::Separator();
    }

    // --- Table des actions ---
    ImGui::SeparatorText("Actions");
    if (ImGui::BeginTable("mappings", 3,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_ScrollY, ImVec2(0, 300)))
    {
        ImGui::TableSetupColumn("Action",   ImGuiTableColumnFlags_WidthFixed, 180);
        ImGui::TableSetupColumn("Binding",  ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("",         ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableHeadersRow();

        for (int i = 0; i < (int)m_actions.size(); i++) {
            auto& a = m_actions[i];
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(a.label);

            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(a.currentBinding.c_str());

            ImGui::TableSetColumnIndex(2);
            ImGui::PushID(i);
            bool capturing = m_capture.active && m_capture.actionIndex == i;
            if (capturing) {
                ImGui::TextColored(ImVec4(1,1,0,1), "...");
            } else {
                if (ImGui::Button("Mapper") && !m_capture.active) {
                    startCapture(i);
                }
                ImGui::SameLine();
                if (ImGui::Button("X")) {
                    InputActionSystem::getInstance().clearBindings(a.action);
                    a.currentBinding = describeBinding(a.action);
                    m_keyboard->saveActionBindings("default_bindings.cfg");
                }
                
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    // --- Boutons globaux ---
    ImGui::Separator();
    if (ImGui::Button("Sauvegarder")) {
        m_keyboard->saveActionBindings("default_bindings.cfg");
        ImGui::OpenPopup("saved_popup");
    }
    ImGui::SameLine();
    if (ImGui::Button("Recharger joysticks")) {
        openJoysticks();
        buildActionList();
    }
    if (ImGui::BeginPopup("saved_popup")) {
        ImGui::Text("Bindings sauvegardes !");
        ImGui::EndPopup();
    }

    ImGui::End();
}