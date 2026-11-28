//
//  R4ConfigPanel.cpp
//  libRealSpace
//
#include "R4ConfigPanel.h"
#ifdef R4GAME_AUDIO
#include "R4Mixer.h"
#endif
#include "R4Renderer.h"
#include "R4Screen.h"

#include "engine/Config.hpp"
#include "engine/EventManager.h"
#include "engine/GameEngine.h"
#include "engine/InputActionSystem.h"

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

namespace {

// the share of the window that the panel covers, in its middle
constexpr float kWindowShare = 0.7f;
// the config.ini that neosc_r4 reads
const char *const kConfigPath = "./assets/config.ini";
// an axis that moves this far from its rest is the axis of the action that waits for an input
constexpr int kAxisMove = 16000;
// the deadzone of the axes without their own (InputActionSystem)
constexpr float kDefaultDeadzone = 0.1f;

enum class Kind {
    BOOL,
    INT,
    FLOAT,
    CHOICE,
    TEXT,
    // width and height of [Window], from the modes of the display
    RESOLUTION
};

// when a change is seen in the game
enum class When {
    NOW,
    MISSION,
    RESTART
};

struct Option {
    const char *section;
    const char *key;
    const char *label;
    Kind kind;
    When when;
    float min;
    float max;
    float fallback;
    // CHOICE: the values in config.ini, and their names; the fallback is the index of a value
    std::vector<const char *> values;
    std::vector<const char *> names;
};

struct Group {
    const char *name;
    std::vector<Option> options;
};

const std::vector<Group> &groups() {
    static const std::vector<Group> list = {
        {"Window",
         {
             {"Window", "width", "Resolution", Kind::RESOLUTION, When::RESTART, 0, 0, 0, {}, {}},
             {"Window", "fullscreen", "Fullscreen", Kind::BOOL, When::RESTART, 0, 1, 0, {}, {}},
         }},
        {"Game",
         {
             {"Game", "show_texture", "Textures", Kind::BOOL, When::NOW, 0, 1, 1, {}, {}},
             {"Game", "show_fog", "Fog", Kind::BOOL, When::NOW, 0, 1, 1, {}, {}},
             {"Game", "clouds_enabled", "Clouds", Kind::BOOL, When::NOW, 0, 1, 0, {}, {}},
             {"Game", "craters", "Craters", Kind::BOOL, When::MISSION, 0, 1, 1, {}, {}},
             {"Game", "object_gouraud", "Gouraud on the Objects", Kind::BOOL, When::NOW, 0, 1, 1, {}, {}},
             {"Game", "object_detail", "Object Detail", Kind::CHOICE, When::RESTART, 0, 0, 0, {"0", "1", "2"}, {"0 (most detail)", "1", "2 (least detail)"}},
             {"Game", "world_detail", "World Detail", Kind::CHOICE, When::RESTART, 0, 0, 0, {"0", "1", "2"}, {"0 (most detail)", "1", "2 (least detail)"}},
             {"Game", "max_view_distance", "View Distance", Kind::INT, When::RESTART, 10000, 300000, 160000, {}, {}},
             {"Game", "terrain_shadow_lift", "Terrain Shadow Lift", Kind::INT, When::MISSION, 0, 50, 20, {}, {}},
             {"Game", "terrain_shadow_range", "Terrain Shadow Range", Kind::INT, When::MISSION, 1, 100, 50, {}, {}},
             {"Game", "terrain_flat", "Flat Terrain", Kind::BOOL, When::MISSION, 0, 1, 0, {}, {}},
             {"Gameplay", "difficulty", "Difficulty", Kind::CHOICE, When::MISSION, 0, 0, 0, {"0", "1", "2"}, {"0", "1", "2"}},
         }},
        {"Video",
         {
             {"Video", "msaa", "MSAA", Kind::CHOICE, When::RESTART, 0, 0, 0, {"0", "2", "4", "8"}, {"Off", "2x", "4x", "8x"}},
             {"video", "fx_fxaa", "FXAA", Kind::BOOL, When::NOW, 0, 1, 0, {}, {}},
             {"video", "fx_scanlines", "Scanlines", Kind::BOOL, When::NOW, 0, 1, 0, {}, {}},
             {"video", "fx_cpc_palette", "CPC Palette", Kind::BOOL, When::NOW, 0, 1, 0, {}, {}},
             {"video", "fx_pixel_scale", "Pixel Scale", Kind::CHOICE, When::NOW, 0, 0, 0, {"1", "2", "3", "4"}, {"1x", "2x", "3x", "4x"}},
             {"Video", "super_eagle_2x", "Super Eagle 2x (320 x 200)", Kind::BOOL, When::RESTART, 0, 1, 0, {}, {}},
             {"Video", "widescreen_ambilight", "Widescreen Ambilight", Kind::BOOL, When::RESTART, 0, 1, 0, {}, {}},
             {"Video", "gl_compatibility", "OpenGL Compatibility (SCRenderer)", Kind::BOOL, When::RESTART, 0, 1, 0, {}, {}},
         }},
        {"Render (R4nkine)",
         {
             {"R4nkine", "vsync", "VSync", Kind::BOOL, When::NOW, 0, 1, 1, {}, {}},
             {"R4nkine", "sky", "Sky Dome", Kind::BOOL, When::NOW, 0, 1, 1, {}, {}},
             {"R4nkine", "wireframe", "Wireframe", Kind::BOOL, When::NOW, 0, 1, 0, {}, {}},
             {"R4nkine", "flat_shading", "Flat Shading", Kind::BOOL, When::NOW, 0, 1, 0, {}, {}},
             {"R4nkine", "lod_scale", "LOD Scale", Kind::FLOAT, When::NOW, 0.05f, 4, 1, {}, {}},
             {"R4nkine", "shadows", "Shadows", Kind::BOOL, When::NOW, 0, 1, 0, {}, {}},
             {"R4nkine", "shadow_map_size", "Shadow Map", Kind::CHOICE, When::NOW, 0, 0, 1, {"1024", "2048", "4096"}, {"1024", "2048", "4096"}},
             {"R4nkine", "shadow_distance", "Shadow Distance", Kind::FLOAT, When::NOW, 100, 20000, 1500, {}, {}},
             {"R4nkine", "shadow_strength", "Shadow on Unlit", Kind::FLOAT, When::NOW, 0, 1, 0.5f, {}, {}},
             {"R4nkine", "tree_shadows", "Tree Shadows", Kind::BOOL, When::NOW, 0, 1, 0, {}, {}},
             {"R4nkine", "forests", "Forests", Kind::CHOICE, When::MISSION, 0, 0, 0, {"off", "noise", "textures"}, {"Off", "Green Terrain + Noise", "Woods of the Textures"}},
             {"R4nkine", "forest_density", "Forest Density", Kind::FLOAT, When::MISSION, 0, 3, 1, {}, {}},
             {"R4nkine", "tree_detail_km", "Detailed Trees (km)", Kind::FLOAT, When::MISSION, 0.5f, 10, 1.5f, {}, {}},
             {"R4nkine", "tree_km", "Trees (km)", Kind::FLOAT, When::MISSION, 1, 50, 6, {}, {}},
             {"R4nkine", "buildings", "Buildings", Kind::BOOL, When::MISSION, 0, 1, 0, {}, {}},
             {"R4nkine", "texture_upscale", "Texture Upscale", Kind::CHOICE, When::MISSION, 0, 0, 0, {"none", "2xsai", "xbr2x", "xbr4x"}, {"None", "2xSaI", "xBR 2x", "xBR 4x"}},
             {"R4nkine", "texture_mipmaps", "Mipmaps", Kind::BOOL, When::MISSION, 0, 1, 0, {}, {}},
             {"R4nkine", "anisotropy", "Anisotropic Filter", Kind::CHOICE, When::RESTART, 0, 0, 0, {"1", "2", "4", "8", "16"}, {"Off", "2x", "4x", "8x", "16x"}},
             {"R4nkine", "sun_terrain", "Terrain Lit by the Sun", Kind::BOOL, When::MISSION, 0, 1, 0, {}, {}},
             {"R4nkine", "terrain_relief", "Relief of the Light", Kind::FLOAT, When::MISSION, 1, 16, 4, {}, {}},
             {"R4nkine", "terrain_lod", "Terrain Detail", Kind::CHOICE, When::MISSION, 0, 0, 0, {"distance", "fine", "medium", "coarse"}, {"By Distance", "Fine", "Medium", "Coarse"}},
         }},
        {"Sound",
         {
             {"Sound", "midi_music", "Music", Kind::CHOICE, When::RESTART, 0, 0, 0, {"0", "1", "2", "3"}, {"AdLib", "General MIDI", "MT-32", "MT-32 Emulation"}},
             {"Sound", "music_volume", "Music Volume", Kind::INT, When::NOW, 0, 100, 100, {}, {}},
             {"Sound", "sfx_volume", "Effects Volume", Kind::INT, When::NOW, 0, 100, 100, {}, {}},
             {"Sound", "sfx_synth", "Effects Synthesizer", Kind::CHOICE, When::RESTART, 0, 0, 0, {"auto", "opl", "mt32"}, {"Same as the Music", "AdLib (OPL)", "MT-32 Emulation"}},
             {"Sound", "digital_fx", "Digitized Effects First", Kind::BOOL, When::MISSION, 0, 1, 1, {}, {}},
             {"Sound", "engine_nearest_only", "Nearest Engine Only", Kind::BOOL, When::MISSION, 0, 1, 0, {}, {}},
             {"Sound", "hrtf", "HRTF (Headphones)", Kind::BOOL, When::RESTART, 0, 1, 0, {}, {}},
             {"Sound", "engine_volume", "Engine Volume", Kind::INT, When::RESTART, 0, 128, 100, {}, {}},
             {"Sound", "soundfont", "SoundFont (General MIDI)", Kind::TEXT, When::RESTART, 0, 0, 0, {}, {}},
             {"Sound", "mt32_rom_dir", "MT-32 ROM Folder", Kind::TEXT, When::RESTART, 0, 0, 0, {}, {}},
             {"Sound", "midi_port", "MIDI Port", Kind::TEXT, When::RESTART, 0, 0, 0, {}, {}},
         }},
        // the last group: its options, then the keys of the flight
        {"Input",
         {
             {"Input", "bindings_file", "Keys File", Kind::TEXT, When::RESTART, 0, 0, 0, {}, {}},
         }},
    };
    return list;
}

struct ActionName {
    SimActionOfst action;
    const char *name;
    // an axis: the sign that the game wants when the axis moves in the direction of the name; 0: a key or a button
    float axis = 0;
};

// the actions of the flight (GameEngine.h), by their place in the game
const std::vector<ActionName> &actionNames() {
    static const std::vector<ActionName> list = {
        {THROTTLE_UP, "Throttle Up"},
        {THROTTLE_DOWN, "Throttle Down"},
        {THROTTLE_10, "Throttle 10%"},
        {THROTTLE_20, "Throttle 20%"},
        {THROTTLE_30, "Throttle 30%"},
        {THROTTLE_40, "Throttle 40%"},
        {THROTTLE_50, "Throttle 50%"},
        {THROTTLE_60, "Throttle 60%"},
        {THROTTLE_70, "Throttle 70%"},
        {THROTTLE_80, "Throttle 80%"},
        {THROTTLE_90, "Throttle 90%"},
        {THROTTLE_100, "Throttle 100%"},
        {AXIS_PITCH, "Axis: Pitch (pull: up)", 1},
        {AXIS_ROLL, "Axis: Roll (right)", 1},
        {AXIS_RUDDER, "Axis: Rudder (right)", 1},
        {AXIS_THROTTLE, "Axis: Throttle (forward: 100%)", -1},
        {AXIS_LOOK_X, "Axis: Look (right)", 1},
        {AXIS_LOOK_Y, "Axis: Look (up)", 1},
        {PITCH_UP, "Pitch Up"},
        {PITCH_DOWN, "Pitch Down"},
        {ROLL_LEFT, "Roll Left"},
        {ROLL_RIGHT, "Roll Right"},
        {RUDDER_LEFT, "Rudder Left"},
        {RUDDER_RIGHT, "Rudder Right"},
        {LANDING_GEAR, "Landing Gear"},
        {TOGGLE_BRAKES, "Brakes"},
        {TOGGLE_FLAPS, "Flaps"},
        {AUTOPILOT, "Autopilot"},
        {FIRE_PRIMARY, "Fire"},
        {WEAPON_MODE_TOGGLE, "Weapon Mode"},
        {SINGLE_TARGET_MODE, "Single Target Mode"},
        {TARGET_NEAREST, "Nearest Target"},
        {CHAFF, "Chaff"},
        {FLARE, "Flare"},
        {EJECT, "Eject"},
        {RADAR_MODE_TOGGLE, "Radar Mode"},
        {RADAR_ZOOM_IN, "Radar Zoom In"},
        {RADAR_ZOOM_OUT, "Radar Zoom Out"},
        {MDFS_RADAR, "Display: Radar"},
        {MDFS_DAMAGE, "Display: Damage"},
        {MDFS_WEAPONS, "Display: Weapons"},
        {MDFS_TARGET_CAMERA, "Display: Target Camera"},
        {SHOW_NAVMAP, "Navigation Map"},
        {LOOK_FORWARD, "Look Forward"},
        {LOOK_LEFT, "Look Left"},
        {LOOK_RIGHT, "Look Right"},
        {LOOK_BEHIND, "Look Behind"},
        {VIEW_COCKPIT, "View: Cockpit"},
        {VIEW_TARGET, "View: Target"},
        {VIEW_BEHIND, "View: Behind"},
        {VIEW_WEAPONS, "View: Weapons"},
        {EYES_ON_TARGET, "Eyes on Target"},
        {COMM_RADIO, "Radio"},
        {COMM_RADIO_M1, "Radio Message 1"},
        {COMM_RADIO_M2, "Radio Message 2"},
        {COMM_RADIO_M3, "Radio Message 3"},
        {COMM_RADIO_M4, "Radio Message 4"},
        {COMM_RADIO_M5, "Radio Message 5"},
        {COMM_RADIO_M6, "Radio Message 6"},
        {COMM_RADIO_M7, "Radio Message 7"},
        {COMM_RADIO_M8, "Radio Message 8"},
        {TOGGLE_MOUSE, "Mouse Control"},
        {MOUSE_X, "Mouse X"},
        {MOUSE_Y, "Mouse Y"},

        {PAUSE, "Pause"},
        {FLIGHT_OPTIONS, "Flight Options"},
        {END_MISSION, "End Mission"},
        {INFINIT_AMMO_TOGGLE, "Infinite Ammo"},
        {SPEC_KEY_1, "Special Key 1"},
        {SPEC_KEY_2, "Special Key 2"},
    };
    return list;
}

InputAction actionOf(SimActionOfst a) {
    return CreateAction(InputAction::SIM_START, a);
}

std::string fmt(const char *f, ...) {
    char buf[256];
    va_list args;
    va_start(args, f);
    std::vsnprintf(buf, sizeof buf, f, args);
    va_end(args);
    return buf;
}

std::string modifiersText(int m) {
    std::string s;
    if (m & KEY_MOD_CTRL) {
        s += "Ctrl+";
    }
    if (m & KEY_MOD_ALT) {
        s += "Alt+";
    }
    if (m & KEY_MOD_SHIFT) {
        s += "Shift+";
    }
    return s;
}

// the name of the device of a binding: its name in SDL when it is plugged in, else the kind
std::string deviceText(const InputBinding &b, const char *kind) {
    if (b.device.empty()) {
        return kind;
    }
    EventManager &events = EventManager::getInstance();
    const int rank = std::max(b.deviceId, 0);
    const std::string second = rank > 0 ? fmt(" #%d", rank + 1) : std::string();
    const int index = events.findJoystick(b.device, rank);
    if (index < 0) {
        return std::string(kind) + second + " (unplugged)";
    }
    return events.getJoystickName(index) + second;
}

std::string bindingText(const InputBinding &b) {
    switch (b.type) {
    case InputType::KEYBOARD:
        return modifiersText(b.modifiers) + SDL_GetScancodeName((SDL_Scancode)b.code);
    case InputType::MOUSE_BUTTON:
        return fmt("Mouse %d", b.code);
    case InputType::MOUSE_AXIS:
    case InputType::MOUSE_POSITION:
        return fmt("Mouse Axis %d", b.code);
    case InputType::GAMEPAD_BUTTON: {
        const char *name = SDL_GameControllerGetStringForButton((SDL_GameControllerButton)b.code);
        return deviceText(b, "Pad") + " " + (name != nullptr ? name : fmt("%d", b.code));
    }
    case InputType::GAMEPAD_AXIS: {
        const char *name = SDL_GameControllerGetStringForAxis((SDL_GameControllerAxis)b.code);
        return deviceText(b, "Pad") + " " + (name != nullptr ? name : fmt("axis %d", b.code)) + (b.scale < 0 ? " (inverted)" : "");
    }
    case InputType::JOYSTICK_BUTTON:
        return deviceText(b, "Joy") + fmt(" Button %d", b.code);
    case InputType::JOYSTICK_AXIS:
        return deviceText(b, "Joy") + fmt(" Axis %d", b.code) + (b.scale < 0 ? " (inverted)" : "");
    }
    return "?";
}

bool isDeviceBinding(const InputBinding &b) {
    return b.type == InputType::GAMEPAD_BUTTON || b.type == InputType::GAMEPAD_AXIS || b.type == InputType::JOYSTICK_BUTTON || b.type == InputType::JOYSTICK_AXIS;
}

bool isAxisBinding(const InputBinding &b) {
    return b.type == InputType::GAMEPAD_AXIS || b.type == InputType::JOYSTICK_AXIS;
}

// the index in EventManager of the device of an event, else -1
int deviceOf(SDL_JoystickID instance) {
    EventManager &events = EventManager::getInstance();
    for (int i = 0; i < events.getJoystickCount(); i++) {
        SDL_Joystick *joystick = events.getJoystick(i);
        if (joystick != nullptr && SDL_JoystickInstanceID(joystick) == instance) {
            return i;
        }
    }
    return -1;
}

std::string bindingsText(InputAction action) {
    const std::vector<InputBinding> *list = InputActionSystem::getInstance().getBindings(action);
    std::string s;
    if (list != nullptr) {
        for (const InputBinding &b : *list) {
            s += (s.empty() ? "" : ", ") + bindingText(b);
        }
    }
    return s.empty() ? "-" : s;
}

int modifiersOf(Uint16 mod) {
    int m = KEY_MOD_NONE;
    if (mod & KMOD_SHIFT) {
        m |= KEY_MOD_SHIFT;
    }
    if (mod & KMOD_CTRL) {
        m |= KEY_MOD_CTRL;
    }
    if (mod & KMOD_ALT) {
        m |= KEY_MOD_ALT;
    }
    return m;
}

bool isModifierKey(SDL_Scancode k) {
    return k == SDL_SCANCODE_LCTRL || k == SDL_SCANCODE_RCTRL || k == SDL_SCANCODE_LSHIFT || k == SDL_SCANCODE_RSHIFT || k == SDL_SCANCODE_LALT || k == SDL_SCANCODE_RALT || k == SDL_SCANCODE_LGUI || k == SDL_SCANCODE_RGUI;
}

// the options read again by the game for a change that applies now
void applyNow() {
    Config &config = Config::getInstance();
    if (R4Renderer *r = R4Renderer::current()) {
        r->settings().load();
        r->show_textured = config.getBool("Game", "show_texture", true);
        r->show_fog = config.getBool("Game", "show_fog", true);
        r->show_clouds = config.getBool("Game", "clouds_enabled", false);
        r->object_gouraud = config.getBool("Game", "object_gouraud", true);
        r4bridge::textureQuality().upscale = r->settings().upscale;
        r4bridge::textureQuality().mipmaps = r->settings().mipmaps;
        SDL_GL_SetSwapInterval(r->settings().vsync ? 1 : 0);
    }
    if (R4Screen *screen = R4Screen::current()) {
        screen->initPostProcess();
    }
#ifdef R4GAME_AUDIO
    if (R4Mixer *mixer = R4Mixer::current()) {
        mixer->loadVolumes();
    }
#endif
}

} // namespace

