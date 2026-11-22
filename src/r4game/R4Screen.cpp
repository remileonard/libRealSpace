//
//  R4Screen.cpp
//  libRealSpace
//
#include "R4Screen.h"
#include "R4ConfigPanel.h"
#include "R4Panel.h"
#include "R4Renderer.h"

#include "engine/Config.hpp"
#include "engine/GLExtensions.h"

#include "r4/ui/controls.h"

#ifdef R4GAME_XR
#include "r4/xr/xr.h"
#else
namespace r4::xr {
class Session {};
} // namespace r4::xr
#endif

#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {

constexpr float kTvDuration = 3.0f;
constexpr int kTvWidth = 400;
constexpr int kTvHeight = 300;

// the 27 colors of the Amstrad CPC: 0, 128 or 255 on each channel
r4::raster::Palette cpcPalette() {
    r4::raster::Palette p;
    const uint8_t levels[3] = {0, 128, 255};
    int n = 0;
    for (uint8_t r : levels) {
        for (uint8_t g : levels) {
            for (uint8_t b : levels) {
                p.colors[n] = r4::raster::rgba(r, g, b);
                n++;
            }
        }
    }
    return p;
}

// "VGA 256" -> "vga256": the names of the palettes in config.ini
std::string paletteKey(const std::string &name) {
    std::string key;
    for (char c : name) {
        if (c != ' ') {
            key += (char)std::tolower((unsigned char)c);
        }
    }
    return key;
}

void *glProc(const char *name) {
    return SDL_GL_GetProcAddress(name);
}

// the matrices of the game and of R4nkine have the same layout: v[column][row]
[[maybe_unused]] Matrix toMatrix(const r4::Mat4 &m) {
    Matrix out;
    std::memcpy(out.v, glm::value_ptr(m), sizeof out.v);
    return out;
}

[[maybe_unused]] r4::Mat4 toMat4(const Matrix &m) {
    return glm::make_mat4(&m.v[0][0]);
}

} // namespace

R4Screen::R4Screen() = default;

R4Screen::~R4Screen() {
    if (R4Renderer *renderer = R4Renderer::current()) {
        renderer->release();
    }
    stopVR();
    configPanel_.reset();
    panel_.reset();
    tv_.reset();
    post_.reset();
    device_.reset();
}

R4Screen *R4Screen::current() {
    if (!RSScreen::hasInstance()) {
        return nullptr;
    }
    return dynamic_cast<R4Screen *>(&RSScreen::instance());
}

bool &R4Screen::gpuTiming() {
    static bool on = false;
    return on;
}

void R4Screen::beginGpuTimer(GpuTimer timer) {
    R4Screen *screen = current();
    if (screen && screen->device_ && gpuTiming()) {
        screen->device_->beginGpuTimer((int)timer);
    }
}

void R4Screen::endGpuTimer() {
    R4Screen *screen = current();
    if (screen && screen->device_) {
        screen->device_->endGpuTimer();
    }
}

double R4Screen::msSince(Uint64 start) {
    return (double)(SDL_GetPerformanceCounter() - start) * 1000.0 / (double)SDL_GetPerformanceFrequency();
}

bool &R4Screen::hostInterop() {
    static bool on = true;
    return on;
}

void R4Screen::countInterop() {
    if (hostInterop()) {
        counters().interops++;
    }
}

R4FrameCounters &R4Screen::counters() {
    static R4FrameCounters counters;
    return counters;
}

