#pragma once
#include <algorithm>
#include <chrono>
#include <cmath>
#include <vector>
#include "../commons/Matrix.h"
#include "../engine/SCRenderer.h"

namespace enhanced {

using EffectVertex = EffectVertex;

struct Color {
    float r;
    float g;
    float b;
    float a;
};

inline Color color(float r, float g, float b, float a) {
    Color c;
    c.r = r;
    c.g = g;
    c.b = b;
    c.a = a;
    return c;
}

inline float smooth(float a, float b, float x) {
    float t = std::min(1.0f, std::max(0.0f, (x - a) / (b - a)));
    return t * t * (3.0f - 2.0f * t);
}

float effectTime();

/** Rigid transform, column major like OpenGL. */
struct Mat4 {
    float m[16];
    Vector3D point(Vector3D p) const;
    Vector3D invDir(Vector3D d) const;
    Vector3D invPoint(Vector3D p) const;
};

/** Camera position and right/up axes in world space, from a modelview matrix (world -> eye). */
struct CameraBasis {
    Vector3D eye;
    Vector3D right;
    Vector3D up;
    static CameraBasis fromView(const float *view);
};

EffectVertex vert(Vector3D p, float u, float v, Color c);

/** Collects triangles; quads and strips are converted. */
class Batch {
public:
    void quad(const EffectVertex &a, const EffectVertex &b, const EffectVertex &c, const EffectVertex &d);
    void stripSegment(const EffectVertex &l0, const EffectVertex &r0, const EffectVertex &l1, const EffectVertex &r1);
    void flush(unsigned int texture, BlendMode mode);

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
