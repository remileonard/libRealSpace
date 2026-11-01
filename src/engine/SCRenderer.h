//
//  gfx.h
//  iff
//
//  Created by Fabien Sanglard on 12/21/2013.
//  Copyright (c) 2013 Fabien Sanglard. All rights reserved.
//

#pragma once
#include <unordered_map>
#include <unordered_set>
#include "Camera.h"
#include "../realspace/AssetManager.h"
#include "../realspace/RSMission.h"
#include "../realspace/RSImage.h"
#include "Texture.h"
#ifdef _WIN32
#include <GL/GL.h>
#include <Windows.h>
#include <algorithm>
#endif
#include <SDL_opengl.h>
#include <SDL_opengl_glext.h>
#define MAP_SIZE 200000
class RSArea;
class RSEntity;
struct MapVertex;
struct Triangle;
typedef struct VertexCache {
    MapVertex *lv1;
    MapVertex *lv2;
    MapVertex *lv3;
    MapVertex *uv1;
    MapVertex *uv2;
    MapVertex *uv3;

} VertexCache;


typedef std::vector<VertexCache> VertexVector;
typedef std::unordered_map<int, VertexVector> TextureVertexMap;
// Petit utilitaire pour le culling
struct Plane {
    float a, b, c, d; // ax + by + cz + d = 0
};
struct AABB {
    Vector3D min;
    Vector3D max;
};
struct EffectVertex {
    float x;
    float y;
    float z;
    float u;
    float v;
    float r;
    float g;
    float b;
    float a;
};
enum class BlendMode { Alpha, Additive };
class SCRenderer {
protected:
    inline static std::unique_ptr<SCRenderer> s_instance{};
    bool initialized;
    int scale;
    VGAPalette palette;
    std::vector<PaletteRange> terrain_ranges;
    bool running;
    bool paused;
    uint32_t counter;
    
    
    Point3D playerPosition;
    TextureVertexMap textureSortedVertex;
    GLuint framebuffer;
    
    std::unordered_map<uint64_t, AABB> blockAABBCache_;
    struct AreaAABBCache {
        std::unordered_map<uint64_t, AABB> byKey;
    };
    std::unordered_map<const RSArea*, AreaAABBCache> aabbCache_;

    void extractFrustumPlanes(Plane planes[6]) const;
    static bool isAABBVisible(const AABB& box, const Plane planes[6]);
    const AABB &computeBlockAABB(RSArea *area, int LOD, int blockId);
    void getNormal(RSEntity *object, Triangle *triangle, Vector3D *normal);
    void renderWorldSkyAndGround();
    void renderSkydome(int rings, int slices);

public:
    Camera camera;
    float verticalOffset{0.45f};
    float fov{45.0f};
    int max_view_distance{160000};
    bool show_fog{true};
    
    bool show_textured{true};
    bool show_clouds{false};
    int lodLevel{0};
    GLuint texture{0};

    int terrain_detail{0};
    GLuint detail_tex{0};
    int water_waves{0};
    GLuint wave_tex{0};
    int texture_filter{0};
    float max_anisotropy{1.0f};

    int32_t width;
    int32_t height;
    Point3D light;
    Vector3D object_light_direction{0.0f, 1.0f, 0.0f};
    float object_light_ambient{0.0f};
    float object_light_share{0.0f};
    Vector3D shading_light_local{0.0f, 1.0f, 0.0f};
    Vector3D shading_eye_local{0.0f, 0.0f, 0.0f};
    std::unordered_map<RSEntity *, std::vector<Vector3D>> object_normals;
    std::vector<Vector3D> &objectNormals(RSEntity *object);

    SCRenderer();
    virtual ~SCRenderer();

    static SCRenderer& getInstance() {
        if (!SCRenderer::hasInstance()) {
            SCRenderer::setInstance(std::make_unique<SCRenderer>());
        }
        SCRenderer& instance = SCRenderer::instance();
        return instance;
    };
    static SCRenderer& instance() {
        return *s_instance;
    }
    static void setInstance(std::unique_ptr<SCRenderer> inst) {
        s_instance = std::move(inst);
    }