void R4Screen::init(int width, int height, bool fullscreen) {
    this->width = width;
    this->height = height;
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS | SDL_INIT_AUDIO) != 0) {
        std::printf("[R4Screen] SDL_Init: %s\n", SDL_GetError());
        return;
    }
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif
#ifdef __APPLE__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#else
    // the game draws everything through R4nkine: a core context; the compatibility one is for SCRenderer, to compare
    const bool compatibility = Config::getInstance().getBool("Video", "gl_compatibility", false);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, compatibility ? SDL_GL_CONTEXT_PROFILE_COMPATIBILITY : SDL_GL_CONTEXT_PROFILE_CORE);
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (fullscreen) {
        flags = (SDL_WindowFlags)(SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);
    }
    m_window = SDL_CreateWindow("Neo Strike Commander (R4nkine)", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, this->width, this->height, flags);
    if (!m_window) {
        std::printf("[R4Screen] SDL_CreateWindow: %s\n", SDL_GetError());
        return;
    }
    int drawableWidth = 0;
    int drawableHeight = 0;
    SDL_GL_GetDrawableSize(m_window, &drawableWidth, &drawableHeight);
    if (drawableWidth > 0 && drawableHeight > 0) {
        this->width = drawableWidth;
        this->height = drawableHeight;
    }
    SDL_GetWindowSize(m_window, &this->logical_width, &this->logical_height);
    m_glContext = SDL_GL_CreateContext(m_window);
    if (!m_glContext) {
        std::printf("[R4Screen] SDL_GL_CreateContext: %s\n", SDL_GetError());
        return;
    }
    SDL_GL_MakeCurrent(m_window, m_glContext);
    if (!initGLExtensions()) {
        std::printf("[R4Screen] the OpenGL extensions of the game are missing\n");
    }
    SDL_GL_SetSwapInterval(R4Settings::flag("vsync", true) ? 1 : 0);
    std::string error;
    device_ = r4::gpu::createGLDevice(glProc, &error);
    if (!device_) {
        std::printf("[R4Screen] R4nkine: %s\n", error.c_str());
        return;
    }
    std::printf("[R4Screen] %s\n", device_->name());
    // in the core context only R4nkine draws: no save of the GL state around its passes
    hostInterop() = Config::getInstance().getBool("Video", "gl_compatibility", false);
    device_->setHostInterop(hostInterop());
    device_->beginHostInterop();
    R4Screen::countInterop();
    post_ = std::make_unique<r4::render::PostProcess>(*device_);
    tv_ = std::make_unique<r4::render::Overlay>(*device_);
    panel_ = std::make_unique<R4Panel>(*device_);
    configPanel_ = std::make_unique<R4ConfigPanel>(*device_);
    device_->endHostInterop();
    tv_->smooth = true;
    tvSurface_ = r4::raster::Surface(kTvWidth, kTvHeight, r4::raster::Format::RGBA8);
    tvSurface_.blend = true;
    initPostProcess();
    openScreen();
    SDL_ShowWindow(m_window);
    if (R4Settings::flag("vr", false)) {
        startVR();
    }
    beginFrame();
}

void R4Screen::openScreen(void) {
    if (!device_) {
        return;
    }
    device_->beginHostInterop();
    R4Screen::countInterop();
    r4::gpu::ClearValues clear;
    device_->beginPass(r4::gpu::RenderTargetHandle{}, 0, 0, this->width, this->height, clear);
    device_->endPass();
    device_->endHostInterop();
}

void R4Screen::initPostProcess() {
    if (!post_) {
        return;
    }
    Config &config = Config::getInstance();
    this->fx_cpc_palette = config.getBool("video", "fx_cpc_palette", false);
    this->fx_scanlines = config.getBool("video", "fx_scanlines", false);
    this->fx_pixel_scale = (float)config.getInt("video", "fx_pixel_scale", 1);
    this->fx_fxaa = config.getBool("video", "fx_fxaa", false);
    r4::render::PostSettings &s = post_->settings;
    s.msaa = std::max(1, config.getInt("Video", "msaa", 1));
    s.fxaa = this->fx_fxaa;
    s.pixelate = std::max(1, (int)std::lround(this->fx_pixel_scale));
    s.scanlines = this->fx_scanlines;
    s.quantize = this->fx_cpc_palette;
    if (this->fx_cpc_palette) {
        s.palette = cpcPalette();
        s.paletteSize = 27;
        s.dither = false;
    }
    // the post process of r4objviewer: each key of [R4nkine] replaces the setting of the game
    s.msaa = std::clamp((int)R4Settings::number("msaa", (float)s.msaa), 1, 8);
    s.fxaa = R4Settings::flag("fxaa", s.fxaa);
    s.pixelate = std::clamp((int)R4Settings::number("pixelate", (float)s.pixelate), 1, 16);
    s.brightness = std::clamp(R4Settings::number("brightness", s.brightness), -1.0f, 1.0f);
    s.contrast = std::clamp(R4Settings::number("contrast", s.contrast), 0.0f, 4.0f);
    s.saturation = std::clamp(R4Settings::number("saturation", s.saturation), 0.0f, 4.0f);
    s.gamma = std::clamp(R4Settings::number("gamma", s.gamma), 0.1f, 4.0f);
    s.tint = R4Settings::color("tint", s.tint);
    s.fade = std::clamp(R4Settings::number("fade", s.fade), 0.0f, 1.0f);
    s.fadeColor = R4Settings::color("fade_color", s.fadeColor);
    std::string palette = config.getString("R4nkine", "palette", "");
    if (palette == "none") {
        s.quantize = false;
    } else if (palette == "cpc27") {
        s.quantize = true;
        s.palette = cpcPalette();
        s.paletteSize = 27;
    } else if (!palette.empty()) {
        for (const r4::ui::NamedPalette &preset : r4::ui::palettePresets()) {
            if (paletteKey(preset.name) == palette) {
                s.quantize = true;
                s.palette = preset.palette;
                s.paletteSize = preset.size;
            }
        }
    }
    s.dither = R4Settings::flag("dither", s.dither);
    s.scanlines = R4Settings::flag("scanlines", s.scanlines);
    s.scanlineStrength = std::clamp(R4Settings::number("scanline_strength", s.scanlineStrength), 0.0f, 1.0f);
    s.scanlineSize = std::clamp(R4Settings::number("scanline_size", s.scanlineSize), 1.0f, 8.0f);
    s.curvature = std::clamp(R4Settings::number("curvature", s.curvature), 0.0f, 0.3f);
    s.aberration = std::clamp(R4Settings::number("aberration", s.aberration), 0.0f, 4.0f);
    s.vignette = std::clamp(R4Settings::number("vignette", s.vignette), 0.0f, 1.0f);
}

