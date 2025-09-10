//
//  main.cpp
//  libRealSpace
//
//  Created by Fabien Sanglard on 1/25/2014.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//

#include "../../strike_commander/precomp.h"
#define FLOPPY 0

int main(int argc, char* argv[]) {
    RSScreen::setInstance(std::make_unique<RSScreen>());
    GameEngine::setInstance(std::make_unique<GameEngine>());

    RSScreen &screen = RSScreen::instance();
    Loader& loader = Loader::getInstance();
    AssetManager& assets = AssetManager::getInstance();
    screen.init(WIDTH,HEIGHT,FULLSCREEN);
    screen.is_spfx_finished = false;
    while (!screen.is_spfx_finished) {
        screen.fxTurnOnTv();
        screen.refresh();
        SDL_PumpEvents();
    }
    assets.SetBase("./assets");
    loader.init();
    // Load all TREs and PAKs
    loader.startLoading([](Loader* loader) {
        AssetManager& assets = AssetManager::getInstance();
        loader->setProgress(0.0f);
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
        } else {
            std::vector<std::string> cdTreFiles = {
                "BIGTRE.TRE",
                "LILTRE.TRE",
                "VOCLIST.TRE"
            };
            assets.ReadISOImage("./SC.DAT");
            assets.init(cdTreFiles);
        }
        loader->setProgress(50.0f);
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
        RSMixer::getInstance().init();
        loader->setProgress(60.0f);
        RSFontManager::getInstance().init();
        loader->setProgress(70.0f);
        // Load assets needed for Conversations (char and background)
        ConvAssetManager::getInstance().init();
        loader->setProgress(90.0f);
        RSSound::getInstance().init();
        RSVGA &VGA = RSVGA::getInstance();
        GameEngine *game = &GameEngine::instance();
        VGA.upscale = true;
        game->init();
        //Add MainMenu activity on the game stack.
        SCMainMenu* main = new SCMainMenu();
        main->init();
        game->addActivity(main);
        SCAnimationPlayer *intro = new SCAnimationPlayer();
        intro->init();
        game->addActivity(intro);
        loader->setProgress(100.0f);
    });
    while (!loader.isLoadingComplete()) {
        loader.runFrame();
        screen.refresh();
        SDL_PumpEvents();
    }
    GameEngine *game = &GameEngine::instance();
    game->run();

    return EXIT_SUCCESS;
}