R4ConfigPanel::R4ConfigPanel(r4::gpu::Device &device) : gui_(device) {
    SDL_SetEventFilter(filter, this);
}

R4ConfigPanel::~R4ConfigPanel() {
    SDL_SetEventFilter(nullptr, nullptr);
}

// while the panel is shown, the keys and the mouse are for it only; the controllers and the joysticks go on to the game, the
// panel only looks at them for the input of an action
int R4ConfigPanel::filter(void *panel, SDL_Event *event) {
    R4ConfigPanel *self = static_cast<R4ConfigPanel *>(panel);
    if (event->type == SDL_KEYDOWN && !event->key.repeat && event->key.keysym.scancode == SDL_SCANCODE_F11 && (event->key.keysym.mod & KMOD_CTRL)) {
        self->toggle_ = true;
    }
    if (!self->shown_) {
        return 1;
    }
    switch (event->type) {
    case SDL_TEXTINPUT:
        self->text_ += event->text.text;
        return 0;
    case SDL_KEYDOWN:
        // the game reads the state of the keyboard (EventManager), not only its events
        SDL_ResetKeyboard();
        switch (event->key.keysym.scancode) {
        case SDL_SCANCODE_BACKSPACE:
            self->editKeys_.push_back(r4::ui::Key::BACKSPACE);
            break;
        case SDL_SCANCODE_DELETE:
            self->editKeys_.push_back(r4::ui::Key::DELETE_FORWARD);
            break;
        case SDL_SCANCODE_LEFT:
            self->editKeys_.push_back(r4::ui::Key::LEFT);
            break;
        case SDL_SCANCODE_RIGHT:
            self->editKeys_.push_back(r4::ui::Key::RIGHT);
            break;
        case SDL_SCANCODE_HOME:
            self->editKeys_.push_back(r4::ui::Key::HOME);
            break;
        case SDL_SCANCODE_END:
            self->editKeys_.push_back(r4::ui::Key::END);
            break;
        case SDL_SCANCODE_RETURN:
        case SDL_SCANCODE_KP_ENTER:
            self->editKeys_.push_back(r4::ui::Key::ENTER);
            break;
        case SDL_SCANCODE_ESCAPE:
            self->editKeys_.push_back(r4::ui::Key::ESCAPE);
            break;
        default:
            break;
        }
        // a text field takes the keys before the change of a key of the flight
        if (self->gui_.wantsKeyboard()) {
            return 0;
        }
        if (self->capturing_ >= 0 && !event->key.repeat && !isModifierKey(event->key.keysym.scancode) && event->key.keysym.scancode != SDL_SCANCODE_F11) {
            if (event->key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                self->cancelled_ = true;
            } else if (actionNames()[(size_t)self->capturing_].axis == 0) {
                self->caught_ = event->key.keysym.scancode;
                self->caughtModifiers_ = modifiersOf(event->key.keysym.mod);
            }
        }
        return 0;
    case SDL_MOUSEWHEEL:
        self->wheel_ += event->wheel.preciseY;
        return 0;
    case SDL_CONTROLLERBUTTONDOWN:
    case SDL_CONTROLLERAXISMOTION:
    case SDL_JOYBUTTONDOWN:
    case SDL_JOYAXISMOTION:
        self->look(*event);
        return 1;
    case SDL_KEYUP:
    case SDL_TEXTEDITING:
    case SDL_MOUSEMOTION:
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        return 0;
    default:
        return 1;
    }
}

