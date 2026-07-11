#include "../../engine/Config.hpp"
#include "../debugger/DebugControlMapping.h"
#include "../../engine/EventManager.h"  
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl2.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include <string>

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_Window* window = SDL_CreateWindow("Config Editor",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        500, 400, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsLight();
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL2_Init();

    const std::string configPath = "./assets/config.ini";
    Config& config = Config::getInstance();
    config.load(configPath);

    // [Window]
    int win_width  = config.getInt("Window", "width", 1920);
    int win_height = config.getInt("Window", "height", 1080);
    bool fullscreen = config.getBool("Window", "fullscreen", false);

    // [Game]
    int object_detail = config.getInt("Game", "object_detail", 0);
    int world_detail  = config.getInt("Game", "world_detail", 0);
    int max_view_distance = config.getInt("Game", "max_view_distance", 300000);
    bool show_texture = config.getBool("Game", "show_texture", true);
    bool show_fog     = config.getBool("Game", "show_fog", true);
    bool clouds_enabled = config.getBool("Game", "clouds_enabled", false);
    // [Video]
    bool fx_cpc_palette  = config.getBool("Video", "fx_cpc_palette", false);
    bool fx_scanlines    = config.getBool("Video", "fx_scanlines", false);
    int  fx_pixel_scale  = config.getInt ("Video", "fx_pixel_scale", 1);
    bool super_eagle_2x  = config.getBool("Video", "super_eagle_2x", true);
    bool fx_fxaa         = config.getBool("Video", "fx_fxaa", true);
    bool widescreen_ambilight = config.getBool("Video", "widescreen_ambilight", true);
    int ambilight_sample_width = config.getInt("Video", "ambilight_sample_width", 1);

    DebugControlMapping controlMapping;
    GameEngine::setInstance(std::make_unique<GameEngine>());
    GameEngine *game = &GameEngine::instance();
    EventManager::getInstance().enableImGuiForwarding(true);
    AssetManager::getInstance().SetBase("./assets/");
    game->init();
    controlMapping.init();
    bool running = true;
    struct Resolution { int w, h, refresh; };
    std::vector<Resolution> resolutions;
    int numModes = SDL_GetNumDisplayModes(0);
    for (int i = 0; i < numModes; i++) {
        SDL_DisplayMode mode;
        if (SDL_GetDisplayMode(0, i, &mode) == 0) {
            // Dédoublonner (même résolution, taux de rafraîchissement différent)
            bool found = false;
            for (auto& r : resolutions)
                if (r.w == mode.w && r.h == mode.h) { found = true; break; }
            if (!found)
                resolutions.push_back({mode.w, mode.h, mode.refresh_rate});
        }
    }
    // Trouver l'index courant
    int selectedRes = 0;
    for (int i = 0; i < (int)resolutions.size(); i++) {
        if (resolutions[i].w == win_width && resolutions[i].h == win_height) {
            selectedRes = i;
            break;
        }
    }
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL2_ProcessEvent(&e);
            if (e.type == SDL_QUIT) running = false;
        }

        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        const ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->Pos);
        ImGui::SetNextWindowSize(vp->Size);
        ImGui::Begin("Configuration", nullptr,
                    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

        if (ImGui::BeginTabBar("tabs")) {
            if (ImGui::BeginTabItem("General")) {
                if (ImGui::CollapsingHeader("Window", ImGuiTreeNodeFlags_DefaultOpen)) {
                    // Construire les labels à la volée
                    if (ImGui::BeginCombo("Resolution", 
                        (std::to_string(resolutions[selectedRes].w) + "x" +
                        std::to_string(resolutions[selectedRes].h)).c_str()))
                    {
                        for (int i = 0; i < (int)resolutions.size(); i++) {
                            std::string label = std::to_string(resolutions[i].w) + "x" +
                                                std::to_string(resolutions[i].h);
                            bool isSelected = (i == selectedRes);
                            if (ImGui::Selectable(label.c_str(), isSelected)) {
                                selectedRes  = i;
                                win_width    = resolutions[i].w;
                                win_height   = resolutions[i].h;
                            }
                            if (isSelected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::Checkbox("Fullscreen", &fullscreen);
                }
                if (ImGui::CollapsingHeader("Game", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::SliderInt("Object Detail", &object_detail, 0, 2);
                    ImGui::SliderInt("World Detail",  &world_detail,  0, 2);
                    ImGui::SliderInt("Max view distance", &max_view_distance, 10000, 300000);
                    ImGui::Checkbox("Show Texture", &show_texture);
                    ImGui::Checkbox("Show Fog", &show_fog);
                }
                if (ImGui::CollapsingHeader("Video", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::Checkbox("CPC Palette",   &fx_cpc_palette);
                    ImGui::Checkbox("Scanlines",     &fx_scanlines);
                    ImGui::InputInt("Pixel Scale",   &fx_pixel_scale);
                    ImGui::Checkbox("FXAA",           &fx_fxaa);
                    ImGui::Checkbox("Super Eagle 2x",&super_eagle_2x);
                    ImGui::Checkbox("Widescreen Ambilight", &widescreen_ambilight);
                    ImGui::SliderInt("Ambilight Sample Width", &ambilight_sample_width, 1, 10);

                }

                ImGui::Separator();
                if (ImGui::Button("Save")) {
                    config.setInt ("Window", "width",       win_width);
                    config.setInt ("Window", "height",      win_height);
                    config.setBool("Window", "fullscreen",  fullscreen);
                    config.setInt ("Game",   "object_detail", object_detail);
                    config.setInt ("Game",   "world_detail",  world_detail);
                    config.setInt ("Game",   "max_view_distance", max_view_distance);
                    config.setBool("Game",   "show_texture",  show_texture);
                    config.setBool("Game",   "show_fog",      show_fog);
                    config.setBool("Video",  "fx_cpc_palette", fx_cpc_palette);
                    config.setBool("Video",  "fx_scanlines",   fx_scanlines);
                    config.setInt ("Video",  "fx_pixel_scale",  fx_pixel_scale);
                    config.setBool("Video",  "super_eagle_2x",  super_eagle_2x);
                    config.setBool("Video",  "fx_fxaa",          fx_fxaa);
                    config.setBool("Video",  "widescreen_ambilight", widescreen_ambilight);
                    config.setInt ("Video",  "ambilight_sample_width", ambilight_sample_width);
                    config.save(configPath);
                }
                ImGui::SameLine();
                if (ImGui::Button("Quit")) running = false;
                ImGui::EndTabItem();
                
            }
            if (ImGui::BeginTabItem("Controls")) { 
                if (controlMapping.m_capture.active) {
                    controlMapping.updateCapture(0.1f);
                }
                controlMapping.renderUI();
                ImGui::EndTabItem(); 
            }
            ImGui::EndTabBar();
        }
        ImGui::End();
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui::Render();
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}