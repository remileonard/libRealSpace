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
#include "DebugGame.h"
#include "DebugStrike.h"
#include "DebugGameFlow.h"
#include "DebugObjectViewer.h"
#include "DebugControllerActivity.h"
#include "DebugAnimationPlayer.h"

#include "../../engine/keyboard.h"
#include "../../engine/EventManager.h"  

#include "../../strike_commander/precomp.h"


DebugGame::DebugGame() {
    printf("DebugGame constructor\n");
}

DebugGame::~DebugGame() {}

void DebugGame::init() {
    // Load Main Palette and Initialize the GL

    VGA.init(1200,800);
    Renderer.init(1200,800);
    // Load the Mouse Cursor
    Mouse.init();
    this->initKeyboard();

    EventManager::getInstance().enableImGuiForwarding(true);
}

void DebugGame::loadSC() {
    Loader& loader = Loader::getInstance();
    Assets.SetBase("./assets");
    
    // Load all TREs and PAKs
    loader.init();
    loader.startLoading([](Loader* loader) {
        AssetManager &Assets = AssetManager::getInstance();
        std::vector<std::string> cdTreFiles = {
            "GAMEFLOW.TRE",
            "MISC.TRE",
            "MISSIONS.TRE",
            "OBJECTS.TRE",
            "SOUND.TRE",
            "TEXTURES.TRE"
        };
        Assets.init(cdTreFiles);
        
        Assets.intel_root_path = "..\\..\\DATA\\INTEL\\";
        Assets.mission_root_path = "..\\..\\DATA\\MISSIONS\\";
        Assets.object_root_path = "..\\..\\DATA\\OBJECTS\\";
        Assets.sound_root_path = "..\\..\\DATA\\SOUND\\";
        Assets.texture_root_path = "..\\..\\DATA\\TXM\\";
        Assets.gameflow_root_path = "..\\..\\DATA\\GAMEFLOW\\";

        Assets.gameflow_filename = Assets.gameflow_root_path+"GAMEFLOW.IFF";
        Assets.optshps_filename = Assets.gameflow_root_path+"OPTSHPS.PAK";
        Assets.optpals_filename = Assets.gameflow_root_path+"OPTPALS.PAK";
        Assets.optfont_filename = Assets.gameflow_root_path+"OPTFONT.IFF";
        Assets.navmap_filename = "..\\..\\DATA\\COCKPITS\\NAVMAP.IFF";
        Assets.conv_pak_filename = Assets.gameflow_root_path+"CONVSHPS.PAK";
        Assets.option_filename = Assets.gameflow_root_path+"OPTIONS.IFF";
        Assets.conv_data_filename = Assets.gameflow_root_path+"CONVDATA.IFF";
        Assets.conv_pal_filename = Assets.gameflow_root_path+"CONVPALS.PAK";
        Assets.txm_filename = Assets.texture_root_path+"TXMPACK.PAK";
        Assets.acc_filename = Assets.texture_root_path+"ACCPACK.PAK";
        Assets.convpak_filename = Assets.gameflow_root_path+"CONV.PAK";
        
        RSFontManager &FontManager = RSFontManager::getInstance();
        SCMouse::getInstance().init();
        RSMixer::getInstance().init();
        RSSound::getInstance().init();
        FontManager.init();
        ConvAssetManager &ConvAssets = ConvAssetManager::getInstance();
        // Load assets needed for Conversations (char and background)
        ConvAssets.init();
        RSSound::getInstance().init();
        DebugGameFlow* main = new DebugGameFlow();
        SCState &GameState = SCState::getInstance();
        GameState.Reset();
        GameState.player_callsign = "Debug";
        GameState.player_name = "Debug Player";
        GameState.player_firstname = "Debug";
        for (int i=0; i<256; i++) {
            GameState.requierd_flags[i] = false;
        }
        main->init();
        GameEngine *Game = &GameEngine::instance();
        Game->addActivity(main);
    });
}


