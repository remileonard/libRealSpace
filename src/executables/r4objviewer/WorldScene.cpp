//
//  WorldScene.cpp - see WorldScene.h.
//
#include "WorldScene.h"

#include "realspace/AssetManager.h"
#include "realspace/RSArea.h"
#include "realspace/RSEntity.h"
#include "realspace/RSMission.h"
#include "realspace/RSPalette.h"
#include "realspace/RSWorld.h"
#include "realspace/TreArchive.h"

#include "../../r4bridge/R4World.h"

#include <SDL.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>

namespace {

std::string upper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
}

// the terrain file in the data folder, whatever the case of its name
std::string findTerrain(const std::string &dataDir, const std::string &tera) {
    for (const std::string &name : {upper(tera) + ".PAK", tera + ".PAK", tera + ".pak"}) {
        SDL_RWops *f = SDL_RWFromFile((dataDir + name).c_str(), "rb");
        if (f) {
            SDL_RWclose(f);
            return name;
        }
    }
    return "";
}

// the world of the missions folder: its terrain and its light
std::unique_ptr<RSWorld> loadWorld(AssetManager &assets, const std::string &world) {
    TreEntry *e = assets.GetEntryByName(upper(assets.mission_root_path + world + ".IFF"));
    if (!e) {
        return nullptr;
    }
    auto w = std::make_unique<RSWorld>();
    w->InitFromRAM(e->data, e->size);
    return w;
}

} // namespace

std::vector<WorldEntry> listWorlds(AssetManager &assets, const std::string &dataDir, size_t own) {
    std::vector<WorldEntry> missions;
    std::vector<WorldEntry> worlds;
    for (size_t t = 0; t < assets.tres.size() && t < own; t++) {
        for (TreEntry *e : assets.tres[t]->entries) {
            std::string file = e->name;
            std::string name = upper(file);
            size_t at = name.find("\\MISSIONS\\");
            if (at == std::string::npos || name.size() < 4 || name.compare(name.size() - 4, 4, ".IFF") != 0) {
                continue;
            }
            std::string label = name.substr(at + 10, name.size() - at - 14);
            RSMission mission;
            mission.InitFromRAM(e->data, e->size);
            WorldEntry entry;
            entry.file = file;
            if (!mission.mission_data.world_filename.empty()) {
                entry.mission = true;
                entry.world = mission.mission_data.world_filename;
                entry.label = label;
            } else {
                RSWorld world;
                world.InitFromRAM(e->data, e->size);
                if (world.tera.empty()) {
                    continue;
                }
                entry.world = label;
                entry.label = label + " (world)";
            }
            std::unique_ptr<RSWorld> world = loadWorld(assets, entry.world);
            if (!world || world->tera.empty()) {
                continue;
            }
            entry.terrain = findTerrain(dataDir, world->tera);
            if (entry.terrain.empty()) {
                continue;
            }
            (entry.mission ? missions : worlds).push_back(entry);
        }
    }
    auto byLabel = [](const WorldEntry &a, const WorldEntry &b) {
        return a.label < b.label;
    };
    std::sort(missions.begin(), missions.end(), byLabel);
    std::sort(worlds.begin(), worlds.end(), byLabel);
    missions.insert(missions.end(), worlds.begin(), worlds.end());
    return missions;
}

WorldScene::WorldScene(AssetManager &assets, RSPalette &palette, r4::render::Renderer &renderer) : assets_(assets), palette_(palette), renderer_(renderer) {
    root_ = r4::scene::make<r4::scene::Group>();
    root_->setName("world (libRealSpace axes)");
    root_->setMatrix(r4bridge::realSpaceToR4());
    terrain_ = r4::scene::make<r4::scene::Switch>();
    terrain_->setName("terrain");
    mapLayer_ = r4::scene::make<r4::scene::Switch>();
    mapLayer_->setName("objects of the map");
    partsLayer_ = r4::scene::make<r4::scene::Switch>();
    partsLayer_->setName("parts of the mission");
    overlays_ = r4::scene::make<r4::scene::Switch>();
    overlays_->setName("overlays (runways)");
    forests_ = r4::scene::make<r4::scene::Switch>();
    forests_->setName("forests");
    forests_->setMask(kForestMask);
    buildings_ = r4::scene::make<r4::scene::Switch>();
    buildings_->setName("buildings");
    root_->addChild(terrain_);
    root_->addChild(overlays_);
    root_->addChild(forests_);
    root_->addChild(buildings_);
    root_->addChild(mapLayer_);
    root_->addChild(partsLayer_);
}

