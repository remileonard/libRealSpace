//
//  Screen->cpp
//  libRealSpace
//
//  Created by Fabien Sanglard on 1/27/2014.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//

#include "RSScreen.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl2.h"
#include <functional>



void RSScreen::setTitle(const char* title){
    SDL_SetWindowTitle(m_window, title);
}

void RSScreen::init(int width, int height, bool fullscreen){
    
    this->width = width;
    this->height = height;
    
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2"); // ou "permonitor" si votre SDL est plus ancienne
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");

    // 1. Initialiser SDL AVANT toute création de fenêtre
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO) != 0) {
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

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (fullscreen) {
        window_flags = (SDL_WindowFlags)(SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL);
    }

    // 3. UNE seule création de fenêtre
    m_window = SDL_CreateWindow("Neo Strike Commander",
                                 SDL_WINDOWPOS_UNDEFINED,
                                 SDL_WINDOWPOS_UNDEFINED,
                                 this->width,
                                 this->height,
                                 window_flags);
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
    SDL_GL_SetSwapInterval(1); // vsync

    openScreen();
    SDL_ShowWindow(m_window);
}

void RSScreen::openScreen(void){
    // Utiliser la taille du framebuffer OpenGL (pixels réels)
    int drawableW = 0, drawableH = 0;
    SDL_GL_GetDrawableSize(m_window, &drawableW, &drawableH);
    if (drawableW <= 0 || drawableH <= 0) {
        drawableW = this->width;
        drawableH = this->height;
    }

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Letterboxing 4:3 dans le framebuffer réel
    int vpX = 0, vpY = 0, vpW = drawableW, vpH = drawableH;
    int targetW = (int)((float)drawableH * (4.0f/3.0f));
    if (targetW <= drawableW) {
        vpW = targetW;
        vpX = (drawableW - vpW) / 2;
    } else {
        int targetH = (int)((float)drawableW * (3.0f/4.0f));
        vpH = targetH;
        vpY = (drawableH - vpH) / 2;
    }

    glViewport(vpX, vpY, vpW, vpH);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void RSScreen::refresh(void){
    SDL_GL_SwapWindow(m_window);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

}

void RSScreen::fxTurnOnTv() {
    // TV turn-on animation variables
    static bool initialized = false;
    static float animationTime = 0.0f;
    static const float animationDuration = 3.0f;  // 1.5 seconds for the whole animation
    static GLuint scanlineTexture = 0;

    // Initialize the scanline texture the first time
    if (!initialized) {
        // Create scanline texture
        glGenTextures(1, &scanlineTexture);
        glBindTexture(GL_TEXTURE_2D, scanlineTexture);
        
        // Create a 1D pattern for scanlines
        const int texHeight = 256;
        unsigned char* scanlineData = new unsigned char[texHeight * 4];
        for (int i = 0; i < texHeight; i++) {
            float intensity = 0.85f + 0.15f * sin(i * 0.5f);
            scanlineData[i * 4 + 0] = static_cast<unsigned char>(255 * intensity);
            scanlineData[i * 4 + 1] = static_cast<unsigned char>(255 * intensity);
            scanlineData[i * 4 + 2] = static_cast<unsigned char>(255 * intensity);
            scanlineData[i * 4 + 3] = 255;
        }
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, scanlineData);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        
        delete[] scanlineData;
        initialized = true;
    }

    // Update animation time
    const float deltaTime = 1.0f / 60.0f;  // Assuming 60fps
    animationTime = (std::min)(animationTime + deltaTime, animationDuration);
    float animProgress = animationTime / animationDuration;
    if (animProgress >= 1.0f) {
        animProgress = 1.0f;  // Clamp to 1.0
        is_spfx_finished = true;  // Mark that the screen is now on
        return;
    }
    // Clear the screen to black
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    // Draw TV turning on effect
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // TV turn-on animation with central point expanding
    if (animProgress < 0.1f) {
        // First phase: Horizontal line expands from center
        float horizontalWidth = animProgress / 0.1f * 2.0f; // 0 to 2.0
        float lineHeight = 0.01f;
        
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);
            glVertex2f(-horizontalWidth/2, -lineHeight/2);
            glVertex2f(horizontalWidth/2, -lineHeight/2);
            glVertex2f(horizontalWidth/2, lineHeight/2);
            glVertex2f(-horizontalWidth/2, lineHeight/2);
        glEnd();
    } else if (animProgress < 0.2f) {
        // Second phase: Vertical expansion after horizontal is complete
        float normalizedProgress = (animProgress - 0.1f) / 0.1f; // 0 to 1.0
        float verticalHeight = normalizedProgress * 2.0f;  // 0 to 2.0
        
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);
            glVertex2f(-1.0f, -verticalHeight/2);
            glVertex2f(1.0f, -verticalHeight/2);
            glVertex2f(1.0f, verticalHeight/2);
            glVertex2f(-1.0f, verticalHeight/2);
        glEnd();
    } else if (animProgress < 0.3f && animProgress >= 0.5f) {
        // Third phase: Full white screen fading out as noise fades in
        float fadeOutWhite = 1.0f - ((animProgress - 0.5f) / 0.2f);
        
        glColor4f(1.0f, 1.0f, 1.0f, fadeOutWhite);
        glBegin(GL_QUADS);
            glVertex2f(-1.0f, -1.0f);
            glVertex2f(1.0f, -1.0f);
            glVertex2f(1.0f, 1.0f);
            glVertex2f(-1.0f, 1.0f);
        glEnd();
    }
    

    // White noise effect fading out
    if (animProgress >= 0.3f && animProgress < 0.7f) {
        float noiseIntensity = 1.0f - (animProgress / 0.8f);
        
        glColor4f(1.0f, 1.0f, 1.0f, noiseIntensity);
        glBegin(GL_QUADS);
        for (int i = 0; i < 10000; i++) {
            float x = static_cast<float>(rand()) / (float) RAND_MAX * 2.0f - 1.0f;
            float y = static_cast<float>(rand()) / (float) RAND_MAX * 2.0f - 1.0f;
            float size = 0.003f + 0.002f * static_cast<float>(rand()) / (float) RAND_MAX;
            
            glVertex2f(x - size, y - size);
            glVertex2f(x + size, y - size);
            glVertex2f(x + size, y + size);
            glVertex2f(x - size, y + size);
        }
        glEnd();
    }

    // Horizontal line sweeping from center
    if (animProgress > 0.3f && animProgress < 0.8f) {
        float lineProgress = (animProgress - 0.2f) / 0.6f;
        float lineHeight = 0.05f;
        float lineY = (lineProgress * 2.0f - 1.0f);
        
        glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
        glBegin(GL_QUADS);
        glVertex2f(-1.0f, lineY - lineHeight);
        glVertex2f(1.0f, lineY - lineHeight);
        glVertex2f(1.0f, lineY + lineHeight);
        glVertex2f(-1.0f, lineY + lineHeight);
        glEnd();
    }

    // Add scanlines and CRT effect
    if (animProgress > 0.5f) {
        float scanlineAlpha = (animProgress - 0.5f) / 0.5f * 0.3f;
        
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, scanlineTexture);
        glColor4f(1.0f, 1.0f, 1.0f, scanlineAlpha);
        
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, 1.0f);
        glTexCoord2f(800.0f, 0.0f); glVertex2f(1.0f, 1.0f);
        glTexCoord2f(800.0f, 600.0f); glVertex2f(1.0f, -1.0f);
        glTexCoord2f(0.0f, 600.0f); glVertex2f(-1.0f, -1.0f);
        glEnd();
        
        glDisable(GL_TEXTURE_2D);
    }

    glDisable(GL_BLEND);

}