// a button for an action of a key or a button, an axis moved far from its rest for an action of an axis
void R4ConfigPanel::look(const SDL_Event &event) {
    if (capturing_ < 0 || caughtDevice_) {
        return;
    }
    const bool axis = actionNames()[(size_t)capturing_].axis != 0;
    // a game controller sends the events of its joystick too
    const SDL_JoystickID which = event.type == SDL_JOYBUTTONDOWN ? event.jbutton.which : event.type == SDL_JOYAXISMOTION ? event.jaxis.which : event.type == SDL_CONTROLLERBUTTONDOWN ? event.cbutton.which : event.caxis.which;
    const bool controller = SDL_GameControllerFromInstanceID(which) != nullptr;
    if (event.type == SDL_CONTROLLERBUTTONDOWN && !axis) {
        caughtType_ = InputType::GAMEPAD_BUTTON;
        caughtCode_ = event.cbutton.button;
        caughtSign_ = 1;
    } else if (event.type == SDL_JOYBUTTONDOWN && !axis && !controller) {
        caughtType_ = InputType::JOYSTICK_BUTTON;
        caughtCode_ = event.jbutton.button;
        caughtSign_ = 1;
    } else if (event.type == SDL_CONTROLLERAXISMOTION && axis && std::abs((int)event.caxis.value) > kAxisMove) {
        caughtType_ = InputType::GAMEPAD_AXIS;
        caughtCode_ = event.caxis.axis;
        caughtSign_ = event.caxis.value > 0 ? 1.0f : -1.0f;
    } else if (event.type == SDL_JOYAXISMOTION && axis && !controller) {
        Sint16 rest = 0;
        SDL_JoystickGetAxisInitialState(SDL_JoystickFromInstanceID(which), event.jaxis.axis, &rest);
        const int moved = (int)event.jaxis.value - (int)rest;
        if (std::abs(moved) <= kAxisMove) {
            return;
        }
        caughtType_ = InputType::JOYSTICK_AXIS;
        caughtCode_ = event.jaxis.axis;
        caughtSign_ = moved > 0 ? 1.0f : -1.0f;
    } else {
        return;
    }
    const int device = deviceOf(which);
    if (device < 0) {
        return;
    }
    caughtGuid_ = EventManager::getInstance().getJoystickGuid(device);
    caughtRank_ = EventManager::getInstance().getJoystickRank(device);
    caughtDevice_ = true;
}