WorldScene::~WorldScene() {
    clear();
}

void WorldScene::clear() {
    terrain_->removeAllChildren();
    overlays_->removeAllChildren();
    forests_->removeAllChildren();
    forestNodes_.clear();
    forestTrees = forestChunks = 0;
    buildings_->removeAllChildren();
    buildings = 0;
    mapLayer_->removeAllChildren();
    partsLayer_->removeAllChildren();
    sceneLayers_.clear();
    scenes_.clear();
    partsWithoutScene = 0;
    areas_.clear();
    overlayTriangles = 0;
    objects_.clear();
    parts_.clear();
    mapObjects_.clear();
    placed_.clear();
    mission_.reset();
    area_.reset();
    world_.reset();
    entities_.clear();
}

r4::scene::NodePtr WorldScene::object(const std::string &name, float *radius) {
    auto it = objects_.find(name);
    if (it == objects_.end()) {
        r4::scene::NodePtr node;
        TreEntry *e = assets_.GetEntryByName(upper(assets_.object_root_path + name + ".IFF"));
        if (e) {
            auto entity = std::make_unique<RSEntity>();
            entity->InitFromRAM(e->data, e->size, name);
            auto model = r4bridge::toModel(entity.get(), *palette_.GetColorPalette(), palette_.ranges, lighting, r4bridge::Axes::REAL_SPACE);
            if (model) {
                node = renderer_.upload(*model);
                node->setName(name);
            }
            entities_[name] = std::move(entity);
        }
        it = objects_.emplace(name, node).first;
    }
    if (radius) {
        *radius = it->second ? it->second->bounds().radius : 0;
    }
    return it->second;
}

bool WorldScene::load(const WorldEntry &entry, std::string &error) {
    Uint64 t0 = SDL_GetPerformanceCounter();
    clear();
    TreEntry *e = assets_.GetEntryByName(entry.file);
    if (!e) {
        error = "not in the TRE: " + entry.file;
        return false;
    }
    if (entry.mission) {
        mission_ = std::make_unique<RSMission>();
        mission_->InitFromRAM(e->data, e->size);
    }
    world_ = loadWorld(assets_, entry.world);
    if (!world_) {
        error = "no world " + entry.world;
        return false;
    }
    worldName = entry.world;
    terrainName = entry.terrain;
    hasLight = world_->has_light;
    lighting = {};
    if (hasLight) {
        float total = (float)(world_->light_ambient_weight + world_->light_weight);
        if (total > 0) {
            lighting.ambient = world_->light_ambient_weight / total;
            lighting.light = world_->light_weight / total;
        }
        r4::Vec3 d{(float)world_->light_direction[0], (float)world_->light_direction[1], (float)world_->light_direction[2]};
        if (r4::length(d) > 0) {
            lightTowards = r4::normalize(d);
        }
    }

    if (!assets_.GetEntryByName(assets_.txm_filename) || !assets_.GetEntryByName(assets_.acc_filename)) {
        error = "no " + assets_.txm_filename + " or " + assets_.acc_filename + " in the TRE (TEXTURES.TRE)";
        return false;
    }
    area_ = std::make_unique<RSArea>();
    area_->InitFromPAKFileName(entry.terrain.c_str());
    buildTerrain();
    if (auto overlays = r4bridge::overlayModel(area_.get(), *palette_.GetColorPalette())) {
        for (const r4::assets::Mesh &m : overlays->meshes) {
            overlayTriangles += m.indices.size() / 3;
        }
        overlays_->addChild(renderer_.upload(*overlays));
    }

    for (MapObject &o : area_->objects) {
        float radius = 0;
        std::string name(o.name, strnlen(o.name, sizeof o.name));
        r4::scene::NodePtr node = object(name, &radius);
        o.entity = entities_.count(name) ? entities_[name].get() : nullptr;
        Placed placed{nullptr, o.entity, r4bridge::mapObjectMatrix(o), false};
        r4::Mat4 m = placedMatrix(placed);
        if (node) {
            placed.node = r4::scene::make<r4::scene::Group>();
            placed.node->setName(name);
            placed.node->setMatrix(m);
            placed.node->addChild(node);
            mapLayer_->addChild(placed.node);
            placed_.push_back(placed);
        }
        mapObjects_.push_back({name, r4::transformPoint(r4bridge::realSpaceToR4(), r4::translationOf(m)), radius, node != nullptr});
    }

    start = {0, 0, 0};
    if (mission_) {
        placeParts();
    }
    std::vector<r4::Vec3> keepOut;
    for (const auto *list : {&parts_, &mapObjects_}) {
        for (const Item &it : *list) {
            keepOut.push_back({it.position.x, -it.position.y, std::max(it.radius * 1.2f, 40.0f)});
        }
    }
    if (auto city = r4bridge::cityModel(area_.get(), *palette_.GetColorPalette(), lighting, keepOut, &buildings)) {
        buildings_->addChild(renderer_.upload(*city));
    }
    buildForests();
    loadMs = (SDL_GetPerformanceCounter() - t0) * 1000.0 / SDL_GetPerformanceFrequency();
    return true;
}

