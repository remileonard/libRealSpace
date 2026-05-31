//
//  Screen->cpp
//  libRealSpace
//
//  Created by Fabien Sanglard on 1/27/2014.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//

#include "RSScreen.h"
#include "Config.hpp"
#include "GLExtensions.h"
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

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (fullscreen) {
        window_flags = (SDL_WindowFlags)(SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);
    }

    // 3. UNE seule création de fenêtre
    m_window = SDL_CreateWindow("Neo Strike Commander",
                                 SDL_WINDOWPOS_UNDEFINED,
                                 SDL_WINDOWPOS_UNDEFINED,
                                 this->width,
                                 this->height,
                                 window_flags);

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
    if (!initGLExtensions()) {
        printf("[RSScreen] Failed to load OpenGL extensions\n");
        return;
    }
    SDL_GL_SetSwapInterval(1); // vsync

    openScreen();
    SDL_ShowWindow(m_window);
    initPostProcess();
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

void RSScreen::initPostProcess() {
    Config &config = Config::getInstance();
    this->fx_cpc_palette = config.getBool("video", "fx_cpc_palette", false);
    this->fx_scanlines   = config.getBool("video", "fx_scanlines", false);
    this->fx_pixel_scale = config.getInt("video", "fx_pixel_scale", 1);
    const char* vertSrc = R"(
        void main() {
            gl_TexCoord[0] = gl_MultiTexCoord0;
            gl_Position = ftransform();
        }
    )";

    // Palette CPC 6128 : 3 niveaux par canal (0.0, 0.5, 1.0)
    // soit 3^3 = 27 couleurs possibles
    const char* fragSrc = R"(
        uniform sampler2D screenTexture;
        uniform float screenWidth;
        uniform float screenHeight;
        uniform int useScanlines;
        uniform int useCPC;
        uniform float pixelScale;

        float bayerThreshold(vec2 fragCoord) {
            mat4 m = mat4(
                0.0, 12.0,  3.0, 15.0,
                8.0,  4.0, 11.0,  7.0,
                2.0, 14.0,  1.0, 13.0,
                10.0,  6.0,  9.0,  5.0
            );
            // Snap au grain du pixel logique avant le modulo 4
            int ix = int(mod(floor(gl_FragCoord.x / pixelScale), 4.0));
            int iy = int(mod(floor(gl_FragCoord.y / pixelScale), 4.0));
            return m[ix][iy] / 16.0;
        }
        void main() {
            vec2 uv = gl_TexCoord[0].xy;
            if (pixelScale > 1.0) {
                vec2 ps = vec2(pixelScale / screenWidth, pixelScale / screenHeight);
                uv = (floor(uv / ps) + 0.5) * ps;
            }
            vec4 color = texture2D(screenTexture, uv);

            if (useCPC == 1) {
                float threshold = bayerThreshold(gl_FragCoord.xy);
                color.rgb = floor(color.rgb * 2.0 + threshold) / 2.0;
            }

            if (useScanlines == 1) {
                float line = mod(floor(uv.y * screenHeight / pixelScale), 2.0);
                color.rgb *= 1.0 - line * 0.4;
            }

            gl_FragColor = color;
        }
    )";

    // Compiler le vertex shader
    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &vertSrc, nullptr);
    glCompileShader(vert);
    GLint ok = 0;
    glGetShaderiv(vert, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512]; glGetShaderInfoLog(vert, 512, nullptr, log);
        printf("[RSScreen] vertex shader error: %s\n", log);
        glDeleteShader(vert); return;
    }

    // Compiler le fragment shader
    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &fragSrc, nullptr);
    glCompileShader(frag);
    glGetShaderiv(frag, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512]; glGetShaderInfoLog(frag, 512, nullptr, log);
        printf("[RSScreen] fragment shader error: %s\n", log);
        glDeleteShader(vert); glDeleteShader(frag); return;
    }

    // Linker
    m_shaderProgram = glCreateProgram();
    glAttachShader(m_shaderProgram, vert);
    glAttachShader(m_shaderProgram, frag);
    glLinkProgram(m_shaderProgram);
    glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512]; glGetProgramInfoLog(m_shaderProgram, 512, nullptr, log);
        printf("[RSScreen] shader link error: %s\n", log);
        glDeleteProgram(m_shaderProgram); m_shaderProgram = 0;
        glDeleteShader(vert); glDeleteShader(frag); return;
    }
    glDeleteShader(vert);
    glDeleteShader(frag);
    m_postProcessReady = true;
    printf("[RSScreen] post-process shader ready\n");
}

void RSScreen::refresh(void) {
    if (m_postProcessReady && (fx_cpc_palette || fx_scanlines)) {
        // 1. Allouer la texture de capture si nécessaire
        if (m_postProcessTexture == 0) {
            glGenTextures(1, &m_postProcessTexture);
            glBindTexture(GL_TEXTURE_2D, m_postProcessTexture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height,
                         0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        }

        // 2. Capturer le backbuffer courant dans la texture
        glBindTexture(GL_TEXTURE_2D, m_postProcessTexture);
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, width, height);

        // 3. Effacer et passer en rendu 2D fullscreen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        GLboolean depthWasEnabled = glIsEnabled(GL_DEPTH_TEST);
        glDisable(GL_DEPTH_TEST);

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        // 4. Appliquer le shader
        glUseProgram(m_shaderProgram);
        glUniform1i(glGetUniformLocation(m_shaderProgram, "screenTexture"), 0);
        glUniform1f(glGetUniformLocation(m_shaderProgram, "screenWidth"),  (float)width);
        glUniform1f(glGetUniformLocation(m_shaderProgram, "screenHeight"), (float)height);
        glUniform1f(glGetUniformLocation(m_shaderProgram, "pixelScale"),   (float)fx_pixel_scale);
        glUniform1i(glGetUniformLocation(m_shaderProgram, "useCPC"),       fx_cpc_palette ? 1 : 0);
        glUniform1i(glGetUniformLocation(m_shaderProgram, "useScanlines"), fx_scanlines   ? 1 : 0);
       
        GLint vp[4];
        glGetIntegerv(GL_VIEWPORT, vp);
        // vp[0]=x, vp[1]=y, vp[2]=w, vp[3]=h

        float u0 = (float)vp[0] / width;
        float u1 = (float)(vp[0] + vp[2]) / width;
        float v0 = (float)vp[1] / height;
        float v1 = (float)(vp[1] + vp[3]) / height;

        GLboolean cullWasEnabled = glIsEnabled(GL_CULL_FACE);
        glDisable(GL_CULL_FACE);

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, m_postProcessTexture);

        glBegin(GL_QUADS);
            glTexCoord2f(u0, v0); glVertex2f(-1.0f, -1.0f);
            glTexCoord2f(u1, v0); glVertex2f( 1.0f, -1.0f);
            glTexCoord2f(u1, v1); glVertex2f( 1.0f,  1.0f);
            glTexCoord2f(u0, v1); glVertex2f(-1.0f,  1.0f);
        glEnd();

        glDisable(GL_TEXTURE_2D);
        glUseProgram(0);

        if (cullWasEnabled) glEnable(GL_CULL_FACE);
        // 5. Restaurer l'état
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
        if (depthWasEnabled) glEnable(GL_DEPTH_TEST);
    }

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
