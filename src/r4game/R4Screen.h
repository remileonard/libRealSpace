//
//  R4Screen.h
//  libRealSpace
//
//  The window of the game on R4nkine: its GL context, its device, its post process and the frame the game draws into.
//
#pragma once

#include "engine/RSScreen.h"

#include "r4/gpu/device.h"
#include "r4/raster/raster.h"
#include "r4/render/overlay.h"
#include "r4/render/postprocess.h"

#include <SDL.h>

#include <memory>

class R4Panel;

// the GPU work of a frame outside the passes of the renderer, counted by r4game and shown by the panel
struct R4FrameCounters {
    // beginHostInterop: each saves and gives back the GL state
    int interops = 0;
    // textures made or changed, and their bytes
    int textureUploads = 0;
    size_t textureBytes = 0;
    // the GL buffers made (two by mesh)
    int buffers = 0;
    // readPixels of the view of the target camera, and their time
    int readbacks = 0;
    double readbackMs = 0;
    // CPU milliseconds: runFrame of the flight (the passes of the renderer included), the passes of the renderer, and the swap of the
    // frame before, where the CPU waits for the GPU
    double gameMs = 0;
    double flushMs = 0;
    double swapMs = 0;
    // in runFrame: R4Strike::app, the views (renderScene, their passes included) and the passes of the renderer
    double appMs = 0;
    double sceneMs = 0;
    double flightFlushMs = 0;
    // the panel of the frame before
    double panelMs = 0;
};

// the slots of the GPU timers of a frame
enum class GpuTimer {
    VIEW,
    TARGET_VIEW,
    SCREEN_320,
    POST_PROCESS,
    PANEL,
    COUNT
};

class R4Screen : public RSScreen {
public:
    R4Screen();
    ~R4Screen() override;

    // R4Screen::instance() when the screen of the game is a R4Screen, else null
    static R4Screen *current();
    // the counters of the frame; refresh gives them to the panel, then starts them again
    static R4FrameCounters &counters();
    // the GL state is saved around the passes of R4nkine only in a compatibility context, where the game can draw with GL
    static bool &hostInterop();
    // a beginHostInterop, counted when it saves the state
    static void countInterop();
    // the milliseconds since a value of SDL_GetPerformanceCounter
    static double msSince(Uint64 start);
    // the GPU timers, off by default: on macOS, their queries slow the frame down and make the times larger
    static bool &gpuTiming();
    // an interval of a GPU timer, when gpuTiming is on
    static void beginGpuTimer(GpuTimer timer);
    static void endGpuTimer();

    void init(int width, int height, bool fullscreen) override;
    void openScreen(void) override;
    void refresh(void) override;
    void fxTurnOnTv() override;
    void initPostProcess() override;

    bool hasDevice() const {
        return device_ != nullptr;
    }
    r4::gpu::Device &device() {
        return *device_;
    }
    // the target of the frame: the one of the post process, null for the window
    r4::gpu::RenderTargetHandle frameTarget() const {
        return frameTarget_;
    }
    // a pass in the frame that keeps what the game drew, in the state of the host
    void beginGamePass();
    void endGamePass();

private:
    void beginFrame();

    std::unique_ptr<r4::gpu::Device> device_;
    std::unique_ptr<r4::render::PostProcess> post_;
    r4::gpu::RenderTargetHandle frameTarget_;
    std::unique_ptr<r4::render::Overlay> tv_;
    std::unique_ptr<R4Panel> panel_;
    Uint64 lastFrame_ = 0;
    double lastSwapMs_ = 0;
    double lastPanelMs_ = 0;
    r4::raster::Surface tvSurface_;
    float tvTime_ = 0;
};