void R4ConfigPanel::show(bool shown) {
    shown_ = shown;
    capturing_ = -1;
    message_.clear();
    text_.clear();
    editKeys_.clear();
    if (shown) {
        gameRelative_ = SDL_GetRelativeMouseMode();
        gameCursor_ = SDL_ShowCursor(SDL_QUERY);
        gameTextInput_ = SDL_IsTextInputActive();
        SDL_StartTextInput();
        wheel_ = 0;
    } else {
        SDL_SetRelativeMouseMode(gameRelative_);
        SDL_ShowCursor(gameCursor_);
        if (!gameTextInput_) {
            SDL_StopTextInput();
        }
    }
}

void R4ConfigPanel::draw(SDL_Window *window, int pixelWidth, int pixelHeight) {
    if (toggle_) {
        toggle_ = false;
        show(!shown_);
    }
    if (!shown_) {
        return;
    }
    if (SDL_GetRelativeMouseMode()) {
        SDL_SetRelativeMouseMode(SDL_FALSE);
    }
    SDL_ShowCursor(SDL_ENABLE);
    // the input that the filter caught for an action: Change replaces the inputs of its family (the keyboard, or the
    // controllers and the joysticks), Add keeps them; the mouse stays
    if (capturing_ >= 0 && (caught_ != SDL_SCANCODE_UNKNOWN || caughtDevice_ || cancelled_)) {
        if (caught_ != SDL_SCANCODE_UNKNOWN || caughtDevice_) {
            InputActionSystem &input = InputActionSystem::getInstance();
            const ActionName &name = actionNames()[(size_t)capturing_];
            const InputAction action = actionOf(name.action);
            InputBinding binding(InputType::KEYBOARD, -1, caught_, 1.0f, caughtModifiers_);
            if (caughtDevice_) {
                binding = InputBinding(caughtType_, caughtRank_, caughtCode_, name.axis != 0 ? caughtSign_ * name.axis : 1.0f);
                binding.device = caughtGuid_;
            }
            if (!adding_) {
                std::vector<InputBinding> keep;
                if (const std::vector<InputBinding> *list = input.getBindings(action)) {
                    keep = *list;
                }
                for (const InputBinding &b : keep) {
                    if (isDeviceBinding(b) == caughtDevice_ && b.type != InputType::MOUSE_BUTTON && b.type != InputType::MOUSE_AXIS && b.type != InputType::MOUSE_POSITION) {
                        input.unbindInput(action, b);
                    }
                }
            }
            input.unbindInput(action, binding);
            input.bindInput(action, binding);
            message_ = std::string(name.name) + ": " + bindingText(binding) + " (not saved)";
        }
        capturing_ = -1;
        caught_ = SDL_SCANCODE_UNKNOWN;
        caughtDevice_ = false;
        cancelled_ = false;
    }
    int pointWidth = 0;
    int pointHeight = 0;
    SDL_GetWindowSize(window, &pointWidth, &pointHeight);
    int mx = 0;
    int my = 0;
    const Uint32 buttons = SDL_GetMouseState(&mx, &my);
    r4::ui::Input input;
    input.mouseX = (float)mx;
    input.mouseY = (float)my;
    input.left = (buttons & SDL_BUTTON_LMASK) != 0;
    input.right = (buttons & SDL_BUTTON_RMASK) != 0;
    input.pressed = (input.left && !leftDown_) || (input.right && !rightDown_);
    input.released = (!input.left && leftDown_) || (!input.right && rightDown_);
    leftDown_ = input.left;
    rightDown_ = input.right;
    input.wheel = wheel_;
    wheel_ = 0;
    input.text = text_;
    input.keys = editKeys_;
    text_.clear();
    editKeys_.clear();

    gui_.beginFrame(pointWidth, pointHeight, input, pointWidth > 0 ? (float)pixelWidth / pointWidth : 1);
    const float w = std::floor(pointWidth * kWindowShare);
    const float h = std::floor(pointHeight * kWindowShare);
    bool open = true;
    gui_.beginPanel("configuration", {std::floor((pointWidth - w) / 2), std::floor((pointHeight - h) / 2), w, h}, "Configuration (Ctrl+F11)", &open);
    gui_.columns(2);
    if (gui_.button("Save")) {
        save();
    }
    gui_.label(message_.c_str(), r4::ui::Font::SMALL);
    gui_.columns(1);
    gui_.label("(mission): at the next mission    (restart): at the next start", r4::ui::Font::SMALL);
    std::vector<std::string> tabs;
    for (const Group &g : groups()) {
        tabs.push_back(g.name);
    }
    tabs.push_back("Devices");
    gui_.tabs("tabs", tab_, tabs);
    // the page of the tab in a frame under it, with its own scroll: the tabs stay at their place
    gui_.beginBox(fmt("page %d", tab_).c_str());
    if (tab_ == (int)groups().size()) {
        devices();
    } else {
        options();
    }
    if (tab_ + 1 == (int)groups().size()) {
        keys();
    }
    gui_.endBox();
    gui_.endPanel();
    gui_.endFrame();
    gui_.render();
    if (!open) {
        show(false);
    }
}

