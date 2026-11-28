//
//  R4Renderer.cpp
//  libRealSpace
//
#include "R4Renderer.h"
#include "R4Screen.h"

#include "R4Bridge.h"
#include "R4World.h"

#include "realspace/RSArea.h"
#include "realspace/RSEntity.h"
#include "realspace/RSMission.h"
#include "engine/Config.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>

namespace {

constexpr float kNear = 1.5f;
constexpr int kTargetWidth = 107;
constexpr int kTargetHeight = 75;
constexpr int kSkyRings = 12;
constexpr int kSkySlices = 48;
// the dome on the far plane, after the opaque items: only the pixels that nothing covers take its colors
constexpr float kSkyRadius = 0.98f;
constexpr int kSkyOrder = 1000;
// the far plane just after the end of the fog, as in r4objviewer
constexpr float kFarAfterFog = 1.02f;
// the ellipsoid of a puff, as SCRenderer::renderClouds
constexpr int kCloudRings = 6;
constexpr int kCloudSlices = 12;
constexpr float kPointPixels = 2.5f;
const float kFogColor[3] = {0.89f, 0.89f, 0.98f};

r4::Mat4 toMat4(Matrix &m) {
    return glm::make_mat4(&m.v[0][0]);
}

// glRotatef in degrees about Y, Z then X, as the drawModel of SCRenderer
r4::Mat4 degreeRotation(Vector3D orientation) {
    r4::Mat4 m = r4::rotate(r4::radians(orientation.x), {0, 1, 0});
    m = m * r4::rotate(r4::radians(orientation.y), {0, 0, 1});
    m = m * r4::rotate(r4::radians(orientation.z), {1, 0, 0});
    return m;
}

// Matrix::rotateM in radians about Y, Z then X, as drawModelWithChilds
r4::Mat4 radianPlacement(Vector3D position, Vector3D orientation) {
    Matrix m;
    m.Clear();
    m.Identity();
    m.translateM(position.x, position.y, position.z);
    m.rotateM(orientation.x, 0.0f, 1.0f, 0.0f);
    m.rotateM(orientation.y, 0.0f, 0.0f, 1.0f);
    m.rotateM(orientation.z, 1.0f, 0.0f, 0.0f);
    return toMat4(m);
}

r4::Vec3 vec(Vector3D v) {
    return {v.x, v.y, v.z};
}

// a perspective projection of the game with another far plane; its near plane and its vertical offset stay
r4::Mat4 withFar(r4::Mat4 p, float zFar, float &zNear) {
    zNear = kNear;
    if (p[2][3] > -0.5f) {
        return p;
    }
    zNear = p[3][2] / (p[2][2] - 1.0f);
    p[2][2] = -(zFar + zNear) / (zFar - zNear);
    p[3][2] = -2.0f * zFar * zNear / (zFar - zNear);
    return p;
}

// the rectangle of the texels that are not clear, in texture coordinates (u0, v0, u1, v1); false when all are clear
bool visibleRect(const uint8_t *rgba, size_t width, size_t height, r4::Vec4 &rect) {
    size_t x0 = width;
    size_t y0 = height;
    size_t x1 = 0;
    size_t y1 = 0;
    for (size_t y = 0; y < height; y++) {
        const uint8_t *row = rgba + y * width * 4;
        for (size_t x = 0; x < width; x++) {
            if (row[x * 4 + 3] != 0) {
                x0 = std::min(x0, x);
                x1 = std::max(x1, x + 1);
                y0 = std::min(y0, y);
                y1 = std::max(y1, y + 1);
            }
        }
    }
    if (x0 >= x1) {
        return false;
    }
    rect = {(float)x0 / width, (float)y0 / height, (float)x1 / width, (float)y1 / height};
    return true;
}

uint32_t packColor(r4::Vec4 c) {
    auto byte = [](float f) {
        return (uint32_t)std::lround(std::clamp(f, 0.0f, 1.0f) * 255.0f);
    };
    return byte(c.x) | byte(c.y) << 8 | byte(c.z) << 16 | byte(c.w) << 24;
}

// the colors of SCRenderer::renderSkydome from the horizon (0) to the zenith (1), the horizon in the color of the fog
r4::Vec3 skyColor(float t, r4::Vec3 horizon) {
    const float keys[] = {0.00f, 0.08f, 0.25f, 0.60f, 1.00f};
    const r4::Vec3 colors[] = {horizon, {0.72f, 0.83f, 0.97f}, {0.40f, 0.62f, 0.92f}, {0.18f, 0.44f, 0.82f}, {0.08f, 0.28f, 0.72f}};
    if (t <= 0.0f) {
        return colors[0];
    }
    if (t >= 1.0f) {
        return colors[4];
    }
    for (int i = 0; i < 4; i++) {
        if (t >= keys[i] && t <= keys[i + 1]) {
            float f = (t - keys[i]) / (keys[i + 1] - keys[i]);
            return colors[i] + f * (colors[i + 1] - colors[i]);
        }
    }
    return colors[4];
}

} // namespace

bool R4Settings::has(const char *key) {
    return !Config::getInstance().getString("R4nkine", key, "").empty();
}

float R4Settings::number(const char *key, float fallback) {
    std::string v = Config::getInstance().getString("R4nkine", key, "");
    return v.empty() ? fallback : (float)std::atof(v.c_str());
}

bool R4Settings::flag(const char *key, bool fallback) {
    return Config::getInstance().getBool("R4nkine", key, fallback);
}

// r, g, b from 0 to 1, separated by commas
r4::Vec3 R4Settings::color(const char *key, r4::Vec3 fallback) {
    std::string v = Config::getInstance().getString("R4nkine", key, "");
    r4::Vec3 c = fallback;
    if (std::sscanf(v.c_str(), "%f , %f , %f", &c.x, &c.y, &c.z) != 3) {
        return fallback;
    }
    return c;
}

void R4Settings::load() {
    Config &config = Config::getInstance();
    shadows = config.getBool("R4nkine", "shadows", shadows);
    shadowMapSize = std::clamp(config.getInt("R4nkine", "shadow_map_size", shadowMapSize), 256, 8192);
    shadowDistance = std::max(100.0f, number("shadow_distance", shadowDistance));
    treeShadows = config.getBool("R4nkine", "tree_shadows", treeShadows);
    std::string mode = config.getString("R4nkine", "forests", "off");
    forests = mode == "noise" || mode == "textures";
    forestMode = mode == "textures" ? r4bridge::ForestMode::TEXTURES : r4bridge::ForestMode::NOISE;
    forestDensity = std::max(0.0f, number("forest_density", forestDensity));
    treeDetailKm = std::max(0.1f, number("tree_detail_km", treeDetailKm));
    treeKm = std::max(treeDetailKm, number("tree_km", treeKm));
    buildings = config.getBool("R4nkine", "buildings", buildings);
    const std::map<std::string, r4::assets::Upscale> upscales{{"none", r4::assets::Upscale::NONE}, {"2xsai", r4::assets::Upscale::SAI_2X}, {"xbr2x", r4::assets::Upscale::XBR_2X}, {"xbr4x", r4::assets::Upscale::XBR_4X}};
    auto it = upscales.find(config.getString("R4nkine", "texture_upscale", "none"));
    upscale = it != upscales.end() ? it->second : r4::assets::Upscale::NONE;
    mipmaps = config.getBool("R4nkine", "texture_mipmaps", mipmaps);
    anisotropy = std::clamp(number("anisotropy", anisotropy), 1.0f, 16.0f);
    sunTerrain = config.getBool("R4nkine", "sun_terrain", sunTerrain);
    terrainRelief = std::clamp(number("terrain_relief", terrainRelief), 1.0f, 16.0f);
    shadowStrength = std::clamp(number("shadow_strength", shadowStrength), 0.0f, 1.0f);
    sunOverride = !config.getString("R4nkine", "sun_heading", "").empty() || !config.getString("R4nkine", "sun_elevation", "").empty();
    sunHeading = number("sun_heading", sunHeading);
    sunElevation = std::clamp(number("sun_elevation", sunElevation), -90.0f, 90.0f);
    lightOverride = has("light_ambient") || has("light_direct");
    ambient = std::clamp(number("light_ambient", ambient), 0.0f, 1.0f);
    direct = std::clamp(number("light_direct", direct), 0.0f, 1.0f);
    fogOverride = has("fog_start") || has("fog_end");
    fogStartKm = std::max(0.0f, number("fog_start", fogStartKm));
    fogEndKm = std::max(fogStartKm + 1, number("fog_end", fogEndKm));
    wireframe = flag("wireframe", wireframe);
    flatShading = flag("flat_shading", flatShading);
    lodScale = std::clamp(number("lod_scale", lodScale), 0.05f, 4.0f);
    const std::map<std::string, TerrainLod> lods{{"distance", TerrainLod::BY_DISTANCE}, {"fine", TerrainLod::FINE}, {"medium", TerrainLod::MEDIUM}, {"coarse", TerrainLod::COARSE}};
    auto lod = lods.find(config.getString("R4nkine", "terrain_lod", "distance"));
    terrainLod = lod != lods.end() ? lod->second : TerrainLod::BY_DISTANCE;
    vsync = flag("vsync", vsync);
    sky = flag("sky", sky);
    skyColor = color("sky_color", skyColor);
}

