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
#include "../../engine/Keyboard.h"
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
    // Crée le clavier (abstraction)
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
    EventManager::getInstance().enableImGuiForwarding(true);
    loadSC();
}

void DebugGame::loadSC() {
    

    Assets.SetBase("./assets");
    // Load all TREs and PAKs
    Loader& loader = Loader::getInstance();
    loader.init();
    loader.startLoading([](Loader* loader) {
        AssetManager &Assets = AssetManager::getInstance();
        ConvAssetManager &ConvAssets = ConvAssetManager::getInstance();
        RSFontManager &FontManager = RSFontManager::getInstance();
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
        
        FontManager.init();

        // Load assets needed for Conversations (char and background)
        ConvAssets.init();
        RSSound::getInstance().init();
    });
    
    //Add MainMenu activity on the game stack.
    DebugGameFlow* main = new DebugGameFlow();
    //SCMainMenu* main = new SCMainMenu();
    GameState.Reset();
    GameState.player_callsign = "Debug";
    GameState.player_name = "Debug Player";
    GameState.player_firstname = "Debug";
    for (int i=0; i<256; i++) {
        GameState.requierd_flags[i] = false;
    }
    main->init();
    this->addActivity(main);
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
        
        FontManager.init();
        loader->setProgress(60.0f);
        // Load assets needed for Conversations (char and background)
        ConvAssets.init();
        loader->setProgress(85.0f);
        RSSound::getInstance().init();
        loader->setProgress(100.0f);
    });
    //Add MainMenu activity on the game stack.
    DebugGameFlow* main = new DebugGameFlow();
    
    GameState.Reset();
    GameState.player_callsign = "Debug";
    GameState.player_name = "Debug Player";
    GameState.player_firstname = "Debug";
    for (int i=0; i<256; i++) {
        GameState.requierd_flags[i] = false;
    }
    main->init();
    this->addActivity(main);
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
        std::vector<std::string> treFiles = {
            "BIGTRE.TRE",
            "LILTRE.TRE",
            "TOBIGTRE.TRE",
            "PACIFIC.DAT"
        };
        Assets.init(treFiles);
        
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
        
        FontManager.init();
        RSSound::getInstance().init();
    });
    //Add MainMenu activity on the game stack.
    DebugStrike * main = new DebugStrike();
    main->init();
    main->setMission("MISN-1A.IFF");
    this->addActivity(main);
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
        
        FontManager.init();
        // Load assets needed for Conversations (char and background)
        ConvAssets.init();
        RSSound::getInstance().init();
    });
    //Add MainMenu activity on the game stack.
    DebugGameFlow* main = new DebugGameFlow();
    
    GameState.Reset();
    GameState.player_callsign = "Debug";
    GameState.player_name = "Debug Player";
    GameState.player_firstname = "Debug";
    for (int i=0; i<256; i++) {
        GameState.requierd_flags[i] = false;
    }
    main->init();
    this->addActivity(main);
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
        
        FontManager.init();
    });
    //Add MainMenu activity on the game stack.
    DebugObjectViewer* main = new DebugObjectViewer();
    main->init();
    this->addActivity(main);
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