//
//  R4ConfigPanel.h
//  libRealSpace
//
//  The configuration of the game in a panel of R4nkine, shown and hidden with Ctrl+F11: the options of config.ini, each with the
//  moment it applies (now, at the next mission, at the next start), and the keys of the actions of the flight. While it is shown,
//  the game gets no key and no mouse: an event filter of SDL keeps the events for the panel, and resets the state of the keyboard
//  that the game reads (EventManager). The controllers and the joysticks still act: the panel only looks at their events.
//
#pragma once

#include "r4/gpu/device.h"
#include "r4/ui/ui.h"

#include "engine/InputActionSystem.h"

#include <SDL.h>

#include <string>
#include <vector>

class R4ConfigPanel {
public:
    explicit R4ConfigPanel(r4::gpu::Device &device);
    ~R4ConfigPanel();
    R4ConfigPanel(const R4ConfigPanel &) = delete;
    R4ConfigPanel &operator=(const R4ConfigPanel &) = delete;

    // after the post process and the panel of R4nkine, in the window
    void draw(SDL_Window *window, int pixelWidth, int pixelHeight);

private:
    static int filter(void *panel, SDL_Event *event);
    void show(bool shown);
    void options();
    void keys();
    void devices();
    void look(const SDL_Event &event);
    void save();

    r4::ui::Context gui_;
    bool shown_ = false;
    // Ctrl+F11, caught by the filter: the state of the keyboard is reset while the panel is shown
    bool toggle_ = false;
    bool leftDown_ = false;
    bool rightDown_ = false;
    float wheel_ = 0;
    SDL_bool gameRelative_ = SDL_FALSE;
    int gameCursor_ = SDL_DISABLE;
    // the tab shown: a group of options, then the keys
    int tab_ = 0;
    // since the last frame, caught by the filter: the text typed and the keys of a text field
    std::string text_;
    std::vector<r4::ui::Key> editKeys_;
    SDL_bool gameTextInput_ = SDL_FALSE;
    // the action that waits for a key, -1 for none; the key that the filter caught for it
    int capturing_ = -1;
    SDL_Scancode caught_ = SDL_SCANCODE_UNKNOWN;
    int caughtModifiers_ = 0;
    // a button or an axis of a controller or a joystick that the filter caught, and its device; Add keeps the inputs
    bool caughtDevice_ = false;
    InputType caughtType_ = InputType::GAMEPAD_BUTTON;
    int caughtCode_ = 0;
    float caughtSign_ = 1;
    std::string caughtGuid_;
    int caughtRank_ = 0;
    bool adding_ = false;
    bool cancelled_ = false;
    std::string message_;
};