// as the sun of r4objviewer: libRealSpace's -Z is the north, +X the east
Vector3D R4Settings::sunTowards(const Vector3D &game) const {
    if (!sunOverride) {
        return game;
    }
    const float h = r4::radians(sunHeading);
    const float e = r4::radians(sunElevation);
    return {std::sin(h) * std::cos(e), std::sin(e), -std::cos(h) * std::cos(e)};
}

bool R4Renderer::BatchKey::operator<(const BatchKey &o) const {
    auto tie = [](const BatchKey &k) {
        return std::make_tuple(k.order, k.texture, (int)k.blend, k.depthTest, k.depthWrite, k.alphaTest, (int)k.primitive);
    };
    return tie(*this) < tie(o);
}

R4Renderer::R4Renderer() = default;

R4Renderer::~R4Renderer() {
    release();
}

R4Renderer *R4Renderer::current() {
    if (!SCRenderer::hasInstance()) {
        return nullptr;
    }
    return dynamic_cast<R4Renderer *>(&SCRenderer::instance());
}

void R4Renderer::release() {
    items_.clear();
    scenes_.clear();
    batches_.clear();
    streams_.clear();
    materials_.clear();
    places_.clear();
    root_.reset();
    sky_.reset();
    skyPlace_.reset();
    models_.clear();
    terrain_.reset();
    overlay_.reset();
    forests_.reset();
    buildings_.reset();
    clouds_.reset();
    textures_.clear();
    effectTextures_.clear();
    area_ = nullptr;
    R4Screen *screen = R4Screen::current();
    if (renderer_ && screen) {
        screen->device().beginHostInterop();
        R4Screen::countInterop();
        screen->device().destroyRenderTarget(targetView_);
        renderer_.reset();
        screen->device().endHostInterop();
    }
    renderer_.reset();
}

void R4Renderer::init(int width, int height) {
    AssetManager &assets = AssetManager::instance();
    Config &config = Config::instance();
    this->max_view_distance = config.getInt("Game", "max_view_distance", 160000);
    this->overlay_offset = config.getInt("Video", "overlay_offset", 200);
    this->overlay_offset_distance = (std::max)(1, config.getInt("Video", "overlay_offset_distance", 250));
    this->counter = 0;
    RSPalette rsPalette;
    TreEntry *entries = (TreEntry *)assets.GetEntryByName("..\\..\\DATA\\PALETTE\\PALETTE.IFF");
    if (entries) {
        rsPalette.initFromFileRam(entries->data, entries->size);
        this->palette = *rsPalette.GetColorPalette();
        this->terrain_ranges = rsPalette.ranges;
    }
    this->setObjectLight(3, 5, -10, -25, 100);
    this->lodLevel = config.getInt("Game", "object_detail", 0);
    this->show_textured = config.getBool("Game", "show_texture", true);
    this->object_gouraud = config.getBool("Game", "object_gouraud", true);
    this->show_fog = config.getBool("Game", "show_fog", true);
    this->terrain_detail = config.getInt("Video", "terrain_detail", 0);
    this->show_clouds = config.getBool("Game", "clouds_enabled", false);
    this->width = width;
    this->height = height;
    camera.setPersective(this->fov, this->width / (float)this->height, kNear, this->max_view_distance + this->max_view_distance * 0.2f);
    light.SetWithCoo(300, 300, 300);
    view_ = cameraView(0, 0, width, height, 0.0f);
    settings_.load();
    r4bridge::textureQuality().upscale = settings_.upscale;
    r4bridge::textureQuality().mipmaps = settings_.mipmaps;

    R4Screen *screen = R4Screen::current();
    if (!screen || !screen->hasDevice()) {
        std::printf("[R4Renderer] no R4Screen: nothing will be drawn\n");
        return;
    }
    screen->device().beginHostInterop();
    R4Screen::countInterop();
    renderer_ = std::make_unique<r4::render::Renderer>(screen->device());
    renderer_->options().anisotropy = settings_.anisotropy;
    r4::gpu::RenderTargetDesc d;
    d.width = kTargetWidth;
    d.height = kTargetHeight;
    targetView_ = screen->device().createRenderTarget(d);
    screen->device().endHostInterop();
    if (!renderer_->error().empty()) {
        std::printf("[R4Renderer] %s\n", renderer_->error().c_str());
    }
    targetPixels_.assign((size_t)kTargetWidth * kTargetHeight * 4, 0);
    root_ = r4::scene::make<r4::scene::Group>();
    root_->setName("pass");
    skyPlace_ = r4::scene::make<r4::scene::Group>();
    skyPlace_->setName("sky (at the eye)");
    initialized = true;
}

bool R4Renderer::pending() const {
    return !items_.empty() || !scenes_.empty() || !batches_.empty() || skyShown_;
}

R4Renderer::View R4Renderer::cameraView(int x, int y, int width, int height, float verticalOffset) {
    View v;
    v.projection = r4::translate({0, verticalOffset, 0}) * glm::make_mat4(camera.getProjectionMatrix()->ToGL());
    v.view = glm::make_mat4(camera.getViewMatrix()->ToGL());
    v.viewport[0] = x;
    v.viewport[1] = y;
    v.viewport[2] = width;
    v.viewport[3] = height;
    return v;
}

void R4Renderer::setView(const View &v) {
    bool same = v.projection == view_.projection && v.view == view_.view && v.toTexture == view_.toTexture && std::memcmp(v.viewport, view_.viewport, sizeof v.viewport) == 0;
    if (!same && pending()) {
        flush();
    }
    view_ = v;
    stack_.assign(1, r4::Mat4(1.0f));
}

void R4Renderer::bindCameraProjectionAndViewViewport(int32_t viewportW, int32_t viewportH, float verticalOffset) {
    setView(cameraView(0, 0, viewportW, viewportH, verticalOffset));
}

void R4Renderer::initRenderCameraView() {
    View v = cameraView(view_.viewport[0], view_.viewport[1], view_.viewport[2], view_.viewport[3], this->verticalOffset);
    v.toTexture = view_.toTexture;
    setView(v);
}

void R4Renderer::pushMatrix() {
    stack_.push_back(stack_.back());
}

void R4Renderer::popMatrix() {
    if (stack_.size() > 1) {
        stack_.pop_back();
    }
}

void R4Renderer::translateMatrix(float x, float y, float z) {
    stack_.back() = stack_.back() * r4::translate({x, y, z});
}

void R4Renderer::rotateMatrix(float degrees, float x, float y, float z) {
    stack_.back() = stack_.back() * r4::rotate(r4::radians(degrees), {x, y, z});
}

void R4Renderer::scaleMatrix(float x, float y, float z) {
    stack_.back() = stack_.back() * r4::scale({x, y, z});
}

r4::Vec3 R4Renderer::clearColor() const {
    return {clearColor_[0], clearColor_[1], clearColor_[2]};
}

void R4Renderer::clearFrame(bool color, bool depth, r4::Vec3 rgb) {
    R4Screen *screen = R4Screen::current();
    if (!renderer_ || !screen) {
        return;
    }
    flush();
    r4::gpu::ClearValues clear;
    clear.color = color;
    clear.depth = depth;
    clear.r = rgb.x;
    clear.g = rgb.y;
    clear.b = rgb.z;
    r4::gpu::Device &device = screen->device();
    device.beginHostInterop();
    R4Screen::countInterop();
    int width = 0;
    int height = 0;
    screen->sceneSize(width, height);
    device.beginPass(screen->sceneTarget(), 0, 0, width, height, clear);
    device.endPass();
    device.endHostInterop();
}

void R4Renderer::clear(void) {
    clearColor_[0] = 0;
    clearColor_[1] = 0;
    clearColor_[2] = 0;
    clearFrame(true, true, clearColor());
}

void R4Renderer::clearDepth() {
    clearFrame(false, true, clearColor());
}

void R4Renderer::setClearColor(uint8_t red, uint8_t green, uint8_t blue) {
    clearColor_[0] = red / 255.0f;
    clearColor_[1] = green / 255.0f;
    clearColor_[2] = blue / 255.0f;
}

