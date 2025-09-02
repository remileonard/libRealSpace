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
class GameEngine{
    
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

    GameEngine();
    ~GameEngine();
    
    virtual void init(void);
    virtual void run(void);
    virtual void terminate(const char* reason, ...);
    
    virtual void log(const char* text, ...);
    virtual void logError(const char* text, ...);
    virtual void addActivity(IActivity* activity);
    virtual void stopTopActivity(void);
    virtual IActivity* getCurrentActivity(void);
    Keyboard* getKeyboard() const { return m_keyboard; }
    virtual void pumpEvents(void);
protected:
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

private:
    inline static std::unique_ptr<GameEngine> s_instance{};
    std::stack<IActivity*> activities;
    
    
};


#endif /* defined(__libRealSpace__Game__) */