// as SCMission::loadMission and SCMissionScene::spawnCast: the player in its area; the others in the area of the first scene whose cast holds them,
// placement 1 as SCMission::placeRelativeToPlayer with the center of that area, heading north, in place of the player; then on the ground when under it;
// the parts of no scene in a hidden layer of their own
void WorldScene::placeParts() {
    MISN &data = mission_->mission_data;
    MISN_PART *player = mission_->getPlayerCoord();
    auto areaOf = [&](int id) -> AREA * {
        return id >= 0 && id < (int)data.areas.size() ? data.areas[id] : nullptr;
    };
    std::vector<int> sceneOf(data.parts.size(), -1);
    for (size_t s = 0; s < data.scenes.size(); s++) {
        for (uint16_t cast : data.scenes[s]->cast) {
            if (cast < data.parts.size() && sceneOf[cast] < 0) {
                sceneOf[cast] = (int)s;
            }
        }
    }

    auto isAircraft = [&](MISN_PART *part) {
        auto it = entities_.find(part->member_name);
        return it != entities_.end() && it->second->entity_type == EntityType::jet;
    };
    auto onGround = [&](MISN_PART *part) {
        float ground = area_->getY(part->position.x, part->position.z);
        if (isAircraft(part) ? std::abs(ground - part->position.y) <= 10 || part->position.y < ground : part->position.y < ground) {
            part->position.y = ground;
        }
    };
    for (size_t i = 0; i < data.parts.size(); i++) {
        MISN_PART *part = data.parts[i];
        object(part->member_name, nullptr);
        if (part == player) {
            if (AREA *a = areaOf(part->area_id); a && part->area_id != r4bridge::AREA_NONE && part->placement == r4bridge::PLACEMENT_IN_AREA) {
                part->position += a->position;
            }
            continue;
        }
        if (part->area_id == r4bridge::AREA_NONE && part->placement == r4bridge::PLACEMENT_IN_AREA) {
            continue;
        }
        if (part->placement != r4bridge::PLACEMENT_IN_AREA) {
            int s = sceneOf[i];
            AREA *a = s >= 0 ? areaOf(data.scenes[s]->area_id) : areaOf(part->area_id);
            if (a) {
                part->position = a->position + part->relative_position;
                part->azymuth = part->relative_azymuth % 360;
            }
        } else if (AREA *a = areaOf(part->area_id)) {
            part->position += a->position;
        }
        onGround(part);
    }

    for (size_t a = 0; a < data.areas.size(); a++) {
        AREA *ar = data.areas[a];
        r4::Vec3 at = r4::transformPoint(r4bridge::realSpaceToR4(), {ar->position.x, ar->position.y, ar->position.z});
        float radius = std::max(1000.0f, 0.5f * (float)std::max(ar->AreaWidth, ar->AreaHeight));
        areas_.push_back({"Area " + std::to_string(a) + ": " + ar->AreaName, at, radius, true});
    }
    std::map<int, std::shared_ptr<r4::scene::Group>> areaNodes;
    auto areaNode = [&](int id) {
        auto it = areaNodes.find(id);
        if (it != areaNodes.end()) {
            return it->second;
        }
        auto g = r4::scene::make<r4::scene::Group>();
        AREA *a = areaOf(id);
        g->setName(a ? "Area " + std::to_string(id) + ": " + a->AreaName : std::string("No area"));
        partsLayer_->addChild(g);
        return areaNodes[id] = g;
    };
    for (size_t s = 0; s < data.scenes.size(); s++) {
        int id = data.scenes[s]->area_id;
        AREA *a = areaOf(id);
        Scene sc;
        sc.label = (a ? std::string(a->AreaName) : std::string("No area")) + " / Scene " + std::to_string(s);
        scenes_.push_back(sc);
        auto layer = r4::scene::make<r4::scene::Switch>();
        layer->setName("Scene " + std::to_string(s));
        areaNode(a ? id : -1)->addChild(layer);
        sceneLayers_.push_back(layer);
    }
    {
        auto layer = r4::scene::make<r4::scene::Switch>();
        layer->setName("no scene");
        layer->setValue(r4::scene::Switch::kNone);
        partsLayer_->addChild(layer);
        sceneLayers_.push_back(layer);
    }
    for (size_t i = 0; i < data.parts.size(); i++) {
        MISN_PART *part = data.parts[i];
        float radius = 0;
        r4::scene::NodePtr node = object(part->member_name, &radius);
        part->entity = entities_.count(part->member_name) ? entities_[part->member_name].get() : nullptr;
        int s = sceneOf[i];
        if (s < 0) {
            partsWithoutScene++;
        } else {
            scenes_[s].parts++;
        }
        const bool flies = isAircraft(part) && part->position.y > area_->getY(part->position.x, part->position.z) + 10;
        Placed placed{nullptr, part->entity, r4bridge::missionPartMatrix(*part), flies};
        r4::Mat4 m = placedMatrix(placed);
        if (node) {
            placed.node = r4::scene::make<r4::scene::Group>();
            placed.node->setName(part->member_name);
            placed.node->setMatrix(m);
            placed.node->addChild(node);
            sceneLayers_[s < 0 ? data.scenes.size() : (size_t)s]->addChild(placed.node);
            placed_.push_back(placed);
        }
        r4::Vec3 at = r4::transformPoint(r4bridge::realSpaceToR4(), r4::translationOf(m));
        std::string label = part->member_name + (part == player ? " (player)" : "");
        parts_.push_back({label, at, radius, node != nullptr, s});
        if (part == player) {
            start = at;
        }
    }
}