void R4Renderer::setObjectLight(int ambient_weight, int light_weight, int c0, int c1, int c2) {
    const float ambient = object_light_ambient;
    const float share = object_light_share;
    SCRenderer::setObjectLight(ambient_weight, light_weight, c0, c1, c2);
    if (ambient != object_light_ambient || share != object_light_share) {
        flush();
        models_.clear();
    }
}

r4bridge::Lighting R4Renderer::objectLighting() const {
    r4bridge::Lighting lighting;
    lighting.ambient = settings_.lightOverride ? settings_.ambient : object_light_ambient;
    lighting.light = settings_.lightOverride ? settings_.direct : object_light_share;
    return lighting;
}

// as the Terrain Detail of r4objviewer: the blocks are the LODs of 3 or 4 levels, the quadtree the LODs of 2 children
void R4Renderer::applyTerrainLod() {
    if (!terrain_ || settings_.terrainLod == R4Settings::TerrainLod::BY_DISTANCE) {
        return;
    }
    const int level = (int)settings_.terrainLod - 1;
    std::vector<r4::scene::Node *> stack{terrain_.get()};
    while (!stack.empty()) {
        r4::scene::Node *n = stack.back();
        stack.pop_back();
        if (n->type() == r4::scene::NodeType::LOD) {
            auto *lod = static_cast<r4::scene::LOD *>(n);
            if (lod->children().size() == 2) {
                // the merged mesh of the far groups only with the coarse level
                const bool coarse = settings_.terrainLod == R4Settings::TerrainLod::COARSE;
                lod->setRanges({0, coarse ? 0.0f : 1e30f, 1e30f});
            } else {
                std::vector<float> ranges = lod->ranges();
                for (size_t i = 0; i < ranges.size(); i++) {
                    ranges[i] = (int)i <= level ? 0.0f : 1e30f;
                }
                lod->setRanges(ranges);
            }
        }
        for (const auto &c : n->children()) {
            stack.push_back(c.get());
        }
    }
}

void R4Renderer::noShadows(RSEntity *object) {
    if (object && unshadowed_.insert(object).second) {
        models_.erase(object);
    }
}

// any thread: toModel reads the entity and the palette
std::shared_ptr<r4::assets::Model> R4Renderer::convert(RSEntity *object) {
    if (!renderer_ || !object || object->vertices.empty()) {
        return nullptr;
    }
    return r4bridge::toModel(object, palette, terrain_ranges, objectLighting(), r4bridge::Axes::REAL_SPACE);
}

r4::scene::NodePtr R4Renderer::upload(RSEntity *object, const std::shared_ptr<r4::assets::Model> &model) {
    r4::scene::NodePtr node;
    if (model && unshadowed_.count(object)) {
        for (r4::assets::Material &m : model->materials) {
            m.castShadows = false;
            m.receiveShadows = false;
        }
    }
    if (model) {
        R4Screen *screen = R4Screen::current();
        screen->device().beginHostInterop();
        R4Screen::countInterop();
        node = renderer_->upload(*model);
        screen->device().endHostInterop();
    }
    models_.emplace(object, node);
    return node;
}

r4::scene::NodePtr R4Renderer::modelOf(RSEntity *object) {
    auto it = models_.find(object);
    if (it != models_.end()) {
        return it->second;
    }
    return upload(object, convert(object));
}

void R4Renderer::preloadModels(const std::vector<RSEntity *> &objects) {
    std::vector<RSEntity *> missing;
    for (RSEntity *object : objects) {
        if (object && !models_.count(object) && std::find(missing.begin(), missing.end(), object) == missing.end()) {
            missing.push_back(object);
        }
    }
    const r4bridge::ImageCacheStats before = r4bridge::imageCacheStats();
    const Uint64 start = SDL_GetPerformanceCounter();
    std::vector<std::shared_ptr<r4::assets::Model>> models(missing.size());
    r4bridge::parallelFor(missing.size(), [&](size_t i) {
        models[i] = convert(missing[i]);
    });
    const double convertMs = R4Screen::msSince(start);
    const Uint64 uploadStart = SDL_GetPerformanceCounter();
    // the GPU from this thread only
    for (size_t i = 0; i < missing.size(); i++) {
        upload(missing[i], models[i]);
    }
    const r4bridge::ImageCacheStats after = r4bridge::imageCacheStats();
    std::printf("[R4Renderer] %zu models: converted in %.0f ms (textures: %zu found, %zu made in %.0f ms on all the cores), sent to the GPU in %.0f ms\n", missing.size(), convertMs, after.hits - before.hits, after.made - before.made, after.makeMs - before.makeMs, R4Screen::msSince(uploadStart));
}

void R4Renderer::useArea(RSArea *area, const std::vector<r4::Vec3> *keepOut) {
    const bool newKeepOut = keepOut != nullptr && *keepOut != keepOut_;
    if ((area == area_ && !newKeepOut) || !renderer_) {
        return;
    }
    flush();
    if (area != area_) {
        // a new mission can free its entities and make others at the same addresses
        models_.clear();
        unshadowed_.clear();
        terrain_.reset();
        overlay_.reset();
        craters_.reset();
        craters_version_ = -1;
        clouds_.reset();
        keepOut_.clear();
    }
    if (keepOut != nullptr) {
        keepOut_ = *keepOut;
    }
    const bool newArea = area != area_;
    const Uint64 start = SDL_GetPerformanceCounter();
    area_ = area;
    forests_.reset();
    buildings_.reset();
    if (!area) {
        return;
    }
    r4bridge::Lighting lighting = objectLighting();
    std::shared_ptr<r4::assets::Model> terrain;
    std::shared_ptr<r4::assets::Model> overlay;
    if (newArea) {
        r4bridge::TerrainLight light;
        light.ranges = &terrain_ranges;
        light.lighting = lighting;
        light.dynamic = settings_.sunTerrain;
        light.relief = settings_.terrainRelief;
        terrain = r4bridge::terrainModel(area, palette, {0, 40000, 100000, 1e30f}, light);
        overlay = r4bridge::overlayModel(area, palette, area->objectOverlay, r4bridge::OVERLAY_IMAGE_BASE, 128.0f, false);
    }
    std::vector<r4::Vec3> around = keepOut_;
    for (const MapObject &object : area->objects) {
        around.push_back({object.position.x, object.position.z, 40.0f});
    }
    size_t buildings = 0;
    std::shared_ptr<r4::assets::Model> city = settings_.buildings ? r4bridge::cityModel(area, palette, lighting, around, &buildings) : nullptr;
    R4Screen *screen = R4Screen::current();
    screen->device().beginHostInterop();
    R4Screen::countInterop();
    if (terrain) {
        terrain_ = renderer_->upload(*terrain);
        applyTerrainLod();
    }
    if (overlay) {
        overlay_ = renderer_->upload(*overlay);
    }
    if (city) {
        buildings_ = renderer_->upload(*city);
        buildings_->setName("buildings");
    }
    if (newArea) {
        clouds_ = makeClouds(area);
    }
    screen->device().endHostInterop();
    size_t trees = 0;
    if (settings_.forests) {
        forests_ = makeForests(area, trees);
    }
    areaStats_.trees = trees;
    areaStats_.buildings = buildings;
    areaStats_.clouds = clouds_ ? clouds_->children().size() : 0;
    areaStats_.ms = (double)(SDL_GetPerformanceCounter() - start) * 1000.0 / (double)SDL_GetPerformanceFrequency();
    const r4bridge::ImageCacheStats cache = r4bridge::imageCacheStats();
    std::printf("[R4Renderer] area in %.0f ms: %zu trees, %zu buildings, shadows %s; textures since the start: %zu found, %zu made in %.0f ms\n", areaStats_.ms, trees, buildings, settings_.shadows ? "on" : "off", cache.hits, cache.made, cache.makeMs);
}

