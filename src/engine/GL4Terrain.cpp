//
//  GL4Terrain.cpp
//  Shader/VBO terrain renderer, see GL4Terrain.h
//
#include "SCRenderer.h"
#include "GL4Terrain.h"
#include "Texture.h"
#include "../realspace/RSArea.h"
#include "../realspace/RSEntity.h"
#include "../realspace/RSImage.h"
#include "../realspace/block_def.h"
#include <SDL.h>
#include <algorithm>
#include <cstddef>
#include <cstdio>

// UV tables of the legacy path (SCRenderer.cpp)
extern float textTrianCoo[2][3][2];
extern float textTrianCoo64[2][3][2];

#ifndef APIENTRY
#define APIENTRY
#endif

namespace {

typedef GLuint (APIENTRY *FnCreateShader)(GLenum);
typedef void (APIENTRY *FnShaderSource)(GLuint, GLsizei, const char *const *, const GLint *);
typedef void (APIENTRY *FnCompileShader)(GLuint);
typedef void (APIENTRY *FnGetShaderiv)(GLuint, GLenum, GLint *);
typedef void (APIENTRY *FnGetShaderInfoLog)(GLuint, GLsizei, GLsizei *, char *);
typedef void (APIENTRY *FnDeleteShader)(GLuint);
typedef GLuint (APIENTRY *FnCreateProgram)(void);
typedef void (APIENTRY *FnAttachShader)(GLuint, GLuint);
typedef void (APIENTRY *FnLinkProgram)(GLuint);
typedef void (APIENTRY *FnGetProgramiv)(GLuint, GLenum, GLint *);
typedef void (APIENTRY *FnGetProgramInfoLog)(GLuint, GLsizei, GLsizei *, char *);
typedef void (APIENTRY *FnDeleteProgram)(GLuint);
typedef void (APIENTRY *FnUseProgram)(GLuint);
typedef GLint (APIENTRY *FnGetUniformLocation)(GLuint, const char *);
typedef void (APIENTRY *FnUniform1i)(GLint, GLint);
typedef void (APIENTRY *FnUniform1f)(GLint, GLfloat);
typedef void (APIENTRY *FnUniform3f)(GLint, GLfloat, GLfloat, GLfloat);
typedef void (APIENTRY *FnUniform3fv)(GLint, GLsizei, const GLfloat *);
typedef void (APIENTRY *FnUniformMatrix4fv)(GLint, GLsizei, GLboolean, const GLfloat *);
typedef void (APIENTRY *FnGenBuffers)(GLsizei, GLuint *);
typedef void (APIENTRY *FnBindBuffer)(GLenum, GLuint);
typedef void (APIENTRY *FnBufferData)(GLenum, ptrdiff_t, const void *, GLenum);
typedef void (APIENTRY *FnDeleteBuffers)(GLsizei, const GLuint *);
typedef void (APIENTRY *FnGenVertexArrays)(GLsizei, GLuint *);
typedef void (APIENTRY *FnBindVertexArray)(GLuint);
typedef void (APIENTRY *FnEnableVertexAttribArray)(GLuint);
typedef void (APIENTRY *FnVertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void *);

struct GLFuncs {
    FnCreateShader CreateShader{};
    FnShaderSource ShaderSource{};
    FnCompileShader CompileShader{};
    FnGetShaderiv GetShaderiv{};
    FnGetShaderInfoLog GetShaderInfoLog{};
    FnDeleteShader DeleteShader{};
    FnCreateProgram CreateProgram{};
    FnAttachShader AttachShader{};
    FnLinkProgram LinkProgram{};
    FnGetProgramiv GetProgramiv{};
    FnGetProgramInfoLog GetProgramInfoLog{};
    FnDeleteProgram DeleteProgram{};
    FnUseProgram UseProgram{};
    FnGetUniformLocation GetUniformLocation{};
    FnUniform1i Uniform1i{};
    FnUniform1f Uniform1f{};
    FnUniform3f Uniform3f{};
    FnUniform3fv Uniform3fv{};
    FnUniformMatrix4fv UniformMatrix4fv{};
    FnGenBuffers GenBuffers{};
    FnBindBuffer BindBuffer{};
    FnBufferData BufferData{};
    FnDeleteBuffers DeleteBuffers{};
    FnGenVertexArrays GenVertexArrays{};
    FnBindVertexArray BindVertexArray{};
    FnEnableVertexAttribArray EnableVertexAttribArray{};
    FnVertexAttribPointer VertexAttribPointer{};
} gl;

template <typename T> bool loadProc(T &fn, const char *name) {
    fn = reinterpret_cast<T>(SDL_GL_GetProcAddress(name));
    if (!fn) {
        printf("[GL4] missing OpenGL function %s\n", name);
        return false;
    }
    return true;
}

bool loadFuncs() {
    bool ok = true;
    ok &= loadProc(gl.CreateShader, "glCreateShader");
    ok &= loadProc(gl.ShaderSource, "glShaderSource");
    ok &= loadProc(gl.CompileShader, "glCompileShader");
    ok &= loadProc(gl.GetShaderiv, "glGetShaderiv");
    ok &= loadProc(gl.GetShaderInfoLog, "glGetShaderInfoLog");
    ok &= loadProc(gl.DeleteShader, "glDeleteShader");
    ok &= loadProc(gl.CreateProgram, "glCreateProgram");
    ok &= loadProc(gl.AttachShader, "glAttachShader");
    ok &= loadProc(gl.LinkProgram, "glLinkProgram");
    ok &= loadProc(gl.GetProgramiv, "glGetProgramiv");
    ok &= loadProc(gl.GetProgramInfoLog, "glGetProgramInfoLog");
    ok &= loadProc(gl.DeleteProgram, "glDeleteProgram");
    ok &= loadProc(gl.UseProgram, "glUseProgram");
    ok &= loadProc(gl.GetUniformLocation, "glGetUniformLocation");
    ok &= loadProc(gl.Uniform1i, "glUniform1i");
    ok &= loadProc(gl.Uniform1f, "glUniform1f");
    ok &= loadProc(gl.Uniform3f, "glUniform3f");
    ok &= loadProc(gl.Uniform3fv, "glUniform3fv");
    ok &= loadProc(gl.UniformMatrix4fv, "glUniformMatrix4fv");
    ok &= loadProc(gl.GenBuffers, "glGenBuffers");
    ok &= loadProc(gl.BindBuffer, "glBindBuffer");
    ok &= loadProc(gl.BufferData, "glBufferData");
    ok &= loadProc(gl.DeleteBuffers, "glDeleteBuffers");
    ok &= loadProc(gl.GenVertexArrays, "glGenVertexArrays");
    ok &= loadProc(gl.BindVertexArray, "glBindVertexArray");
    ok &= loadProc(gl.EnableVertexAttribArray, "glEnableVertexAttribArray");
    ok &= loadProc(gl.VertexAttribPointer, "glVertexAttribPointer");
    return ok;
}

// Same math as the fixed-function path: palette colour, shadow lift (SCRenderer::toneColor),
// linear fog on eye-space depth, GL_DECAL texturing (colour replaced by the texture where alpha = 1).
const char *kVertexShader = R"GLSL(#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in float aTone;
layout(location = 2) in vec2 aUV;
uniform mat4 uProj;
uniform mat4 uMV;
uniform vec3 uPalette[256];
uniform float uShadowLift;
uniform float uShadowRange;
out vec3 vColor;
out vec2 vUV;
out float vEyeDist;
void main() {
    vec3 c = uPalette[int(aTone + 0.5)];
    float lum = dot(c, vec3(0.299, 0.587, 0.114));
    if (lum > 0.0 && lum < uShadowRange) {
        float depth = 1.0 - lum / uShadowRange;
        float factor = (lum + uShadowLift * depth * depth) / lum;
        c = min(c * factor, vec3(1.0));
    }
    vec4 eye = uMV * vec4(aPos, 1.0);
    vEyeDist = abs(eye.z);
    vColor = c;
    vUV = aUV;
    gl_Position = uProj * eye;
}
)GLSL";

