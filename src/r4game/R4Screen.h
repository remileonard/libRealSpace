//
//  R4Screen.h
//  libRealSpace
//
//  The window of the game on R4nkine: its GL context, its device, its post process and the frame the game draws into.
//
#pragma once

#include "engine/RSScreen.h"

#include "r4/gpu/device.h"
#include "r4/math.h"
#include "r4/raster/raster.h"
#include "r4/render/overlay.h"
#include "r4/render/postprocess.h"

#include <SDL.h>

#include <memory>

class R4Panel;
class R4ConfigPanel;

namespace r4::xr {
class Session;
}

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
    // in the APP step: the shots of SCStrike moved, the nodes made and dropped, the traversal of the callbacks, the triggers
    double appShotsMs = 0;
    double appFindMs = 0;
    double appCallbacksMs = 0;
    double appTriggersMs = 0;
    double sceneMs = 0;
    double flightFlushMs = 0;
    // the panel of the frame before
    double panelMs = 0;
    // VR, in the flight (in gameMs): xrWaitFrame and the views of the headset (beginFrame), the passes of the eyes sent out of
    // renderScene, the copy of the eyes into the swapchains and xrEndFrame
    double vrWaitMs = 0;
    double vrEyesMs = 0;
    double vrSubmitMs = 0;
    // VR, out of the flight: a frame of the headset with the screen of the menus (wait, copy and xrEndFrame)
    double vrScreenMs = 0;
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
    // the target of the 3D views: the eye of the headset that SCStrike draws, else the frame
    r4::gpu::RenderTargetHandle sceneTarget() const;
    void sceneSize(int &width, int &height) const;
    // a pass in the frame that keeps what the game drew, in the state of the host
    void beginGamePass();
    void endGamePass();

    // VR ([R4nkine] vr = 1 in config.ini): the flight in the headset, the other screens of the game on a flat screen in front of the eyes
    bool vrActive() const;
    // the head of now is the front and the eye of the cockpit
    void vrRecenter();
    uint32_t vrStereoEyeCount() const override;
    bool vrPrepareStereoFrame(bool &outShouldRender) override;
    bool vrBeginStereoEye(uint32_t eye, const Matrix &worldFromLocal, float metersToWorld, float zNear, float zFar, VRStereoEyeRenderInfo &out) override;
    void vrEndStereoFrame() override;

private:
    void beginFrame();
    void startVR();
    void stopVR();
    // a frame of the headset with the frame of the window on its screen, when SCStrike did not draw the eyes
    void vrScreenFrame();
    void vrMirror();

    std::unique_ptr<r4::gpu::Device> device_;
    std::unique_ptr<r4::render::PostProcess> post_;
    r4::gpu::RenderTargetHandle frameTarget_;
    std::unique_ptr<r4::render::Overlay> tv_;
    std::unique_ptr<R4Panel> panel_;
    std::unique_ptr<R4ConfigPanel> configPanel_;
    Uint64 lastFrame_ = 0;
    double lastSwapMs_ = 0;
    double lastPanelMs_ = 0;
    r4::raster::Surface tvSurface_;
    float tvTime_ = 0;

    std::unique_ptr<r4::xr::Session> xr_;
    // the frame of the window when there is no post process: the source of the screen of the headset
    r4::gpu::RenderTargetHandle vrFrame_;
    int vrFrameWidth_ = 0;
    int vrFrameHeight_ = 0;
    // a frame of the headset is begun; the eye drawn, else -1; SCStrike ended the frame of the headset in this frame of the game, and drew its eyes
    bool vrFrameBegun_ = false;
    int vrEye_ = -1;
    bool vrEyesShown_ = false;
    bool vrEyesDrawn_ = false;
    // the tracking space in the cockpit: the head of the last recenter at its origin, looking to -Z
    r4::Mat4 vrCenter_ = r4::Mat4(1.0f);
    bool vrCentered_ = false;
};
