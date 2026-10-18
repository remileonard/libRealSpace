//
//  GL4Terrain.h
//  Optional shader/VBO render path for the terrain (renderer = gl4).
//
//  The legacy fixed-function path stays the default and the fallback.
//  This path runs inside the same (compatibility) OpenGL context, so
//  the rest of the engine keeps drawing with the legacy code.
//
#pragma once
#include <SDL_opengl.h>
#include <map>
#include <tuple>
#include <vector>

class SCRenderer;
class RSArea;

class GL4Terrain {
public:
    GL4Terrain();
    ~GL4Terrain(); // deliberately no GL calls: the context may already be gone at exit

    // Needs a current GL context. Returns false (and logs why) if the path is unusable.
    bool init();
    bool ready() const { return program != 0; }

    // Frees all cached block meshes (area change, settings change).
    void clear();

    // Draws the terrain (untextured + textured pass) for the given visible blocks.
    // Expects the caller to have set depth/cull/front-face state like the legacy path.
    void draw(SCRenderer &r, RSArea *area, int LOD, const std::vector<int> &blocks);

private:
    struct TexRange {
        int imageId;
        GLint first;
        GLsizei count;
    };
    struct Mesh {
        GLuint vbo{0};
        GLsizei plainCount{0};
        std::vector<TexRange> ranges;
    };
    typedef std::tuple<const RSArea *, int, int> Key;

    Mesh &getMesh(SCRenderer &r, RSArea *area, int LOD, int blockId);
    void build(SCRenderer &r, RSArea *area, int LOD, int blockId, Mesh &out);
    void bindMesh(const Mesh &m);

    std::map<Key, Mesh> meshes;
    GLuint program{0};
    GLuint vao{0};
    bool builtTextured{true};
    bool builtFlat{false};

    GLint uProj{-1}, uMV{-1}, uPalette{-1}, uShadowLift{-1}, uShadowRange{-1};
    GLint uTextured{-1}, uTex{-1}, uFog{-1}, uFogStart{-1}, uFogEnd{-1}, uFogColor{-1};
};
