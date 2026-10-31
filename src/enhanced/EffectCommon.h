#pragma once
#include <algorithm>
#include <chrono>
#include <cmath>
#include <vector>
#include "../engine/SCRenderer.h"

namespace enhanced {

using EffectVertex = SCRenderer::EffectVertex;

struct Vec3 {
    float x;
    float y;
    float z;
};

struct Color {
    float r;
    float g;
    float b;
    float a;
};

inline Vec3 vec3(float x, float y, float z) {
    Vec3 v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

inline Color color(float r, float g, float b, float a) {
    Color c;
    c.r = r;
    c.g = g;
    c.b = b;
    c.a = a;
    return c;
}

inline Vec3 operator+(Vec3 a, Vec3 b) {
    return vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

inline Vec3 operator-(Vec3 a, Vec3 b) {
    return vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

inline Vec3 operator*(Vec3 a, float s) {
    return vec3(a.x * s, a.y * s, a.z * s);
}

inline float dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3 cross(Vec3 a, Vec3 b) {
    return vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

inline float length(Vec3 a) {
    return sqrtf(dot(a, a));
}

inline Vec3 normalized(Vec3 a, Vec3 fallback) {
    float l = length(a);
    if (l < 1e-6f) {
        return fallback;
    }
    return a * (1.0f / l);
}

inline float smooth(float a, float b, float x) {
    float t = std::min(1.0f, std::max(0.0f, (x - a) / (b - a)));
    return t * t * (3.0f - 2.0f * t);
}

float effectTime();

/** Rigid transform, column major like OpenGL. */
struct Mat4 {
    float m[16];
    Vec3 point(Vec3 p) const;
    Vec3 invDir(Vec3 d) const;
    Vec3 invPoint(Vec3 p) const;
};

/** Camera position and right/up axes in world space, from a modelview matrix (world -> eye). */
struct CameraBasis {
    Vec3 eye;
    Vec3 right;
    Vec3 up;
    static CameraBasis fromView(const float *view);
};

EffectVertex vert(Vec3 p, float u, float v, Color c);

/** Collects triangles; quads and strips are converted. */
class Batch {
public:
    void quad(const EffectVertex &a, const EffectVertex &b, const EffectVertex &c, const EffectVertex &d);
    void stripSegment(const EffectVertex &l0, const EffectVertex &r0, const EffectVertex &l1, const EffectVertex &r1);
    void flush(unsigned int texture, SCRenderer::BlendMode mode);

private:
    std::vector<EffectVertex> vertices;
};

/** Procedural textures shared by all effects, created on first use (needs the GL context). */
struct EffectTextures {
    unsigned int noise{0};
    unsigned int glow{0};
    unsigned int smoke{0};
    static EffectTextures &get();
};

}  // namespace enhanced
