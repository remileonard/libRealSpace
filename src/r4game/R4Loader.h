//
//  R4Loader.h
//  libRealSpace
//
//  The loading screen of the game shown by an overlay of R4nkine.
//
#pragma once

#include "engine/Loader.h"

#include "r4/raster/raster.h"
#include "r4/render/overlay.h"

#include <memory>

class R4Loader : public Loader {
public:
    R4Loader();
    ~R4Loader();

protected:
    void presentSurface(SDL_Surface *surface) override;

private:
    std::unique_ptr<r4::render::Overlay> overlay_;
    r4::raster::Surface image_;
};