void R4Screen::beginFrame() {
    frameTarget_ = r4::gpu::RenderTargetHandle{};
    const bool post = post_ && post_->active();
    if (!post && !xr_) {
        return;
    }
    device_->beginHostInterop();
    R4Screen::countInterop();
    if (post) {
        frameTarget_ = post_->begin(this->width, this->height);
    } else {
        // in VR, the frame is drawn into a target: the screen of the headset shows it
        if (!vrFrame_ || vrFrameWidth_ != this->width || vrFrameHeight_ != this->height) {
            device_->destroyRenderTarget(vrFrame_);
            r4::gpu::RenderTargetDesc d;
            d.width = this->width;
            d.height = this->height;
            vrFrame_ = device_->createRenderTarget(d);
            vrFrameWidth_ = this->width;
            vrFrameHeight_ = this->height;
        }
        frameTarget_ = vrFrame_;
    }
    r4::gpu::ClearValues clear;
    device_->beginPass(frameTarget_, 0, 0, this->width, this->height, clear);
    device_->endPass();
    device_->endHostInterop();
    device_->bindForHost(frameTarget_);
}

void R4Screen::beginGamePass() {
    if (R4Renderer *renderer = R4Renderer::current()) {
        renderer->flush();
    }
    device_->beginHostInterop();
    R4Screen::countInterop();
    r4::gpu::ClearValues keep;
    keep.color = false;
    keep.depth = false;
    beginGpuTimer(GpuTimer::SCREEN_320);
    device_->beginPass(frameTarget_, 0, 0, this->width, this->height, keep);
}

void R4Screen::endGamePass() {
    device_->endPass();
    device_->endGpuTimer();
    device_->endHostInterop();
}

r4::gpu::RenderTargetHandle R4Screen::sceneTarget() const {
#ifdef R4GAME_XR
    if (xr_ && vrEye_ >= 0) {
        return xr_->eye(vrEye_).target;
    }
#endif
    return frameTarget_;
}

void R4Screen::sceneSize(int &width, int &height) const {
    width = this->width;
    height = this->height;
#ifdef R4GAME_XR
    if (xr_ && vrEye_ >= 0) {
        width = xr_->eye(vrEye_).width;
        height = xr_->eye(vrEye_).height;
    }
#endif
}

bool R4Screen::vrActive() const {
    return xr_ != nullptr;
}

#ifdef R4GAME_XR

namespace {

// after beginFrame: the first frame of the headset, the menu button or a click of a stick recenter
void vrInput(r4::xr::Session &xr, bool centered, R4Screen &screen) {
    bool recenter = !centered;
    for (int h = 0; h < r4::xr::kHands; h++) {
        const r4::xr::Controller &c = xr.controller((r4::xr::Hand)h);
        recenter = recenter || c.menu.pressed || c.stickClick.pressed;
    }
    if (recenter) {
        screen.vrRecenter();
    }
}

} // namespace

