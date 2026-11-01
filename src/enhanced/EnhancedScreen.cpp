//
//  Screen->cpp
//  libRealSpace
//
//  Created by Fabien Sanglard on 1/27/2014.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//

#include "EnhancedScreen.h"
#include "../engine/Config.hpp"
#include "../engine/GLExtensions.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl2.h"
#include <functional>

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif

void EnhancedScreen::init(int width, int height, bool fullscreen){
    
    this->width = width;
    this->height = height;
    
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2"); // ou "permonitor" si votre SDL est plus ancienne
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");

    
    // 1. Initialiser SDL AVANT toute création de fenêtre
    if (SDL_Init(SDL_INIT_VIDEO |SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS | SDL_INIT_AUDIO) != 0) {
        printf("Unable to initialize SDL: %s\n", SDL_GetError());
        return;
    }
    
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    // 2. Attributs OpenGL adaptés macOS 10.14 (2.1 max)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);

    // [Video] msaa: 0 = off (default), 2/4/8 = multisample anti-aliasing of the window framebuffer
    int msaa = Config::getInstance().getInt("Video", "msaa", 0);
    if (msaa > 1) {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, msaa);
    }

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (fullscreen) {
        window_flags = (SDL_WindowFlags)(SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);
    }

    // 3. UNE seule création de fenêtre
    m_window = SDL_CreateWindow("Enhanced Strike Commander",
                                 SDL_WINDOWPOS_UNDEFINED,
                                 SDL_WINDOWPOS_UNDEFINED,
                                 this->width,
                                 this->height,
                                 window_flags);
    if (m_window == nullptr && msaa > 1) {
        printf("[EnhancedScreen] msaa %d not available (%s), continuing without\n", msaa, SDL_GetError());
        msaa = 0;
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
        m_window = SDL_CreateWindow("Neo Strike Commander",
                                     SDL_WINDOWPOS_UNDEFINED,
                                     SDL_WINDOWPOS_UNDEFINED,
                                     this->width,
                                     this->height,
                                     window_flags);
    }

    int drawableW = 0, drawableH = 0;
    SDL_GL_GetDrawableSize(m_window, &drawableW, &drawableH);
    if (drawableW <= 0 || drawableH <= 0) {
        drawableW = this->width;
        drawableH = this->height;
    } else {
        this->width = drawableW;
        this->height = drawableH;
    }
    SDL_GetWindowSize(m_window, &this->logical_width, &this->logical_height);


    if (!m_window) {
        printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
        return;
    }

    m_glContext = SDL_GL_CreateContext(m_window);
    if (!m_glContext) {
        printf("SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        return;
    }
    SDL_GL_MakeCurrent(m_window, m_glContext);
    if (msaa > 1) {
        int samples = 0;
        SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &samples);
        printf("[EnhancedScreen] msaa requested %d, framebuffer has %d samples\n", msaa, samples);
        if (samples > 1) {
            glEnable(GL_MULTISAMPLE);
        }
    }
    if (!initGLExtensions()) {
        printf("[EnhancedScreen] Failed to load OpenGL extensions\n");
        return;
    }
    SDL_GL_SetSwapInterval(1); // vsync

    openScreen();
    SDL_ShowWindow(m_window);
    initPostProcess();
}