const char *kFragmentShader = R"GLSL(#version 330 core
in vec3 vColor;
in vec2 vUV;
in float vEyeDist;
uniform sampler2D uTex;
uniform int uTextured;
uniform int uFog;
uniform float uFogStart;
uniform float uFogEnd;
uniform vec3 uFogColor;
out vec4 oColor;
void main() {
    vec3 c = vColor;
    if (uTextured != 0) {
        vec4 t = texture(uTex, vUV);
        c = mix(c, t.rgb, t.a);
    }
    if (uFog != 0) {
        float f = clamp((uFogEnd - vEyeDist) / (uFogEnd - uFogStart), 0.0, 1.0);
        c = mix(uFogColor, c, f);
    }
    oColor = vec4(c, 1.0);
}
)GLSL";

GLuint compileShader(GLenum type, const char *src, const char *what) {
    GLuint s = gl.CreateShader(type);
    gl.ShaderSource(s, 1, &src, nullptr);
    gl.CompileShader(s);
    GLint ok = 0;
    gl.GetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048] = {0};
        gl.GetShaderInfoLog(s, sizeof(log), nullptr, log);
        printf("[GL4] %s shader error:\n%s\n", what, log);
        gl.DeleteShader(s);
        return 0;
    }
    return s;
}

struct TerrainVertex {
    float x, y, z;
    float tone;
    float u, v;
};
static_assert(sizeof(TerrainVertex) == 24, "TerrainVertex layout");

} // namespace

