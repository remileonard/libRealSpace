//
//  Game.h
//  libRealSpace
//
//  Created by Fabien Sanglard on 1/25/2014.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//

#ifndef __libRealSpace__Game__
#define __libRealSpace__Game__

#define HEIGHT 1080
#define WIDTH 1920
#define FULLSCREEN 1
#include <stack>
#include <vector>
#include <string>
#include <SDL.h>
#include "../commons/Maths.h"
#include "SCMouse.h"
#include "RSScreen.h"
#include "RSVGA.h"
#include "SCRenderer.h"
#include "IActivity.h"
#include "Loader.h"
#include "keyboard.h"
#include "../realspace/AssetManager.h"
#include "../realspace/RSSound.h"
#include "../realspace/ConvAssetManager.h"
#include "../strike_commander/SCState.h"
#include "../realspace/RSFontManager.h"

enum SimActionOfst {
    THROTTLE_UP = 1,
    THROTTLE_DOWN = 2,
    THROTTLE_10 = 3,
    THROTTLE_20 = 4,
    THROTTLE_30 = 5,
    THROTTLE_40 = 6,
    THROTTLE_50 = 7,
    THROTTLE_60 = 8,
    THROTTLE_70 = 9,
    THROTTLE_80 = 10,
    THROTTLE_90 = 11,
    THROTTLE_100 = 12,
    AUTOPILOT = 13,
    LOOK_LEFT = 14,
    LOOK_RIGHT = 15,
    LOOK_BEHIND = 16,
    LOOK_FORWARD = 17,
    PITCH_UP = 18,
    PITCH_DOWN = 19,
    ROLL_LEFT = 20,
    ROLL_RIGHT = 21,
    FIRE_PRIMARY = 22,
    TOGGLE_MOUSE = 23,
    LANDING_GEAR = 24,
    TOGGLE_BRAKES = 25,
    TOGGLE_FLAPS = 26,
    TARGET_NEAREST = 27,
    MDFS_RADAR = 28,
    MDFS_DAMAGE = 29,
    MDFS_WEAPONS = 30,
    SHOW_NAVMAP = 31,
    CHAFF = 32,
    FLARE = 33,
    RADAR_ZOOM_IN = 34,
    RADAR_ZOOM_OUT = 35,
    COMM_RADIO = 36,
    COMM_RADIO_M1 = 37,
    COMM_RADIO_M2 = 38,
    COMM_RADIO_M3 = 39,
    COMM_RADIO_M4 = 40,
    COMM_RADIO_M5 = 41,
    VIEW_TARGET = 42,
    VIEW_BEHIND = 43,
    VIEW_COCKPIT = 44,
    VIEW_WEAPONS = 45,
    MDFS_TARGET_CAMERA = 46,
    PAUSE = 47,
    EYES_ON_TARGET = 48,
    END_MISSION = 49,
    MOUSE_X = 50,
    MOUSE_Y = 51,
    COMM_RADIO_M6 = 52,
    COMM_RADIO_M7 = 53,
    COMM_RADIO_M8 = 54,
    CONTROLLER_STICK_LEFT_X = 55,
    CONTROLLER_STICK_LEFT_Y = 56,
    CONTROLLER_STICK_RIGHT_X = 57,
    CONTROLLER_STICK_RIGHT_Y = 58,
    RADAR_MODE_TOGGLE = 59,
    SPEC_KEY_1 = 60,
    SPEC_KEY_2 = 61
};

class GameEngine{
private:
    inline static std::unique_ptr<GameEngine> s_instance{};
    
protected:
    std::stack<IActivity*> activities;
    Loader *loader{nullptr};
    Keyboard *m_keyboard{nullptr};
    RSVGA &VGA = RSVGA::getInstance();
    SCRenderer &Renderer = SCRenderer::instance();
    SCMouse &Mouse = SCMouse::getInstance();
    AssetManager &Assets = AssetManager::getInstance();
    RSSound &Sound = RSSound::getInstance();
    ConvAssetManager &ConvAssets = ConvAssetManager::getInstance();
    SCState &GameState = SCState::getInstance();
    RSFontManager &FontManager = RSFontManager::getInstance();
    RSScreen *Screen = &RSScreen::getInstance();
    Point2D lastControllerPosition{0,0};

public:
    static GameEngine& getInstance() {
        if (!GameEngine::hasInstance()) {
            GameEngine::setInstance(std::make_unique<GameEngine>());
        }
        GameEngine& instance = GameEngine::instance();
        return instance;
    };
    static GameEngine& instance() {
        return *s_instance;
    }
    static void setInstance(std::unique_ptr<GameEngine> inst) {
        s_instance = std::move(inst);
    }
    static bool hasInstance() { return (bool)s_instance; }

    bool direct_mouse_control{false};
    GameEngine();
    ~GameEngine();
    
    virtual void init(void);
    virtual void run(void);
    virtual void terminate(const char* reason, ...);
    
    virtual void log(const char* text, ...);
    virtual void logError(const char* text, ...);
    virtual void addActivity(IActivity* activity);
    bool hasActivity() { return !activities.empty(); };
    virtual void stopTopActivity(void);
    virtual IActivity* getCurrentActivity(void);
    Keyboard* getKeyboard() const { return m_keyboard; }
    void initKeyboard();
    virtual void pumpEvents(void);

};


#endif /* defined(__libRealSpace__Game__) */
