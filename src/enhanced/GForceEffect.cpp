#include "GForceEffect.h"
#include <cstdio>
#include <cstdlib>
#include "../engine/GLExtensions.h"

namespace enhanced {
namespace {

const int RING_SEGMENTS = 64;
const int RING_STEPS = 24;
const float PI_F = 3.14159265f;
// the overlay sits at the near plane of its own orthographic projection
const float OVERLAY_Z = 0.99f;

float clamp01(float v) {
    return (std::min)(1.0f, (std::max)(0.0f, v));
}

void addFullQuad(Batch &batch, float aspect, Color c) {
    batch.quad(vert(Vector3D(-aspect, -1.0f, OVERLAY_Z), 0.5f, 0.5f, c), vert(Vector3D(aspect, -1.0f, OVERLAY_Z), 0.5f, 0.5f, c),
               vert(Vector3D(aspect, 1.0f, OVERLAY_Z), 0.5f, 0.5f, c), vert(Vector3D(-aspect, 1.0f, OVERLAY_Z), 0.5f, 0.5f, c));
}

}  // namespace

void GForceEffect::configure(float newThreshold, float newStrength) {
    this->threshold = (std::min)(9.0f, (std::max)(2.0f, newThreshold));
    this->strength = (std::min)(2.0f, (std::max)(0.0f, newStrength));
}

void GForceEffect::createTextures() {
    if (this->white != 0) {
        return;
    }
    const unsigned char whitePixels[16] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};
    this->white = SCRenderer::getInstance().createTexture(2, 2, whitePixels, false);
}

void GForceEffect::update(float gLoad, bool active) {
    const float now = effectTime();
    float dt = this->lastTime < 0.0f ? 0.0f : now - this->lastTime;
    this->lastTime = now;
    dt = (std::min)(0.1f, (std::max)(0.0f, dt));

    if (!active) {
        this->smoothedG = 1.0f;
        this->tunnel = 0.0f;
        this->grey = 0.0f;
        this->black = 0.0f;
        this->red = 0.0f;
        return;
    }
    float g = gLoad;
    if (!(g > -20.0f && g < 20.0f)) {
        g = 1.0f;  // NaN / garbage guard
    }
    // testing aid: NEOSC_GTEST=9 forces a G load, no need to pull hard
    if (const char *forced = getenv("NEOSC_GTEST")) {
        g = (float)atof(forced);
    }
    // the body reacts slowly: onset ~1.2 s, recovery ~0.7 s
    const float tau = g > this->smoothedG ? 1.2f : 0.7f;
    this->smoothedG += (g - this->smoothedG) * (1.0f - expf(-dt / tau));

    const float over = this->smoothedG - this->threshold;
    float t = clamp01(over / 4.0f);
    const float gr = clamp01((over - 1.0f) / 3.0f);
    const float bl = clamp01((over - 3.0f) / 2.5f) * 0.65f;  // full blackout only well above the grey-out
    const float rd = clamp01((-this->smoothedG - 1.0f) / 2.5f);
    t = (std::max)(t, rd * 0.5f);
    this->tunnel = clamp01(t * this->strength);
    this->grey = clamp01(gr * this->strength);
    this->black = clamp01(bl * this->strength);
    this->red = clamp01(rd * this->strength);
}

void GForceEffect::draw() {
    if (this->tunnel < 0.002f && this->grey < 0.002f && this->black < 0.002f && this->red < 0.002f) {
        return;
    }
    this->createTextures();

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    if (viewport[2] <= 0 || viewport[3] <= 0) {
        return;
    }
    const float aspect = (float)viewport[2] / (float)viewport[3];

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-aspect, aspect, -1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    Batch batch;
    // grey-out and red-out: the colours drain towards grey, or towards dark red at negative G
    const float tx = powf(this->tunnel, 0.6f);
    const float greyAlpha = (std::min)(1.0f, this->grey * 0.85f + tx * 0.30f);
    if (greyAlpha > 0.002f) {
        addFullQuad(batch, aspect, color(0.42f, 0.42f, 0.42f, greyAlpha));
        batch.flush(this->white, BlendMode::Alpha);
    }
    if (this->red > 0.002f) {
        addFullQuad(batch, aspect, color(0.60f, 0.03f, 0.02f, this->red * 0.65f));
        batch.flush(this->white, BlendMode::Alpha);
    }
    // tunnel vision: dark ring that closes towards the centre (distance in units of the half screen height)
    if (this->tunnel > 0.002f) {
        const float edge = 1.35f + (0.30f - 1.35f) * tx;
        const float strengthAlpha = (std::min)(1.0f, tx * 1.6f + this->red * 0.6f);
        const float radiusMax = sqrtf(aspect * aspect + 1.0f) + 0.02f;
        for (int step = 0; step < RING_STEPS; step++) {
            const float r0 = radiusMax * (float)step / (float)RING_STEPS;
            const float r1 = radiusMax * (float)(step + 1) / (float)RING_STEPS;
            const float a0 = smooth(edge - 0.45f, edge + 0.15f, r0) * strengthAlpha;
            const float a1 = smooth(edge - 0.45f, edge + 0.15f, r1) * strengthAlpha;
            if (a0 < 0.002f && a1 < 0.002f) {
                continue;
            }
            for (int seg = 0; seg < RING_SEGMENTS; seg++) {
                const float t0 = 2.0f * PI_F * (float)seg / (float)RING_SEGMENTS;
                const float t1 = 2.0f * PI_F * (float)(seg + 1) / (float)RING_SEGMENTS;
                const Color c0 = color(0.0f, 0.0f, 0.0f, a0);
                const Color c1 = color(0.0f, 0.0f, 0.0f, a1);
                batch.quad(vert(Vector3D(cosf(t0) * r0, sinf(t0) * r0, OVERLAY_Z), 0.5f, 0.5f, c0),
                           vert(Vector3D(cosf(t1) * r0, sinf(t1) * r0, OVERLAY_Z), 0.5f, 0.5f, c0),
                           vert(Vector3D(cosf(t1) * r1, sinf(t1) * r1, OVERLAY_Z), 0.5f, 0.5f, c1),
                           vert(Vector3D(cosf(t0) * r1, sinf(t0) * r1, OVERLAY_Z), 0.5f, 0.5f, c1));
            }
        }
        batch.flush(this->white, BlendMode::Alpha);
    }
    // blackout over the whole picture
    if (this->black > 0.002f) {
        addFullQuad(batch, aspect, color(0.0f, 0.0f, 0.0f, this->black));
        batch.flush(this->white, BlendMode::Alpha);
    }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

}  // namespace enhanced
