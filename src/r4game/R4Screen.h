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

#include <memory>

class R4Screen : public RSScreen {
public:
    R4Screen();
    ~R4Screen() override;

    // R4Screen::instance() when the screen of the game is a R4Screen, else null
    static R4Screen *current();

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
    r4::raster::Surface tvSurface_;
    float tvTime_ = 0;
};
