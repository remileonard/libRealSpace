#include "ExhaustEffect.h"
#include <cstdio>

namespace enhanced {

const float ExhaustEffect::NOZZLE_RADIUS = 1.0f;

Vec3 ExhaustEffect::toWorld(float x, float y, float z) const {
    return model.point(vec3(nozzle.x + x, nozzle.y + y, nozzle.z + z));
}

// Unit vector across the plume, facing the camera (in the nozzle frame); *facing = how much we look from the side (0..1)
Vec3 ExhaustEffect::sideAxis(float x, float *facing) const {
    float wl = sqrtf(eye.z * eye.z + eye.y * eye.y);
    float vl = sqrtf((eye.x - x) * (eye.x - x) + eye.y * eye.y + eye.z * eye.z);
    *facing = vl > 1e-6f ? wl / vl : 0.0f;
    if (wl > 1e-6f) {
        return vec3(0.0f, eye.z / wl, -eye.y / wl);
    }
    return vec3(0.0f, 1.0f, 0.0f);
}

// Camera-facing strip about the (-x) axis with soft edges and scrolling turbulence from the noise texture
void ExhaustEffect::addStrip(float length, float width, float swell, Color c, float repeat, float speed, float phase) {
    const int SEGMENTS = 22;
    EffectVertex prevLeft;
    EffectVertex prevRight;
    for (int i = 0; i <= SEGMENTS; i++) {
        float u = (float)i / SEGMENTS;
        float x = -u * length;
        float facing;
        Vec3 w = sideAxis(x, &facing);
        float r = NOZZLE_RADIUS * width * (1.0f + swell * sqrtf(u)) * (1.0f - powf(u, 1.7f)) + 0.02f;
        float a = c.a * powf(1.0f - u, 1.1f) * std::min(1.0f, 0.35f + u * 14.0f) * smooth(0.04f, 0.40f, facing);
        float cool = 1.0f - 0.38f * u;
        Color col = color(c.r * cool, c.g * cool * (1.0f - 0.25f * u), c.b * cool * (1.0f - 0.55f * u), std::min(1.0f, a));
        float tv = u * repeat - time * speed + phase;
        EffectVertex left = vert(toWorld(x, w.y * r, w.z * r), 0.0f, tv, col);
        EffectVertex right = vert(toWorld(x, -w.y * r, -w.z * r), 1.0f, tv, col);
        if (i > 0) {
            strips.stripSegment(prevLeft, prevRight, left, right);
        }
        prevLeft = left;
        prevRight = right;
    }
}

void ExhaustEffect::addSprite(float x, float radius, Color c) {
    if (c.a <= 0.003f) {
        return;
    }
    c.a = std::min(1.0f, c.a);
    EffectVertex q[4];
    for (int k = 0; k < 4; k++) {
        float sx = (k == 0 || k == 3) ? -1.0f : 1.0f;
        float sy = (k < 2) ? -1.0f : 1.0f;
        Vec3 off = (camRight * sx + camUp * sy) * radius;
        q[k] = vert(toWorld(x + off.x, off.y, off.z), sx * 0.5f + 0.5f, sy * 0.5f + 0.5f, c);
    }
    glows.quad(q[0], q[1], q[2], q[3]);
}

// Mach diamond: short spindle along the axis
void ExhaustEffect::addDiamond(float x, float halfLength, float halfWidth, float alpha) {
    if (alpha <= 0.003f) {
        return;
    }
    float facing;
    Vec3 w = sideAxis(x, &facing);
    Color c = color(0.80f, 0.88f, 1.0f, std::min(1.0f, alpha * smooth(0.05f, 0.4f, facing)));
    glows.quad(vert(toWorld(x - halfLength, -w.y * halfWidth, -w.z * halfWidth), 0.0f, 0.0f, c),
               vert(toWorld(x - halfLength, w.y * halfWidth, w.z * halfWidth), 1.0f, 0.0f, c),
               vert(toWorld(x + halfLength, w.y * halfWidth, w.z * halfWidth), 1.0f, 1.0f, c),
               vert(toWorld(x + halfLength, -w.y * halfWidth, -w.z * halfWidth), 0.0f, 1.0f, c));
}

// hot exhaust gas at military power: short, pale blue-orange, with Mach diamonds
void ExhaustEffect::addHotGas() {
    float R = NOZZLE_RADIUS;
    float length = (1.0f + 3.4f * dry) * (1.0f + 0.5f * (flicker - 1.0f));
    addStrip(length, 0.90f, 0.08f, color(1.0f, 0.55f, 0.20f, (0.07f + 0.30f * dry) * (1.0f - 0.6f * ab)), 0.45f * length, 2.2f, 0.0f);
    addStrip(length * 0.75f, 0.55f, 0.05f, color(0.65f, 0.78f, 1.0f, (0.10f + 0.45f * dry) * (1.0f - 0.5f * ab)), 0.5f * length, 3.1f, 0.37f);
    float intensity = smooth(0.22f, 0.65f, t) * (1.0f - 0.45f * ab) * flicker;
    for (int k = 0; k < 5; k++) {
        float x = -(0.55f + 0.85f * k) * (0.7f + 0.3f * dry) * R * (1.0f + 0.35f * ab);
        addDiamond(x, 0.32f * R, 0.30f * R * (1.0f - 0.10f * k), 0.55f * intensity * expf(-0.45f * k));
    }
    addSprite(-0.12f, R * (1.05f + 0.35f * dry), color(1.0f, 0.62f + 0.25f * dry, 0.30f + 0.4f * dry, 0.10f + 0.50f * dry));
}

// afterburner: long turbulent plume, white-blue core, orange mantle
void ExhaustEffect::addAfterburner() {
    float R = NOZZLE_RADIUS;
    float length = (6.0f + 10.5f * abp) * (1.0f + 1.0f * (flicker - 1.0f)) * (0.55f + 0.45f * ab);
    float A = ab * (0.70f + 0.30f * abp);
    addStrip(length, 1.10f + 0.22f * abp, 0.38f, color(1.0f, 0.34f, 0.06f, 0.55f * A), 0.22f * length, 4.2f, 0.0f);
    addStrip(length * 0.92f, 1.00f + 0.20f * abp, 0.34f, color(1.0f, 0.42f, 0.08f, 0.40f * A), 0.30f * length, 5.1f, 0.5f);
    addStrip(length * 0.78f, 0.68f, 0.28f, color(1.0f, 0.72f, 0.28f, 0.60f * A), 0.28f * length, 6.3f, 0.21f);
    addStrip(length * 0.50f, 0.36f, 0.18f, color(0.80f, 0.90f, 1.0f, 0.85f * A), 0.35f * length, 7.4f, 0.77f);
    for (int k = 0; k < 7; k++) {
        float x = -(0.9f + 1.25f * k) * R * (1.0f + 0.40f * abp);
        if (-x > length * 0.8f) {
            break;
        }
        addDiamond(x, 0.45f * R, 0.38f * R * (1.0f - 0.07f * k), 0.45f * A * expf(-0.30f * k) * flicker);
    }
    addSprite(-0.10f, R * (1.5f + 0.5f * abp), color(1.0f, 0.82f, 0.50f, 0.65f * A * flicker));
    addSprite(-0.30f, R * 2.6f, color(1.0f, 0.42f, 0.10f, 0.22f * A));
    addSprite(-length * 0.30f, R * (2.4f + 0.8f * abp), color(1.0f, 0.38f, 0.08f, 0.10f * A));
    // looking straight into the jet: axial glow stack replaces the edge-on strips
    for (int k = 0; k < 4; k++) {
        addSprite(-length * (0.08f + 0.2f * k), R * (1.9f - 0.3f * k), color(1.0f, 0.55f - 0.08f * k, 0.15f, 0.30f * A * endon));
    }
    addSprite(-0.4f, R * 1.1f, color(1.0f, 0.92f, 0.78f, 0.55f * A * endon * flicker));
}

void ExhaustEffect::traceStage(const void *key, int thrust) {
    int stage = ab > 0.001f ? 2 : 1;
    int &last = stages[key];
    if (last != stage) {
        printf("[enhanced] exhaust %p: %s (thrust %d)\n", key, stage == 2 ? "afterburner" : "hot gas", thrust);
        last = stage;
    }
}

void ExhaustEffect::draw(const void *key, const Mat4 &modelMatrix, Vec3 nozzlePos, int thrust, const float *view) {
    if (thrust < 8) {
        if (stages[key] != 0) {
            printf("[enhanced] exhaust %p: off (thrust %d)\n", key, thrust);
            stages[key] = 0;
        }
        return;
    }
    EffectTextures &tex = EffectTextures::get();
    model = modelMatrix;
    nozzle = nozzlePos;
    t = std::min(1.0f, std::max(0.0f, thrust / 100.0f));
    dry = smooth(0.08f, 0.72f, t);
    ab = smooth(0.70f, 0.76f, t);
    abp = std::min(1.0f, std::max(0.0f, (t - 0.72f) / 0.28f));
    time = effectTime();
    flicker = 1.0f + 0.045f * sinf(time * 37.0f) + 0.035f * sinf(time * 61.0f + 1.3f) + 0.03f * sinf(time * 13.7f + 0.4f);

    CameraBasis cam = CameraBasis::fromView(view);
    eye = model.invPoint(cam.eye) - nozzle;
    camRight = model.invDir(cam.right);
    camUp = model.invDir(cam.up);

    float vlen = length(eye);
    endon = vlen > 1e-6f ? 1.0f - sqrtf(eye.y * eye.y + eye.z * eye.z) / vlen : 0.0f;
    endon = smooth(0.0f, 0.6f, endon);
    if (eye.x > 0.0f) {
        endon = 0.0f;
    }

    addHotGas();
    if (ab > 0.001f) {
        addAfterburner();
    }
    traceStage(key, thrust);
    strips.flush(tex.noise, BlendMode::Additive);
    glows.flush(tex.glow, BlendMode::Additive);
}

}  // namespace enhanced
