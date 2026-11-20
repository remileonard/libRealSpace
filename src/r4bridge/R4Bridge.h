//
//  R4Bridge.h
//  libRealSpace
//
//  The objects that libRealSpace reads (RSEntity, RSPalette) as models of R4nkine.
#pragma once

#include "r4/assets/model.h"
#include "r4/assets/upscale.h"

#include <memory>
#include <vector>

class RSEntity;
struct VGAPalette;
struct PaletteRange;

namespace r4bridge {

// ambient + light * max(N.L, 0), as SCRenderer::objectIntensity
struct Lighting {
    float ambient = 3.0f / 8.0f;
    float light = 5.0f / 8.0f;
};

// REAL_SPACE keeps libRealSpace's Y up, for the models of a world turned at its root by realSpaceToR4()
enum class Axes {
    R4,
    REAL_SPACE
};

// (x, y, z) -> (x, -z, y), as r4::axes::fromYUpMatrix()
r4::Mat4 realSpaceToR4();

// the first color of a ramp: the whole ramp as for the faces of the objects, or that color itself as for the texels
enum class RampFirst {
    WHOLE_RAMP,
    ITSELF
};

// 256 colors x 32 intensities, as SCRenderer::shadeAlongRamp: the image the PaletteRamp material reads
r4::assets::Image rampImage(const VGAPalette &palette, const std::vector<PaletteRange> &ranges, RampFirst first = RampFirst::WHOLE_RAMP);

// the clear texels take the color of their opaque neighbors, so that filtering does not darken the edges
void bleedClearTexels(r4::assets::Image &image);

// applies to the models made from then on; the default is that of SCRenderer: linear, no mipmaps, no upscale
struct TextureQuality {
    r4::assets::Upscale upscale = r4::assets::Upscale::NONE;
    bool mipmaps = false;
};
TextureQuality &textureQuality();

// bled, upscaled and mipmapped as textureQuality(), clamped to its edges
void finishTexture(r4::assets::Image &image);

// An RSEntity as SCRenderer::drawModel draws it: its LODs, a color pass, a texture pass on LVL0 and a clear pass (property 2, additive).
std::shared_ptr<r4::assets::Model> toModel(RSEntity *entity, const VGAPalette &palette, const std::vector<PaletteRange> &ranges, Lighting lighting = {}, Axes axes = Axes::R4);

} // namespace r4bridge