void R4Screen::startVR() {
    if (xr_ || !device_) {
        return;
    }
    r4::xr::Settings s;
    s.appName = "Neo Strike Commander";
    s.seated = true;
    s.samples = std::clamp((int)R4Settings::number("vr_msaa", 4), 1, 8);
    s.renderScale = std::clamp(R4Settings::number("vr_scale", 1), 0.25f, 2.0f);
    std::string error;
    device_->beginHostInterop();
    R4Screen::countInterop();
    xr_ = r4::xr::Session::create(*device_, s, &error);
    device_->endHostInterop();
    if (!xr_) {
        std::printf("[R4Screen] VR: %s\n", error.c_str());
        return;
    }
    std::printf("[R4Screen] VR: %s, %s\n", xr_->runtimeName().c_str(), xr_->systemName().c_str());
    // the headset gives the rhythm of the frames (xrWaitFrame): the window does not wait for its screen
    SDL_GL_SetSwapInterval(0);
    vrCentered_ = false;
}

void R4Screen::stopVR() {
    if (!xr_ || !device_) {
        return;
    }
    device_->beginHostInterop();
    R4Screen::countInterop();
    xr_.reset();
    device_->destroyRenderTarget(vrFrame_);
    device_->endHostInterop();
    vrFrame_ = r4::gpu::RenderTargetHandle{};
    vrFrameWidth_ = 0;
    vrFrameHeight_ = 0;
    vrFrameBegun_ = false;
    vrEye_ = -1;
    SDL_GL_SetSwapInterval(R4Settings::flag("vsync", true) ? 1 : 0);
}

void R4Screen::vrRecenter() {
    if (!xr_) {
        return;
    }
    // the head of now at the origin, its heading along -Z: the eye of the cockpit and its front
    const r4::Mat4 &head = xr_->head();
    const r4::Vec3 at = r4::Vec3(head[3]);
    r4::Vec3 forward = -r4::Vec3(head[2]);
    forward.y = 0;
    float yaw = 0;
    if (glm::length(forward) > 1e-4f) {
        yaw = std::atan2(-forward.x, -forward.z);
    }
    vrCenter_ = glm::inverse(glm::translate(r4::Mat4(1.0f), at) * glm::rotate(r4::Mat4(1.0f), yaw, r4::Vec3(0, 1, 0)));
    vrCentered_ = true;
}

uint32_t R4Screen::vrStereoEyeCount() const {
    return xr_ && xr_->running() ? (uint32_t)xr_->eyeCount() : 0u;
}

bool R4Screen::vrPrepareStereoFrame(bool &outShouldRender) {
    outShouldRender = false;
    if (!xr_ || !xr_->running() || vrFrameBegun_ || vrEyesShown_) {
        return false;
    }
    if (R4Renderer *renderer = R4Renderer::current()) {
        renderer->flush();
    }
    device_->beginHostInterop();
    R4Screen::countInterop();
    xr_->setOrigin(r4::Mat4(1.0f));
    vrFrameBegun_ = true;
    const Uint64 wait = SDL_GetPerformanceCounter();
    outShouldRender = xr_->beginFrame();
    counters().vrWaitMs += msSince(wait);
    if (outShouldRender) {
        vrInput(*xr_, vrCentered_, *this);
    }
    device_->endHostInterop();
    return true;
}

bool R4Screen::vrBeginStereoEye(uint32_t eye, const Matrix &worldFromLocal, float metersToWorld, float zNear, float zFar, VRStereoEyeRenderInfo &out) {
    if (!xr_ || !vrFrameBegun_ || eye >= (uint32_t)xr_->eyeCount()) {
        return false;
    }
    // the passes of the eye before go into its target
    const Uint64 start = SDL_GetPerformanceCounter();
    if (R4Renderer *renderer = R4Renderer::current()) {
        renderer->flush();
    }
    const r4::xr::Eye &e = xr_->eye((int)eye);
    r4::Mat4 local = vrCenter_ * e.world;
    local[3] = r4::Vec4(r4::Vec3(local[3]) * metersToWorld, 1.0f);
    const r4::Mat4 world = toMat4(worldFromLocal) * local;
    // the frustum of the eye with the near and the far of the game: its off-axis terms do not change
    r4::Mat4 projection = e.camera.projection;
    projection[2][2] = -(zFar + zNear) / (zFar - zNear);
    projection[3][2] = -2.0f * zFar * zNear / (zFar - zNear);
    out.width = e.width;
    out.height = e.height;
    out.projection = toMatrix(projection);
    out.view = toMatrix(glm::inverse(world));
    vrEye_ = (int)eye;
    vrEyesDrawn_ = true;
    device_->beginHostInterop();
    R4Screen::countInterop();
    r4::gpu::ClearValues clear;
    device_->beginPass(e.target, 0, 0, e.width, e.height, clear);
    device_->endPass();
    device_->endHostInterop();
    counters().vrEyesMs += msSince(start);
    return true;
}