void R4ConfigPanel::options() {
    Config &config = Config::getInstance();
    const std::vector<Group> &list = groups();
    if (tab_ >= 0 && tab_ < (int)list.size()) {
        for (const Option &o : list[(size_t)tab_].options) {
            const std::string label = std::string(o.label) + (o.when == When::MISSION ? " (mission)" : o.when == When::RESTART ? " (restart)" : "") + "##" + o.section + o.key;
            bool changed = false;
            if (o.kind == Kind::BOOL) {
                bool v = config.getBool(o.section, o.key, o.fallback != 0);
                if (gui_.toggle(label.c_str(), v)) {
                    config.setBool(o.section, o.key, v);
                    changed = true;
                }
            } else if (o.kind == Kind::CHOICE) {
                const std::string now = config.getString(o.section, o.key, o.values[(size_t)o.fallback]);
                int selected = (int)o.fallback;
                for (size_t i = 0; i < o.values.size(); i++) {
                    if (now == o.values[i]) {
                        selected = (int)i;
                    }
                }
                std::vector<std::string> names(o.names.begin(), o.names.end());
                if (gui_.select(label.c_str(), selected, names)) {
                    config.setString(o.section, o.key, o.values[(size_t)selected]);
                    changed = true;
                }
            } else if (o.kind == Kind::RESOLUTION) {
                // the modes of the display of the window, and the size of config.ini when it is not one of them
                const std::string now = fmt("%dx%d", config.getInt("Window", "width", 800), config.getInt("Window", "height", 600));
                std::vector<std::string> sizes;
                SDL_Window *window = SDL_GL_GetCurrentWindow();
                const int display = window != nullptr ? std::max(0, SDL_GetWindowDisplayIndex(window)) : 0;
                for (int m = 0; m < SDL_GetNumDisplayModes(display); m++) {
                    SDL_DisplayMode mode;
                    if (SDL_GetDisplayMode(display, m, &mode) == 0) {
                        const std::string size = fmt("%dx%d", mode.w, mode.h);
                        if (std::find(sizes.begin(), sizes.end(), size) == sizes.end()) {
                            sizes.push_back(size);
                        }
                    }
                }
                if (std::find(sizes.begin(), sizes.end(), now) == sizes.end()) {
                    sizes.insert(sizes.begin(), now);
                }
                int selected = (int)(std::find(sizes.begin(), sizes.end(), now) - sizes.begin());
                if (gui_.select(label.c_str(), selected, sizes)) {
                    int w = 0;
                    int h = 0;
                    if (std::sscanf(sizes[(size_t)selected].c_str(), "%dx%d", &w, &h) == 2) {
                        config.setInt("Window", "width", w);
                        config.setInt("Window", "height", h);
                        changed = true;
                    }
                }
            } else if (o.kind == Kind::TEXT) {
                std::string v = config.getString(o.section, o.key, "");
                if (gui_.inputText(label.c_str(), v)) {
                    config.setString(o.section, o.key, v);
                    changed = true;
                }
            } else {
                const std::string now = config.getString(o.section, o.key, "");
                float v = now.empty() ? o.fallback : (float)std::atof(now.c_str());
                const bool whole = o.kind == Kind::INT;
                if (gui_.slider(label.c_str(), v, o.min, o.max, whole ? "%.0f" : "%.2f", !whole && o.max / std::max(o.min, 0.01f) > 50)) {
                    config.setString(o.section, o.key, whole ? fmt("%d", (int)std::lround(v)) : fmt("%g", v));
                    changed = true;
                }
            }
            if (changed && o.when == When::NOW) {
                applyNow();
            }
            if (changed) {
                message_ = "changed (not saved)";
            }
        }
    }
}

