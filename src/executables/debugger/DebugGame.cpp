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

#include "../../strike_commander/precomp.h"
extern RSScreen *Screen;
extern SCMouse Mouse;
extern AssetManager Assets;
extern SCRenderer Renderer;
extern RSVGA VGA;

DebugGame::DebugGame() {
    printf("DebugGame constructor\n");
}

DebugGame::~DebugGame() {}

void DebugGame::init() {

    // Load Main Palette and Initialize the GL
    Screen->init(1200,800,0);
    VGA.init(1200,800, &Assets);
    Renderer.init(1200,800, &Assets);

    // Load the Mouse Cursor
    Mouse.init();
    testMissionSC();
}

void DebugGame::PumpEvents(void) {

    SDL_PumpEvents();

    // Mouse
    SDL_Event mouseEvents[5];
    int numMouseEvents = SDL_PeepEvents(mouseEvents, 5, SDL_PEEKEVENT, SDL_MOUSEMOTION, SDL_MOUSEWHEEL);
    ImGui_ImplSDL2_ProcessEvent(mouseEvents);
    for (int i = 0; i < numMouseEvents; i++) {
        SDL_Event *event = &mouseEvents[i];

        switch (event->type) {
        case SDL_MOUSEMOTION:

            Point2D newPosition;
            newPosition.x = event->motion.x;
            newPosition.y = event->motion.y;

            newPosition.x = static_cast<int>(newPosition.x * 320 / Screen->width);
            newPosition.y = static_cast<int>(newPosition.y * 200 / Screen->height);

            Mouse.SetPosition(newPosition);

            break;

        case SDL_MOUSEBUTTONDOWN:
            // printf("SDL_MOUSEBUTTONDOWN %d\n",event->button.button);

            Mouse.buttons[event->button.button - 1].event = MouseButton::PRESSED;
            break;
        case SDL_MOUSEBUTTONUP:
            // printf("SDL_MOUSEBUTTONUP %d\n",event->button.button);
            Mouse.buttons[event->button.button - 1].event = MouseButton::RELEASED;
            break;
        default:
            break;
        }
    }


    // System events
    SDL_Event sysEvents[5];
    int numSysEvents = SDL_PeepEvents(sysEvents, 5, SDL_PEEKEVENT, SDL_FIRSTEVENT, SDL_SYSWMEVENT);
    for (int i = 0; i < numSysEvents; i++) {
        SDL_Event *event = &sysEvents[i];

        switch (event->type) {
        case SDL_APP_TERMINATING:
            Terminate("System request.");
            break;
        case SDL_QUIT:
            Terminate("System request.");
            break;

        // Verify is we should be capturing the mouse or not.
        case SDL_WINDOWEVENT:
            if (event->window.event == SDL_WINDOWEVENT_ENTER) {
                Mouse.SetVisible(true);
                SDL_ShowCursor(SDL_DISABLE);
                return;
            }

            if (event->window.event == SDL_WINDOWEVENT_LEAVE) {
                Mouse.SetVisible(false);
                SDL_ShowCursor(SDL_ENABLE);
                return;
            }

            if (event->window.event == SDL_WINDOWEVENT_CLOSE) {
                Terminate("System request.");
                break;
            }
            
            break;
        default:
            break;
        }
    }
}

void DebugGame::loadSC() {
    
    Assets.SetBase("./assets");
    // Load all TREs and PAKs
    
    std::vector<std::string> cdTreFiles = {
        "BIGTRE.TRE",
        "LILTRE.TRE",
        "VOCLIST.TRE"
    };
    Assets.ReadISOImage("./SC.DAT");
    Assets.init(cdTreFiles);
    
    Assets.intel_root_path = "..\\..\\DATA\\INTEL\\";
    Assets.mission_root_path = "..\\..\\DATA\\MISSIONS\\";
    Assets.object_root_path = "..\\..\\DATA\\OBJECTS\\";
    Assets.sound_root_path = "..\\..\\DATA\\SOUNDS\\";
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
    
    FontManager.init(&Assets);

    // Load assets needed for Conversations (char and background)
    ConvAssets.init();

    //Add MainMenu activity on the game stack.
    SCMainMenu* main = new SCMainMenu();
    main->init();
    Game->AddActivity(main);
    SCAnimationPlayer *intro = new SCAnimationPlayer();
    intro->init();
    Game->AddActivity(intro);
    Game->StopTopActivity();
}


void DebugGame::testMissionSC() {
    
    Assets.SetBase("./assets");
    // Load all TREs and PAKs
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
    Assets.sound_root_path = "..\\..\\DATA\\SOUNDS\\";
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
    
    FontManager.init(&Assets);

    // Load assets needed for Conversations (char and background)
    ConvAssets.init();

    //Add MainMenu activity on the game stack.
    DebugStrike * main = new DebugStrike();
    main->init();
    main->setMission("MISN-1A.IFF");
    Game->AddActivity(main);
    
}

void DebugGame::loadTO() {
    Game->StopTopActivity();
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
    Assets.sound_root_path = "..\\..\\DATA\\SOUNDS\\";
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
    
    FontManager.init(&Assets);
    // Load assets needed for Conversations (char and background)
    ConvAssets.init();

    SCMainMenu *to_gf = new SCMainMenu();
    to_gf->init();
    Game->AddActivity(to_gf);
}

void DebugGame::loadPacific() {
    Game->StopTopActivity();
    Assets.SetBase("./assets");
    // Load all TREs and PAKs
    
    std::vector<std::string> cdTreFiles = {
        "PACIFIC.DAT",
    };
    Assets.init(cdTreFiles);
    
    FontManager.init(&Assets);

    // Load assets needed for Conversations (char and background)
    // ConvAssets.init();

    //Add MainMenu activity on the game stack.
    SCObjectViewer* main = new SCObjectViewer();
    main->init();
    Game->AddActivity(main);
}

void DebugGame::Run() {

    IActivity *currentActivity;

    while (activities.size() > 0) {

        PumpEvents();

        // Clear the screen
        // enderer.Clear();

        // Allow the active activity to Run and Render
        currentActivity = activities.top();

        if (currentActivity->IsRunning()) {
            currentActivity->Focus();
            currentActivity->runFrame();
        } else {
            activities.pop();
            delete currentActivity;
        }

        // Swap GL buffer
        Screen->Refresh();

        // Flush all events since they should all have been interpreted.
        SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);

        // Also clear the Mouse flags.
        Mouse.FlushEvents();
    }
}

void DebugGame::Terminate(const char *reason, ...) {
    Log("Terminating: ");
    va_list args;
    va_start(args, reason);
    vfprintf(stdout, reason, args);
    va_end(args);
    Log("\n");
    exit(0);
}

void DebugGame::Log(const char *text, ...) {
    va_list args;
    va_start(args, text);
    vfprintf(stdout, text, args);
    va_end(args);
}

void DebugGame::LogError(const char *text, ...) {
    va_list args;
    va_start(args, text);
    vfprintf(stderr, text, args);
    va_end(args);
}

void DebugGame::AddActivity(IActivity *activity) {
    if (activities.size()>0) {
        IActivity *currentActivity;
        currentActivity = activities.top();
        currentActivity->UnFocus();
    }
    activity->Start();
    this->activities.push(activity);
}

void DebugGame::StopTopActivity(void) {
    IActivity *currentActivity;
    currentActivity = activities.top();
    currentActivity->Stop();
}

IActivity *DebugGame::GetCurrentActivity(void) { 
    if (activities.empty()) {
        return nullptr; // No activity is running
    }
    return activities.top();
}