void DebugGame::loadSCCD() {
    
    Assets.SetBase("./assets");
    // Load all TREs and PAKs
    Loader& loader = Loader::getInstance();
    loader.init();
    loader.startLoading([](Loader* loader) {
        AssetManager &Assets = AssetManager::getInstance();
        ConvAssetManager &ConvAssets = ConvAssetManager::getInstance();
        RSFontManager &FontManager = RSFontManager::getInstance();
        loader->setProgress(0.0f);
        Assets.ReadISOImage("./SC.DAT");
        loader->setProgress(20.0f);
        std::vector<std::string> treFiles = {
            "BIGTRE.TRE",
            "LILTRE.TRE",
            "VOCLIST.TRE"
        };
        Assets.init(treFiles);
        loader->setProgress(50.0f);
        Assets.intel_root_path = "..\\..\\DATA\\INTEL\\";
        Assets.mission_root_path = "..\\..\\DATA\\MISSIONS\\";
        Assets.object_root_path = "..\\..\\DATA\\OBJECTS\\";
        Assets.sound_root_path = "..\\..\\DATA\\SOUND\\";
        Assets.texture_root_path = "..\\..\\DATA\\TXM\\";
        Assets.gameflow_root_path = "..\\..\\DATA\\GAMEFLOW\\";

        Assets.gameflow_filename = Assets.gameflow_root_path+"GAMEFLOW.IFF";
        Assets.optshps_filename = Assets.gameflow_root_path+"OPTSHPS.PAK";
        Assets.optpals_filename = Assets.gameflow_root_path+"OPTPALS.PAK";
        Assets.optfont_filename = Assets.gameflow_root_path+"OPTFONT.IFF";
        Assets.navmap_filename = "..\\..\\DATA\\COCKPITS\\NAVMAP.IFF";
        Assets.conv_pak_filename = Assets.gameflow_root_path+"CONVSHPS.PAK";
        Assets.option_filename = Assets.gameflow_root_path+"OPTIONS.IFF";
        Assets.conv_data_filename = Assets.gameflow_root_path+"CONVDATA.IFF";
        Assets.conv_pal_filename = Assets.gameflow_root_path+"CONVPALS.PAK";
        Assets.txm_filename = Assets.texture_root_path+"TXMPACK.PAK";
        Assets.acc_filename = Assets.texture_root_path+"ACCPACK.PAK";
        Assets.convpak_filename = Assets.gameflow_root_path+"CONV.PAK";
        SCMouse::getInstance().init();
        RSMixer::getInstance().init();
        FontManager.init();
        loader->setProgress(60.0f);
        // Load assets needed for Conversations (char and background)
        ConvAssets.init();
        loader->setProgress(85.0f);
        RSSound::getInstance().init();
        DebugGameFlow* main = new DebugGameFlow();
        SCState &GameState = SCState::getInstance();
        GameState.Reset();
        GameState.player_callsign = "Debug";
        GameState.player_name = "Debug Player";
        GameState.player_firstname = "Debug";
        for (int i=0; i<256; i++) {
            GameState.requierd_flags[i] = false;
        }
        main->init();
        GameEngine *Game = &GameEngine::instance();
        Game->addActivity(main);
        loader->setProgress(100.0f);
    });
    
    
}


