//
//  Game.cpp
//  libRealSpace
//
//  Created by Fabien Sanglard on 1/25/2014.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//

#include "imgui.h"
#include "imgui_impl_opengl2.h"
#include "imgui_impl_sdl2.h"
#include "GameEngine.h"
#include "Loader.h"
#include "InputActionSystem.h"  
#include "EventManager.h"
#include "keyboard.h" 
#include "gametimer.h"

GameEngine::GameEngine() {
}

GameEngine::~GameEngine() {
    if (m_keyboard != nullptr) {
        delete m_keyboard;
        m_keyboard = nullptr;
    }
}


void GameEngine::init() {

    // Load Main Palette and Initialize the GL

    VGA.init(WIDTH,HEIGHT);
    Renderer.init(WIDTH,HEIGHT);
    // Load the Mouse Cursor
    Mouse.init();
    // Crée le clavier (abstraction)
    this->initKeyboard();

}

void GameEngine::initKeyboard() {
    m_keyboard = new Keyboard();
    // Enregistrer actions souris (position + boutons)
    m_keyboard->registerAction(InputAction::MOUSE_POS_X);
    m_keyboard->registerAction(InputAction::MOUSE_POS_Y);
    m_keyboard->registerAction(InputAction::MOUSE_LEFT);
    m_keyboard->registerAction(InputAction::MOUSE_MIDDLE);
    m_keyboard->registerAction(InputAction::MOUSE_RIGHT);

    // Bind position absolue (axis: 0 = X, 1 = Y)
    m_keyboard->bindMousePositionToAction(InputAction::MOUSE_POS_X, 0, 1.0f);
    m_keyboard->bindMousePositionToAction(InputAction::MOUSE_POS_Y, 1, 1.0f);

    // Bind boutons (indices SDL : left=1, middle=2, right=3)
    m_keyboard->bindMouseButtonToAction(InputAction::MOUSE_LEFT,   SDL_BUTTON_LEFT);
    m_keyboard->bindMouseButtonToAction(InputAction::MOUSE_MIDDLE, SDL_BUTTON_MIDDLE);
    m_keyboard->bindMouseButtonToAction(InputAction::MOUSE_RIGHT,  SDL_BUTTON_RIGHT);

    m_keyboard->registerAction(InputAction::KEY_ESCAPE);
    m_keyboard->bindKeyToAction(InputAction::KEY_ESCAPE, SDL_SCANCODE_ESCAPE);

    // Créer et enregistrer les actions du simulateur
    std::vector<SimActionOfst> allActions = {
        SimActionOfst::THROTTLE_UP,
        SimActionOfst::THROTTLE_DOWN,
        SimActionOfst::THROTTLE_10,
        SimActionOfst::THROTTLE_20,
        SimActionOfst::THROTTLE_30,
        SimActionOfst::THROTTLE_40,
        SimActionOfst::THROTTLE_50,
        SimActionOfst::THROTTLE_60,
        SimActionOfst::THROTTLE_70,
        SimActionOfst::THROTTLE_80,
        SimActionOfst::THROTTLE_90,
        SimActionOfst::THROTTLE_100,
        SimActionOfst::AUTOPILOT,
        SimActionOfst::LOOK_LEFT,
        SimActionOfst::LOOK_RIGHT,
        SimActionOfst::LOOK_BEHIND,
        SimActionOfst::LOOK_FORWARD,
        SimActionOfst::PITCH_UP,
        SimActionOfst::PITCH_DOWN,
        SimActionOfst::ROLL_LEFT,
        SimActionOfst::ROLL_RIGHT,
        SimActionOfst::FIRE_PRIMARY,
        SimActionOfst::TOGGLE_MOUSE,
        SimActionOfst::LANDING_GEAR,
        SimActionOfst::TOGGLE_BRAKES,
        SimActionOfst::TOGGLE_FLAPS,
        SimActionOfst::TARGET_NEAREST,
        SimActionOfst::MDFS_RADAR,
        SimActionOfst::MDFS_DAMAGE,
        SimActionOfst::MDFS_WEAPONS,
        SimActionOfst::SHOW_NAVMAP,
        SimActionOfst::CHAFF,
        SimActionOfst::FLARE,
        SimActionOfst::RADAR_ZOOM_IN,
        SimActionOfst::RADAR_ZOOM_OUT,
        SimActionOfst::COMM_RADIO,
        SimActionOfst::COMM_RADIO_M1,
        SimActionOfst::COMM_RADIO_M2,
        SimActionOfst::COMM_RADIO_M3,
        SimActionOfst::COMM_RADIO_M4,
        SimActionOfst::COMM_RADIO_M5,
        SimActionOfst::VIEW_TARGET,
        SimActionOfst::VIEW_BEHIND,
        SimActionOfst::VIEW_COCKPIT,
        SimActionOfst::VIEW_WEAPONS,
        SimActionOfst::MDFS_TARGET_CAMERA,
        SimActionOfst::PAUSE,
        SimActionOfst::EYES_ON_TARGET,
        SimActionOfst::END_MISSION,
        SimActionOfst::MOUSE_X,
        SimActionOfst::MOUSE_Y,
        SimActionOfst::COMM_RADIO_M6,
        SimActionOfst::COMM_RADIO_M7,
        SimActionOfst::COMM_RADIO_M8,
        SimActionOfst::CONTROLLER_STICK_LEFT_X,
        SimActionOfst::CONTROLLER_STICK_LEFT_Y,
        SimActionOfst::CONTROLLER_STICK_RIGHT_X,
        SimActionOfst::CONTROLLER_STICK_RIGHT_Y,
        SimActionOfst::RADAR_MODE_TOGGLE
    };
    
    for (auto action : allActions) {
        m_keyboard->registerAction(CreateAction(InputAction::SIM_START, action));
    }
    // Association des touches aux actions
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::THROTTLE_UP), SDL_SCANCODE_EQUALS);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::THROTTLE_DOWN), SDL_SCANCODE_MINUS);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::THROTTLE_10), SDL_SCANCODE_1);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::THROTTLE_20), SDL_SCANCODE_2);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::THROTTLE_30), SDL_SCANCODE_3);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::THROTTLE_40), SDL_SCANCODE_4);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::THROTTLE_50), SDL_SCANCODE_5);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::THROTTLE_60), SDL_SCANCODE_6);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::THROTTLE_70), SDL_SCANCODE_7);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::THROTTLE_80), SDL_SCANCODE_8);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::THROTTLE_90), SDL_SCANCODE_9);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::THROTTLE_100), SDL_SCANCODE_0);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::AUTOPILOT), SDL_SCANCODE_A);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::LOOK_LEFT), SDL_SCANCODE_F3);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::LOOK_RIGHT), SDL_SCANCODE_F4);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::LOOK_BEHIND), SDL_SCANCODE_F5);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::LOOK_FORWARD), SDL_SCANCODE_F1);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::PITCH_UP), SDL_SCANCODE_UP);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::PITCH_DOWN), SDL_SCANCODE_DOWN);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::ROLL_LEFT), SDL_SCANCODE_LEFT);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::ROLL_RIGHT), SDL_SCANCODE_RIGHT);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::FIRE_PRIMARY), SDL_SCANCODE_SPACE);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::TOGGLE_MOUSE), SDL_SCANCODE_M);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::LANDING_GEAR), SDL_SCANCODE_L);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::TOGGLE_BRAKES), SDL_SCANCODE_B);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::TOGGLE_FLAPS), SDL_SCANCODE_F);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::TARGET_NEAREST), SDL_SCANCODE_T);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::MDFS_RADAR), SDL_SCANCODE_R);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::MDFS_DAMAGE), SDL_SCANCODE_D);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::MDFS_WEAPONS), SDL_SCANCODE_W);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::SHOW_NAVMAP), SDL_SCANCODE_N);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::CHAFF), SDL_SCANCODE_SEMICOLON);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::FLARE),SDL_SCANCODE_APOSTROPHE);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::RADAR_ZOOM_IN), SDL_SCANCODE_LEFTBRACKET);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::RADAR_ZOOM_OUT), SDL_SCANCODE_RIGHTBRACKET);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::COMM_RADIO), SDL_SCANCODE_C);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::COMM_RADIO_M1), SDL_SCANCODE_1);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::COMM_RADIO_M2), SDL_SCANCODE_2);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::COMM_RADIO_M3), SDL_SCANCODE_3);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::COMM_RADIO_M4), SDL_SCANCODE_4);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::COMM_RADIO_M5), SDL_SCANCODE_5);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::VIEW_TARGET), SDL_SCANCODE_F7);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::VIEW_BEHIND), SDL_SCANCODE_F2);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::VIEW_COCKPIT), SDL_SCANCODE_F6);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::VIEW_WEAPONS), SDL_SCANCODE_F8);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::MDFS_TARGET_CAMERA), SDL_SCANCODE_F9);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::SPEC_KEY_1), SDL_SCANCODE_F10);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::SPEC_KEY_2), SDL_SCANCODE_F11);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::PAUSE), SDL_SCANCODE_P);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::EYES_ON_TARGET), SDL_SCANCODE_Y);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::END_MISSION), SDL_SCANCODE_ESCAPE);
    m_keyboard->bindMousePositionToAction(CreateAction(InputAction::SIM_START, SimActionOfst::MOUSE_X), 0, 1.0f);
    m_keyboard->bindMousePositionToAction(CreateAction(InputAction::SIM_START, SimActionOfst::MOUSE_Y), 1, 1.0f);
    m_keyboard->bindMouseButtonToAction(CreateAction(InputAction::SIM_START, SimActionOfst::FIRE_PRIMARY), SDL_BUTTON_LEFT);
    m_keyboard->bindMouseButtonToAction(CreateAction(InputAction::SIM_START, SimActionOfst::TARGET_NEAREST), SDL_BUTTON_MIDDLE);
    m_keyboard->bindMouseButtonToAction(CreateAction(InputAction::SIM_START, SimActionOfst::MDFS_WEAPONS), SDL_BUTTON_RIGHT);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::COMM_RADIO_M6),SDL_SCANCODE_6);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::COMM_RADIO_M7),SDL_SCANCODE_7);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::COMM_RADIO_M8),SDL_SCANCODE_8);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::SIM_START, SimActionOfst::RADAR_MODE_TOGGLE), SDL_SCANCODE_V);
    m_keyboard->bindGamepadButtonToAction(CreateAction(InputAction::SIM_START, SimActionOfst::FIRE_PRIMARY), 0, SDL_CONTROLLER_BUTTON_A);
    m_keyboard->bindGamepadButtonToAction(CreateAction(InputAction::SIM_START, SimActionOfst::TARGET_NEAREST), 0, SDL_CONTROLLER_BUTTON_B);
    m_keyboard->bindGamepadButtonToAction(CreateAction(InputAction::SIM_START, SimActionOfst::MDFS_RADAR), 0, SDL_CONTROLLER_BUTTON_X);
    m_keyboard->bindGamepadButtonToAction(CreateAction(InputAction::SIM_START, SimActionOfst::MDFS_WEAPONS), 0, SDL_CONTROLLER_BUTTON_Y);
    m_keyboard->bindGamepadButtonToAction(CreateAction(InputAction::SIM_START, SimActionOfst::THROTTLE_DOWN), 0, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
    m_keyboard->bindGamepadButtonToAction(CreateAction(InputAction::SIM_START, SimActionOfst::THROTTLE_UP), 0, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
    m_keyboard->bindGamepadButtonToAction(CreateAction(InputAction::SIM_START, SimActionOfst::TOGGLE_BRAKES), 0, SDL_CONTROLLER_BUTTON_DPAD_UP);
    m_keyboard->bindGamepadButtonToAction(CreateAction(InputAction::SIM_START, SimActionOfst::TOGGLE_FLAPS), 0, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
    m_keyboard->bindGamepadButtonToAction(CreateAction(InputAction::SIM_START, SimActionOfst::LANDING_GEAR), 0, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
    m_keyboard->bindGamepadButtonToAction(CreateAction(InputAction::SIM_START, SimActionOfst::AUTOPILOT), 0, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
    m_keyboard->bindGamepadButtonToAction(CreateAction(InputAction::SIM_START, SimActionOfst::PAUSE), 0, SDL_CONTROLLER_BUTTON_START);
    m_keyboard->bindGamepadButtonToAction(CreateAction(InputAction::SIM_START, SimActionOfst::SHOW_NAVMAP), 0, SDL_CONTROLLER_BUTTON_BACK);

    m_keyboard->bindGamepadAxisToAction(CreateAction(InputAction::SIM_START, SimActionOfst::CONTROLLER_STICK_LEFT_X), 0, SDL_CONTROLLER_AXIS_LEFTX, 1.0f);
    m_keyboard->bindGamepadAxisToAction(CreateAction(InputAction::SIM_START, SimActionOfst::CONTROLLER_STICK_LEFT_Y), 0, SDL_CONTROLLER_AXIS_LEFTY, -1.0f);

    m_keyboard->bindGamepadAxisToAction(CreateAction(InputAction::SIM_START, SimActionOfst::CONTROLLER_STICK_RIGHT_X), 0, SDL_CONTROLLER_AXIS_RIGHTX, 1.0f);
    m_keyboard->bindGamepadAxisToAction(CreateAction(InputAction::SIM_START, SimActionOfst::CONTROLLER_STICK_RIGHT_Y), 0, SDL_CONTROLLER_AXIS_RIGHTY, -1.0f);

    m_keyboard->bindGamepadAxisToAction(InputAction::CONTROLLER_STICK_LEFT_X, 0, SDL_CONTROLLER_AXIS_LEFTX, 1.0f);
    m_keyboard->bindGamepadAxisToAction(InputAction::CONTROLLER_STICK_LEFT_Y, 0, SDL_CONTROLLER_AXIS_LEFTY, -1.0f);
    m_keyboard->bindGamepadButtonToAction(InputAction::CONTROLLER_BUTTON_A, 0, SDL_CONTROLLER_BUTTON_A);
    m_keyboard->bindGamepadButtonToAction(InputAction::CONTROLLER_BUTTON_B, 0, SDL_CONTROLLER_BUTTON_B);
    m_keyboard->bindGamepadButtonToAction(InputAction::CONTROLLER_BUTTON_BACK, 0, SDL_CONTROLLER_BUTTON_BACK);
    m_keyboard->bindGamepadButtonToAction(InputAction::CONTROLLER_BUTTON_START, 0, SDL_CONTROLLER_BUTTON_START);

    m_keyboard->bindMouseAxisToAction(InputAction::MOUSE_DIFF_X, 0, 1.0f);
    m_keyboard->bindMouseAxisToAction(InputAction::MOUSE_DIFF_Y, 1, 1.0f);
}

void GameEngine::pumpEvents(void) {

    // Met à jour tout (Keyboard encapsule InputActionSystem/EventManager)
    m_keyboard->update();

    // Position absolue (pixels fenêtre)
    int px, py;
    m_keyboard->getMouseAbsolutePosition(px, py);

    float mx = m_keyboard->getActionValue(InputAction::MOUSE_DIFF_X);
    float my = m_keyboard->getActionValue(InputAction::MOUSE_DIFF_Y);
    
    float cx = m_keyboard->getActionValue(InputAction::CONTROLLER_STICK_LEFT_X);
    float cy = m_keyboard->getActionValue(InputAction::CONTROLLER_STICK_LEFT_Y);

    
    
    // Conversion vers l’espace 320x200 legacy
    Point2D newPosition;
    if (cx != 0 || cy != 0) {
        lastControllerPosition.x += cx * 4;
        lastControllerPosition.y -= cy * 4;
    } else if (mx != 0 || my != 0) {
        if (direct_mouse_control) {
            lastControllerPosition.x = (px * 320) / Screen->width;
            lastControllerPosition.y = (py * 200) / Screen->height;
        } else {
            lastControllerPosition.x += mx;
            lastControllerPosition.y += my;
        }
        
    }
    if (lastControllerPosition.x < 0) lastControllerPosition.x = 0;
    if (lastControllerPosition.y < 0) lastControllerPosition.y = 0;
    if (lastControllerPosition.x > 320) lastControllerPosition.x = 320;
    if (lastControllerPosition.y > 200) lastControllerPosition.y = 200; 
    newPosition.x = lastControllerPosition.x;
    newPosition.y = lastControllerPosition.y;
    Mouse.setPosition(newPosition);
    Mouse.setVisible(true);
    // --- Fusion souris + contrôleur sur les 3 boutons legacy (0:Left,1:Middle,2:Right)
    // On combine les états "down" de chaque source puis on émet UNE transition.
    static bool prevDown[3] = { false, false, false };

    auto isDown = [&](InputAction a) -> bool {
        // Hypothèse: getActionValue(a) retourne 1.0f si enfoncé, 0.0f sinon.
        // Si ce n'est pas le cas, exposez un isActionDown(a) et utilisez-le ici.
        return m_keyboard->getActionValue(a) > 0.5f;
    };

    bool currentDown[3];
    currentDown[0] = isDown(InputAction::MOUSE_LEFT)   || isDown(InputAction::CONTROLLER_BUTTON_A);
    currentDown[1] = isDown(InputAction::MOUSE_MIDDLE) || isDown(InputAction::CONTROLLER_BUTTON_B);
    currentDown[2] = isDown(InputAction::MOUSE_RIGHT); // ajoutez ici d'autres boutons pad si besoin

    for (int i = 0; i < 3; ++i) {
        MouseButton::EventType e = MouseButton::NONE;
        if ( currentDown[i] && !prevDown[i] ) {
            e = MouseButton::PRESSED;
        } else if ( !currentDown[i] && prevDown[i] ) {
            e = MouseButton::RELEASED;
        }
        Mouse.buttons[i].event = e;
        prevDown[i] = currentDown[i];
    }

    if (EventManager::getInstance().shouldQuit()) {
        terminate("System request.");
        return;
    }
}

void GameEngine::run() {

    IActivity *currentActivity;
    while (activities.size() > 0) {
        GameTimer::getInstance().update();
        pumpEvents();

        currentActivity = activities.top();
        if (currentActivity->isRunning()) {
            currentActivity->focus();
            currentActivity->runFrame();
        } else {
            activities.pop();
            delete currentActivity;
        }

        Screen->refresh();

        Mouse.flushEvents(); // On peut le garder si sa logique interne reste valable.
    }
}

void GameEngine::terminate(const char *reason, ...) {
    log("Terminating: ");
    va_list args;
    va_start(args, reason);
    vfprintf(stdout, reason, args);
    va_end(args);
    log("\n");
    exit(0);
}

void GameEngine::log(const char *text, ...) {
    va_list args;
    va_start(args, text);
    vfprintf(stdout, text, args);
    va_end(args);
}

void GameEngine::logError(const char *text, ...) {
    va_list args;
    va_start(args, text);
    vfprintf(stderr, text, args);
    va_end(args);
}

void GameEngine::addActivity(IActivity *activity) {
    if (activities.size()>0) {
        IActivity *currentActivity;
        currentActivity = activities.top();
        currentActivity->unFocus();
    }
    activity->start();
    this->activities.push(activity);
}

void GameEngine::stopTopActivity(void) {
    IActivity *currentActivity;
    currentActivity = activities.top();
    currentActivity->stop();
}

IActivity *GameEngine::getCurrentActivity(void) { return activities.top(); }
