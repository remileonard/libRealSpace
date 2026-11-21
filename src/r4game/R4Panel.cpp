//
//  R4Panel.cpp
//  libRealSpace
//
#include "R4Panel.h"
#include "R4Renderer.h"

#include "r4/ui/controls.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <string>

namespace {

constexpr float kPanelWidth = 300;
constexpr size_t kHistory = 240;

std::string fmt(const char *f, ...) {
    char buf[256];
    va_list args;
    va_start(args, f);
    std::vsnprintf(buf, sizeof buf, f, args);
    va_end(args);
    return buf;
}

void push(std::deque<float> &values, double v) {
    values.push_back((float)v);
    while (values.size() > kHistory) {
        values.pop_front();
    }
}

// the last frames, at the scale of their peak
void plot(r4::ui::Context &gui, const std::deque<float> &values, float minScale, r4::ui::Color color, const char *name) {
    float buf[kHistory] = {};
    const size_t n = values.size();
    float sum = 0;
    float peak = 0;
    for (size_t i = 0; i < n; i++) {
        buf[kHistory - n + i] = values[i];
        sum += values[i];
        peak = std::max(peak, values[i]);
    }
    float scale = minScale;
    while (scale < peak * 1.1f) {
        scale *= 2;
    }
    gui.plot(buf, (int)kHistory, scale, color, fmt("%s %.2f ms", name, n ? sum / n : 0.0f).c_str());
}

const char *yes(bool on) {
    return on ? "on" : "off";
}

} // namespace

R4Panel::R4Panel(r4::gpu::Device &device) : device_(device), gui_(device) {
    vsync_ = R4Settings::flag("vsync", true);
    SDL_AddEventWatch(watch, this);
}

R4Panel::~R4Panel() {
    SDL_DelEventWatch(watch, this);
}

int R4Panel::watch(void *panel, SDL_Event *event) {
    R4Panel *self = static_cast<R4Panel *>(panel);
    if (event->type == SDL_MOUSEWHEEL && self->shown_) {
        self->wheel_ += event->wheel.preciseY;
    }
    return 0;
}

// the game hides the cursor in its window, and its menus use the relative mouse
void R4Panel::show(bool shown) {
    shown_ = shown;
    if (shown) {
        gameRelative_ = SDL_GetRelativeMouseMode();
        gameCursor_ = SDL_ShowCursor(SDL_QUERY);
        wheel_ = 0;
    } else {
        SDL_SetRelativeMouseMode(gameRelative_);
        SDL_ShowCursor(gameCursor_);
    }
}