void DebugGame::testMissionSC() {
    
    Assets.SetBase("./assets");
    // Load all TREs and PAKs
    Loader& loader = Loader::getInstance();
    loader.init();
    loader.startLoading([](Loader* loader) {
        AssetManager &Assets = AssetManager::getInstance();
        ConvAssetManager &ConvAssets = ConvAssetManager::getInstance();
        RSFontManager &FontManager = RSFontManager::getInstance();
        
        Assets.ReadISOImage("./SC.DAT");
        loader->setProgress(20.0f);
        std::vector<std::string> treFiles = {
            "BIGTRE.TRE",
            "LILTRE.TRE",
            "TOBIGTRE.TRE",
            "PACIFIC.DAT"
        };
        Assets.init(treFiles);
        loader->setProgress(30.0f);
        Assets.intel_root_path = "..\\..\\DATA\\INTEL\\";
        Assets.mission_root_path = "..\\..\\DATA\\MISSIONS\\";
        Assets.object_root_path = "..\\..\\DATA\\OBJECTS\\";
        Assets.sound_root_path = "..\\..\\DATA\\SOUND\\";
        Assets.texture_root_path = "..\\..\\DATA\\TXM\\";
        Assets.gameflow_root_path = "..\\..\\DATA\\GAMEFLOW\\";

        Assets.gameflow_filename = Assets.gameflow_root_path+"GAMEFLOW.IFF";
        Assets.optshps_filename = Assets.gameflow_root_path+"OPTSHPS.PAK";
        Assets.optpals_filename = Assets.gameflow_root_path+"OPTPALS.PAK";
        Assets.optfont_filename = Assets.gameflow_root_path+"OPTFONT.IFF";
        Assets.navmap_filename = "..\\..\\DATA\\COCKPITS\\NAVMAP.IFF";
        Assets.conv_pak_filename = Assets.gameflow_root_path+"CONVSHPS.PAK";
        Assets.option_filename = Assets.gameflow_root_path+"OPTIONS.IFF";
        Assets.conv_data_filename = Assets.gameflow_root_path+"CONVDATA.IFF";
        Assets.conv_pal_filename = Assets.gameflow_root_path+"CONVPALS.PAK";
        Assets.txm_filename = Assets.texture_root_path+"TXMPACK.PAK";
        Assets.acc_filename = Assets.texture_root_path+"ACCPACK.PAK";
        Assets.convpak_filename = Assets.gameflow_root_path+"CONV.PAK";
        SCMouse::getInstance().init();
        FontManager.init();
        RSSound::getInstance().init();
        RSMixer::getInstance().init();
        //Add MainMenu activity on the game stack.
        loader->setProgress(100.0f);
        DebugStrike * main = new DebugStrike();
        main->init();
        main->setMission("MISN-1A.IFF");
        GameEngine *Game = &GameEngine::instance();
        Game->addActivity(main);
    });
}

void DebugGame::loadTO() {
    Loader& loader = Loader::getInstance();
    loader.init();
    loader.startLoading([](Loader* loader) {
        AssetManager &Assets = AssetManager::getInstance();
        ConvAssetManager &ConvAssets = ConvAssetManager::getInstance();
        RSFontManager &FontManager = RSFontManager::getInstance();
        Assets.SetBase("./assets");
        // Load all TREs and PAKs
        
        std::vector<std::string> cdTreFiles = {
            "TOBIGTRE.TRE",
            "LILTRE.TRE",
            "TOVOCLST.TRE"
        };
        Assets.ReadISOImage("./SC.DAT");
        Assets.init(cdTreFiles);
        Assets.intel_root_path = "..\\..\\DATA\\INTEL\\";
        Assets.mission_root_path = "..\\..\\DATA\\MISSIONS\\";
        Assets.object_root_path = "..\\..\\DATA\\OBJECTS\\";
        Assets.sound_root_path = "..\\..\\DATA\\SOUND\\";
        Assets.texture_root_path = "..\\..\\DATA\\TXM\\";
        Assets.gameflow_root_path = "..\\..\\DATA\\GAMEFLOW\\";

        Assets.gameflow_filename = Assets.gameflow_root_path+"GAMEFLO2.IFF";
        Assets.optshps_filename = Assets.gameflow_root_path+"OPTSHPS.PAK";
        Assets.optpals_filename = Assets.gameflow_root_path+"OPTPALS.PAK";
        Assets.optfont_filename = Assets.gameflow_root_path+"OPTFONT.IFF";
        Assets.navmap_filename = "..\\..\\DATA\\COCKPITS\\NAVMAP2.IFF";
        Assets.conv_pak_filename = Assets.gameflow_root_path+"CONVSHPS.PAK";
        Assets.option_filename = Assets.gameflow_root_path+"OPTIONS.IFF";
        Assets.conv_data_filename = Assets.gameflow_root_path+"CONVDATA.IFF";
        Assets.conv_pal_filename = Assets.gameflow_root_path+"CONVPALS.PAK";
        Assets.txm_filename = Assets.texture_root_path+"TXMPACK.PAK";
        Assets.acc_filename = Assets.texture_root_path+"ACCPACK.PAK";
        Assets.convpak_filename = Assets.gameflow_root_path+"CONV2.PAK";
        SCMouse::getInstance().init();
        FontManager.init();
        // Load assets needed for Conversations (char and background)
        ConvAssets.init();
        RSSound::getInstance().init();
        RSMixer::getInstance().init();
        DebugGameFlow* main = new DebugGameFlow();
        SCState &GameState = SCState::getInstance();
        GameState.Reset();
        GameState.player_callsign = "Debug";
        GameState.player_name = "Debug Player";
        GameState.player_firstname = "Debug";
        for (int i=0; i<256; i++) {
            GameState.requierd_flags[i] = false;
        }
        
        main->init();
        GameEngine *Game = &GameEngine::instance();
        Game->addActivity(main);
    });
}


