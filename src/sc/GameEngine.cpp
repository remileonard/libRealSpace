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
#include "precomp.h"

GameEngine::GameEngine() {}

GameEngine::~GameEngine() {}

void GameEngine::Init() {

    // Load all TREs and PAKs
    std::vector<std::string> treFiles = {
        "GAMEFLOW.TRE",
        "OBJECTS.TRE",
        "MISC.TRE",
        "SOUND.TRE",
        "MISSIONS.TRE",
        "TEXTURES.TRE"
    };
    Assets.Init(treFiles);
    Mixer.Init(&Assets);
    FontManager.Init(&Assets);

    // Load assets needed for Conversations (char and background)
    ConvAssets.Init();

    // Load Main Palette and Initialize the GL
    Screen.Init(WIDTH,HEIGHT,FULLSCREEN);

    VGA.Init(WIDTH,HEIGHT, &Assets);

    Renderer.Init(WIDTH,HEIGHT, &Assets);

    // Load the Mouse Cursor
    Mouse.Init();
}

void GameEngine::PumpEvents(void) {

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

            newPosition.x = static_cast<int>(newPosition.x * 320 / Screen.width);
            newPosition.y = static_cast<int>(newPosition.y * 200 / Screen.height);

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

void GameEngine::Run() {

    IActivity *currentActivity;

    while (activities.size() > 0) {

        PumpEvents();

        // Clear the screen
        // enderer.Clear();

        // Allow the active activity to Run and Render
        currentActivity = activities.top();

        if (currentActivity->IsRunning()) {
            currentActivity->Focus();
            currentActivity->RunFrame();
        } else {
            activities.pop();
            delete currentActivity;
        }

        // Swap GL buffer
        Screen.Refresh();

        // Flush all events since they should all have been interpreted.
        SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);

        // Also clear the Mouse flags.
        Mouse.FlushEvents();
    }
}

void GameEngine::Terminate(const char *reason, ...) {
    Log("Terminating: ");
    va_list args;
    va_start(args, reason);
    vfprintf(stdout, reason, args);
    va_end(args);
    Log("\n");
    exit(0);
}

void GameEngine::Log(const char *text, ...) {
    va_list args;
    va_start(args, text);
    vfprintf(stdout, text, args);
    va_end(args);
}

void GameEngine::LogError(const char *text, ...) {
    va_list args;
    va_start(args, text);
    vfprintf(stderr, text, args);
    va_end(args);
}

void GameEngine::AddActivity(IActivity *activity) {
    if (activities.size()>0) {
        IActivity *currentActivity;
        currentActivity = activities.top();
        currentActivity->UnFocus();
    }
    activity->Start();
    this->activities.push(activity);
}

void GameEngine::StopTopActivity(void) {
    IActivity *currentActivity;
    currentActivity = activities.top();
    currentActivity->Stop();
}

IActivity *GameEngine::GetCurrentActivity(void) { return activities.top(); }