    static bool hasInstance() { return (bool)s_instance; }
    virtual int nearestPaletteColour(int r, int g, int b);
    virtual void ensureDetailTexture();
    virtual void renderTerrainDetail(RSArea *area, int LOD, const std::vector<int> &blocks);
    virtual void ensureWaveTexture();
    virtual void renderWaterWaves(RSArea *area, int LOD, const std::vector<int> &blocks);
    virtual void applyTextureFilter(const Texture *texture); // for the texture currently bound, call before glTexImage2D
    virtual void init(int width, int height);
    virtual void clear(void);
    virtual void resetCameraPerspective();
    virtual void drawParticle(Vector3D pos, float alpha);
    virtual void drawModel(RSEntity *object, size_t lodLevel, Vector3D position, Vector3D orientation, Vector3D ajustement, float scale);
    virtual void drawModel(RSEntity *object, size_t lodLevel, Vector3D position, Vector3D orientation, Vector3D ajustement);
    virtual void drawModel(RSEntity *object, size_t lodLevel, Vector3D position, Vector3D orientation);
    virtual void drawModel(RSEntity *object, Vector3D position, Vector3D orientation);
    virtual void drawLine(Vector3D start, Vector3D end, Vector3D color, Vector3D orientation);
    virtual void drawLine(Vector3D start, Vector3D end, Vector3D color, Vector3D orientation, Vector3D position);
    virtual void drawLine(Vector3D start, Vector3D end, Vector3D color);
    virtual void drawPoint(Vector3D point, Vector3D color, Vector3D pos, Vector3D orientation);
    virtual void drawSprite(Vector3D pos, Texture *tex, float zoom);
    virtual void drawModelWithChilds(RSEntity *object, size_t lodLevel, Vector3D position, Vector3D orientation, int wheel_index, int thrust, std::vector<std::tuple<Vector3D, RSEntity *>> weaps_load);
    virtual void drawModel(RSEntity *object, size_t lodLevel);
    virtual void drawModelColorPass(RSEntity *object, size_t lodLevel, std::vector<Vector3D> &vertexNormals, float ambientLamber, Vector3D lightEye, float *MV);
    virtual void drawModelTexturePass(RSEntity *object, size_t lodLevel, std::vector<Vector3D> &vertexNormals, float ambientLamber, Vector3D lightEye, float *MV);
    virtual void drawModelTransparentPass(RSEntity *object, size_t lodLevel, std::vector<Vector3D> &vertexNormals, float ambientLamber, Vector3D lightEye, float *MV);
    virtual void drawBillboard(Vector3D pos, Texture *tex, float size, float alpha = 1.0f);
    virtual unsigned int createTexture(int w, int h, const unsigned char *rgba, bool repeatT);
    virtual void drawTriangles(const EffectVertex *vertices, size_t count, unsigned int texture, BlendMode blend, bool depthWrite = false);
    virtual void drawTracer(Vector3D pos, Vector3D velocityDir, float length, float pixelWidth, Vector3D color, float alpha);
    virtual void displayModel(RSEntity *object, size_t lodLevel);
    virtual void createTextureInGPU(Texture *texture);
    virtual void uploadTextureContentToGPU(Texture *texture);
    virtual void deleteTextureInGPU(Texture *texture);
    virtual void drawTexturedQuad(Vector3D pos, Vector3D orientation,  std::vector<Vector3D> quad, Texture *tex);
    virtual void renderEllipsoid(float cx, float cy, float cz, float rx, float ry, float rz, int rings, int slices, float r, float g, float b, float baseAlpha);
    virtual VGAPalette *getPalette(void);
    virtual void renderClouds(RSArea *area);
    virtual void renderTexturedTriangle(MapVertex *tri0, MapVertex *tri1, MapVertex *tri2, RSArea *area, int triangleType, RSImage *image, float colors[3][4]);
    virtual void renderColoredTriangle(MapVertex *tri0, MapVertex *tri1, MapVertex *tri2, float colors[3][4]);
    virtual bool isTextured(MapVertex *tri0, MapVertex *tri1, MapVertex *tri2);
    virtual void renderQuad(MapVertex *currentVertex, MapVertex *rightVertex, MapVertex *bottomRightVertex, MapVertex *bottomVertex, RSArea *area);
    virtual void renderBlock(RSArea *area, int LOD, int i, bool renderTexture, const std::unordered_set<int>* skipRight  = nullptr, const std::unordered_set<int>* skipBottom = nullptr);
    virtual void renderWorldSolid(RSArea *area, int LOD, int verticesPerBlock);
    virtual void renderWorldByID(RSArea *area, int LOD, int verticesPerBlock, int blockId);
    virtual void renderObjects(RSArea *area, size_t blockID);
    virtual void renderLineCube(Vector3D position, int32_t size);
    virtual void renderBBox(Vector3D position, Point3D min, Point3D max);
    virtual Camera *getCamera(void);
    virtual void setLight(Point3D *position);
    virtual void setObjectLight(int ambient_weight, int light_weight, int c0, int c1, int c2);
    virtual void setClearColor(uint8_t red, uint8_t green, uint8_t blue);
    virtual void prepare(RSEntity *object);
    virtual void renderMissionObjects(RSMission *mission);
    virtual void renderMapOverlay(RSArea *area);
    virtual void setPlayerPosition(Point3D *position);
    virtual void renderWorldToTexture(RSArea *area);
    virtual void initRenderToTexture();
    virtual void getRenderToTexture();
    virtual void initRenderCameraView();
    virtual void InvalidateAABBCache();
    virtual void InvalidateAABBCache(RSArea* area);
    virtual void InvalidateAABBForBlock(RSArea* area, int LOD, int blockId);
    virtual void PrecomputeAABBs(RSArea* area, int minLOD, int maxLOD);
    virtual void bindCameraProjectionAndView(float verticalOffset);
    virtual void bindCameraProjectionAndViewViewport(int32_t viewportW, int32_t viewportH, float verticalOffset);
    virtual float objectIntensity(Vector3D normal);
    virtual Vector3D shadeAlongRamp(int colour, float intensity);
};