void DebugGame::testObjects() {
    Assets.SetBase("./assets");
    // Load all TREs and PAKs
    Loader& loader = Loader::getInstance();
    loader.init();
    loader.startLoading([](Loader* loader) {
        AssetManager &Assets = AssetManager::getInstance();
        ConvAssetManager &ConvAssets = ConvAssetManager::getInstance();
        RSFontManager &FontManager = RSFontManager::getInstance();
        std::vector<std::string> cdTreFiles = {
            "TOBIGTRE.TRE",
            "LILTRE.TRE"
        };
        Assets.ReadISOImage("./SC.DAT");
        Assets.init(cdTreFiles);
        Assets.intel_root_path = "..\\..\\DATA\\INTEL\\";
        Assets.mission_root_path = "..\\..\\DATA\\MISSIONS\\";
        Assets.object_root_path = "..\\..\\DATA\\OBJECTS\\";
        Assets.sound_root_path = "..\\..\\DATA\\SOUND\\";
        Assets.texture_root_path = "..\\..\\DATA\\TXM\\";
        Assets.gameflow_root_path = "..\\..\\DATA\\GAMEFLOW\\";

        Assets.gameflow_filename = Assets.gameflow_root_path+"GAMEFLO2.IFF";
        Assets.optshps_filename = Assets.gameflow_root_path+"OPTSHPS.PAK";
        Assets.optpals_filename = Assets.gameflow_root_path+"OPTPALS.PAK";
        Assets.optfont_filename = Assets.gameflow_root_path+"OPTFONT.IFF";
        Assets.navmap_filename = "..\\..\\DATA\\COCKPITS\\NAVMAP2.IFF";
        Assets.conv_pak_filename = Assets.gameflow_root_path+"CONVSHPS.PAK";
        Assets.option_filename = Assets.gameflow_root_path+"OPTIONS.IFF";
        Assets.conv_data_filename = Assets.gameflow_root_path+"CONVDATA.IFF";
        Assets.conv_pal_filename = Assets.gameflow_root_path+"CONVPALS.PAK";
        Assets.txm_filename = Assets.texture_root_path+"TXMPACK.PAK";
        Assets.acc_filename = Assets.texture_root_path+"ACCPACK.PAK";
        Assets.convpak_filename = Assets.gameflow_root_path+"CONV2.PAK";
        SCMouse::getInstance().init();
        RSMixer::getInstance().init();
        RSSound::getInstance().init();
        FontManager.init();
        DebugObjectViewer* main = new DebugObjectViewer();
        main->init();
        GameEngine *Game = &GameEngine::instance();
        Game->addActivity(main);
    });
    
}