void R4ConfigPanel::keys() {
    gui_.label("Change: then a key, with Ctrl, Alt or Shift, or a button of a controller (Esc: cancel). For an axis: move it in the", r4::ui::Font::SMALL);
    gui_.label("direction of its name. Change replaces the keys, or the controllers, Add keeps them; Clear keeps the mouse.", r4::ui::Font::SMALL);
    InputActionSystem &input = InputActionSystem::getInstance();
    const std::vector<ActionName> &list = actionNames();
    for (size_t i = 0; i < list.size(); i++) {
        const InputAction action = actionOf(list[i].action);
        gui_.columns(5);
        gui_.label(list[i].name);
        gui_.label(capturing_ == (int)i ? (list[i].axis != 0 ? "move an axis..." : "press a key or a button...") : bindingsText(action).c_str());
        if (gui_.button(fmt("Change##%zu", i).c_str())) {
            capturing_ = (int)i;
            adding_ = false;
            caught_ = SDL_SCANCODE_UNKNOWN;
            caughtDevice_ = false;
            cancelled_ = false;
        }
        if (gui_.button(fmt("Add##%zu", i).c_str())) {
            capturing_ = (int)i;
            adding_ = true;
            caught_ = SDL_SCANCODE_UNKNOWN;
            caughtDevice_ = false;
            cancelled_ = false;
        }
        std::vector<InputBinding> bindings;
        if (const std::vector<InputBinding> *now = input.getBindings(action)) {
            bindings = *now;
        }
        if (gui_.button(fmt("Clear##%zu", i).c_str())) {
            for (const InputBinding &b : bindings) {
                if (b.type == InputType::KEYBOARD || isDeviceBinding(b)) {
                    input.unbindInput(action, b);
                }
            }
            message_ = std::string(list[i].name) + ": no input (not saved)";
        }
        // the settings of each axis of the action
        gui_.columns(4);
        for (size_t k = 0; k < bindings.size(); k++) {
            InputBinding b = bindings[k];
            if (!isAxisBinding(b)) {
                continue;
            }
            bool changed = false;
            gui_.label(("    " + bindingText(b)).c_str(), r4::ui::Font::SMALL);
            float deadzone = b.deadzone >= 0 ? b.deadzone : kDefaultDeadzone;
            if (gui_.slider(fmt("Deadzone##%zu %zu", i, k).c_str(), deadzone, 0, 0.5f)) {
                b.deadzone = deadzone;
                changed = true;
            }
            if (gui_.slider(fmt("Curve##%zu %zu", i, k).c_str(), b.curve, 1, 3)) {
                changed = true;
            }
            bool inverted = b.scale < 0;
            if (gui_.toggle(fmt("Inverted##%zu %zu", i, k).c_str(), inverted)) {
                b.scale = -b.scale;
                changed = true;
            }
            if (changed) {
                input.setBinding(action, k, b);
                message_ = std::string(list[i].name) + ": " + bindingText(b) + " (not saved)";
            }
        }
    }
    gui_.columns(1);
}