// the forests of r4objviewer: chunks of trees, each a LOD with an instancer by species, the full tree near and the simple one far
r4::scene::NodePtr R4Renderer::makeForests(RSArea *area, size_t &trees) {
    // the trees in full up to treeNear, simple up to treeFar, none beyond; the units of the game are meters
    const float treeNear = settings_.treeDetailKm * 1000.0f;
    const float treeFar = settings_.treeKm * 1000.0f;
    r4bridge::ForestSettings fs;
    fs.mode = settings_.forestMode;
    fs.density = settings_.forestDensity;
    fs.palette = &palette;
    for (const r4::Vec3 &k : keepOut_) {
        fs.keepOut.push_back({k.x, k.y, std::max(k.z * 2, 150.0f)});
    }
    for (const MapObject &object : area->objects) {
        fs.keepOut.push_back({object.position.x, object.position.z, 150.0f});
    }
    std::vector<r4bridge::ForestChunk> chunks = r4bridge::forestChunks(area, fs);
    if (chunks.empty() || fs.density <= 0) {
        return nullptr;
    }
    r4bridge::Lighting lighting = objectLighting();
    R4Screen *screen = R4Screen::current();
    std::vector<r4::scene::InstancePart> parts[r4bridge::TREE_SPECIES_COUNT][2];
    screen->device().beginHostInterop();
    R4Screen::countInterop();
    for (int sp = 0; sp < r4bridge::TREE_SPECIES_COUNT; sp++) {
        for (int level = 0; level < 2; level++) {
            auto model = r4bridge::treeModel(sp, level != 0, palette, terrain_ranges, lighting);
            parts[sp][level] = r4::scene::collectParts(*renderer_->upload(*model));
        }
    }
    screen->device().endHostInterop();
    auto forests = r4::scene::make<r4::scene::Group>();
    forests->setName("forests");
    forests->setMask(kForestMask);
    std::map<std::pair<int, int>, std::shared_ptr<r4::scene::Group>> blocks;
    for (r4bridge::ForestChunk &chunk : chunks) {
        auto lod = r4::scene::make<r4::scene::LOD>();
        lod->setCenter(chunk.center);
        lod->setRanges({0, treeFar + chunk.radius});
        auto group = r4::scene::make<r4::scene::Group>();
        r4bridge::plantForest(area, fs, chunk);
        for (int sp = 0; sp < r4bridge::TREE_SPECIES_COUNT; sp++) {
            auto instancer = r4::scene::make<r4::scene::Instancer>();
            instancer->setUp({0, 1, 0});
            instancer->addLevel(parts[sp][0], treeNear);
            instancer->addLevel(parts[sp][1], treeFar);
            trees += chunk.trees[sp].size();
            instancer->setPlacements(std::move(chunk.trees[sp]));
            // on the GPU once: no work for each tree in a frame, fewer trees beyond the detailed ones
            instancer->setStatic(treeNear);
            group->addChild(instancer);
        }
        lod->addChild(group);
        int bx = (int)(chunk.cx * fs.chunk / BLOCK_WIDTH);
        int bz = (int)(chunk.cz * fs.chunk / BLOCK_WIDTH);
        auto &block = blocks[{bx, bz}];
        if (!block) {
            block = r4::scene::make<r4::scene::Group>();
            forests->addChild(block);
        }
        block->addChild(lod);
    }
    return forests;
}

void R4Renderer::addScene(const r4::scene::NodePtr &root) {
    if (root) {
        scenes_.push_back(root);
    }
}

void R4Renderer::placeModel(RSEntity *object, const r4::Mat4 &matrix) {
    if (!renderer_ || !object || object->vertices.empty()) {
        return;
    }
    if (r4::scene::NodePtr node = modelOf(object)) {
        items_.push_back({node, matrix});
    }
}

void R4Renderer::drawModel(RSEntity *object, size_t lodLevel, Vector3D position, Vector3D orientation, Vector3D ajustement, float scale) {
    (void)lodLevel;
    placeModel(object, model() * r4::translate(vec(position)) * degreeRotation(orientation) * r4::translate(vec(ajustement)) * r4::scale({scale, scale, scale}));
}

void R4Renderer::drawModel(RSEntity *object, size_t lodLevel, Vector3D position, Vector3D orientation, Vector3D ajustement) {
    (void)lodLevel;
    placeModel(object, model() * r4::translate(vec(position)) * degreeRotation(orientation) * r4::translate(vec(ajustement)));
}

void R4Renderer::drawModel(RSEntity *object, size_t lodLevel, Vector3D position, Vector3D orientation) {
    (void)lodLevel;
    placeModel(object, model() * r4::translate(vec(position)) * degreeRotation(orientation));
}

void R4Renderer::drawModel(RSEntity *object, Vector3D position, Vector3D orientation) {
    placeModel(object, model() * radianPlacement(position, orientation));
}

void R4Renderer::drawModel(RSEntity *object, size_t lodLevel) {
    (void)lodLevel;
    placeModel(object, model());
}

void R4Renderer::drawModelWithChilds(RSEntity *object, size_t lodLevel, Vector3D position, Vector3D orientation, int wheel_index, int thrust, std::vector<std::tuple<Vector3D, RSEntity *>> weaps_load) {
    (void)lodLevel;
    if (object == nullptr) {
        return;
    }
    const r4::Mat4 base = model() * radianPlacement(position, orientation);
    placeModel(object, base);
    if (wheel_index && (int)object->chld.size() > wheel_index) {
        placeModel(object->chld[wheel_index]->objct, base);
    }
    if (thrust > 50 && !object->chld.empty()) {
        const RSEntity::CHLD *c = object->chld[0];
        placeModel(c->objct, base * r4::translate({c->z, c->y, c->x}) * r4::scale({1 + thrust / 100.0f, 1, 1}));
    }
    for (auto &weapon : weaps_load) {
        if (std::get<1>(weapon) != nullptr) {
            placeModel(std::get<1>(weapon), base * r4::translate(vec(std::get<0>(weapon))));
        }
    }
}

void R4Renderer::drawModelColorPass(RSEntity *object, size_t lodLevel, std::vector<Vector3D> &vertexNormals, float ambientLamber, Vector3D lightEye, float *MV) {
}

void R4Renderer::drawModelTexturePass(RSEntity *object, size_t lodLevel, std::vector<Vector3D> &vertexNormals, float ambientLamber, Vector3D lightEye, float *MV) {
}

void R4Renderer::drawModelTransparentPass(RSEntity *object, size_t lodLevel, std::vector<Vector3D> &vertexNormals, float ambientLamber, Vector3D lightEye, float *MV) {
}

// SCRenderer::displayModel loops forever on its own; nothing calls it
void R4Renderer::displayModel(RSEntity *object, size_t lodLevel) {
}

void R4Renderer::renderObjects(RSArea *area, size_t blockID) {
    (void)blockID;
    for (const MapObject &object : area->objects) {
        placeModel(object.entity, model() * r4bridge::mapObjectMatrix(object));
    }
}

void R4Renderer::renderMissionObjects(RSMission *mission) {
    for (MISN_PART *part : mission->mission_data.parts) {
        if (part == mission->getPlayerCoord() || part->entity == nullptr) {
            continue;
        }
        placeModel(part->entity, model() * r4bridge::missionPartMatrix(*part));
    }
}

std::vector<r4::assets::Vertex> &R4Renderer::batch(const BatchKey &key) {
    return batches_[key];
}

void R4Renderer::vertex(std::vector<r4::assets::Vertex> &to, Vector3D p, float u, float v, r4::Vec4 color) {
    r4::assets::Vertex out;
    out.position = r4::transformPoint(model(), vec(p));
    out.normal = {0, 1, 0};
    out.uv = {u, v};
    out.color = packColor(color);
    to.push_back(out);
}

// corners from the first texel (0, 0) around to (0, 1), as the quads of SCRenderer
void R4Renderer::quad(const BatchKey &key, const Vector3D corners[4], r4::Vec4 color, r4::Vec4 rect) {
    std::vector<r4::assets::Vertex> &to = batch(key);
    const float uv[4][2] = {{rect.x, rect.y}, {rect.z, rect.y}, {rect.z, rect.w}, {rect.x, rect.w}};
    // the point of the quad at (u, v), between its corners
    auto at = [&](float u, float v) {
        return corners[0] * ((1 - u) * (1 - v)) + corners[1] * (u * (1 - v)) + corners[2] * (u * v) + corners[3] * ((1 - u) * v);
    };
    const int order[6] = {0, 1, 2, 0, 2, 3};
    for (int i : order) {
        vertex(to, at(uv[i][0], uv[i][1]), uv[i][0], uv[i][1], color);
    }
}

void R4Renderer::lines(const Vector3D *ends, size_t count, r4::Vec4 color) {
    BatchKey key;
    key.blend = Blend::ALPHA;
    key.depthTest = false;
    key.order = 1;
    key.primitive = r4::assets::Primitive::LINES;
    std::vector<r4::assets::Vertex> &to = batch(key);
    for (size_t i = 0; i < count; i++) {
        vertex(to, ends[i], 0, 0, color);
    }
}

void R4Renderer::cubeLines(const r4::Mat4 &matrix, r4::Vec4 front, r4::Vec4 back, r4::Vec4 sides) {
    pushMatrix();
    stack_.back() = matrix;
    const Vector3D f[8] = {{-0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, 0.5f}, {0.5f, 0.5f, 0.5f}, {0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}, {-0.5f, -0.5f, 0.5f}};
    const Vector3D b[8] = {{-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f}};
    const Vector3D s[8] = {{-0.5f, -0.5f, 0.5f}, {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}, {0.5f, 0.5f, -0.5f}, {-0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, -0.5f}};
    lines(f, 8, front);
    lines(b, 8, back);
    lines(s, 8, sides);
    popMatrix();
}