void DebugGame::testController() {
    DebugControllerActivity* main = new DebugControllerActivity();
    main->init();
    GameEngine *Game = &GameEngine::instance();
    Game->addActivity(main);
}

void DebugGame::testMidGames() {
    Assets.SetBase("./assets");
    // Load all TREs and PAKs
    Loader& loader = Loader::getInstance();
    loader.init();
    loader.startLoading([](Loader* loader) {
        AssetManager &Assets = AssetManager::getInstance();
        ConvAssetManager &ConvAssets = ConvAssetManager::getInstance();
        RSFontManager &FontManager = RSFontManager::getInstance();
        std::vector<std::string> cdTreFiles = {
            "BIGTRE.TRE",
            "LILTRE.TRE"
        };
        Assets.ReadISOImage("./SC.DAT");
        Assets.init(cdTreFiles);
        Assets.intel_root_path = "..\\..\\DATA\\INTEL\\";
        Assets.mission_root_path = "..\\..\\DATA\\MISSIONS\\";
        Assets.object_root_path = "..\\..\\DATA\\OBJECTS\\";
        Assets.sound_root_path = "..\\..\\DATA\\SOUND\\";
        Assets.texture_root_path = "..\\..\\DATA\\TXM\\";
        Assets.gameflow_root_path = "..\\..\\DATA\\GAMEFLOW\\";

        Assets.gameflow_filename = Assets.gameflow_root_path+"GAMEFLO2.IFF";
        Assets.optshps_filename = Assets.gameflow_root_path+"OPTSHPS.PAK";
        Assets.optpals_filename = Assets.gameflow_root_path+"OPTPALS.PAK";
        Assets.optfont_filename = Assets.gameflow_root_path+"OPTFONT.IFF";
        Assets.navmap_filename = "..\\..\\DATA\\COCKPITS\\NAVMAP2.IFF";
        Assets.conv_pak_filename = Assets.gameflow_root_path+"CONVSHPS.PAK";
        Assets.option_filename = Assets.gameflow_root_path+"OPTIONS.IFF";
        Assets.conv_data_filename = Assets.gameflow_root_path+"CONVDATA.IFF";
        Assets.conv_pal_filename = Assets.gameflow_root_path+"CONVPALS.PAK";
        Assets.txm_filename = Assets.texture_root_path+"TXMPACK.PAK";
        Assets.acc_filename = Assets.texture_root_path+"ACCPACK.PAK";
        Assets.convpak_filename = Assets.gameflow_root_path+"CONV2.PAK";
        FontManager.init();
        SCMouse::getInstance().init();
        RSMixer::getInstance().init();
        RSSound::getInstance().init();
        DebugAnimationPlayer* main = new DebugAnimationPlayer();
        main->init();
        main->pause = true;
        GameEngine *Game = &GameEngine::instance();
        Game->addActivity(main);
    });
}

void DebugGame::loadPacific() {
    Assets.SetBase("./assets");
    // Load all TREs and PAKs
    
    std::vector<std::string> cdTreFiles = {
        "PACIFIC.DAT",
    };
    Assets.init(cdTreFiles);
    
    FontManager.init();

    SCObjectViewer* main = new SCObjectViewer();
    main->init();
    this->addActivity(main);
}

void DebugGame::run() {
    Loader &loader = Loader::instance();
    IActivity *currentActivity;
    while (activities.size() > 0) {
        pumpEvents();
        if (!loader.isLoadingComplete()) {
            loader.runFrame();
            SDL_Delay(10);
        } else {
            currentActivity = activities.top();
            if (currentActivity->isRunning()) {
                currentActivity->focus();
                currentActivity->runFrame();
            } else {
                activities.pop();
                delete currentActivity;
            }
        }
        Screen->refresh();
        Mouse.flushEvents(); // On peut le garder si sa logique interne reste valable.
    }
}