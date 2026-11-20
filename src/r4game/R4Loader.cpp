//
//  R4Loader.cpp
//  libRealSpace
//
#include "R4Loader.h"

#include "R4Screen.h"

R4Loader::R4Loader() = default;

R4Loader::~R4Loader() = default;

void R4Loader::presentSurface(SDL_Surface *surface) {
    R4Screen *screen = R4Screen::current();
    if (surface == nullptr || screen == nullptr) {
        return;
    }
    if (image_.width() != surface->w || image_.height() != surface->h) {
        image_ = r4::raster::Surface(surface->w, surface->h, r4::raster::Format::RGBA8);
    }
    SDL_LockSurface(surface);
    for (int y = 0; y < surface->h; y++) {
        const uint32_t *from = (const uint32_t *)((const uint8_t *)surface->pixels + (size_t)y * surface->pitch);
        uint8_t *to = image_.pixels() + (size_t)y * image_.pitch();
        for (int x = 0; x < surface->w; x++) {
            uint8_t r = 0;
            uint8_t g = 0;
            uint8_t b = 0;
            uint8_t a = 0;
            SDL_GetRGBA(from[x], surface->format, &r, &g, &b, &a);
            to[x * 4] = r;
            to[x * 4 + 1] = g;
            to[x * 4 + 2] = b;
            to[x * 4 + 3] = 255;
        }
    }
    SDL_UnlockSurface(surface);
    image_.touch();
    screen->beginGamePass();
    if (!overlay_) {
        overlay_ = std::make_unique<r4::render::Overlay>(screen->device());
        overlay_->smooth = true;
    }
    overlay_->update(image_);
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    r4::render::Overlay::fit(4.0f / 3.0f, screen->width, screen->height, x, y, w, h);
    overlay_->draw(x, y, w, h, screen->width, screen->height);
    screen->endGamePass();
}