void R4Renderer::drawLine(Vector3D start, Vector3D end, Vector3D color, Vector3D orientation) {
    pushMatrix();
    stack_.back() = model() * r4::translate(vec(start)) * degreeRotation(orientation);
    const Vector3D ends[2] = {{0, 0, 0}, end};
    lines(ends, 2, {color.x, color.y, color.z, 1});
    popMatrix();
}

void R4Renderer::drawLine(Vector3D start, Vector3D end, Vector3D color, Vector3D orientation, Vector3D position) {
    pushMatrix();
    stack_.back() = model() * r4::translate(vec(position)) * degreeRotation(orientation);
    const Vector3D ends[2] = {start, end};
    lines(ends, 2, {color.x, color.y, color.z, 1});
    popMatrix();
}

void R4Renderer::drawLine(Vector3D start, Vector3D end, Vector3D color) {
    pushMatrix();
    stack_.back() = model() * r4::translate(vec(start));
    const Vector3D ends[2] = {{0, 0, 0}, end};
    lines(ends, 2, {color.x, color.y, color.z, 1});
    popMatrix();
}

// glPointSize(5): a square of 5 pixels facing the eye; the points and the lines in the last bin, as the game draws them without depth after the rest
void R4Renderer::drawPoint(Vector3D point, Vector3D color, Vector3D pos, Vector3D orientation) {
    const r4::Vec3 at = r4::transformPoint(model() * r4::translate(vec(pos)) * degreeRotation(orientation), vec(point));
    const r4::Mat4 &v = view_.view;
    const r4::Vec3 right{v[0][0], v[1][0], v[2][0]};
    const r4::Vec3 up{v[0][1], v[1][1], v[2][1]};
    const r4::Vec3 eye = r4::translationOf(r4::inverse(v));
    const float pixels = (float)std::max(1, view_.viewport[3]);
    const float half = kPointPixels * r4::length(at - eye) * std::tan(r4::radians(this->fov) * 0.5f) / (pixels * 0.5f);
    Vector3D corners[4];
    const r4::Vec3 offsets[4] = {-right - up, right - up, right + up, -right + up};
    for (int i = 0; i < 4; i++) {
        r4::Vec3 c = at + offsets[i] * half;
        corners[i] = {c.x, c.y, c.z};
    }
    BatchKey key;
    key.blend = Blend::ALPHA;
    key.depthTest = false;
    key.order = 1;
    pushMatrix();
    stack_.back() = r4::Mat4(1.0f);
    quad(key, corners, {color.x, color.y, color.z, 1});
    popMatrix();
}

void R4Renderer::drawParticle(Vector3D pos, float alpha) {
    pushMatrix();
    translateMatrix(pos.x, pos.y, pos.z);
    BatchKey key;
    key.blend = Blend::ALPHA;
    key.depthWrite = false;
    const Vector3D a[4] = {{1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, -1.0f}, {-1.0f, 1.0f, -1.0f}, {-1.0f, -1.0f, 1.0f}};
    const Vector3D b[4] = {{-1.0f, -1.0f, -1.0f}, {-1.0f, 1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, -1.0f, 1.0f}};
    quad(key, a, {1, 1, 1, alpha});
    quad(key, b, {1, 1, 1, alpha});
    popMatrix();
}

void R4Renderer::drawSprite(Vector3D pos, Texture *tex, float zoom) {
    if (!tex) {
        return;
    }
    BatchKey key;
    key.texture = textureOf(tex).id;
    key.blend = Blend::ALPHA;
    key.depthWrite = false;
    const float s = 4.0f * zoom + 1.0f;
    const Vector3D a[4] = {{s, -s, -s}, {s, s, -s}, {-s, s, -s}, {-s, -s, s}};
    const Vector3D b[4] = {{-s, -s, -s}, {-s, s, -s}, {s, s, s}, {s, -s, s}};
    pushMatrix();
    translateMatrix(pos.x, pos.y, pos.z);
    quad(key, a, {1, 1, 1, 1});
    quad(key, b, {1, 1, 1, 1});
    popMatrix();
}

void R4Renderer::drawBillboard(Vector3D pos, Texture *tex, float size, float alpha) {
    if (!initialized || tex == nullptr) {
        return;
    }
    Point3D cameraPos = camera.getPosition();
    Vector3D look = {cameraPos.x - pos.x, cameraPos.y - pos.y, cameraPos.z - pos.z};
    look.Normalize();
    Vector3D globalUp = {0.0f, 1.0f, 0.0f};
    Vector3D right = globalUp.CrossProduct(&look);
    right.Normalize();
    Vector3D up = look.CrossProduct(&right);
    const float h = size * 0.5f;
    const Vector3D corners[4] = {pos - right * h - up * h, pos + right * h - up * h, pos + right * h + up * h, pos - right * h + up * h};
    BatchKey key;
    key.texture = textureOf(tex).id;
    key.blend = Blend::ALPHA;
    key.alphaTest = true;
    quad(key, corners, {1, 1, 1, alpha});
}

void R4Renderer::drawTriangles(const EffectVertex *vertices, size_t count, unsigned int texture, BlendMode blend, bool depthWrite) {
    if (vertices == nullptr || count < 3) {
        return;
    }
    BatchKey key;
    auto it = effectTextures_.find(texture);
    key.texture = it != effectTextures_.end() ? it->second.id : 0;
    key.blend = blend == BlendMode::Additive ? Blend::ADDITIVE : Blend::ALPHA;
    key.depthWrite = depthWrite;
    std::vector<r4::assets::Vertex> &to = batch(key);
    for (size_t i = 0; i < count - count % 3; i++) {
        const EffectVertex &v = vertices[i];
        vertex(to, {v.x, v.y, v.z}, v.u, v.v, {v.r, v.g, v.b, v.a});
    }
}

void R4Renderer::drawTracer(Vector3D pos, Vector3D velocityDir, float length, float pixelWidth, Vector3D color, float alpha) {
    if (!initialized) {
        return;
    }
    Point3D camPos = camera.getPosition();
    Vector3D look = {camPos.x - pos.x, camPos.y - pos.y, camPos.z - pos.z};
    float dist = look.Length();
    if (dist < 1e-4f) {
        return;
    }
    look = look * (1.0f / dist);
    float dirLen = velocityDir.Length();
    if (dirLen < 1e-4f) {
        return;
    }
    Vector3D dir = velocityDir * (1.0f / dirLen);
    float dotLook = dir.DotProduct(&look);
    Vector3D upRaw = dir - look * dotLook;
    float upRawLen = upRaw.Length();
    Vector3D up;
    if (upRawLen > 1e-4f) {
        up = upRaw * (1.0f / upRawLen);
    } else {
        Vector3D worldUp = {0.0f, 1.0f, 0.0f};
        up = worldUp - look * worldUp.DotProduct(&look);
        float l = up.Length();
        up = (l > 1e-4f) ? up * (1.0f / l) : Vector3D{1.0f, 0.0f, 0.0f};
    }
    Vector3D right = up.CrossProduct(&look);
    right.Normalize();
    float fovRad = this->fov * (float)M_PI / 180.0f;
    float halfWidth = (pixelWidth * dist * tanf(fovRad * 0.5f)) / (this->height * 0.5f);
    float halfLen = std::max((length * 0.5f) * upRawLen, halfWidth);
    Vector3D rOff = right * halfWidth;
    Vector3D uOff = up * halfLen;
    Vector3D front = pos + uOff;
    Vector3D back = pos - uOff;
    const Vector3D corners[4] = {back - rOff, back + rOff, front + rOff, front - rOff};
    BatchKey key;
    key.blend = Blend::ADDITIVE;
    key.depthWrite = false;
    quad(key, corners, {color.x, color.y, color.z, alpha});
}

void R4Renderer::drawTexturedQuad(Vector3D pos, Vector3D orientation, std::vector<Vector3D> quadCorners, Texture *tex) {
    if (!tex || quadCorners.size() < 4) {
        return;
    }
    // the panel of the target camera in front of the view (SCVirtualCockpit::drawPanel): 320 x 200, with no place and no turn
    const bool targetOverlay = tex->width == 320 && tex->height == 200 && pos.x == 0 && pos.y == 0 && pos.z == 0 && orientation.x == 0 && orientation.y == 0 && orientation.z == 0;
    if (targetOverlay && !settings_.targetOverlay) {
        return;
    }
    BatchKey key;
    key.texture = textureOf(tex).id;
    key.blend = Blend::ALPHA;
    key.alphaTest = true;
    auto it = textures_.find(tex);
    r4::Vec4 rect{0, 0, 1, 1};
    if (it != textures_.end()) {
        if (it->second.empty) {
            return;
        }
        rect = it->second.visible;
    }
    pushMatrix();
    stack_.back() = model() * r4::translate(vec(pos)) * degreeRotation(orientation);
    quad(key, quadCorners.data(), {1, 1, 1, 1}, rect);
    popMatrix();
}

