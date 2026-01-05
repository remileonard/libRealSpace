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
#include "../../engine/gametimer.h"
#include "../../engine/desktoptimer.h"
#define FLOPPY 0


int main(int argc, char* argv[]) {
    GameTimer::getInstance().setTimer(std::make_unique<DesktopTimer>());
    
    RSScreen::setInstance(std::make_unique<DebugScreen>());
    GameEngine::setInstance(std::make_unique<DebugGame>());
    RSScreen &screen = RSScreen::instance();  
    Loader& loader = Loader::getInstance();
    AssetManager& assets = AssetManager::getInstance();
    screen.init(1200,800,0);
    assets.SetBase("./assets");
    loader.init();
    screen.is_spfx_finished = false;
    while (!screen.is_spfx_finished) {
        screen.fxTurnOnTv();
        screen.refresh();
        SDL_PumpEvents();
    }
    loader.startLoading([&assets](Loader* loader) {
        loader->setProgress(0.0f);
        RSFontManager& FontManager = RSFontManager::getInstance();
        // Load all TREs and PAKs
        std::vector<std::string> cdTreFiles = {
            "BIGTRE.TRE",
            "LILTRE.TRE",
        };
        loader->setProgress(10.0f);
        assets.ReadISOImage("./SC.DAT");
        loader->setProgress(20.0f);
        assets.init(cdTreFiles);
        loader->setProgress(30.0f);
        
        loader->setProgress(35.0f);
        FontManager.init();
        loader->setProgress(40.0f);
        GameEngine *game = &GameEngine::instance();
        
        
        loader->setProgress(60.0f);
        
        loader->setProgress(80.0f);
        DebugEmptyActivity *main = new DebugEmptyActivity();
        game->init();
        main->init();
        game->addActivity(main);
        loader->setProgress(100.0f);
    });
    
     
    // Boucle de chargement
    while (!loader.isLoadingComplete()) {
        loader.runFrame();
        screen.refresh();
        SDL_Delay(10);
        SDL_PumpEvents();
    }
    loader.close();
    
    GameEngine *game = &GameEngine::instance();
    game->run();
    return EXIT_SUCCESS;
}