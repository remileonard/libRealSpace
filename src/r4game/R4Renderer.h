//
//  R4Renderer.h
//  libRealSpace
//
//  The renderer of the game on R4nkine, without any GL call. The calls of the game are kept: the models with the model matrix of
//  the renderer, the effects as triangles, lines and points batched by state, the sky, the graphs of the scenes. A pass of the
//  renderer draws them with the camera of the game before anything that needs the frame: a change of view, the 320 x 200 screen,
//  the end of the frame.
//
#pragma once

#include "engine/SCRenderer.h"

#include "R4World.h"

#include "r4/render/renderer.h"
#include "r4/scene/node.h"

#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

// the options of the section [R4nkine] of config.ini; the defaults keep the look of the game
struct R4Settings {
    bool shadows = false;
    int shadowMapSize = 2048;
    float shadowDistance = 1500;
    bool treeShadows = false;
    bool forests = false;
    r4bridge::ForestMode forestMode = r4bridge::ForestMode::NOISE;
    float forestDensity = 1;
    bool buildings = false;
    r4::assets::Upscale upscale = r4::assets::Upscale::NONE;
    bool mipmaps = false;
    float anisotropy = 1;
    // the terrain lit by the sun along its ramps, its normals from the relief times terrainRelief, rather than by the light of the data
    bool sunTerrain = false;
    float terrainRelief = 4;
    // the part of their color that the unlit (the terrain of the data, the runways) lose in the shadow
    float shadowStrength = 0.5f;
    // the sun of the settings rather than the light of the objects of the game; degrees, the heading from the north towards the east
    bool sunOverride = false;
    float sunHeading = 0;
    float sunElevation = 45;
    // where the light comes from, in libRealSpace's axes
    Vector3D sunTowards(const Vector3D &game) const;
    // the light of the objects: ambient + direct x max(N.L, 0); without the keys, the one of the game (setObjectLight)
    bool lightOverride = false;
    float ambient = 0.375f;
    float direct = 0.625f;
    // the fog in km, from the eye; without the keys, the one of the game (0.40 and 0.93 x max_view_distance)
    bool fogOverride = false;
    float fogStartKm = 64;
    float fogEndKm = 149;
    bool wireframe = false;
    bool flatShading = false;
    // multiplies the distances of the LODs of the terrain, of the objects and of the trees
    float lodScale = 1;
    // the levels of the terrain: by distance, or one level everywhere
    enum class TerrainLod {
        BY_DISTANCE,
        FINE,
        MEDIUM,
        COARSE
    };
    TerrainLod terrainLod = TerrainLod::BY_DISTANCE;
    bool vsync = true;
    // reads config.ini
    void load();
    // a key of [R4nkine], fallback when it is not there
    static bool has(const char *key);
    static float number(const char *key, float fallback);
    static bool flag(const char *key, bool fallback);
    static r4::Vec3 color(const char *key, r4::Vec3 fallback);
};

class R4Renderer : public SCRenderer {
public:
    // the mask of the forests, out of the shadow pass without treeShadows
    static constexpr uint32_t kForestMask = 1u << 31;

    R4Renderer();
    ~R4Renderer() override;

    // SCRenderer::instance() when the renderer of the game is a R4Renderer, else null
    static R4Renderer *current();

    bool active() const {
        return renderer_ != nullptr;
    }
    // draws what was kept since the last flush
    void flush();
    // frees the GPU objects, before the device of the screen goes
    void release();

    // a graph drawn by the next pass, in world space (libRealSpace axes)
    void addScene(const r4::scene::NodePtr &root);
    // the frame cleared, then the sky and the clouds of the area in the next pass, as the start of renderWorldSolid
    void beginWorld(RSArea *area);
    // the node of an entity, made at its first use
    r4::scene::NodePtr modelOf(RSEntity *object);
    // the terrain, the runways, the forests and the buildings of the area, made when the area or keepOut changes; null without them
    // keepOut: x, z, radius in libRealSpace where no tree nor building goes; null keeps the one of the area
    void useArea(RSArea *area, const std::vector<r4::Vec3> *keepOut = nullptr);
    const r4::scene::NodePtr &forests() const {
        return forests_;
    }
    const r4::scene::NodePtr &buildings() const {
        return buildings_;
    }
    R4Settings &settings() {
        return settings_;
    }
    const r4::scene::NodePtr &terrain() const {
        return terrain_;
    }
    const r4::scene::NodePtr &overlay() const {
        return overlay_;
    }

