//
//  main.cpp
//  libRealSpace
//
//  Created by Fabien Sanglard on 1/25/2014.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//

#include "../../strike_commander/precomp.h"
#include "DebugGame.h"
#include "DebugScreen.h"
#include "DebugEmptyActivity.h"
#define FLOPPY 0


int main(int argc, char* argv[]) {
    
    RSScreen::setInstance(std::make_unique<DebugScreen>());
    GameEngine::setInstance(std::make_unique<DebugGame>());
    RSScreen &screen = RSScreen::instance();  
    Loader& loader = Loader::getInstance();
    AssetManager& assets = AssetManager::getInstance();
    screen.init(1200,800,0);
    assets.SetBase("./assets");

    screen.is_spfx_finished = false;
    while (!screen.is_spfx_finished) {
        screen.fxTurnOnTv();
        screen.refresh();
        SDL_PumpEvents();
    }
    
    
     
    GameEngine *game = &GameEngine::instance();
    DebugEmptyActivity *main = new DebugEmptyActivity();
    game->init();
    main->init();
    game->addActivity(main);
    game->run();
    return EXIT_SUCCESS;
}