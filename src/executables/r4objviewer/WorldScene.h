//
//  WorldScene.h - a mission, or a world alone, loaded by libRealSpace and drawn by R4nkine.
//
#pragma once

#include "../../r4bridge/R4Bridge.h"
#include "../../r4bridge/R4World.h"

#include "r4/render/renderer.h"
#include "r4/scene/node.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

class AssetManager;
class RSArea;
class RSEntity;
class RSMission;
class RSPalette;
class RSWorld;

// an entry of the list: a mission, or a world without a mission
struct WorldEntry {
    std::string label;   // MISN-A1, or TURKEY (world)
    std::string file;    // ..\..\DATA\MISSIONS\MISN-A1.IFF
    std::string world;   // SYRIA22
    std::string terrain; // SYRIA.PAK
    bool mission = false;
};

// in the first own TRE files: those of the game, the next ones only complete it
std::vector<WorldEntry> listWorlds(AssetManager &assets, const std::string &dataDir, size_t own = 1000);

class WorldScene {
public:
    WorldScene(AssetManager &assets, RSPalette &palette, r4::render::Renderer &renderer);
    ~WorldScene();

    bool load(const WorldEntry &entry, std::string &error);
    // 0 by distance (the LOD nodes), 1..3 a fixed level everywhere (the world_detail of the game)
    void setTerrainLod(int mode);
    // baked (the colors of RSArea) or dynamic (the sun on normals from the relief)
    void setTerrainLight(bool dynamic, float relief = 4);
    float terrainRelief() const {
        return terrainRelief_;
    }
    bool terrainLightDynamic() const {
        return terrainDynamic_;
    }
    // R4nkine's axes; false out of the map
    bool groundHeight(r4::Vec3 p, float &height) const;
    // the map, without the skirts that run to the edge of the world
    r4::Sphere mapBounds() const;

    r4::scene::NodePtr root() const {
        return root_;
    }
    RSArea *area() const {
        return area_.get();
    }
    void showTerrain(bool on);
    void showMapObjects(bool on);
    void showParts(bool on);
    void showOverlays(bool on);
    // the mask of the forests: they cast no shadow when RenderOptions::shadowMask leaves this bit out
    static constexpr uint32_t kForestMask = 1u << 31;
    // the map objects and mission parts set on the terrain, the flying aircraft only raised over it (r4bridge::onGround)
    void setOnGround(bool on);
    bool onGround() const {
        return onGround_;
    }
    void showForests(bool on);
    void showBuildings(bool on);
    size_t buildings = 0; // in the cities
    // 0: none
    void setForestDensity(float density);
    void setForestMode(r4bridge::ForestMode mode);
    r4bridge::ForestMode forestMode() const {
        return forestMode_;
    }
    float forestDensity() const {
        return forestDensity_;
    }
    size_t forestTrees = 0;
    size_t forestChunks = 0;
    // the detailed trees up to the first distance, the simple ones up to the second
    const float kTreeNear = 1500;
    const float kTreeFar = 6000;
    // scene -1: the parts of no scene
    void showScene(int scene, bool on);

    struct Item {
        std::string name;
        r4::Vec3 position; // R4nkine's axes
        float radius;
        bool found;     // its object is in the TRE
        int scene = -2; // the scene that makes it appear; -1: none, -2: not a part
    };
    struct Scene {
        std::string label;
        size_t parts = 0;
    };
    const std::vector<Scene> &scenes() const {
        return scenes_;
    }
    size_t partsWithoutScene = 0;
    const std::vector<Item> &parts() const {
        return parts_;
    }
    const std::vector<Item> &mapObjects() const {
        return mapObjects_;
    }
    // entering one starts its scenes
    const std::vector<Item> &areas() const {
        return areas_;
    }
    std::string worldName;
    std::string terrainName;
    size_t terrainTriangles = 0;
    size_t terrainTextures = 0;
    size_t overlayTriangles = 0;
    size_t terrainBlocks = 0;
    const std::vector<float> kTerrainRanges{0, 40000, 100000, 1e30f};
    double loadMs = 0;
    bool hasLight = false;
    r4::Vec3 lightTowards{0, 0, 1}; // R4nkine's axes, towards the light
    r4bridge::Lighting lighting;
    r4::Vec3 start{0, 0, 0}; // where to look first: the player, or the middle

private:
    // loaded once and shared; null when it is not in the TRE
    r4::scene::NodePtr object(const std::string &name, float *radius);
    void clear();
    // as the game makes them appear
    void placeParts();
    void buildTerrain();
    void buildForests();

    AssetManager &assets_;
    RSPalette &palette_;
    r4::render::Renderer &renderer_;
    std::unique_ptr<RSMission> mission_;
    std::unique_ptr<RSWorld> world_;
    std::unique_ptr<RSArea> area_;
    struct Placed {
        std::shared_ptr<r4::scene::Group> node;
        const RSEntity *entity;
        r4::Mat4 matrix;
        bool flies;
    };
    std::vector<Placed> placed_;
    bool onGround_ = true;
    r4::Mat4 placedMatrix(const Placed &p) const;
    std::map<std::string, std::unique_ptr<RSEntity>> entities_;
    std::map<std::string, r4::scene::NodePtr> objects_;
    std::shared_ptr<r4::scene::Group> root_;
    std::shared_ptr<r4::scene::Switch> terrain_;
    std::shared_ptr<r4::scene::Switch> overlays_;
    std::shared_ptr<r4::scene::Switch> mapLayer_;
    std::shared_ptr<r4::scene::Switch> partsLayer_;
    std::shared_ptr<r4::scene::Switch> forests_;
    std::shared_ptr<r4::scene::Switch> buildings_;
    std::vector<std::shared_ptr<r4::scene::Switch>> sceneLayers_; // the scenes, then "no scene"
    std::vector<Scene> scenes_;
    std::vector<Item> parts_;
    std::vector<Item> mapObjects_;
    std::vector<Item> areas_;
    bool terrainDynamic_ = false;
    float terrainRelief_ = 4;
    float forestDensity_ = 1;
    r4bridge::ForestMode forestMode_ = r4bridge::ForestMode::NOISE;
    int terrainLod_ = 0;
    std::vector<r4::scene::LOD *> terrainLods_;
    // the quadtree over the blocks: its LODs and their ranges by distance
    std::vector<std::pair<r4::scene::LOD *, std::vector<float>>> terrainQuads_;
    struct ForestNode {
        r4bridge::ForestChunk chunk;
        std::shared_ptr<r4::scene::Instancer> trees[r4bridge::TREE_SPECIES_COUNT];
    };
    std::vector<ForestNode> forestNodes_;
    r4bridge::ForestSettings forestSettings_;
};