GL4Terrain::GL4Terrain() {}

GL4Terrain::~GL4Terrain() {}

bool GL4Terrain::init() {
    const char *ver = (const char *)glGetString(GL_VERSION);
    int major = 0, minor = 0;
    if (ver) {
        sscanf(ver, "%d.%d", &major, &minor);
    }
    printf("[GL4] OpenGL %s\n", ver ? ver : "(unknown)");
    if (major < 3 || (major == 3 && minor < 3)) {
        printf("[GL4] needs OpenGL 3.3 or newer, falling back to the legacy renderer\n");
        return false;
    }
    if (!loadFuncs()) {
        printf("[GL4] could not load OpenGL functions, falling back to the legacy renderer\n");
        return false;
    }
    GLuint vs = compileShader(GL_VERTEX_SHADER, kVertexShader, "vertex");
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, kFragmentShader, "fragment");
    if (!vs || !fs) {
        return false;
    }
    GLuint p = gl.CreateProgram();
    gl.AttachShader(p, vs);
    gl.AttachShader(p, fs);
    gl.LinkProgram(p);
    gl.DeleteShader(vs);
    gl.DeleteShader(fs);
    GLint ok = 0;
    gl.GetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048] = {0};
        gl.GetProgramInfoLog(p, sizeof(log), nullptr, log);
        printf("[GL4] program link error:\n%s\n", log);
        gl.DeleteProgram(p);
        return false;
    }
    program = p;
    uProj = gl.GetUniformLocation(p, "uProj");
    uMV = gl.GetUniformLocation(p, "uMV");
    uPalette = gl.GetUniformLocation(p, "uPalette");
    uShadowLift = gl.GetUniformLocation(p, "uShadowLift");
    uShadowRange = gl.GetUniformLocation(p, "uShadowRange");
    uTextured = gl.GetUniformLocation(p, "uTextured");
    uTex = gl.GetUniformLocation(p, "uTex");
    uFog = gl.GetUniformLocation(p, "uFog");
    uFogStart = gl.GetUniformLocation(p, "uFogStart");
    uFogEnd = gl.GetUniformLocation(p, "uFogEnd");
    uFogColor = gl.GetUniformLocation(p, "uFogColor");
    gl.GenVertexArrays(1, &vao);
    printf("[GL4] terrain shader ready\n");
    return true;
}

void GL4Terrain::clear() {
    if (program) {
        for (auto &kv : meshes) {
            if (kv.second.vbo) {
                gl.DeleteBuffers(1, &kv.second.vbo);
            }
        }
    }
    meshes.clear();
}

GL4Terrain::Mesh &GL4Terrain::getMesh(SCRenderer &r, RSArea *area, int LOD, int blockId) {
    Key key(area, LOD, blockId);
    auto it = meshes.find(key);
    if (it != meshes.end()) {
        return it->second;
    }
    Mesh &m = meshes[key];
    build(r, area, LOD, blockId, m);
    return m;
}

