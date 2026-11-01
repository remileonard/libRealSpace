#include "EffectCommon.h"
#include <cstdint>

namespace enhanced {
namespace {

const int LATTICE = 64;

float latticeAt(const std::vector<float> &lat, int px, int py, int a, int b) {
    return lat[(size_t)(((b % py) + py) % py) * LATTICE + (((a % px) + px) % px)];
}

// Tileable value noise (periodic lattice px x py)
float noise2(const std::vector<float> &lat, int px, int py, float x, float y) {
    float fx = x * px;
    float fy = y * py;
    int ix = (int)floorf(fx);
    int iy = (int)floorf(fy);
    float tx = fx - ix;
    float ty = fy - iy;
    tx = tx * tx * (3.0f - 2.0f * tx);
    ty = ty * ty * (3.0f - 2.0f * ty);
    float a = latticeAt(lat, px, py, ix, iy) * (1 - tx) + latticeAt(lat, px, py, ix + 1, iy) * tx;
    float b = latticeAt(lat, px, py, ix, iy + 1) * (1 - tx) + latticeAt(lat, px, py, ix + 1, iy + 1) * tx;
    return a * (1 - ty) + b * ty;
}

std::vector<float> lattice(uint32_t seed) {
    std::vector<float> lat(LATTICE * LATTICE);
    for (auto &v : lat) {
        seed = seed * 1664525u + 1013904223u;
        v = (float)((seed >> 8) & 0xffff) / 65535.0f;
    }
    return lat;
}

void setPixel(std::vector<uint8_t> &px, size_t i, int r, int g, int b, float a) {
    px[i] = (uint8_t)r;
    px[i + 1] = (uint8_t)g;
    px[i + 2] = (uint8_t)b;
    px[i + 3] = (uint8_t)(std::min(1.0f, std::max(0.0f, a)) * 255.0f);
}

// streaks: finer across the flow (x) than along it (y)
unsigned int makeNoise() {
    std::vector<float> lat = lattice(0x9e3779b9);
    const int N = 128;
    std::vector<uint8_t> px((size_t)N * N * 4);
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            float fx = (x + 0.5f) / N;
            float fy = (float)y / N;
            float n = 0.50f * noise2(lat, 6, 3, fx, fy) + 0.30f * noise2(lat, 12, 6, fx, fy) +
                      0.20f * noise2(lat, 24, 12, fx, fy);
            float prof = powf(sinf(3.14159265f * fx), 1.3f);
            setPixel(px, ((size_t)y * N + x) * 4, 255, 255, 255, prof * (0.20f + 0.80f * n * n * 1.6f));
        }
    }
    return SCRenderer::getInstance().createTexture(N, N, px.data(), true);
}

unsigned int makeGlow() {
    const int G = 64;
    std::vector<uint8_t> gp((size_t)G * G * 4);
    for (int y = 0; y < G; y++) {
        for (int x = 0; x < G; x++) {
            float dx = (x + 0.5f) / G * 2.0f - 1.0f;
            float dy = (y + 0.5f) / G * 2.0f - 1.0f;
            float r2 = dx * dx + dy * dy;
            float a = r2 >= 1.0f ? 0.0f : (1.0f - r2) * (1.0f - r2);
            a = a * (0.35f + 0.65f * expf(-r2 * 6.0f));
            setPixel(gp, ((size_t)y * G + x) * 4, 255, 255, 255, a);
        }
    }
    return SCRenderer::getInstance().createTexture(G, G, gp.data(), false);
}

// soft round body with a billowy edge, lighter on the upper left so the puffs read as volumes
unsigned int makeSmoke() {
    std::vector<float> lat = lattice(0x1b873593);
    const int N = 128;
    std::vector<uint8_t> px((size_t)N * N * 4);
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            float fx = (x + 0.5f) / N;
            float fy = (y + 0.5f) / N;
            float dx = fx * 2.0f - 1.0f;
            float dy = fy * 2.0f - 1.0f;
            float r = sqrtf(dx * dx + dy * dy);
            float n = 0.50f * noise2(lat, 4, 4, fx, fy) + 0.30f * noise2(lat, 8, 8, fx, fy) +
                      0.20f * noise2(lat, 16, 16, fx, fy);
            float edge = smooth(1.0f, 0.35f, r + (n - 0.5f) * 0.55f);
            float core = 0.55f + 0.45f * n;
            float lit = std::min(1.0f, std::max(0.0f, 0.5f - 0.5f * (dx + dy) * 0.7f + (n - 0.5f) * 0.6f));
            int shade = (int)(255.0f * (0.80f + 0.20f * lit));
            setPixel(px, ((size_t)y * N + x) * 4, shade, shade, shade, edge * core);
        }
    }
    return SCRenderer::getInstance().createTexture(N, N, px.data(), false);
}

}  // namespace


float effectTime() {
    static const auto t0 = std::chrono::steady_clock::now();
    return std::chrono::duration<float>(std::chrono::steady_clock::now() - t0).count();
}

Vec3 Mat4::point(Vec3 p) const {
    return vec3(m[0] * p.x + m[4] * p.y + m[8] * p.z + m[12], m[1] * p.x + m[5] * p.y + m[9] * p.z + m[13],
                m[2] * p.x + m[6] * p.y + m[10] * p.z + m[14]);
}

Vec3 Mat4::invDir(Vec3 d) const {
    return vec3(m[0] * d.x + m[1] * d.y + m[2] * d.z, m[4] * d.x + m[5] * d.y + m[6] * d.z,
                m[8] * d.x + m[9] * d.y + m[10] * d.z);
}

Vec3 Mat4::invPoint(Vec3 p) const {
    return invDir(vec3(p.x - m[12], p.y - m[13], p.z - m[14]));
}

CameraBasis CameraBasis::fromView(const float *view) {
    CameraBasis c;
    c.right = vec3(view[0], view[4], view[8]);
    c.up = vec3(view[1], view[5], view[9]);
    float e[3];
    for (int i = 0; i < 3; i++) {
        const float *col = &view[i * 4];
        float d = col[0] * col[0] + col[1] * col[1] + col[2] * col[2];
        e[i] = -(col[0] * view[12] + col[1] * view[13] + col[2] * view[14]) / (d > 1e-9f ? d : 1.0f);
    }
    c.eye = vec3(e[0], e[1], e[2]);
    return c;
}

EffectVertex vert(Vec3 p, float u, float v, Color c) {
    EffectVertex out;
    out.x = p.x;
    out.y = p.y;
    out.z = p.z;
    out.u = u;
    out.v = v;
    out.r = c.r;
    out.g = c.g;
    out.b = c.b;
    out.a = c.a;
    return out;
}

void Batch::quad(const EffectVertex &a, const EffectVertex &b, const EffectVertex &c, const EffectVertex &d) {
    vertices.push_back(a);
    vertices.push_back(b);
    vertices.push_back(c);
    vertices.push_back(a);
    vertices.push_back(c);
    vertices.push_back(d);
}

void Batch::stripSegment(const EffectVertex &l0, const EffectVertex &r0, const EffectVertex &l1, const EffectVertex &r1) {
    quad(l0, r0, r1, l1);
}

void Batch::flush(unsigned int texture, BlendMode mode) {
    if (!vertices.empty()) {
        SCRenderer::getInstance().drawTriangles(vertices.data(), vertices.size(), texture, mode, false);
    }
    vertices.clear();
}

EffectTextures &EffectTextures::get() {
    static EffectTextures t;
    if (t.noise == 0) {
        t.noise = makeNoise();
        t.glow = makeGlow();
        t.smoke = makeSmoke();
    }
    return t;
}

}  // namespace enhanced