    using SCRenderer::drawLine;
    using SCRenderer::drawModel;

    void init(int width, int height) override;
    void clear(void) override;
    void clearDepth() override;
    void setClearColor(uint8_t red, uint8_t green, uint8_t blue) override;
    void setObjectLight(int ambient_weight, int light_weight, int c0, int c1, int c2) override;

    void bindCameraProjectionAndViewViewport(int32_t viewportW, int32_t viewportH, float verticalOffset) override;
    void initRenderCameraView() override;
    void pushMatrix() override;
    void popMatrix() override;
    void translateMatrix(float x, float y, float z) override;
    void rotateMatrix(float degrees, float x, float y, float z) override;
    void scaleMatrix(float x, float y, float z) override;

    void drawModel(RSEntity *object, size_t lodLevel, Vector3D position, Vector3D orientation, Vector3D ajustement, float scale) override;
    void drawModel(RSEntity *object, size_t lodLevel, Vector3D position, Vector3D orientation, Vector3D ajustement) override;
    void drawModel(RSEntity *object, size_t lodLevel, Vector3D position, Vector3D orientation) override;
    void drawModel(RSEntity *object, Vector3D position, Vector3D orientation) override;
    void drawModel(RSEntity *object, size_t lodLevel) override;
    void drawModelWithChilds(RSEntity *object, size_t lodLevel, Vector3D position, Vector3D orientation, int wheel_index, int thrust, std::vector<std::tuple<Vector3D, RSEntity *>> weaps_load) override;
    void drawModelColorPass(RSEntity *object, size_t lodLevel, std::vector<Vector3D> &vertexNormals, float ambientLamber, Vector3D lightEye, float *MV) override;
    void drawModelTexturePass(RSEntity *object, size_t lodLevel, std::vector<Vector3D> &vertexNormals, float ambientLamber, Vector3D lightEye, float *MV) override;
    void drawModelTransparentPass(RSEntity *object, size_t lodLevel, std::vector<Vector3D> &vertexNormals, float ambientLamber, Vector3D lightEye, float *MV) override;
    void displayModel(RSEntity *object, size_t lodLevel) override;
    void renderObjects(RSArea *area, size_t blockID) override;
    void renderMissionObjects(RSMission *mission) override;

    void drawParticle(Vector3D pos, float alpha) override;
    void drawLine(Vector3D start, Vector3D end, Vector3D color, Vector3D orientation) override;
    void drawLine(Vector3D start, Vector3D end, Vector3D color, Vector3D orientation, Vector3D position) override;
    void drawLine(Vector3D start, Vector3D end, Vector3D color) override;
    void drawPoint(Vector3D point, Vector3D color, Vector3D pos, Vector3D orientation) override;
    void drawSprite(Vector3D pos, Texture *tex, float zoom) override;
    void drawBillboard(Vector3D pos, Texture *tex, float size, float alpha = 1.0f) override;
    void drawTriangles(const EffectVertex *vertices, size_t count, unsigned int texture, BlendMode blend, bool depthWrite = false) override;
    void drawTracer(Vector3D pos, Vector3D velocityDir, float length, float pixelWidth, Vector3D color, float alpha) override;
    void drawTexturedQuad(Vector3D pos, Vector3D orientation, std::vector<Vector3D> quad, Texture *tex) override;
    void renderEllipsoid(float cx, float cy, float cz, float rx, float ry, float rz, int rings, int slices, float r, float g, float b, float baseAlpha) override;
    void renderClouds(RSArea *area) override;
    void renderLineCube(Vector3D position, int32_t size) override;
    void renderBBox(Vector3D position, Point3D min, Point3D max) override;

    unsigned int createTexture(int w, int h, const unsigned char *rgba, bool repeatT) override;
    void createTextureInGPU(Texture *texture) override;
    void uploadTextureContentToGPU(Texture *texture) override;
    void deleteTextureInGPU(Texture *texture) override;
    void applyTextureFilter(const Texture *texture) override;
    void ensureDetailTexture() override;
    void ensureWaveTexture() override;

