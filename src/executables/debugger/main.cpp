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
#define FLOPPY 0


int main(int argc, char* argv[]) {
    
    RSScreen::setInstance(std::make_unique<DebugScreen>());
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
    loader.startLoading([](Loader* loader) {
        loader->setProgress(0.0f);
        AssetManager& assets = AssetManager::getInstance();
        RSMixer& Mixer = RSMixer::getInstance();
        RSFontManager& FontManager = RSFontManager::getInstance();
        // Load all TREs and PAKs
        if (FLOPPY) {
            std::vector<std::string> treFiles = {
                "GAMEFLOW.TRE",
                "OBJECTS.TRE",
                "MISC.TRE",
                "SOUND.TRE",
                "MISSIONS.TRE",
                "TEXTURES.TRE"
            };
            assets.init(treFiles);
            loader->setProgress(80.0f);
        } else {
            std::vector<std::string> cdTreFiles = {
                "BIGTRE.TRE",
                "LILTRE.TRE",
                "VOCLIST.TRE"
            };
            loader->setProgress(10.0f);
            assets.ReadISOImage("./SC.DAT");
            loader->setProgress(20.0f);
            assets.init(cdTreFiles);
            loader->setProgress(30.0f);
        }
        Mixer.init();
        loader->setProgress(35.0f);
        FontManager.init();
        loader->setProgress(40.0f);
        // Load assets needed for Conversations (char and background)
        assets.intel_root_path = "..\\..\\DATA\\INTEL\\";
        assets.mission_root_path = "..\\..\\DATA\\MISSIONS\\";
        assets.object_root_path = "..\\..\\DATA\\OBJECTS\\";
        assets.sound_root_path = "..\\..\\DATA\\SOUND\\";
        assets.texture_root_path = "..\\..\\DATA\\TXM\\";
        assets.gameflow_root_path = "..\\..\\DATA\\GAMEFLOW\\";

        assets.gameflow_filename = assets.gameflow_root_path+"GAMEFLOW.IFF";
        assets.optshps_filename = assets.gameflow_root_path+"OPTSHPS.PAK";
        assets.optpals_filename = assets.gameflow_root_path+"OPTPALS.PAK";
        assets.optfont_filename = assets.gameflow_root_path+"OPTFONT.IFF";
        
        assets.navmap_filename = "..\\..\\DATA\\COCKPITS\\NAVMAP.IFF";
        assets.conv_pak_filename = assets.gameflow_root_path+"CONVSHPS.PAK";
        assets.option_filename = assets.gameflow_root_path+"OPTIONS.IFF";
        assets.conv_data_filename = assets.gameflow_root_path+"CONVDATA.IFF";
        assets.conv_pal_filename = assets.gameflow_root_path+"CONVPALS.PAK";
        assets.txm_filename = assets.texture_root_path+"TXMPACK.PAK";
        assets.acc_filename = assets.texture_root_path+"ACCPACK.PAK";
        assets.convpak_filename = assets.gameflow_root_path+"CONV.PAK";
        // Load assets needed for Conversations (char and background)
        loader->setProgress(100.0f);
    });

    // Boucle de chargement
    while (!loader.isLoadingComplete()) {
        loader.runFrame();
        screen.refresh();
        SDL_PumpEvents();
    }
    loader.close();
    GameEngine::setInstance(std::make_unique<DebugGame>());
    GameEngine &game = GameEngine::getInstance();
    game.init();
    game.run();

    return EXIT_SUCCESS;
}