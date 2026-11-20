//
//  R4VGA.h
//  libRealSpace
//
//  The 320 x 200 screen of the game (menus, conversations, films, 2D cockpit) shown by the overlays of R4nkine, over what the game drew.
//
#pragma once

#include "engine/RSVGA.h"

#include "r4/raster/raster.h"
#include "r4/render/overlay.h"

#include <memory>

class R4VGA : public RSVGA {
public:
    R4VGA();
    ~R4VGA() override;

    void activate(void) override;

protected:
    void initTextures() override;
    void displayBuffer(uint32_t *buffer, int bufferWidth, int bufferHeight) override;

private:
    void makeOverlays();

    std::unique_ptr<r4::render::Overlay> screen_;
    std::unique_ptr<r4::render::Overlay> leftBand_;
    std::unique_ptr<r4::render::Overlay> rightBand_;
    r4::raster::Surface image_;
    r4::raster::Surface leftEdge_;
    r4::raster::Surface rightEdge_;
};