r4::Mat4 WorldScene::placedMatrix(const Placed &p) const {
    return onGround_ ? r4bridge::onGround(area_.get(), p.entity, p.matrix, p.flies) : p.matrix;
}

void WorldScene::setOnGround(bool on) {
    onGround_ = on;
    for (const Placed &p : placed_) {
        p.node->setMatrix(placedMatrix(p));
    }
}

void WorldScene::showOverlays(bool on) {
    overlays_->setValue(on ? r4::scene::Switch::kAll : r4::scene::Switch::kNone);
}
void WorldScene::showScene(int scene, bool on) {
    size_t i = scene < 0 ? sceneLayers_.size() - 1 : (size_t)scene;
    if (i < sceneLayers_.size()) {
        sceneLayers_[i]->setValue(on ? r4::scene::Switch::kAll : r4::scene::Switch::kNone);
    }
}

void WorldScene::buildTerrain() {
    terrain_->removeAllChildren();
    terrainLods_.clear();
    terrainQuads_.clear();
    terrainTriangles = terrainTextures = 0;
    terrainBlocks = 0;
    if (!area_) {
        return;
    }
    r4bridge::TerrainLight light;
    light.dynamic = terrainDynamic_;
    light.ranges = &palette_.ranges;
    light.lighting = lighting;
    light.relief = terrainRelief_;
    auto model = r4bridge::terrainModel(area_.get(), *palette_.GetColorPalette(), kTerrainRanges, light);
    if (!model) {
        return;
    }
    for (const r4::assets::Node &n : model->nodes) {
        if (n.name.compare(0, 4, "LVL0") == 0) {
            for (int mi : n.meshes) {
                if (model->meshes[mi].material == 0) {
                    terrainTriangles += model->meshes[mi].indices.size() / 3;
                }
            }
        }
    }
    terrainTextures = r4bridge::terrainImages(area_.get()).size();
    r4::scene::NodePtr node = renderer_.upload(*model);
    terrain_->addChild(node);
    std::vector<r4::scene::Node *> stack{node.get()};
    while (!stack.empty()) {
        r4::scene::Node *n = stack.back();
        stack.pop_back();
        if (n->type() == r4::scene::NodeType::LOD) {
            auto *lod = static_cast<r4::scene::LOD *>(n);
            if (lod->children().size() == 2) {
                terrainQuads_.push_back({lod, lod->ranges()});
            } else {
                terrainLods_.push_back(lod);
            }
        }
        for (const auto &c : n->children()) {
            stack.push_back(c.get());
        }
    }
    terrainBlocks = terrainLods_.size();
    setTerrainLod(terrainLod_);
}