// the controllers and the joysticks plugged in, their axes and the buttons pressed
void R4ConfigPanel::devices() {
    EventManager &events = EventManager::getInstance();
    if (events.getJoystickCount() == 0) {
        gui_.label("No controller or joystick.");
    }
    for (int d = 0; d < events.getJoystickCount(); d++) {
        SDL_GameController *controller = events.getGameController(d);
        SDL_Joystick *joystick = events.getJoystick(d);
        if (joystick == nullptr) {
            continue;
        }
        gui_.columns(1);
        gui_.space();
        gui_.label(fmt("%s (%s)", events.getJoystickName(d).c_str(), controller != nullptr ? "controller" : "joystick").c_str());
        gui_.label(("GUID " + events.getJoystickGuid(d)).c_str(), r4::ui::Font::SMALL);
        std::string pressed;
        gui_.columns(3);
        if (controller != nullptr) {
            for (int a = 0; a < SDL_CONTROLLER_AXIS_MAX; a++) {
                float v = SDL_GameControllerGetAxis(controller, (SDL_GameControllerAxis)a) / 32767.0f;
                const char *name = SDL_GameControllerGetStringForAxis((SDL_GameControllerAxis)a);
                gui_.slider(fmt("%s##%d %d", name != nullptr ? name : "?", d, a).c_str(), v, -1, 1, "%.2f", false, false);
            }
            for (int k = 0; k < SDL_CONTROLLER_BUTTON_MAX; k++) {
                if (SDL_GameControllerGetButton(controller, (SDL_GameControllerButton)k)) {
                    const char *name = SDL_GameControllerGetStringForButton((SDL_GameControllerButton)k);
                    pressed += std::string(pressed.empty() ? "" : ", ") + (name != nullptr ? name : "?");
                }
            }
        } else {
            for (int a = 0; a < SDL_JoystickNumAxes(joystick); a++) {
                float v = SDL_JoystickGetAxis(joystick, a) / 32767.0f;
                gui_.slider(fmt("Axis %d##%d", a, d).c_str(), v, -1, 1, "%.2f", false, false);
            }
            for (int k = 0; k < SDL_JoystickNumButtons(joystick); k++) {
                if (SDL_JoystickGetButton(joystick, k)) {
                    pressed += (pressed.empty() ? "" : ", ") + fmt("%d", k);
                }
            }
        }
        gui_.columns(1);
        gui_.label(("Buttons pressed: " + (pressed.empty() ? std::string("-") : pressed)).c_str());
    }
    gui_.columns(1);
}

void R4ConfigPanel::save() {
    Config &config = Config::getInstance();
    std::string bindings = config.getString("Input", "bindings_file", "none");
    if (bindings == "none" || bindings.empty()) {
        bindings = "default_bindings.cfg";
        config.setString("Input", "bindings_file", bindings);
    }
    config.save(kConfigPath);
    const bool keys = InputActionSystem::getInstance().saveBindings(bindings);
    message_ = std::string("saved: ") + kConfigPath + (keys ? ", " + bindings : ", keys NOT saved");
}