void R4Panel::draw(SDL_Window *window, int pixelWidth, int pixelHeight, r4::render::PostSettings *post, double frameMs, const R4FrameCounters &counters) {
    const Uint8 *keys = SDL_GetKeyboardState(nullptr);
    const bool toggle = keys[SDL_SCANCODE_F11] != 0;
    if (toggle && !toggleDown_) {
        show(!shown_);
    }
    toggleDown_ = toggle;
    if (shown_) {
        if (SDL_GetRelativeMouseMode()) {
            SDL_SetRelativeMouseMode(SDL_FALSE);
        }
        SDL_ShowCursor(SDL_ENABLE);
    }
    R4Renderer *renderer = R4Renderer::current();
    R4Renderer::FrameStats frame = renderer ? renderer->takeFrameStats() : R4Renderer::FrameStats{};
    push(frameMs_, frameMs);
    push(cullMs_, frame.sum.cullMs);
    push(drawMs_, frame.sum.drawMs);
    double gpu[r4::gpu::Device::kGpuTimers] = {};
    const bool gpuKnown = device_.gpuTimes(gpu);
    double gpuTotal = 0;
    for (int i = 0; i < (int)GpuTimer::COUNT; i++) {
        gpuTotal += gpu[i];
    }
    push(gpuMs_, gpuTotal);
    if (!shown_ || !renderer) {
        return;
    }
    int pointWidth = 0;
    int pointHeight = 0;
    SDL_GetWindowSize(window, &pointWidth, &pointHeight);
    int mx = 0;
    int my = 0;
    const Uint32 buttons = SDL_GetMouseState(&mx, &my);
    r4::ui::Input input;
    input.mouseX = (float)mx;
    input.mouseY = (float)my;
    input.left = (buttons & SDL_BUTTON_LMASK) != 0;
    input.right = (buttons & SDL_BUTTON_RMASK) != 0;
    input.pressed = (input.left && !leftDown_) || (input.right && !rightDown_);
    input.released = (!input.left && leftDown_) || (!input.right && rightDown_);
    leftDown_ = input.left;
    rightDown_ = input.right;
    input.wheel = wheel_;
    wheel_ = 0;

    gui_.beginFrame(pointWidth, pointHeight, input, pointWidth > 0 ? (float)pixelWidth / pointWidth : 1);
    gui_.beginPanel("r4nkine", {(float)pointWidth - kPanelWidth, 0, kPanelWidth, (float)pointHeight}, "R4nkine (F12)");
    if (gui_.section("Performance", openPerformance_)) {
        performance(frameMs);
        const r4::render::RenderStats &s = frame.sum;
        gui_.table({
            {"Passes", fmt("%d", frame.passes)},
            {"Draw Calls", fmt("%u", s.gpu.drawCalls)},
            {"Triangles", fmt("%u", s.gpu.triangles)},
            {"Pipeline / Texture Changes", fmt("%u / %u", s.gpu.pipelineChanges, s.gpu.textureChanges)},
            {"Nodes Visited / Culled", fmt("%u / %u", s.nodesVisited, s.nodesCulled)},
            {"Drawables", fmt("%u", s.drawables)},
            {"Instances Visited / Culled", fmt("%u / %u", s.instancesVisited, s.instancesCulled)},
            {"Opaque / Transparent Items", fmt("%u / %u", s.opaqueItems, s.transparentItems)},
            {"Shadow Casters", fmt("%u", s.shadowCasters)},
            {"Shadow / Sort / Upload", fmt("%.2f / %.2f / %.2f ms", s.shadowMs, s.sortMs, s.uploadMs)},
        });
        gui_.toggle("GPU Times (slow on macOS)", R4Screen::gpuTiming());
        if (R4Screen::gpuTiming()) {
            gui_.label(gpuKnown ? "GPU (a frame completed by the GPU)" : "GPU (no time yet)", r4::ui::Font::SMALL);
            gui_.table({
                {"View", fmt("%.2f ms", gpu[(int)GpuTimer::VIEW])},
                {"Target Camera", fmt("%.2f ms", gpu[(int)GpuTimer::TARGET_VIEW])},
                {"320 x 200 Screen", fmt("%.2f ms", gpu[(int)GpuTimer::SCREEN_320])},
                {"Post Process", fmt("%.2f ms", gpu[(int)GpuTimer::POST_PROCESS])},
                {"Panel", fmt("%.2f ms", gpu[(int)GpuTimer::PANEL])},
                {"Total", fmt("%.2f ms", gpuTotal)},
            });
        }
        gui_.label("Outside the passes", r4::ui::Font::SMALL);
        gui_.table({
            {"GL State Saves", fmt("%d", counters.interops)},
            {"Textures Sent", fmt("%d (%.1f KB)", counters.textureUploads, counters.textureBytes / 1024.0)},
            {"GL Buffers Made", fmt("%d", counters.buffers)},
            {"Target Readbacks", fmt("%d (%.2f ms)", counters.readbacks, counters.readbackMs)},
        });
        gui_.label("CPU", r4::ui::Font::SMALL);
        gui_.table({
            {"Flight (runFrame)", fmt("%.2f ms", counters.gameMs)},
            {"   APP (R4Mission::app)", fmt("%.2f ms", counters.appMs)},
            {"      Shots Moved", fmt("%.2f ms", counters.appShotsMs)},
            {"      Nodes Found", fmt("%.2f ms", counters.appFindMs)},
            {"      Callbacks", fmt("%.2f ms", counters.appCallbacksMs)},
            {"      Triggers", fmt("%.2f ms", counters.appTriggersMs)},
            {"   Views (renderScene)", fmt("%.2f ms", counters.sceneMs)},
            {"   Simulation and Rest", fmt("%.2f ms", std::max(0.0, counters.gameMs - counters.appMs - counters.sceneMs))},
            {"Renderer Passes", fmt("%.2f ms", counters.flushMs)},
            {"   in the Flight", fmt("%.2f ms", counters.flightFlushMs)},
            {"Panel", fmt("%.2f ms", counters.panelMs)},
            {"Swap (wait for the GPU)", fmt("%.2f ms", counters.swapMs)},
            {"Outside the Flight", fmt("%.2f ms", std::max(0.0, frameMs - counters.gameMs - counters.panelMs - counters.swapMs))},
        });
    }
    if (gui_.section("Area", openArea_)) {
        area();
    }
    if (gui_.section("Render", openRender_)) {
        render();
        if (gui_.toggle("VSync", vsync_)) {
            SDL_GL_SetSwapInterval(vsync_ ? 1 : 0);
        }
    }
    if (post && gui_.section("Post Process", openPost_)) {
        r4::ui::postProcessControls(gui_, *post);
    }
    gui_.endPanel();
    gui_.endFrame();
    gui_.render();
}

