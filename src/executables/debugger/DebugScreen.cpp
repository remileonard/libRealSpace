//
//  Screen->cpp
//  libRealSpace
//
//  Created by Fabien Sanglard on 1/27/2014.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//

#include "DebugScreen.h"
#include "DebugGame.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl2.h"
#include "../../engine/EventManager.h"
static SDL_Window *sdlWindow;
static SDL_Renderer *sdlRenderer;

void DebugScreen::setVideoProperties() {
    RSVGA& vga = RSVGA::instance();
    static int r = 0;
    static int g = 0;
    static int b = 0;
    ImGui::Begin("Video Properties", nullptr);
    ImGui::SliderFloat("Contrast", &vga.contrastFactor, 0.5f, 2.0f);
    ImGui::SliderFloat("Brightness", &vga.brightnessFactor, 0.5f, 2.0f);
    
    ImGui::SliderInt("Tint R", &r, 0, 255);
    ImGui::SliderInt("Tint G", &g, 0, 255);
    ImGui::SliderInt("Tint B", &b, 0, 255);
    vga.tintR = static_cast<uint8_t>(r);
    vga.tintG = static_cast<uint8_t>(g);
    vga.tintB = static_cast<uint8_t>(b);
    ImGui::SliderFloat("Tint Intensity", &vga.tintIntensity, 0.0f, 1.0f);
    
    ImGui::End();
}

DebugScreen::DebugScreen() {}

DebugScreen::~DebugScreen(){
    
}

void DebugScreen::setTitle(const char* title){
    SDL_SetWindowTitle(sdlWindow, title);
}