// the puffs of the clouds: before the terrain and without depth, as SCRenderer::renderClouds
void R4Renderer::renderEllipsoid(float cx, float cy, float cz, float rx, float ry, float rz, int rings, int slices, float r, float g, float b, float baseAlpha) {
    BatchKey key;
    key.blend = Blend::ALPHA;
    key.depthTest = false;
    key.depthWrite = false;
    key.order = -1;
    std::vector<r4::assets::Vertex> &to = batch(key);
    auto at = [&](float phi, float theta, Vector3D &p, float &a) {
        p = {cx + rx * cosf(phi) * cosf(theta), cy + ry * sinf(phi), cz + rz * cosf(phi) * sinf(theta)};
        a = baseAlpha * (sinf(phi) + 1.0f) * 0.5f;
    };
    for (int ring = 0; ring < rings; ++ring) {
        float phi0 = (float)M_PI * ring / rings - (float)M_PI_2;
        float phi1 = (float)M_PI * (ring + 1) / rings - (float)M_PI_2;
        for (int s = 0; s < slices; ++s) {
            float t0 = 2.0f * (float)M_PI * s / slices;
            float t1 = 2.0f * (float)M_PI * (s + 1) / slices;
            Vector3D p[4];
            float a[4];
            at(phi0, t0, p[0], a[0]);
            at(phi1, t0, p[1], a[1]);
            at(phi1, t1, p[2], a[2]);
            at(phi0, t1, p[3], a[3]);
            const int order[6] = {0, 1, 2, 0, 2, 3};
            for (int i : order) {
                vertex(to, p[i], 0, 0, {r, g, b, a[i]});
            }
        }
    }
}

// the clouds of the area are a graph, made once by useArea
void R4Renderer::renderClouds(RSArea *area) {
    useArea(area);
    if (clouds_) {
        items_.push_back({clouds_, r4::Mat4(1.0f)});
    }
}

// SCRenderer::renderClouds as a graph: a geode by cloud with all its puffs in one mesh, culled by its bounds.
// The puffs are white, so their blend gives the same color in any order: no sort by the distance.
r4::scene::NodePtr R4Renderer::makeClouds(RSArea *area) {
    if (area->clouds.empty()) {
        return nullptr;
    }
    // after the opaque items and the dome of the sky, with the depth of the world: the relief in front of a cloud hides it
    BatchKey key;
    key.blend = Blend::ALPHA;
    key.depthTest = true;
    key.depthWrite = false;
    key.order = 0;
    std::shared_ptr<r4::render::MaterialInstance> material = materialOf(key);
    auto clouds = r4::scene::make<r4::scene::Group>();
    clouds->setName("clouds");
    const uint32_t row = kCloudSlices + 1;
    for (const Cloud &c : area->clouds) {
        r4::assets::Mesh mesh;
        mesh.name = "cloud";
        for (const CloudPuff &p : c.puffs) {
            const uint32_t base = (uint32_t)mesh.vertices.size();
            for (int ring = 0; ring <= kCloudRings; ring++) {
                const float phi = (float)M_PI * ring / kCloudRings - (float)M_PI_2;
                for (int s = 0; s <= kCloudSlices; s++) {
                    const float theta = 2.0f * (float)M_PI * s / kCloudSlices;
                    r4::assets::Vertex v;
                    v.position = {c.position.x + p.ox + p.rx * cosf(phi) * cosf(theta), c.position.y + p.oy + p.ry * sinf(phi), c.position.z + p.oz + p.rz * cosf(phi) * sinf(theta)};
                    v.normal = {0, 1, 0};
                    v.color = packColor({1, 1, 1, c.alpha * (sinf(phi) + 1.0f) * 0.5f});
                    mesh.vertices.push_back(v);
                }
            }
            for (uint32_t ring = 0; ring < (uint32_t)kCloudRings; ring++) {
                for (uint32_t s = 0; s < (uint32_t)kCloudSlices; s++) {
                    const uint32_t a = base + ring * row + s;
                    const uint32_t b = a + row;
                    const uint32_t quad[6] = {a, b, b + 1, a, b + 1, a + 1};
                    mesh.indices.insert(mesh.indices.end(), quad, quad + 6);
                }
            }
        }
        auto geode = r4::scene::make<r4::scene::Geode>();
        geode->setName("cloud");
        geode->addDrawable({renderer_->uploadMesh(mesh), material});
        R4Screen::counters().buffers += 2;
        clouds->addChild(geode);
    }
    return clouds;
}

void R4Renderer::renderLineCube(Vector3D position, int32_t size) {
    const float s = (float)size;
    cubeLines(model() * r4::translate(vec(position)) * r4::scale({s, s, s}), {1, 0, 0, 1}, {0, 1, 0, 1}, {0, 0, 1, 1});
}

void R4Renderer::renderBBox(Vector3D position, Point3D min, Point3D max) {
    cubeLines(model() * r4::translate(vec(position)) * r4::scale({max.x - min.x, max.y - min.y, max.z - min.z}), {1, 0, 0, 1}, {0, 1, 0, 1}, {0, 0, 1, 1});
}

r4::gpu::TextureHandle R4Renderer::textureOf(Texture *tex) {
    if (!renderer_ || !tex || !tex->data || tex->width == 0 || tex->height == 0) {
        return {};
    }
    auto it = textures_.find(tex);
    if (it != textures_.end() && it->second.width == tex->width && it->second.height == tex->height) {
        return it->second.handle;
    }
    r4::assets::Image image;
    image.width = (int)tex->width;
    image.height = (int)tex->height;
    image.channels = 4;
    image.clampS = true;
    image.clampT = true;
    image.mipmaps = false;
    image.pixels.assign(tex->data, tex->data + tex->width * tex->height * 4);
    R4Screen *screen = R4Screen::current();
    screen->device().beginHostInterop();
    R4Screen::countInterop();
    r4::gpu::TextureHandle handle = renderer_->uploadImage(image, false);
    R4Screen::counters().textureUploads++;
    R4Screen::counters().textureBytes += image.pixels.size();
    screen->device().endHostInterop();
    TextureEntry &entry = textures_[tex];
    entry = {handle, tex->width, tex->height, image.pixels};
    entry.empty = !visibleRect(tex->data, tex->width, tex->height, entry.visible);
    tex->initialized = true;
    tex->id = handle.id;
    return handle;
}

unsigned int R4Renderer::createTexture(int w, int h, const unsigned char *rgba, bool repeatT) {
    if (!renderer_ || !rgba || w <= 0 || h <= 0) {
        return 0;
    }
    r4::assets::Image image;
    image.width = w;
    image.height = h;
    image.channels = 4;
    image.clampS = true;
    image.clampT = !repeatT;
    image.mipmaps = false;
    image.pixels.assign(rgba, rgba + (size_t)w * h * 4);
    R4Screen *screen = R4Screen::current();
    screen->device().beginHostInterop();
    R4Screen::countInterop();
    r4::gpu::TextureHandle handle = renderer_->uploadImage(image, false);
    R4Screen::counters().textureUploads++;
    R4Screen::counters().textureBytes += image.pixels.size();
    screen->device().endHostInterop();
    unsigned int id = nextEffectTexture_++;
    effectTextures_[id] = handle;
    return id;
}

void R4Renderer::createTextureInGPU(Texture *texture) {
    textureOf(texture);
}

void R4Renderer::uploadTextureContentToGPU(Texture *texture) {
    auto it = textures_.find(texture);
    if (it == textures_.end() || it->second.width != texture->width || it->second.height != texture->height || !texture->data) {
        textureOf(texture);
        return;
    }
    // the instruments of the 3D cockpit are drawn again at each frame, mostly the same
    std::vector<uint8_t> &sent = it->second.sent;
    const size_t bytes = texture->width * texture->height * 4;
    if (sent.size() == bytes && std::memcmp(sent.data(), texture->data, bytes) == 0) {
        return;
    }
    sent.assign(texture->data, texture->data + bytes);
    it->second.empty = !visibleRect(texture->data, texture->width, texture->height, it->second.visible);
    R4Screen *screen = R4Screen::current();
    screen->device().beginHostInterop();
    R4Screen::countInterop();
    screen->device().updateTexture(it->second.handle, 0, 0, (int)texture->width, (int)texture->height, texture->data);
    R4Screen::counters().textureUploads++;
    R4Screen::counters().textureBytes += bytes;
    screen->device().endHostInterop();
}