    void renderWorldSolid(RSArea *area, int LOD, int verticesPerBlock) override;
    void renderWorldByID(RSArea *area, int LOD, int verticesPerBlock, int blockId) override;
    void renderBlock(RSArea *area, int LOD, int i, bool renderTexture, const std::unordered_set<int> *skipRight = nullptr, const std::unordered_set<int> *skipBottom = nullptr) override;
    void renderTexturedTriangle(MapVertex *tri0, MapVertex *tri1, MapVertex *tri2, RSArea *area, int triangleType, RSImage *image, float colors[3][4]) override;
    void renderColoredTriangle(MapVertex *tri0, MapVertex *tri1, MapVertex *tri2, float colors[3][4]) override;
    void renderMapOverlay(RSArea *area) override;
    void renderTerrainDetail(RSArea *area, int LOD, const std::vector<int> &blocks) override;
    void renderWaterWaves(RSArea *area, int LOD, const std::vector<int> &blocks) override;

    void initRenderToTexture() override;
    void renderWorldToTexture(RSArea *area) override;
    void getRenderToTexture() override;
    void readRenderToTexture(std::vector<uint8_t> &rgba) override;

private:
    enum class Blend {
        NONE,
        ALPHA,
        ADDITIVE
    };
    // the state of a batch of effects: one mesh and one material each pass
    struct BatchKey {
        uint32_t texture = 0;
        Blend blend = Blend::NONE;
        bool depthTest = true;
        bool depthWrite = true;
        bool alphaTest = false;
        r4::assets::Primitive primitive = r4::assets::Primitive::TRIANGLES;
        int order = 0;
        bool operator<(const BatchKey &o) const;
    };
    // the camera, the viewport and the target of a pass
    struct View {
        r4::Mat4 projection{1.0f};
        r4::Mat4 view{1.0f};
        int viewport[4] = {0, 0, 0, 0};
        bool toTexture = false;
    };
    struct Item {
        r4::scene::NodePtr node;
        r4::Mat4 world;
    };
    struct TextureEntry {
        r4::gpu::TextureHandle handle;
        size_t width = 0;
        size_t height = 0;
    };

    bool pending() const;
    void setView(const View &v);
    View cameraView(int x, int y, int width, int height, float verticalOffset);
    const r4::Mat4 &model() const {
        return stack_.back();
    }
    void placeModel(RSEntity *object, const r4::Mat4 &matrix);
    std::vector<r4::assets::Vertex> &batch(const BatchKey &key);
    void vertex(std::vector<r4::assets::Vertex> &to, Vector3D p, float u, float v, r4::Vec4 color);
    void quad(const BatchKey &key, const Vector3D corners[4], r4::Vec4 color);
    void lines(const Vector3D *ends, size_t count, r4::Vec4 color);
    void cubeLines(const r4::Mat4 &matrix, r4::Vec4 front, r4::Vec4 back, r4::Vec4 sides);
    r4::gpu::TextureHandle textureOf(Texture *tex);
    std::shared_ptr<r4::render::MaterialInstance> materialOf(const BatchKey &key);
    r4::scene::NodePtr sky();
    void clearFrame(bool color, bool depth);
    r4::scene::NodePtr makeForests(RSArea *area, size_t &trees);
    r4bridge::Lighting objectLighting() const;
    void applyTerrainLod();

    std::unique_ptr<r4::render::Renderer> renderer_;
    std::unordered_map<RSEntity *, r4::scene::NodePtr> models_;
    RSArea *area_ = nullptr;
    r4::scene::NodePtr terrain_;
    r4::scene::NodePtr overlay_;
    r4::scene::NodePtr forests_;
    r4::scene::NodePtr buildings_;
    std::vector<r4::Vec3> keepOut_;
    R4Settings settings_;
    std::vector<r4::Mat4> stack_{r4::Mat4(1.0f)};
    View view_;
    std::vector<Item> items_;
    std::vector<r4::scene::NodePtr> scenes_;
    std::map<BatchKey, std::vector<r4::assets::Vertex>> batches_;
    std::map<BatchKey, std::shared_ptr<r4::render::MaterialInstance>> materials_;
    bool skyShown_ = false;
    r4::scene::NodePtr sky_;
    std::shared_ptr<r4::scene::Group> skyPlace_;
    std::shared_ptr<r4::scene::Group> root_;
    std::vector<std::shared_ptr<r4::scene::Group>> places_;
    std::unordered_map<const Texture *, TextureEntry> textures_;
    std::unordered_map<unsigned int, r4::gpu::TextureHandle> effectTextures_;
    unsigned int nextEffectTexture_ = 1;
    r4::gpu::RenderTargetHandle targetView_;
    std::vector<uint8_t> targetPixels_;
    float clearColor_[3] = {0, 0, 0};
};