void R4Screen::vrEndStereoFrame() {
    if (!xr_ || !vrFrameBegun_) {
        return;
    }
    const Uint64 start = SDL_GetPerformanceCounter();
    if (R4Renderer *renderer = R4Renderer::current()) {
        renderer->flush();
    }
    vrEye_ = -1;
    const Uint64 submit = SDL_GetPerformanceCounter();
    counters().vrEyesMs += msSince(start);
    device_->beginHostInterop();
    R4Screen::countInterop();
    xr_->endFrame(vrEyesDrawn_);
    device_->endHostInterop();
    counters().vrSubmitMs += msSince(submit);
    vrFrameBegun_ = false;
    vrEyesShown_ = true;
}

void R4Screen::vrScreenFrame() {
    const Uint64 start = SDL_GetPerformanceCounter();
    xr_->setOrigin(r4::Mat4(1.0f));
    if (xr_->beginFrame()) {
        vrInput(*xr_, vrCentered_, *this);
    }
    if (frameTarget_) {
        r4::xr::Screen screen;
        screen.source = frameTarget_;
        screen.sourceWidth = this->width;
        screen.sourceHeight = this->height;
        const float distance = std::clamp(R4Settings::number("vr_screen_distance", 2.0f), 0.5f, 20.0f);
        screen.height = std::clamp(R4Settings::number("vr_screen_height", 1.5f), 0.1f, 20.0f);
        screen.width = screen.height * (float)this->width / (float)std::max(1, this->height);
        screen.pose = glm::inverse(vrCenter_) * glm::translate(r4::Mat4(1.0f), r4::Vec3(0, 0, -distance));
        xr_->showScreen(screen);
    }
    xr_->endFrame(false);
    counters().vrScreenMs += msSince(start);
}

void R4Screen::vrMirror() {
    // the left eye in the window, centered at its aspect
    const r4::xr::Eye &e = xr_->eye(0);
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    r4::render::Overlay::fit((float)e.width / (float)std::max(1, e.height), this->width, this->height, x, y, w, h);
    r4::gpu::ClearValues clear;
    device_->beginPass(r4::gpu::RenderTargetHandle{}, 0, 0, this->width, this->height, clear);
    device_->endPass();
    device_->copyRenderTargetToWindow(e.target, x, this->height - y - h, w, h);
}

#else

void R4Screen::startVR() {
    std::printf("[R4Screen] VR: R4nkine is built without r4::xr (OpenXR)\n");
}

void R4Screen::stopVR() {
}

void R4Screen::vrRecenter() {
}

uint32_t R4Screen::vrStereoEyeCount() const {
    return 0;
}

bool R4Screen::vrPrepareStereoFrame(bool &outShouldRender) {
    outShouldRender = false;
    return false;
}

bool R4Screen::vrBeginStereoEye(uint32_t, const Matrix &, float, float, float, VRStereoEyeRenderInfo &) {
    return false;
}

void R4Screen::vrEndStereoFrame() {
}

void R4Screen::vrScreenFrame() {
}

void R4Screen::vrMirror() {
}

#endif