void R4Renderer::deleteTextureInGPU(Texture *texture) {
    textures_.erase(texture);
}

void R4Renderer::applyTextureFilter(const Texture *texture) {
}

void R4Renderer::ensureDetailTexture() {
}

void R4Renderer::ensureWaveTexture() {
}

r4::scene::NodePtr R4Renderer::sky() {
    if (sky_ || !renderer_) {
        return sky_;
    }
    // SCRenderer::renderSkydome of radius 1 about the origin, scaled to the far plane at each pass: the sky from the color of the fog to
    // blue above, the color of the fog below, where the terrain stops at the far plane
    const float radius = 1.0f;
    const r4::Vec3 fog{kFogColor[0], kFogColor[1], kFogColor[2]};
    r4::assets::Mesh mesh;
    mesh.name = "sky";
    auto point = [&](float phi, float theta, r4::Vec3 color) {
        r4::assets::Vertex v;
        v.position = {radius * cosf(phi) * cosf(theta), radius * sinf(phi), radius * cosf(phi) * sinf(theta)};
        v.normal = {0, 1, 0};
        v.color = packColor({color.x, color.y, color.z, 1});
        mesh.vertices.push_back(v);
    };
    for (int half = 0; half < 2; half++) {
        for (int r = 0; r < kSkyRings; ++r) {
            float t0 = (float)r / kSkyRings;
            float t1 = (float)(r + 1) / kSkyRings;
            float sign = half == 0 ? 1.0f : -1.0f;
            float phi0 = sign * (float)M_PI_2 * t0;
            float phi1 = sign * (float)M_PI_2 * t1;
            r4::Vec3 c0 = half == 0 ? skyColor(t0, fog) : fog;
            r4::Vec3 c1 = half == 0 ? skyColor(t1, fog) : fog;
            for (int s = 0; s < kSkySlices; ++s) {
                float a = 2.0f * (float)M_PI * s / kSkySlices;
                float b = 2.0f * (float)M_PI * (s + 1) / kSkySlices;
                uint32_t base = (uint32_t)mesh.vertices.size();
                point(phi0, a, c0);
                point(phi1, a, c1);
                point(phi1, b, c1);
                point(phi0, b, c0);
                const uint32_t order[6] = {0, 1, 2, 0, 2, 3};
                for (uint32_t i : order) {
                    mesh.indices.push_back(base + i);
                }
            }
        }
    }
    r4::assets::Material m;
    m.name = "sky";
    m.shading = r4::assets::Shading::UNLIT;
    m.vertexColors = true;
    m.diffuse = {1, 1, 1, 1};
    m.doubleSided = true;
    m.depthWrite = false;
    m.fog = false;
    m.drawOrder = kSkyOrder;
    m.castShadows = false;
    m.receiveShadows = false;
    R4Screen *screen = R4Screen::current();
    screen->device().beginHostInterop();
    R4Screen::countInterop();
    auto geode = r4::scene::make<r4::scene::Geode>();
    geode->setName("sky dome");
    geode->addDrawable({renderer_->uploadMesh(mesh), renderer_->createMaterial(m)});
    R4Screen::counters().buffers += 2;
    screen->device().endHostInterop();
    sky_ = geode;
    skyPlace_->addChild(sky_);
    return sky_;
}

std::shared_ptr<r4::render::MaterialInstance> R4Renderer::materialOf(const BatchKey &key) {
    auto it = materials_.find(key);
    if (it != materials_.end()) {
        return it->second;
    }
    r4::assets::Material m;
    m.name = "effect";
    m.shading = r4::assets::Shading::UNLIT;
    m.vertexColors = true;
    m.diffuse = {1, 1, 1, 1};
    m.doubleSided = true;
    m.castShadows = false;
    m.receiveShadows = false;
    m.depthTest = key.depthTest;
    m.depthWrite = key.depthWrite;
    m.drawOrder = key.order;
    if (key.blend == Blend::ALPHA) {
        m.alpha = r4::assets::AlphaMode::BLEND;
        m.blendDepthWrite = key.depthWrite;
    } else if (key.blend == Blend::ADDITIVE) {
        m.alpha = r4::assets::AlphaMode::ADDITIVE;
    }
    if (key.alphaTest) {
        m.alphaTest = 0.1f;
    }
    r4::gpu::TextureHandle texture;
    texture.id = key.texture;
    auto material = renderer_->createMaterial(m, texture);
    materials_[key] = material;
    return material;
}

std::shared_ptr<r4::render::GpuMesh> R4Renderer::streamMesh(const BatchKey &key, const std::vector<r4::assets::Vertex> &vertices) {
    r4::gpu::Device &device = R4Screen::current()->device();
    Stream &stream = streams_[key];
    stream.next = (stream.next + 1) % kStreamMeshes;
    std::shared_ptr<r4::render::GpuMesh> &mesh = stream.meshes[stream.next];
    size_t &capacity = stream.capacity[stream.next];
    if (!mesh || capacity < vertices.size()) {
        capacity = std::max<size_t>(vertices.size() * 2, 1024);
        std::vector<uint32_t> indices(capacity);
        for (uint32_t i = 0; i < (uint32_t)capacity; i++) {
            indices[i] = i;
        }
        mesh = std::shared_ptr<r4::render::GpuMesh>(new r4::render::GpuMesh, [&device](r4::render::GpuMesh *m) {
            device.destroyBuffer(m->vertices);
            device.destroyBuffer(m->indices);
            delete m;
        });
        mesh->name = "effects";
        mesh->vertices = device.createBuffer(r4::gpu::BufferUsage::VERTEX, nullptr, capacity * sizeof(r4::assets::Vertex), true);
        mesh->indices = device.createBuffer(r4::gpu::BufferUsage::INDEX, indices.data(), capacity * sizeof(uint32_t), false);
        R4Screen::counters().buffers += 2;
    }
    device.updateBuffer(mesh->vertices, 0, vertices.data(), vertices.size() * sizeof(r4::assets::Vertex));
    mesh->indexCount = (uint32_t)vertices.size();
    mesh->primitive = key.primitive;
    r4::Box box;
    for (const r4::assets::Vertex &v : vertices) {
        box.extend(v.position);
    }
    mesh->bounds = r4::sphereOf(box);
    mesh->box = box;
    return mesh;
}

void R4Renderer::flush() {
    R4Screen *screen = R4Screen::current();
    if (!pending()) {
        return;
    }
    const Uint64 start = SDL_GetPerformanceCounter();
    if (!renderer_ || !screen) {
        items_.clear();
        scenes_.clear();
        batches_.clear();
        skyShown_ = false;
        return;
    }
    const r4::Vec3 eye = r4::translationOf(r4::inverse(view_.view));
    const float fogStart = settings_.fogOverride ? settings_.fogStartKm * 1000.0f : this->max_view_distance * 0.40f;
    const float fogEnd = settings_.fogOverride ? settings_.fogEndKm * 1000.0f : this->max_view_distance * 0.93f;
    const float zFar = show_fog ? fogEnd * kFarAfterFog : (float)this->max_view_distance * 1.2f;
    float zNear = kNear;
    const r4::Mat4 projection = withFar(view_.projection, zFar, zNear);
    r4::gpu::Device &device = screen->device();
    device.beginHostInterop();
    R4Screen::countInterop();
    root_->removeAllChildren();
    if (skyShown_ && sky()) {
        skyPlace_->setMatrix(r4::translate(eye) * r4::scale(r4::Vec3(zFar * kSkyRadius)));
        root_->addChild(skyPlace_);
    }
    for (const r4::scene::NodePtr &scene : scenes_) {
        root_->addChild(scene);
    }
    while (places_.size() < items_.size()) {
        places_.push_back(r4::scene::make<r4::scene::Group>());
    }
    for (size_t i = 0; i < items_.size(); i++) {
        places_[i]->removeAllChildren();
        places_[i]->addChild(items_[i].node);
        places_[i]->setMatrix(items_[i].world);
        root_->addChild(places_[i]);
    }
    for (auto &[key, vertices] : batches_) {
        if (vertices.empty()) {
            continue;
        }
        auto geode = r4::scene::make<r4::scene::Geode>();
        geode->addDrawable({streamMesh(key, vertices), materialOf(key)});
        root_->addChild(geode);
    }

    r4::render::Camera cam;
    cam.position = eye;
    cam.view = view_.view;
    cam.projection = projection;
    cam.znear = zNear;
    cam.zfar = zFar;
    cam.mask = cameraMask_;
    cameraMask_ = 0xffffffffu;
    r4::render::Environment &env = renderer_->environment();
    env.fog = show_fog;
    env.fogStart = fogStart;
    env.fogEnd = fogEnd;
    env.fogColor = settings_.sky ? r4::Vec3{kFogColor[0], kFogColor[1], kFogColor[2]} : settings_.skyColor;
    const Vector3D sun = settings_.sunTowards(object_light_direction);
    r4::Vec3 towards{sun.x, sun.y, sun.z};
    env.lightDirection = -r4::normalize(towards);
    env.time = (float)SDL_GetTicks() / 1000.0f;
    r4::render::RenderOptions &options = renderer_->options();
    options.clear = false;
    // the view of the target camera is 107 x 75: no texture layers, and the LODs as far as in a frame of its height (the renderer
    // multiplies the distances by lodScale)
    const bool targetView = view_.toTexture;
    options.textures = show_textured && !targetView;
    options.rampGouraud = object_gouraud;
    options.flatShading = settings_.flatShading;
    options.lodScale = targetView ? settings_.lodScale * (float)std::max(kTargetHeight, this->height) / kTargetHeight : settings_.lodScale;
    renderer_->setWireframe(settings_.wireframe);
    options.shadows = settings_.shadows && !view_.toTexture;
    options.shadowMapSize = settings_.shadowMapSize;
    options.shadowDistance = settings_.shadowDistance;
    options.shadowCasterReach = 8000;
    options.shadowMask = settings_.treeShadows ? 0xffffffffu : ~kForestMask;
    options.shadowStrength = settings_.shadowStrength;
    renderer_->setTarget(view_.toTexture ? targetView_ : screen->sceneTarget());
    R4Screen::beginGpuTimer(view_.toTexture ? GpuTimer::TARGET_VIEW : GpuTimer::VIEW);
    renderer_->render(*root_, cam, view_.viewport[0], view_.viewport[1], view_.viewport[2], view_.viewport[3]);
    device.endGpuTimer();
    addStats(renderer_->stats());
    root_->removeAllChildren();
    for (size_t i = 0; i < items_.size(); i++) {
        places_[i]->removeAllChildren();
    }
    device.endHostInterop();
    items_.clear();
    scenes_.clear();
    batches_.clear();
    skyShown_ = false;
    R4Screen::counters().flushMs += R4Screen::msSince(start);
}

