//
//  R4Panel.h
//  libRealSpace
//
//  The panel of R4nkine over the frame of the game, shown and hidden with F12: the performance of the renderer and its settings.
//  It reads the mouse and the keyboard of SDL each frame, and the wheel by an event watch: the events stay with the game.
//  While it is shown, the cursor is shown and the mouse is not relative.
//
#pragma once

#include "R4Screen.h"

#include "r4/gpu/device.h"
#include "r4/render/postprocess.h"
#include "r4/ui/ui.h"

#include <SDL.h>

#include <deque>

class R4Panel {
public:
    explicit R4Panel(r4::gpu::Device &device);
    ~R4Panel();
    R4Panel(const R4Panel &) = delete;
    R4Panel &operator=(const R4Panel &) = delete;

    // after the post process, in the window; frameMs: the time since the last frame; counters: the work of the last frame
    void draw(SDL_Window *window, int pixelWidth, int pixelHeight, r4::render::PostSettings *post, double frameMs, const R4FrameCounters &counters);

private:
    static int watch(void *panel, SDL_Event *event);
    void show(bool shown);
    void performance(double frameMs);
    void area();
    void render();

    r4::gpu::Device &device_;
    r4::ui::Context gui_;
    bool shown_ = false;
    bool toggleDown_ = false;
    bool leftDown_ = false;
    bool rightDown_ = false;
    float wheel_ = 0;
    SDL_bool gameRelative_ = SDL_FALSE;
    int gameCursor_ = SDL_DISABLE;
    bool openPerformance_ = true;
    bool openArea_ = true;
    bool openRender_ = true;
    bool openPost_ = false;
    bool vsync_ = true;
    std::deque<float> frameMs_;
    std::deque<float> cullMs_;
    std::deque<float> drawMs_;
    std::deque<float> gpuMs_;
};