void DebugScreen::init(int w, int h, bool fullscreen){
    width = w;
    height = h;

    // 1. Initialiser SDL AVANT toute création de fenêtre
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS | SDL_INIT_AUDIO) != 0) {
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
    sdlWindow = SDL_CreateWindow("Neo Strike Commander DebugTool",
                                 SDL_WINDOWPOS_UNDEFINED,
                                 SDL_WINDOWPOS_UNDEFINED,
                                 this->width,
                                 this->height,
                                 window_flags);
    if (!sdlWindow) {
        printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
        return;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(sdlWindow);
    if (!gl_context) {
        printf("SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        return;
    }
    SDL_GL_MakeCurrent(sdlWindow, gl_context);
    SDL_GL_SetSwapInterval(1); // vsync

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsLight();

    ImGui_ImplSDL2_InitForOpenGL(sdlWindow, gl_context);
    ImGui_ImplOpenGL2_Init();

    glGenTextures(1, &this->screen_texture);
    glBindTexture(GL_TEXTURE_2D, this->screen_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, this->width, this->height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    RSScreen::openScreen();

    SDL_ShowWindow(sdlWindow);
}

void DebugScreen::refresh(void){
    DebugGame *debugGameInstance = static_cast<DebugGame*>(&GameEngine::instance());
    RSVGA *VGA = &RSVGA::instance();

    static bool s_gameChildFocusedPrev = false; // mémorise le focus du Child "Game" à la frame précédente
    {
        ImGuiIO& io = ImGui::GetIO();
        if (s_gameChildFocusedPrev) {
            // Couper la navigation gamepad ImGui cette frame
            io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;
        } else {
            // Autoriser la navigation gamepad ImGui
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        }
    }

    glBindTexture(GL_TEXTURE_2D, this->screen_texture);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, this->width, this->height, 0);
    glViewport(0,0,this->width,this->height);			// Reset The Current Viewport
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);				// Black Background
    glClear(GL_COLOR_BUFFER_BIT| GL_DEPTH_BUFFER_BIT);
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    bool p_open = true;
    static bool showVideoProps = false;
    if (ImGui::Begin("##Fullscreen window", &p_open, flags)) {
        if (ImGui::BeginMenuBar()) {
            if (debugGameInstance != nullptr) {
                if (ImGui::BeginMenu("File")) {
                    if (ImGui::BeginMenu("Strike Commander")) {
                        if (ImGui::MenuItem("Strike Commander Floppy")) {
                            this->show_ui = true;
                            this->show_mouse = false;
                            debugGameInstance->loadSC();
                        }
                        if (ImGui::MenuItem("Strike Commander CD")) {
                            this->show_ui = true;
                            this->show_mouse = false;
                            debugGameInstance->loadSCCD();
                        }
                        if (ImGui::MenuItem("Tactical Operations")) {
                            this->show_ui = true;
                            this->show_mouse = false;
                            debugGameInstance->loadTO();   
                        }
                        if (ImGui::MenuItem("Test mission SC")) {
                            this->show_ui = true;
                            this->show_mouse = false;
                            debugGameInstance->testMissionSC();
                        }
                        if (ImGui::MenuItem("Test Objects")) {
                            this->show_ui = true;
                            this->show_mouse = true;
                            debugGameInstance->testObjects();
                        }
                        if (ImGui::MenuItem("MidGames Editor")) {
                            this->show_ui = true;
                            this->show_mouse = true;
                            debugGameInstance->testMidGames();
                        }
                        ImGui::EndMenu();
                    }
                    if (ImGui::BeginMenu("Pacific Strike")) {
                        if (ImGui::MenuItem("Pacific Strike Objects")) {
                            this->show_ui = true;
                            debugGameInstance->loadPacific();   
                        }
                        if (ImGui::MenuItem("Pacific Strike GamePlay")) {
                            this->show_ui = true;
                            debugGameInstance->loadPacificConv();   
                        }
                        if (ImGui::MenuItem("Pacific Strike Mission")) {
                            this->show_ui = true;
                            debugGameInstance->loadPacificMISN();   
                        }
                        ImGui::EndMenu();
                    }
                    if (ImGui::BeginMenu("Wings of Glory")) {
                        if (ImGui::MenuItem("Wings of Glory Objects")) {
                            this->show_ui = true;
                            debugGameInstance->loadWingsOfGlory();   
                        }
                        ImGui::EndMenu();
                    }
                    if (ImGui::MenuItem("Test Inputs")) {
                        this->show_ui = true;
                        this->show_mouse = true;
                        debugGameInstance->testController();
                    }
                    
                    if (VGA != nullptr) {
                        if (ImGui::MenuItem("Toggle upscale", nullptr, VGA->upscale)) {
                            VGA->upscale = !VGA->upscale;
                        }
                    }
                    if (ImGui::MenuItem("Toggle UI", nullptr, this->show_ui)) {
                        this->show_ui = !this->show_ui;
                    }
                    if (ImGui::MenuItem("Toggle Mouse", nullptr, this->show_mouse)) {
                        this->show_mouse = !this->show_mouse;
                    }
                    
                    if (ImGui::MenuItem("Video Properties", nullptr, showVideoProps)) {
                        showVideoProps = !showVideoProps;
                    }
                    
                    if (ImGui::MenuItem("Exit")) {
                        exit(0);
                    }
                    ImGui::EndMenu();
                }
                if (debugGameInstance->hasActivity()) {
                    IActivity* act = debugGameInstance->getCurrentActivity();
                    if (act != nullptr) {
                        act->renderMenu();    
                    }
                }
            }
            
            ImGui::EndMenuBar();
        }
        ImVec2 winsize = ImGui::GetContentRegionAvail();
        float width = winsize.x * 0.70f;
        if (debugGameInstance == nullptr  || !debugGameInstance->hasActivity() || !this->show_ui) {
            width = winsize.x; // If no game instance, use full width
        }
        ImGui::BeginChild("Game", ImVec2(width, 0), ImGuiChildFlags_Border | ImGuiWindowFlags_NoSavedSettings);
        if (!show_mouse) {
            const bool gameChildFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_None);
            const bool gameChildHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_None);
            if (gameChildHovered) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_None);
                EventManager::getInstance().enableImGuiKeyboardCapture(false);
            } else {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
                EventManager::getInstance().enableImGuiKeyboardCapture(true);
            }
            s_gameChildFocusedPrev = gameChildFocused;
        } else {
            if (force_keyboard_capture) {
                EventManager::getInstance().enableImGuiKeyboardCapture(true);
            }
        }
        ImVec2 avail_size = ImGui::GetContentRegionAvail();
        ImGui::Image((void*)(intptr_t)this->screen_texture, avail_size, {0, 1}, {1, 0});
        ImGui::EndChild();
        if (debugGameInstance != nullptr && debugGameInstance->hasActivity() && this->show_ui) {
            ImGui::SameLine();
            // Calculate the remaining width for the Console child
            float remainingWidth = ImGui::GetContentRegionAvail().x;
            ImGui::BeginChild("Console", ImVec2(remainingWidth, 0), ImGuiChildFlags_Border | ImGuiWindowFlags_NoSavedSettings);
            IActivity* act = debugGameInstance->getCurrentActivity();
            if (act != nullptr) {
                act->renderUI();    
            }
            ImGui::EndChild();
        }
        if (this->show_ui && showVideoProps) {
            setVideoProperties();
        }
        ImGui::End();
    }
    ImGui::Render();
    ImDrawData *dt=ImGui::GetDrawData();
    if (dt != nullptr) {
        ImGui_ImplOpenGL2_RenderDrawData(dt);
    }
    SDL_GL_SwapWindow(sdlWindow);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);				// Black Background
    glClear(GL_COLOR_BUFFER_BIT| GL_DEPTH_BUFFER_BIT);
}