// Mirrors SCRenderer::renderBlock / renderQuad / triangleColors / renderTexturedTriangle,
// but writes the triangles into a vertex buffer instead of calling glVertex.
void GL4Terrain::build(SCRenderer &r, RSArea *area, int LOD, int blockId, Mesh &out) {
    AreaBlock *block = area->GetAreaBlockByID(LOD, blockId);
    if (block == nullptr) {
        return;
    }
    const bool showTex = r.show_textured;
    const uint32_t sideSize = block->sideSize;

    std::vector<TerrainVertex> plain;
    std::map<int, std::vector<TerrainVertex>> byTex;
    std::map<int, bool> smallImage; // image width < 128 -> other UV table

    auto emit = [&](std::vector<TerrainVertex> &dst, MapVertex *a, MapVertex *b, MapVertex *c,
                    const float (*uv)[2]) {
        MapVertex *vs[3] = {a, b, c};
        uint8_t base = r.highestVertex(a, b, c)->type;
        int tones[3];
        for (int i = 0; i < 3; i++) {
            tones[i] = r.terrainTone(base, vs[i]->flag);
        }
        if (r.terrain_flat) {
            int avg = (tones[0] + tones[1] + tones[2]) / 3;
            tones[0] = tones[1] = tones[2] = avg;
        }
        for (int i = 0; i < 3; i++) {
            TerrainVertex v;
            v.x = (float)vs[i]->v.x;
            v.y = (float)vs[i]->v.y;
            v.z = (float)vs[i]->v.z;
            v.tone = (float)tones[i];
            v.u = uv ? uv[i][0] : 0.0f;
            v.v = uv ? uv[i][1] : 0.0f;
            dst.push_back(v);
        }
    };

    auto texTri = [&](int imageId, int triangleType, MapVertex *a, MapVertex *b, MapVertex *c) {
        auto sm = smallImage.find(imageId);
        if (sm == smallImage.end()) {
            RSImage *img = area->GetImageByID(imageId);
            if (!img) {
                return;
            }
            sm = smallImage.emplace(imageId, img->width < 128).first;
        }
        const float (*coords)[3][2] = sm->second ? textTrianCoo64 : textTrianCoo;
        emit(byTex[imageId], a, b, c, coords[triangleType]);
    };

    auto quad = [&](MapVertex *cur, MapVertex *right, MapVertex *bottomRight, MapVertex *bottom) {
        if (cur->lowerImageID == 0xFF || !showTex) {
            emit(plain, cur, bottomRight, bottom, nullptr);
        }
        if (cur->upperImageID == 0xFF || !showTex) {
            emit(plain, cur, right, bottomRight, nullptr);
        }
        if (showTex) {
            if (cur->lowerImageID != 0xFF) {
                texTri(cur->lowerImageID, 0, cur, bottomRight, bottom);
            }
            if (cur->upperImageID != 0xFF) {
                texTri(cur->upperImageID, 1, cur, right, bottomRight);
            }
        }
    };

    for (uint32_t x = 0; x < sideSize - 1; x++) {
        for (uint32_t y = 0; y < sideSize - 1; y++) {
            quad(&block->vertice[x + y * sideSize], &block->vertice[(x + 1) + y * sideSize],
                 &block->vertice[(x + 1) + (y + 1) * sideSize], &block->vertice[x + (y + 1) * sideSize]);
        }
    }

    // Seam to the right neighbour
    if (blockId % BLOCK_PER_MAP_SIDE != 17) {
        AreaBlock *rightBlock = area->GetAreaBlockByID(LOD, blockId + 1);
        if (rightBlock) {
            for (uint32_t y = 0; y < sideSize - 1; y++) {
                quad(block->GetVertice(block->sideSize - 1, y), rightBlock->GetVertice(0, y),
                     rightBlock->GetVertice(0, y + 1), block->GetVertice(block->sideSize - 1, y + 1));
            }
        }
    }

    // Seam to the bottom neighbour
    if (blockId / BLOCK_PER_MAP_SIDE != 17) {
        AreaBlock *bottomBlock = area->GetAreaBlockByID(LOD, blockId + BLOCK_PER_MAP_SIDE);
        if (bottomBlock) {
            for (uint32_t x = 0; x < sideSize - 1; x++) {
                quad(block->GetVertice(x, block->sideSize - 1), block->GetVertice(x + 1, block->sideSize - 1),
                     bottomBlock->GetVertice(x + 1, 0), bottomBlock->GetVertice(x, 0));
            }
        }
    }

    // Corner quad towards the bottom-right neighbour
    if (blockId % BLOCK_PER_MAP_SIDE != 17 && blockId / BLOCK_PER_MAP_SIDE != 17) {
        AreaBlock *rightBlock = area->GetAreaBlockByID(LOD, blockId + 1);
        AreaBlock *rightBottomBlock = area->GetAreaBlockByID(LOD, blockId + 1 + BLOCK_PER_MAP_SIDE);
        AreaBlock *bottomBlock = area->GetAreaBlockByID(LOD, blockId + BLOCK_PER_MAP_SIDE);
        if (rightBlock && rightBottomBlock && bottomBlock) {
            quad(block->GetVertice(block->sideSize - 1, block->sideSize - 1),
                 rightBlock->GetVertice(0, block->sideSize - 1), rightBottomBlock->GetVertice(0, 0),
                 bottomBlock->GetVertice(block->sideSize - 1, 0));
        }
    }

    std::vector<TerrainVertex> all;
    all.reserve(plain.size());
    all.insert(all.end(), plain.begin(), plain.end());
    out.plainCount = (GLsizei)plain.size();
    for (auto &kv : byTex) {
        TexRange tr;
        tr.imageId = kv.first;
        tr.first = (GLint)all.size();
        tr.count = (GLsizei)kv.second.size();
        out.ranges.push_back(tr);
        all.insert(all.end(), kv.second.begin(), kv.second.end());
    }
    if (!all.empty()) {
        gl.GenBuffers(1, &out.vbo);
        gl.BindBuffer(GL_ARRAY_BUFFER, out.vbo);
        gl.BufferData(GL_ARRAY_BUFFER, (ptrdiff_t)(all.size() * sizeof(TerrainVertex)), all.data(), GL_STATIC_DRAW);
        gl.BindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void GL4Terrain::bindMesh(const Mesh &m) {
    gl.BindBuffer(GL_ARRAY_BUFFER, m.vbo);
    gl.EnableVertexAttribArray(0);
    gl.EnableVertexAttribArray(1);
    gl.EnableVertexAttribArray(2);
    gl.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), (const void *)0);
    gl.VertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), (const void *)12);
    gl.VertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), (const void *)16);
}