void WorldScene::buildForests() {
    forests_->removeAllChildren();
    forestNodes_.clear();
    forestTrees = forestChunks = 0;
    if (!area_ || forestDensity_ <= 0) {
        return;
    }
    forestSettings_ = {};
    forestSettings_.density = forestDensity_;
    forestSettings_.mode = forestMode_;
    forestSettings_.palette = palette_.GetColorPalette();
    for (const auto *list : {&parts_, &mapObjects_}) {
        for (const Item &it : *list) {
            forestSettings_.keepOut.push_back({it.position.x, -it.position.y, std::max(it.radius * 2, 150.0f)});
        }
    }
    std::vector<r4bridge::ForestChunk> chunks = r4bridge::forestChunks(area_.get(), forestSettings_);
    if (chunks.empty()) {
        return;
    }
    std::vector<r4::scene::InstancePart> parts[r4bridge::TREE_SPECIES_COUNT][2];
    for (int sp = 0; sp < r4bridge::TREE_SPECIES_COUNT; sp++) {
        for (int level = 0; level < 2; level++) {
            auto model = r4bridge::treeModel(sp, level != 0, *palette_.GetColorPalette(), palette_.ranges, lighting);
            parts[sp][level] = r4::scene::collectParts(*renderer_.upload(*model));
        }
    }
    forestNodes_.resize(chunks.size());
    std::map<std::pair<int, int>, std::shared_ptr<r4::scene::Group>> blocks;
    for (size_t i = 0; i < chunks.size(); i++) {
        ForestNode &f = forestNodes_[i];
        f.chunk = std::move(chunks[i]);
        auto lod = r4::scene::make<r4::scene::LOD>();
        lod->setName("trees " + std::to_string(f.chunk.cx) + "," + std::to_string(f.chunk.cz));
        lod->setCenter(f.chunk.center);
        lod->setRanges({0, kTreeFar + f.chunk.radius});
        auto group = r4::scene::make<r4::scene::Group>();
        for (int sp = 0; sp < r4bridge::TREE_SPECIES_COUNT; sp++) {
            f.trees[sp] = r4::scene::make<r4::scene::Instancer>();
            f.trees[sp]->setName(std::string(r4bridge::isConifer(sp) ? "conifers" : "broadleaves") + (r4bridge::isJungleTree(sp) ? " (jungle)" : ""));
            f.trees[sp]->setUp({0, 1, 0});
            f.trees[sp]->addLevel(parts[sp][0], kTreeNear);
            f.trees[sp]->addLevel(parts[sp][1], kTreeFar);
            group->addChild(f.trees[sp]);
        }
        r4bridge::plantForest(area_.get(), forestSettings_, f.chunk);
        for (int sp = 0; sp < r4bridge::TREE_SPECIES_COUNT; sp++) {
            forestTrees += f.chunk.trees[sp].size();
            f.trees[sp]->setPlacements(std::move(f.chunk.trees[sp]));
            f.chunk.trees[sp].clear();
        }
        lod->addChild(group);
        int bx = (int)(f.chunk.cx * forestSettings_.chunk / BLOCK_WIDTH);
        int bz = (int)(f.chunk.cz * forestSettings_.chunk / BLOCK_WIDTH);
        auto &block = blocks[{bx, bz}];
        if (!block) {
            block = r4::scene::make<r4::scene::Group>();
            block->setName("trees of block " + std::to_string(bx) + "," + std::to_string(bz));
            forests_->addChild(block);
        }
        block->addChild(lod);
    }
    forestChunks = forestNodes_.size();
}

