//
//  R4Data.h
//  libRealSpace
//
//  The identifiers of the data of the game that r4bridge relies on.
//
#pragma once

namespace r4bridge {

// the images of TXMPACK (TEXTURES.TRE), the same on every map
enum TextureId {
    TEXTURE_MEADOW_WOOD = 216,
    TEXTURE_WOOD = 217,
    TEXTURE_JUNGLE_A = 218,
    TEXTURE_JUNGLE_B = 219,
    TEXTURE_HARBOR_A = 223,
    TEXTURE_HARBOR_B = 224,
    TEXTURE_HARBOR_C = 225,
    TEXTURE_HARBOR_D = 226,
    TEXTURE_HARBOR_E = 227,
    TEXTURE_HARBOR_F = 228,
    TEXTURE_DOWNTOWN_A = 229,
    TEXTURE_DOWNTOWN_B = 230,
    TEXTURE_HOUSES = 231,
    TEXTURE_FIELDS = 232,
    TEXTURE_FARM = 233,
    TEXTURE_VILLAGE_GREEN_A = 248,
    TEXTURE_VILLAGE_GREEN_B = 249,
    TEXTURE_VILLAGE_SAND_A = 250,
    TEXTURE_VILLAGE_SAND_B = 251,
    // the image of a triangle without texture (MapVertex::lowerImageID, upperImageID)
    TEXTURE_NONE = 0xFF
};

// the terrain type of a vertex of RSArea: the ramp of its colors
enum TerrainType {
    TERRAIN_WATER = 0,
    TERRAIN_SAND = 1,
    TERRAIN_ROCK = 2,
    TERRAIN_GREEN = 3,
    TERRAIN_JUNGLE = 4
};

// the colors of the palette of the game (PALETTE.IFF)
enum PaletteIndex {
    PALETTE_GREEN_RAMP = 64,
    PALETTE_BROWN_RAMP = 96,
    PALETTE_JUNGLE_RAMP = 128,
    PALETTE_CLEAR = 255
};

// an overlay triangle of color c has the image c + OVERLAY_IMAGE_BASE, but
// the color PALETTE_CLEAR, which is its palette color
enum {
    OVERLAY_IMAGE_BASE = 255
};

// the properties of a face of RSEntity: its texture is the face (drawn
// solid, the depth written), else a decal over its color
enum FaceProperty {
    FACE_TEXTURE_SOLID = 6,
    FACE_TEXTURE_SOLID_ALT = 9
};

// a face of RSEntity drawn from both sides (Triangle::flags[2], Attr::props2)
enum {
    FACE_TWO_SIDED = 1
};

// the side of a block of LVL0 in vertices
enum {
    BLOCK_SIDE_LVL0 = 20
};

// a part of a mission (MISN_PART): no area, and where it appears
enum {
    AREA_NONE = 255
};
enum PartPlacement {
    PLACEMENT_IN_AREA = 0
};

} // namespace r4bridge