void R4Screen::refresh(void) {
    if (!device_) {
        SDL_GL_SwapWindow(m_window);
        return;
    }
    if (R4Renderer *renderer = R4Renderer::current()) {
        renderer->flush();
    }
#ifdef R4GAME_XR
    if (xr_) {
        device_->beginHostInterop();
        R4Screen::countInterop();
        const bool alive = xr_->pollEvents();
        if (alive && !vrEyesShown_ && xr_->running()) {
            vrScreenFrame();
        }
        device_->endHostInterop();
        if (!alive) {
            std::printf("[R4Screen] VR: the session of the headset is finished\n");
            stopVR();
        }
    }
#endif
    if (frameTarget_) {
        device_->bindForHost(r4::gpu::RenderTargetHandle{});
        device_->beginHostInterop();
        R4Screen::countInterop();
        beginGpuTimer(GpuTimer::POST_PROCESS);
        if (vrFrame_ && frameTarget_.id == vrFrame_.id) {
            device_->copyRenderTargetToWindow(vrFrame_, 0, 0, this->width, this->height);
        } else if (post_ && post_->active()) {
            post_->end(0, 0, this->width, this->height, this->width, this->height);
        }
        device_->endGpuTimer();
        if (xr_ && vrEyesDrawn_) {
            vrMirror();
        }
        device_->endHostInterop();
    }
    vrEyesShown_ = false;
    vrEyesDrawn_ = false;
    const Uint64 now = SDL_GetPerformanceCounter();
    const double frameMs = lastFrame_ ? (double)(now - lastFrame_) * 1000.0 / (double)SDL_GetPerformanceFrequency() : 0.0;
    lastFrame_ = now;
    counters().swapMs = lastSwapMs_;
    counters().panelMs = lastPanelMs_;
    const R4FrameCounters frame = counters();
    counters() = {};
    device_->beginHostInterop();
    R4Screen::countInterop();
    beginGpuTimer(GpuTimer::PANEL);
    const Uint64 panel = SDL_GetPerformanceCounter();
    panel_->draw(m_window, this->width, this->height, &post_->settings, frameMs, frame);
    configPanel_->draw(m_window, this->width, this->height);
    lastPanelMs_ = msSince(panel);
    device_->endGpuTimer();
    device_->endGpuFrame();
    device_->takeStats();
    device_->endHostInterop();
    const Uint64 swap = SDL_GetPerformanceCounter();
    SDL_GL_SwapWindow(m_window);
    lastSwapMs_ = msSince(swap);
    device_->beginHostInterop();
    R4Screen::countInterop();
    r4::gpu::ClearValues clear;
    device_->beginPass(r4::gpu::RenderTargetHandle{}, 0, 0, this->width, this->height, clear);
    device_->endPass();
    device_->endHostInterop();
    beginFrame();
}

void R4Screen::fxTurnOnTv() {
    if (!device_) {
        is_spfx_finished = true;
        return;
    }
    tvTime_ = std::min(tvTime_ + 1.0f / 60.0f, kTvDuration);
    const float t = tvTime_ / kTvDuration;
    if (t >= 1.0f) {
        is_spfx_finished = true;
        return;
    }
    const uint32_t white = r4::raster::rgba(255, 255, 255);
    auto alpha = [](float a) {
        return r4::raster::rgba(255, 255, 255, (uint8_t)std::clamp(a * 255.0f, 0.0f, 255.0f));
    };
    const int w = kTvWidth;
    const int h = kTvHeight;
    tvSurface_.clear(r4::raster::rgba(0, 0, 0));
    if (t < 0.1f) {
        const int lineWidth = (int)(t / 0.1f * w);
        tvSurface_.fillRect((w - lineWidth) / 2, h / 2 - 1, lineWidth, 2, white);
    } else if (t < 0.2f) {
        const int lineHeight = (int)((t - 0.1f) / 0.1f * h);
        tvSurface_.fillRect(0, (h - lineHeight) / 2, w, std::max(lineHeight, 2), white);
    }
    if (t >= 0.3f && t < 0.7f) {
        const uint32_t noise = alpha(1.0f - t / 0.8f);
        for (int i = 0; i < 4000; i++) {
            const int size = 1 + std::rand() % 2;
            tvSurface_.fillRect(std::rand() % w, std::rand() % h, size, size, noise);
        }
    }
    if (t > 0.3f && t < 0.8f) {
        const float lineProgress = (t - 0.2f) / 0.6f;
        const int lineY = (int)((1.0f - lineProgress) * h);
        tvSurface_.fillRect(0, lineY - h / 40, w, h / 20, alpha(0.8f));
    }
    if (t > 0.5f) {
        const uint32_t scanline = alpha((t - 0.5f) / 0.5f * 0.3f * 0.15f);
        for (int y = 0; y < h; y += 2) {
            tvSurface_.hline(0, w - 1, y, scanline);
        }
    }
    tvSurface_.touch();
    beginGamePass();
    tv_->update(tvSurface_);
    tv_->draw(0, 0, this->width, this->height, this->width, this->height);
    endGamePass();
}