void WorldScene::setForestMode(r4bridge::ForestMode mode) {
    forestMode_ = mode;
    if (area_) {
        buildForests();
    }
}

void WorldScene::setForestDensity(float density) {
    forestDensity_ = density;
    if (area_) {
        buildForests();
    }
}

void WorldScene::showBuildings(bool on) {
    buildings_->setValue(on ? r4::scene::Switch::kAll : r4::scene::Switch::kNone);
}

void WorldScene::showForests(bool on) {
    forests_->setValue(on ? r4::scene::Switch::kAll : r4::scene::Switch::kNone);
}

void WorldScene::setTerrainLight(bool dynamic, float relief) {
    terrainDynamic_ = dynamic;
    terrainRelief_ = relief;
    if (area_) {
        buildTerrain();
    }
}

r4::Sphere WorldScene::mapBounds() const {
    return {{0, 0, 0}, BLOCK_WIDTH * BLOCK_PER_MAP_SIDE * 0.5f * std::sqrt(2.0f)};
}

bool WorldScene::groundHeight(r4::Vec3 p, float &height) const {
    if (!area_) {
        return false;
    }
    const float half = BLOCK_WIDTH * BLOCK_PER_MAP_SIDE_DIV_2 - 1;
    if (std::abs(p.x) >= half || std::abs(p.y) >= half) {
        return false;
    }
    height = area_->getY(p.x, -p.y);
    return true;
}

void WorldScene::setTerrainLod(int mode) {
    terrainLod_ = mode;
    std::vector<float> ranges = kTerrainRanges;
    if (mode > 0) {
        int level = mode - 1;
        for (int i = 0; i <= 3; i++) {
            ranges[i] = i <= level ? 0.0f : 1e30f;
        }
    }
    for (r4::scene::LOD *lod : terrainLods_) {
        lod->setRanges(ranges);
    }
    for (auto &[lod, byDistance] : terrainQuads_) {
        if (mode == 0) {
            lod->setRanges(byDistance);
        } else if (mode == NUM_LODS) {
            lod->setRanges({0, 0, 1e30f});
        } else {
            lod->setRanges({0, 1e30f, 1e30f});
        }
    }
}

void WorldScene::showTerrain(bool on) {
    terrain_->setValue(on ? r4::scene::Switch::kAll : r4::scene::Switch::kNone);
}
void WorldScene::showMapObjects(bool on) {
    mapLayer_->setValue(on ? r4::scene::Switch::kAll : r4::scene::Switch::kNone);
}
void WorldScene::showParts(bool on) {
    partsLayer_->setValue(on ? r4::scene::Switch::kAll : r4::scene::Switch::kNone);
}
