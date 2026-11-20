//
//  R4VGA.cpp
//  libRealSpace
//
#include "R4VGA.h"

#include "R4Screen.h"

#include <algorithm>
#include <cstring>

R4VGA::R4VGA() = default;

R4VGA::~R4VGA() = default;

void R4VGA::activate(void) {
}

void R4VGA::initTextures() {
}

void R4VGA::makeOverlays() {
    R4Screen *screen = R4Screen::current();
    if (screen == nullptr || screen_) {
        return;
    }
    screen_ = std::make_unique<r4::render::Overlay>(screen->device());
    leftBand_ = std::make_unique<r4::render::Overlay>(screen->device());
    rightBand_ = std::make_unique<r4::render::Overlay>(screen->device());
    leftBand_->smooth = true;
    rightBand_->smooth = true;
}

void R4VGA::displayBuffer(uint32_t *buffer, int bufferWidth, int bufferHeight) {
    R4Screen *screen = R4Screen::current();
    if (buffer == nullptr || bufferWidth <= 0 || bufferHeight <= 0 || screen == nullptr) {
        return;
    }
    screen->beginGamePass();
    makeOverlays();
    const int windowWidth = screen->width;
    const int windowHeight = screen->height;
    const int gameWidth = std::min(windowWidth, (int)((float)windowHeight * (4.0f / 3.0f)));
    const int sideWidth = (windowWidth - gameWidth) / 2;
    const int gameX = sideWidth;

    if (image_.width() != bufferWidth || image_.height() != bufferHeight) {
        image_ = r4::raster::Surface(bufferWidth, bufferHeight, r4::raster::Format::RGBA8);
    }
    std::memcpy(image_.pixels(), buffer, (size_t)bufferWidth * bufferHeight * 4);
    image_.touch();
    screen_->smooth = this->upscale;
    screen_->update(image_);

    const bool bands = this->widescreen_ambilight && this->wide_screen && sideWidth > 0;
    if (bands) {
        const int edge = std::clamp(this->ambilight_sample_width, 1, bufferWidth / 4);
        if (leftEdge_.width() != edge || leftEdge_.height() != bufferHeight) {
            leftEdge_ = r4::raster::Surface(edge, bufferHeight, r4::raster::Format::RGBA8);
            rightEdge_ = r4::raster::Surface(edge, bufferHeight, r4::raster::Format::RGBA8);
        }
        for (int y = 0; y < bufferHeight; y++) {
            const uint32_t *row = buffer + (size_t)y * bufferWidth;
            std::memcpy(leftEdge_.pixels() + (size_t)y * edge * 4, row, (size_t)edge * 4);
            std::memcpy(rightEdge_.pixels() + (size_t)y * edge * 4, row + bufferWidth - edge, (size_t)edge * 4);
        }
        leftEdge_.touch();
        rightEdge_.touch();
        leftBand_->update(leftEdge_);
        rightBand_->update(rightEdge_);
        const float opacity = std::clamp(this->ambilight_intensity, 0.0f, 1.0f);
        leftBand_->opacity = opacity;
        rightBand_->opacity = opacity;
    }

    if (bands) {
        leftBand_->draw(0, 0, sideWidth + 1, windowHeight, windowWidth, windowHeight);
        rightBand_->draw(gameX + gameWidth - 1, 0, sideWidth, windowHeight, windowWidth, windowHeight);
    }
    screen_->draw(gameX, 0, gameWidth, windowHeight, windowWidth, windowHeight);
    screen->endGamePass();
}
