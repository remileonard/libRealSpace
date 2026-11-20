//
//  R4World.h
//  libRealSpace
//
//  The worlds of libRealSpace for R4nkine: terrain, objects of the map and of the missions,
//  all in libRealSpace's axes (Y up), turned once at the root by realSpaceToR4().
//
#pragma once

#include "R4Data.h"
#include "R4Bridge.h"

#include "r4/assets/model.h"
#include "r4/scene/node.h"

#include <map>
#include <set>
#include <memory>
#include <vector>

class RSArea;
class RSEntity;
struct MapObject;
struct MISN_PART;
struct VGAPalette;

namespace r4bridge {

// The terrain of an area: a LOD node by block over the 3 levels of RSArea, seams included, as SCRenderer::renderBlock.
// Textured triangles carry their image over their colors (the GL_DECAL of the game); lodRanges are those of the LOD nodes.
// Baked light uses the colors of RSArea; dynamic light darkens the brightest tone of each ramp by the sun. Null without blocks.
struct TerrainLight {
    bool dynamic = false;
    const std::vector<PaletteRange> *ranges = nullptr; // the ramps of the palette
    Lighting lighting;
    // multiplies the heights for the normals only
    float relief = 1;
};
// the images of the textured triangles of an area, at all its levels
std::set<int> terrainImages(RSArea *area);

std::shared_ptr<r4::assets::Model> terrainModel(RSArea *area, const VGAPalette &palette, const std::vector<float> &lodRanges = {0, 40000, 100000, 1e30f}, const TerrainLight &light = {});

// The runways of an area, as SCRenderer::renderMapOverlay: a mesh by overlay drawn without depth after the terrain, in drawList order. Null without overlay.
std::shared_ptr<r4::assets::Model> overlayModel(RSArea *area, const VGAPalette &palette);

// NOISE: the green terrain types under a noise of woods and clearings; TEXTURES: the trees painted in the textures
enum class ForestMode {
    NOISE,
    TEXTURES
};

struct ForestSettings {
    ForestMode mode = ForestMode::NOISE;
    float density = 1; // multiplies all the densities
    // NOISE: perKm2 x 25 at the heart of the woods, x 0.1 between them
    float perKm2 = 30;
    float treeScale = 1.4f; // the size of the trees
    // the jungle (type 4) is most of its map: its woods larger, less dense
    float jungleDensity = 0.4f;
    // NOISE: per km2 under these textures
    std::map<int, float> images{{TEXTURE_WOOD, 250}, {TEXTURE_MEADOW_WOOD, 120}, {TEXTURE_JUNGLE_A, 300}, {TEXTURE_JUNGLE_B, 300}};
    // NOISE: no tree under these
    std::set<int> bare{TEXTURE_HARBOR_B, TEXTURE_HARBOR_C, TEXTURE_DOWNTOWN_A, TEXTURE_HOUSES};
    // TEXTURES: a tree per areaPerTree on the tree texels of the textures with minTreeShare at least, but notWoods
    const VGAPalette *palette = nullptr; // to read the textures
    std::set<int> notWoods{TEXTURE_FIELDS, TEXTURE_FARM};
    float minTreeShare = 0.08f;
    float areaPerTree = 400;

    std::shared_ptr<std::map<int, std::vector<uint32_t>>> treeTexels = std::make_shared<std::map<int, std::vector<uint32_t>>>();
    // small, so that the chunks out of the view are culled whole
    float chunk = 2000;
    std::vector<r4::Vec3> keepOut; // x, z, radius (libRealSpace)
    uint32_t seed = 1;
};
// a conifer and a broadleaf, in the green of the meadows then of the jungle
enum TreeSpecies {
    TREE_CONIFER = 0,
    TREE_BROADLEAF = 1,
    TREE_JUNGLE_CONIFER = 2,
    TREE_JUNGLE_BROADLEAF = 3,
    TREE_SPECIES_COUNT = 4
};
inline bool isConifer(int species) {
    return species == TREE_CONIFER || species == TREE_JUNGLE_CONIFER;
}
inline bool isJungleTree(int species) {
    return species == TREE_JUNGLE_CONIFER || species == TREE_JUNGLE_BROADLEAF;
}

// The forests of an area by chunks with some green, the same at each load (seed), none near the runways and keepOut; plantForest fills the trees of one.
struct ForestChunk {
    int cx = 0; // its place in the grid of chunks
    int cz = 0;
    r4::Vec3 center; // bounds, the trees included
    float radius = 0;
    std::vector<r4::scene::Placement> trees[TREE_SPECIES_COUNT]; // by species, about Y (libRealSpace's up)
};
std::vector<ForestChunk> forestChunks(RSArea *area, const ForestSettings &settings = {});
void plantForest(RSArea *area, const ForestSettings &settings, ForestChunk &chunk);

// A low poly tree in the palette, about 25 high, its foot at 0; simple: the version for the distance.
std::shared_ptr<r4::assets::Model> treeModel(int species, bool simple, const VGAPalette &palette, const std::vector<PaletteRange> &ranges, Lighting lighting = {});

// The buildings of the cells whose TXMPACK texture shows a city, village, farm or harbor: boxes whose roof is that texture.
// The rectangles are read by hand (kBuildings), the houses found from their roofs; none within keepOut (x, z, radius). Null without any.
std::shared_ptr<r4::assets::Model> cityModel(RSArea *area, const VGAPalette &palette, Lighting lighting = {}, const std::vector<r4::Vec3> &keepOut = {}, size_t *buildings = nullptr);

// as SCRenderer::renderObjects
r4::Mat4 mapObjectMatrix(const MapObject &object);

// as SCRenderer::renderMissionObjects
r4::Mat4 missionPartMatrix(const MISN_PART &part);

// the height of the ground of the area under a point, on the LVL0 triangles as terrainModel draws them; false out of the map
bool groundHeight(RSArea *area, float x, float z, float &height);

// the matrix moved so that the lowest vertex of the entity, turned by it, is on the ground of the area at its origin; a flying object is only raised
r4::Mat4 onGround(RSArea *area, const RSEntity *entity, const r4::Mat4 &matrix, bool flies);

} // namespace r4bridge