void R4Panel::performance(double frameMs) {
    float sum = 0;
    for (float v : frameMs_) {
        sum += v;
    }
    const float mean = frameMs_.empty() ? 0.0f : sum / (float)frameMs_.size();
    gui_.label(fmt("%.0f FPS, %.2f ms (last %.2f ms)", mean > 0 ? 1000.0f / mean : 0.0f, mean, frameMs).c_str(), r4::ui::Font::BOLD);
    plot(gui_, frameMs_, 16, r4::ui::Color::rgb(0.5f, 1, 0.5f), "frame");
    plot(gui_, cullMs_, 0.25f, r4::ui::Color::rgb(0.4f, 0.8f, 1), "cull");
    plot(gui_, drawMs_, 0.5f, r4::ui::Color::rgb(1, 0.6f, 0.3f), "draw");
    if (R4Screen::gpuTiming()) {
        plot(gui_, gpuMs_, 1, r4::ui::Color::rgb(1, 0.4f, 0.8f), "gpu");
    }
}

void R4Panel::area() {
    R4Renderer *renderer = R4Renderer::current();
    const R4Renderer::AreaStats &a = renderer->areaStats();
    const R4Settings &s = renderer->settings();
    const char *forestModes[] = {"noise", "textures"};
    const char *upscales[] = {"none", "2xsai", "xbr2x", "xbr4x"};
    const char *terrainLods[] = {"distance", "fine", "medium", "coarse"};
    gui_.table({
        {"Made in", fmt("%.0f ms", a.ms)},
        {"Trees", fmt("%zu", a.trees)},
        {"Buildings", fmt("%zu", a.buildings)},
        {"Clouds", fmt("%zu", a.clouds)},
    });
    gui_.label("Made with the area (config.ini)", r4::ui::Font::SMALL);
    gui_.table({
        {"forests", s.forests ? forestModes[(int)s.forestMode] : "off"},
        {"forest_density", fmt("%.2f", s.forestDensity)},
        {"buildings", yes(s.buildings)},
        {"texture_upscale", upscales[(int)s.upscale]},
        {"texture_mipmaps", yes(s.mipmaps)},
        {"anisotropy", fmt("%.0f", s.anisotropy)},
        {"sun_terrain", yes(s.sunTerrain)},
        {"terrain_relief", fmt("%.0f", s.terrainRelief)},
        {"terrain_lod", terrainLods[(int)s.terrainLod]},
        {"light", s.lightOverride ? fmt("%.3f + %.3f", s.ambient, s.direct) : std::string("the game's")},
    });
}

// the settings that the next pass reads; the others need a new area
void R4Panel::render() {
    R4Renderer *renderer = R4Renderer::current();
    R4Settings &s = renderer->settings();
    gui_.columns(2);
    gui_.toggle("Wireframe", s.wireframe);
    gui_.toggle("Flat Shading", s.flatShading);
    gui_.toggle("Textures", renderer->show_textured);
    gui_.toggle("Gouraud", renderer->object_gouraud);
    gui_.toggle("Fog", renderer->show_fog);
    gui_.toggle("Clouds", renderer->show_clouds);
    gui_.toggle("Sky", s.sky);
    gui_.toggle("Target Overlay", s.targetOverlay);
    gui_.toggle("Shadows", s.shadows);
    gui_.toggle("Tree Shadows", s.treeShadows);
    gui_.columns(1);
    gui_.color("Background without Sky", s.skyColor);
    gui_.slider("LOD Scale", s.lodScale, 0.05f, 4, "%.2f", true);
    gui_.slider("Shadow Distance", s.shadowDistance, 100, 20000, "%.0f", true);
    gui_.slider("Shadow on Unlit", s.shadowStrength, 0, 1);
    gui_.toggle("Sun of the Settings", s.sunOverride);
    gui_.slider("Sun Heading", s.sunHeading, -180, 180, "%.0f", false, s.sunOverride);
    gui_.slider("Sun Elevation", s.sunElevation, -90, 90, "%.0f", false, s.sunOverride);
    gui_.toggle("Fog of the Settings", s.fogOverride);
    gui_.slider("Fog Start (km)", s.fogStartKm, 1, 600, "%.0f", true, s.fogOverride);
    gui_.slider("Fog End (km)", s.fogEndKm, 2, 1200, "%.0f", true, s.fogOverride);
}
