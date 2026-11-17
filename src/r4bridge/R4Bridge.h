//
//  R4Bridge.h
//  libRealSpace
//
//  The bridge between libRealSpace and R4nkine: the objects that libRealSpace
//  reads (RSEntity, RSPalette) become models of R4nkine, ready to upload and
//  to draw with its renderer. Nothing is read again: the vertices, faces,
//  levels of detail, texture coordinates and images are taken from the
//  RSEntity as libRealSpace parsed them.
//
#pragma once

#include "r4/assets/model.h"

#include <memory>
#include <vector>

class RSEntity;
struct VGAPalette;
struct PaletteRange;

namespace r4bridge {

// The lighting of the game (SCRenderer::objectIntensity): intensity =
// ambient + light * max(N.L, 0), as SCRenderer::setObjectLight(3, 5, ...) makes it.
struct Lighting {
    float ambient = 3.0f / 8.0f;
    float light = 5.0f / 8.0f;
};

// The table of the shades of the palette, as SCRenderer::shadeAlongRamp
// computes them: 256 colors x 32 intensities, the image R4nkine's
// PaletteRamp material reads (row k: intensity k / 31).
r4::assets::Image rampImage(const VGAPalette &palette, const std::vector<PaletteRange> &ranges);

// An RSEntity as a model of R4nkine, as SCRenderer::drawModel draws it:
// - the axes: the game has Y up, R4nkine Z up (a turn of 90 degrees about X);
//   the faces of the game turn clockwise (glFrontFace(GL_CW)), they are
//   turned the other way for R4nkine;
// - the levels of detail (LVL0, LVL1...) become a LOD node, at their distances;
// - the color pass: the PaletteRamp material (the palette index in the red
//   of the vertex colors; color for a triangle, color - 1 for a quad);
// - the texture pass, on LVL0: the image through the palette, index 255
//   clear; property 6 or 9 the texture is the face, else a decal over it;
// - the clear pass (property 2): palette color - 1, added (AlphaMode::Additive);
// - two-sided: flags[2] == 1 for a triangle, props2 == 1 of its ATTR for a quad.
std::shared_ptr<r4::assets::Model> toModel(RSEntity *entity, const VGAPalette &palette,
                                           const std::vector<PaletteRange> &ranges, Lighting lighting = {});

} // namespace r4bridge