void GL4Terrain::draw(SCRenderer &r, RSArea *area, int LOD, const std::vector<int> &blocks) {
    if (!program) {
        return;
    }
    if (builtTextured != r.show_textured || builtFlat != r.terrain_flat) {
        clear();
        builtTextured = r.show_textured;
        builtFlat = r.terrain_flat;
    }

    std::vector<Mesh *> visible;
    visible.reserve(blocks.size());
    for (int id : blocks) {
        Mesh &m = getMesh(r, area, LOD, id);
        if (m.vbo) {
            visible.push_back(&m);
        }
    }

    GLfloat proj[16], mv[16];
    glGetFloatv(GL_PROJECTION_MATRIX, proj);
    glGetFloatv(GL_MODELVIEW_MATRIX, mv);

    GLfloat pal[256 * 3];
    VGAPalette *palette = r.getPalette();
    for (int i = 0; i < 256; i++) {
        const Texel *t = palette->GetRGBColor((uint8_t)i);
        pal[i * 3 + 0] = t->r / 255.0f;
        pal[i * 3 + 1] = t->g / 255.0f;
        pal[i * 3 + 2] = t->b / 255.0f;
    }

    gl.UseProgram(program);
    gl.UniformMatrix4fv(uProj, 1, GL_FALSE, proj);
    gl.UniformMatrix4fv(uMV, 1, GL_FALSE, mv);
    gl.Uniform3fv(uPalette, 256, pal);
    gl.Uniform1f(uShadowLift, r.terrain_shadow_lift);
    gl.Uniform1f(uShadowRange, r.terrain_shadow_range);
    gl.Uniform1i(uTex, 0);
    gl.Uniform1i(uFog, r.show_fog ? 1 : 0);
    gl.Uniform1f(uFogStart, r.max_view_distance * 0.40f);
    gl.Uniform1f(uFogEnd, r.max_view_distance * 0.93f);
    gl.Uniform3f(uFogColor, 0.89f, 0.89f, 0.98f);

    gl.BindVertexArray(vao);

    // Pass 1: untextured triangles
    gl.Uniform1i(uTextured, 0);
    for (Mesh *m : visible) {
        if (m->plainCount > 0) {
            bindMesh(*m);
            glDrawArrays(GL_TRIANGLES, 0, m->plainCount);
        }
    }

    // Pass 2: textured triangles, grouped by texture
    std::map<int, std::vector<std::pair<Mesh *, const TexRange *>>> byTex;
    for (Mesh *m : visible) {
        for (const TexRange &tr : m->ranges) {
            byTex[tr.imageId].push_back(std::make_pair(m, &tr));
        }
    }
    gl.Uniform1i(uTextured, 1);
    for (auto &kv : byTex) {
        RSImage *image = area->GetImageByID(kv.first);
        if (!image || !image->GetTexture()) {
            continue;
        }
        glBindTexture(GL_TEXTURE_2D, image->GetTexture()->getTextureID());
        for (auto &item : kv.second) {
            bindMesh(*item.first);
            glDrawArrays(GL_TRIANGLES, item.second->first, item.second->count);
        }
    }

    // Leave the state clean for the legacy immediate-mode code that follows
    glBindTexture(GL_TEXTURE_2D, 0);
    gl.BindVertexArray(0);
    gl.BindBuffer(GL_ARRAY_BUFFER, 0);
    gl.UseProgram(0);
}
