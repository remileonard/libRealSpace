#include "DamageSmokeEffect.h"
#include <cstdint>

namespace enhanced {

namespace {

float hash01(uint32_t h, uint32_t k) {
    h = (h ^ (k * 0x9e3779b1u)) * 0x85ebca6bu;
    h ^= h >> 13;
    h *= 0xc2b2ae35u;
    h ^= h >> 16;
    return (float)(h & 0xffff) / 65535.0f;
}

uint32_t pointHash(Vec3 p, float scale) {
    return ((uint32_t)(int)(p.x * scale)) * 73856093u ^ ((uint32_t)(int)(p.z * scale)) * 19349663u;
}

}  // namespace

void DamageSmokeEffect::addBillboard(Batch &batch, Vec3 center, float size, float rotation, Color c) const {
    Vec3 look = eye - center;
    float ll = length(look);
    if (ll < 1e-4f) {
        return;
    }
    look = look * (1.0f / ll);
    Vec3 right = normalized(vec3(look.z, 0.0f, -look.x), vec3(1.0f, 0.0f, 0.0f));
    Vec3 up = cross(look, right);
    float cs = cosf(rotation) * size;
    float sn = sinf(rotation) * size;
    Vec3 rx = right * cs + up * sn;
    Vec3 ux = right * -sn + up * cs;
    batch.quad(vert(center - rx - ux, 0.0f, 0.0f, c), vert(center + rx - ux, 1.0f, 0.0f, c),
               vert(center + rx + ux, 1.0f, 1.0f, c), vert(center - rx + ux, 0.0f, 1.0f, c));
}

void DamageSmokeEffect::addSmoke(const std::vector<Vec3> &points, float intensity) {
    const size_t n = points.size();
    std::vector<size_t> order(n);
    for (size_t i = 0; i < n; i++) {
        order[i] = i;
    }
    std::sort(order.begin(), order.end(), [this, &points](size_t a, size_t b) {
        return length(points[a] - eye) > length(points[b] - eye);
    });

    Batch batch;
    for (size_t i : order) {
        const float age = (float)i / (float)(n - 1);
        const uint32_t h = pointHash(points[i], 7.0f);
        const float base = (6.0f + 24.0f * powf(age, 0.7f)) * (0.55f + 0.45f * intensity);
        const float fade = smooth(0.0f, 0.10f, age) * powf(1.0f - age, 0.8f) * (0.35f + 0.65f * intensity);
        // dense black core near the aircraft, greyer and thinner further back
        const float g = 0.05f + 0.34f * smooth(0.0f, 0.85f, age) + 0.30f * (1.0f - intensity);
        const float tint = (1.0f - age) * 0.04f;
        for (int k = 0; k < 4; k++) {
            float ox = (hash01(h, 11 + k) - 0.5f) * 2.0f;
            float oy = (hash01(h, 21 + k) - 0.5f) * 2.0f;
            float oz = (hash01(h, 31 + k) - 0.5f) * 2.0f;
            float spread = base * (0.10f + 0.32f * age);
            Vec3 c = vec3(points[i].x + ox * spread, points[i].y + oy * spread * 0.8f + age * 1.5f, points[i].z + oz * spread);
            float size = base * (0.62f + 0.38f * hash01(h, 41 + k));
            float spin = hash01(h, 61 + k) * 0.12f + 0.10f;
            float rot = hash01(h, 51 + k) * 6.2831853f + time * spin * (k & 1 ? 1.0f : -1.0f);
            addBillboard(batch, c, size, rot, color(g + tint, g + tint * 0.6f, g, std::min(1.0f, 0.95f * fade)));
        }
    }
    batch.flush(EffectTextures::get().smoke, SCRenderer::BlendMode::Alpha);
}

// flickering tongues of flame along the first part of the trail, plus flying embers (additive)
void DamageSmokeEffect::addFire(const std::vector<Vec3> &points, float intensity) {
    const float strength = 0.65f + 0.35f * intensity;
    const size_t m = std::min<size_t>(points.size(), 14);
    Batch batch;
    for (size_t i = 0; i < m; i++) {
        const float t = (float)i / (float)(m - 1);
        const uint32_t h = pointHash(points[i], 5.0f);
        for (int k = 0; k < 3; k++) {
            float flick = 0.60f + 0.40f * sinf(time * (21.0f + 8.0f * k) + (float)i * 2.3f + (float)k * 1.7f);
            float jx = (hash01(h, 71 + k) - 0.5f) * (1.0f + 5.0f * t);
            float jy = (hash01(h, 81 + k) - 0.5f) * (1.0f + 3.0f * t);
            float jz = (hash01(h, 91 + k) - 0.5f) * (1.0f + 5.0f * t);
            Vec3 c = vec3(points[i].x + jx, points[i].y + jy + t * 3.0f, points[i].z + jz);
            float size = (3.2f + 8.5f * t) * (0.75f + 0.5f * hash01(h, 101 + k)) * (0.85f + 0.25f * flick);
            // white-yellow core -> orange -> deep red towards the tip
            float a = powf(1.0f - t, 0.8f) * 0.60f * flick * strength;
            addBillboard(batch, c, size, 0.0f, color(1.0f, 0.92f - 0.72f * t, 0.50f * (1.0f - t) * (1.0f - t), std::min(1.0f, a)));
        }
    }
    const uint32_t tick = (uint32_t)(time * 14.0f);
    for (size_t e = 0; e < m * 3; e++) {
        size_t i = e / 3;
        const float t = (float)i / (float)(m - 1);
        const uint32_t h = pointHash(points[i], 5.0f);
        float sp = 1.5f + 9.0f * t;
        Vec3 c = vec3(points[i].x + (hash01(h, 200 + e) - 0.5f) * 2.0f * sp, points[i].y + (hash01(h, 300 + e) - 0.2f) * 1.5f * sp,
                      points[i].z + (hash01(h, 400 + e) - 0.5f) * 2.0f * sp);
        float twinkle = hash01(h + tick, 500 + (uint32_t)e) > 0.35f ? 1.0f : 0.15f;
        addBillboard(batch, c, 0.55f + 0.5f * hash01(h, 600 + e), 0.0f,
                     color(1.0f, 0.78f - 0.35f * t, 0.30f, 0.95f * twinkle * (1.0f - 0.6f * t)));
    }
    batch.flush(EffectTextures::get().glow, SCRenderer::BlendMode::Additive);
}

void DamageSmokeEffect::draw(const std::vector<Vec3> &points, float intensity, bool burning, const float *view) {
    if (points.size() < 2) {
        return;
    }
    time = effectTime();
    eye = CameraBasis::fromView(view).eye;
    float inten = std::min(1.0f, std::max(0.0f, intensity));
    addSmoke(points, inten);
    if (burning) {
        addFire(points, inten);
    }
}

}  // namespace enhanced