void R4Renderer::addStats(const r4::render::RenderStats &s) {
    r4::render::RenderStats &t = frame_.sum;
    frame_.passes++;
    t.nodesVisited += s.nodesVisited;
    t.nodesCulled += s.nodesCulled;
    t.drawables += s.drawables;
    t.instancesVisited += s.instancesVisited;
    t.instancesCulled += s.instancesCulled;
    t.opaqueItems += s.opaqueItems;
    t.transparentItems += s.transparentItems;
    t.shadowCasters += s.shadowCasters;
    t.shadowMs += s.shadowMs;
    t.cullMs += s.cullMs;
    t.sortMs += s.sortMs;
    t.uploadMs += s.uploadMs;
    t.drawMs += s.drawMs;
    t.gpu.drawCalls += s.gpu.drawCalls;
    t.gpu.triangles += s.gpu.triangles;
    t.gpu.pipelineChanges += s.gpu.pipelineChanges;
    t.gpu.textureChanges += s.gpu.textureChanges;
}

R4Renderer::FrameStats R4Renderer::takeFrameStats() {
    FrameStats s = frame_;
    frame_ = {};
    return s;
}

void R4Renderer::beginWorld(RSArea *area) {
    useArea(area);
    clearFrame(true, true, settings_.sky ? clearColor() : settings_.skyColor);
    skyShown_ = settings_.sky;
    if (this->show_clouds) {
        renderClouds(area);
    }
}

// Cratères posés en cours de mission (RSArea::addCrater) : leur géométrie est refaite quand la carte en a changé la liste.
void R4Renderer::rebuildCraters(RSArea *area) {
    craters_version_ = area->craters_version;
    std::shared_ptr<r4::assets::Model> model = r4bridge::overlayModel(area, palette, area->craters, r4bridge::OVERLAY_IMAGE_BASE, 64.0f, true);
    if (!model) {
        craters_.reset();
        return;
    }
    R4Screen *screen = R4Screen::current();
    screen->device().beginHostInterop();
    R4Screen::countInterop();
    craters_ = renderer_->upload(*model);
    screen->device().endHostInterop();
    std::printf("[R4Renderer] %zu crateres (version %d)\n", area->craters.size(), craters_version_);
}

const r4::scene::NodePtr &R4Renderer::cratersOf(RSArea *area) {
    if (area != nullptr && area->craters_version != craters_version_) {
        this->rebuildCraters(area);
    }
    return craters_;
}

void R4Renderer::renderWorldSolid(RSArea *area, int LOD, int verticesPerBlock) {
    (void)LOD;
    (void)verticesPerBlock;
    beginWorld(area);
    if (terrain_) {
        items_.push_back({terrain_, r4::Mat4(1.0f)});
    }
    if (overlay_) {
        items_.push_back({overlay_, r4::Mat4(1.0f)});
    }
    if (const r4::scene::NodePtr &craters = this->cratersOf(area)) {
        items_.push_back({craters, r4::Mat4(1.0f)});
    }
    if (forests_) {
        items_.push_back({forests_, r4::Mat4(1.0f)});
    }
    if (buildings_) {
        items_.push_back({buildings_, r4::Mat4(1.0f)});
    }
}

// the terrain is drawn whole by renderWorldSolid
void R4Renderer::renderWorldByID(RSArea *area, int LOD, int verticesPerBlock, int blockId) {
}

void R4Renderer::renderBlock(RSArea *area, int LOD, int i, bool renderTexture, const std::unordered_set<int> *skipRight, const std::unordered_set<int> *skipBottom) {
}

void R4Renderer::renderTexturedTriangle(MapVertex *tri0, MapVertex *tri1, MapVertex *tri2, RSArea *area, int triangleType, RSImage *image, float colors[3][4]) {
}

void R4Renderer::renderColoredTriangle(MapVertex *tri0, MapVertex *tri1, MapVertex *tri2, float colors[3][4]) {
}

void R4Renderer::renderMapOverlay(RSArea *area) {
}

void R4Renderer::renderTerrainDetail(RSArea *area, int LOD, const std::vector<int> &blocks) {
}

void R4Renderer::renderWaterWaves(RSArea *area, int LOD, const std::vector<int> &blocks) {
}

// the view of the target camera: its own target of 107 x 75, read back by getRenderToTexture
void R4Renderer::initRenderToTexture() {
    View v = view_;
    v.viewport[0] = 0;
    v.viewport[1] = 0;
    v.viewport[2] = kTargetWidth;
    v.viewport[3] = kTargetHeight;
    v.toTexture = true;
    setView(v);
    R4Screen *screen = R4Screen::current();
    if (!renderer_ || !screen) {
        return;
    }
    r4::gpu::ClearValues clear;
    if (!settings_.sky) {
        clear.r = settings_.skyColor.x;
        clear.g = settings_.skyColor.y;
        clear.b = settings_.skyColor.z;
    }
    r4::gpu::Device &device = screen->device();
    device.beginHostInterop();
    R4Screen::countInterop();
    device.beginPass(targetView_, 0, 0, kTargetWidth, kTargetHeight, clear);
    device.endPass();
    device.endHostInterop();
}

void R4Renderer::renderWorldToTexture(RSArea *area) {
    useArea(area);
    skyShown_ = settings_.sky;
    if (terrain_) {
        items_.push_back({terrain_, r4::Mat4(1.0f)});
    }
}

void R4Renderer::getRenderToTexture() {
    flush();
    R4Screen *screen = R4Screen::current();
    if (renderer_ && screen) {
        r4::gpu::ClearValues keep;
        keep.color = false;
        keep.depth = false;
        r4::gpu::Device &device = screen->device();
        device.beginHostInterop();
        R4Screen::countInterop();
        device.beginPass(targetView_, 0, 0, kTargetWidth, kTargetHeight, keep);
        const Uint64 start = SDL_GetPerformanceCounter();
        // the image of the frame before: readPixels would wait for the GPU to finish the frame
        device.readPixelsAsync(0, 0, kTargetWidth, kTargetHeight, targetPixels_.data());
        R4Screen::counters().readbacks++;
        R4Screen::counters().readbackMs += (double)(SDL_GetPerformanceCounter() - start) * 1000.0 / (double)SDL_GetPerformanceFrequency();
        device.endPass();
        device.endHostInterop();
    }
    View v = view_;
    v.toTexture = false;
    v.viewport[0] = 0;
    v.viewport[1] = 0;
    v.viewport[2] = this->width;
    v.viewport[3] = this->height;
    setView(v);
}

void R4Renderer::readRenderToTexture(std::vector<uint8_t> &rgba) {
    std::memcpy(rgba.data(), targetPixels_.data(), std::min(rgba.size(), targetPixels_.size()));
}
