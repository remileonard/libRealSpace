#include "CockpitGlassEffect.h"
#include <cstdint>
#include <cstdio>

namespace enhanced {
namespace {

const int MAX_IMPACTS = 6;
// bubble canopy outline: half width in units of the half view height for the given height (device coordinates, -1 bottom, 1 top)
const int DOME_POINTS = 7;
const float DOME_Y[DOME_POINTS] = {-1.0f, -0.5f, 0.0f, 0.5f, 0.75f, 0.9f, 1.0f};
const float DOME_HALF_WIDTH[DOME_POINTS] = {1.90f, 1.85f, 1.70f, 1.30f, 1.14f, 0.90f, 0.57f};
const int DOME_ROWS = 24;
const int DOME_COLUMNS = 24;
// the glass is placed behind the cockpit model, so frame and panels stay in front of it
const float GLASS_DISTANCE = 12.0f;

float domeHalfWidth(float y) {
    for (int i = 0; i + 1 < DOME_POINTS; i++) {
        if (y <= DOME_Y[i + 1]) {
            float t = (y - DOME_Y[i]) / (DOME_Y[i + 1] - DOME_Y[i]);
            return DOME_HALF_WIDTH[i] + (DOME_HALF_WIDTH[i + 1] - DOME_HALF_WIDTH[i]) * t;
        }
    }
    return DOME_HALF_WIDTH[DOME_POINTS - 1];
}

struct Segment {
    float x0;
    float y0;
    float x1;
    float y1;
    float width;
};

uint32_t nextRandom(uint32_t &state) {
    state = state * 1664525u + 1013904223u;
    return state >> 8;
}

float randomUnit(uint32_t &state) {
    return (float)(nextRandom(state) & 0xffff) / 65535.0f;
}

float distanceToSegment(float px, float py, const Segment &s) {
    float dx = s.x1 - s.x0;
    float dy = s.y1 - s.y0;
    float len2 = dx * dx + dy * dy;
    float t = len2 > 1e-9f ? ((px - s.x0) * dx + (py - s.y0) * dy) / len2 : 0.0f;
    t = std::min(1.0f, std::max(0.0f, t));
    float cx = s.x0 + dx * t - px;
    float cy = s.y0 + dy * t - py;
    return sqrtf(cx * cx + cy * cy);
}

void addCrackArm(std::vector<Segment> &segments, uint32_t &rng, float x, float y, float angle, float length, float width,
                 int depth) {
    const int steps = 7;
    float step = length / steps;
    for (int i = 0; i < steps; i++) {
        float next = angle + (randomUnit(rng) - 0.5f) * 0.45f;
        float nx = x + cosf(next) * step;
        float ny = y + sinf(next) * step;
        segments.push_back({x, y, nx, ny, width});
        if (depth > 0 && randomUnit(rng) < 0.30f) {
            float side = randomUnit(rng) < 0.5f ? -1.0f : 1.0f;
            addCrackArm(segments, rng, nx, ny, next + side * (0.5f + randomUnit(rng) * 0.6f), length * 0.45f,
                        width * 0.7f, depth - 1);
        }
        x = nx;
        y = ny;
        angle = next;
        width *= 0.93f;
    }
}

// star shaped impact: hole in the middle, radial cracks with branches and two partial rings around it
unsigned int makeCracks() {
    const int N = 256;
    uint32_t rng = 0x2545f491u;
    std::vector<Segment> segments;
    const int arms = 11;
    for (int a = 0; a < arms; a++) {
        float angle = (float)a / arms * 6.2831853f + (randomUnit(rng) - 0.5f) * 0.35f;
        addCrackArm(segments, rng, 0.0f, 0.0f, angle, 0.55f + randomUnit(rng) * 0.4f, 0.010f, 2);
    }
    for (int ring = 0; ring < 2; ring++) {
        float radius = 0.16f + 0.17f * ring;
        float start = randomUnit(rng) * 6.2831853f;
        const int pieces = 14;
        for (int i = 0; i < pieces; i++) {
            if (randomUnit(rng) < 0.30f) {
                continue;
            }
            float a0 = start + (float)i / pieces * 6.2831853f;
            float a1 = start + (float)(i + 1) / pieces * 6.2831853f;
            segments.push_back({cosf(a0) * radius, sinf(a0) * radius, cosf(a1) * radius, sinf(a1) * radius, 0.006f});
        }
    }
    std::vector<uint8_t> px((size_t)N * N * 4);
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            float fx = ((x + 0.5f) / N) * 2.0f - 1.0f;
            float fy = ((y + 0.5f) / N) * 2.0f - 1.0f;
            float r = sqrtf(fx * fx + fy * fy);
            float best = 1.0f;
            for (const Segment &s : segments) {
                best = std::min(best, std::max(0.0f, distanceToSegment(fx, fy, s) - s.width));
            }
            float line = smooth(0.012f, 0.0f, best);
            float hole = smooth(0.055f, 0.030f, r);
            float fade = smooth(1.0f, 0.45f, r);
            float a = std::max(line * fade, hole);
            size_t i = ((size_t)y * N + x) * 4;
            px[i] = 255;
            px[i + 1] = 255;
            px[i + 2] = 255;
            px[i + 3] = (uint8_t)(std::min(1.0f, a) * 255.0f);
        }
    }
    return SCRenderer::getInstance().createTexture(N, N, px.data(), false);
}

// dust specks and a few thin scratches, mostly invisible until the sun hits the glass
unsigned int makeScratches() {
    const int N = 512;
    uint32_t rng = 0x7f4a7c15u;
    std::vector<float> field((size_t)N * N, 0.0f);
    for (int i = 0; i < 900; i++) {
        int cx = (int)(randomUnit(rng) * N);
        int cy = (int)(randomUnit(rng) * N);
        float strength = 0.15f + randomUnit(rng) * 0.35f;
        int radius = 1 + (int)(randomUnit(rng) * 2.0f);
        for (int dy = -radius; dy <= radius; dy++) {
            for (int dx = -radius; dx <= radius; dx++) {
                int x = cx + dx;
                int y = cy + dy;
                if (x >= 0 && y >= 0 && x < N && y < N) {
                    float d = sqrtf((float)(dx * dx + dy * dy)) / (radius + 0.5f);
                    field[(size_t)y * N + x] += strength * std::max(0.0f, 1.0f - d);
                }
            }
        }
    }
    for (int i = 0; i < 45; i++) {
        float x = randomUnit(rng) * N;
        float y = randomUnit(rng) * N;
        float angle = randomUnit(rng) * 6.2831853f;
        float length = 20.0f + randomUnit(rng) * 90.0f;
        float strength = 0.25f + randomUnit(rng) * 0.5f;
        for (int s = 0; s < (int)length; s++) {
            int px = (int)(x + cosf(angle) * s);
            int py = (int)(y + sinf(angle) * s);
            if (px >= 0 && py >= 0 && px < N && py < N) {
                float along = sinf(3.14159265f * s / length);
                field[(size_t)py * N + px] += strength * along;
            }
        }
    }
    std::vector<uint8_t> out((size_t)N * N * 4);
    for (size_t i = 0; i < (size_t)N * N; i++) {
        out[i * 4] = 255;
        out[i * 4 + 1] = 255;
        out[i * 4 + 2] = 255;
        out[i * 4 + 3] = (uint8_t)(std::min(1.0f, field[i]) * 255.0f);
    }
    return SCRenderer::getInstance().createTexture(N, N, out.data(), false);
}

// soft band profile across the texture, used for the reflection streak
unsigned int makeStreak() {
    const int W = 64;
    const int H = 4;
    std::vector<uint8_t> px((size_t)W * H * 4);
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            float t = (x + 0.5f) / W;
            float a = powf(sinf(3.14159265f * t), 2.0f);
            size_t i = ((size_t)y * W + x) * 4;
            px[i] = 255;
            px[i + 1] = 255;
            px[i + 2] = 255;
            px[i + 3] = (uint8_t)(a * 255.0f);
        }
    }
    return SCRenderer::getInstance().createTexture(W, H, px.data(), false);
}

}  // namespace

void CockpitGlassEffect::createTextures() {
    if (white != 0) {
        return;
    }
    const unsigned char whitePixels[16] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};
    white = SCRenderer::getInstance().createTexture(2, 2, whitePixels, false);
    scratches = makeScratches();
    cracks = makeCracks();
    streak = makeStreak();
}

void CockpitGlassEffect::updateImpacts(float damage) {
    static const Impact table[MAX_IMPACTS] = {
        {-0.90f, 0.20f, 0.34f, 0.3f},
        {1.00f, 0.45f, 0.30f, 1.1f},
        {0.05f, 0.75f, 0.28f, 2.0f},
        {-0.40f, 0.55f, 0.24f, 0.7f},
        {0.55f, 0.10f, 0.26f, 2.6f},
        {-1.30f, -0.15f, 0.26f, 1.6f},
    };
    int wanted = 0;
    if (damage >= 0.15f) {
        wanted = std::min(MAX_IMPACTS, 1 + (int)((damage - 0.15f) / 0.15f));
    }
    if (wanted == impactCount) {
        return;
    }
    impactCount = wanted;
    impacts.assign(table, table + wanted);
    printf("[enhanced] cockpit glass: %d impacts (damage %.2f)\n", wanted, damage);
}

Vec3 CockpitGlassEffect::screenPoint(const ViewContext &v, float nx, float ny) const {
    float ex = v.distance * (nx + v.proj[8]) / v.proj[0];
    float ey = v.distance * (ny + v.proj[9]) / v.proj[5];
    return v.cam.eye + v.forward * v.distance + v.cam.right * ex + v.cam.up * ey;
}

// centre in normalized device coordinates, half size in units of the view height, rotation in screen space
void CockpitGlassEffect::addScreenQuad(Batch &batch, const ViewContext &v, float cx, float cy, float hx, float hy,
                                       float rotation, Color c) const {
    const float cs = cosf(rotation);
    const float sn = sinf(rotation);
    const float ux[4] = {-1.0f, 1.0f, 1.0f, -1.0f};
    const float uy[4] = {-1.0f, -1.0f, 1.0f, 1.0f};
    const float tu[4] = {0.0f, 1.0f, 1.0f, 0.0f};
    const float tv[4] = {0.0f, 0.0f, 1.0f, 1.0f};
    EffectVertex q[4];
    for (int i = 0; i < 4; i++) {
        float rx = ux[i] * hx * cs - uy[i] * hy * sn;
        float ry = ux[i] * hx * sn + uy[i] * hy * cs;
        q[i] = vert(this->screenPoint(v, cx + rx * v.aspectFix, cy + ry), tu[i], tv[i], c);
    }
    batch.quad(q[0], q[1], q[2], q[3]);
}

// fills the bubble canopy outline with a grid; colour, alpha and texture coordinates follow the style
void CockpitGlassEffect::addDome(Batch &batch, const ViewContext &v, const DomeStyle &style) const {
    EffectVertex grid[DOME_ROWS + 1][DOME_COLUMNS + 1];
    for (int r = 0; r <= DOME_ROWS; r++) {
        float y = -1.0f + 2.0f * r / DOME_ROWS;
        float hw = domeHalfWidth(y);
        float t = 0.5f * (y + 1.0f);
        for (int c = 0; c <= DOME_COLUMNS; c++) {
            float side = -1.0f + 2.0f * c / DOME_COLUMNS;
            float x = side * hw;
            // no visible outline: everything fades out towards the canopy contour, the sides get the stronger reflections
            float fade = smooth(1.0f, 0.78f, fabsf(side)) * smooth(1.0f, 0.82f, y);
            float a = (style.bottom.a + (style.top.a - style.bottom.a) * t + style.edgeAlpha * fabsf(side) * side * side) * fade;
            Color col = color(style.bottom.r + (style.top.r - style.bottom.r) * t,
                              style.bottom.g + (style.top.g - style.bottom.g) * t,
                              style.bottom.b + (style.top.b - style.bottom.b) * t, std::max(0.0f, a));
            float u = 0.5f;
            float w = 0.5f;
            if (style.uvMode == 1) {
                u = std::min(1.0f, std::max(0.0f, x * 0.5f + 0.5f));
                w = t;
            } else if (style.uvMode == 2) {
                float s = x * cosf(style.bandAngle) + y * sinf(style.bandAngle) - style.bandOffset;
                u = std::min(1.0f, std::max(0.0f, s / (2.0f * style.bandWidth) + 0.5f));
            }
            grid[r][c] = vert(this->screenPoint(v, x * v.aspectFix, y), u, w, col);
        }
    }
    for (int r = 0; r < DOME_ROWS; r++) {
        for (int c = 0; c < DOME_COLUMNS; c++) {
            batch.quad(grid[r][c], grid[r][c + 1], grid[r + 1][c + 1], grid[r + 1][c]);
        }
    }
}

void CockpitGlassEffect::draw(const float *view, const float *proj, Vec3 sunDir, float damage) {
    if (proj[0] < 1e-4f || proj[5] < 1e-4f) {
        return;
    }
    this->createTextures();
    this->updateImpacts(damage);

    ViewContext v;
    v.cam = CameraBasis::fromView(view);
    v.forward = vec3(-view[2], -view[6], -view[10]);
    v.proj = proj;
    v.distance = GLASS_DISTANCE;
    v.aspectFix = proj[0] / proj[5];

    // sun position relative to the view direction
    Vec3 sun = normalized(sunDir, vec3(0.0f, 1.0f, 0.0f));
    const float sx = dot(v.cam.right, sun);
    const float sy = dot(v.cam.up, sun);
    const float sz = dot(v.forward, sun);
    const float glare = sz > 0.0f ? powf(sz, 5.0f) * smooth(-0.05f, 0.25f, sun.y) : 0.0f;

    Batch batch;
    DomeStyle style;

    // milky haze over the whole canopy, denser towards the bottom and the sides, stronger against the sun
    style = {color(0.60f, 0.76f, 1.0f, 0.03f + 0.25f * glare), color(0.60f, 0.76f, 1.0f, 0.06f + 0.25f * glare), 0.26f, 0, 0, 0, 0};
    this->addDome(batch, v, style);
    batch.flush(white, BlendMode::Alpha);

    // reflections of the cockpit interior: soft bright patches in the outer parts of the glass
    const float patchAlpha = 0.10f + 0.10f * glare;
    this->addScreenQuad(batch, v, -1.15f * v.aspectFix, 0.15f, 0.75f, 0.95f, 0.15f, color(0.80f, 0.90f, 1.0f, patchAlpha));
    this->addScreenQuad(batch, v, 1.20f * v.aspectFix, 0.30f, 0.65f, 0.85f, -0.2f, color(0.80f, 0.90f, 1.0f, patchAlpha * 0.8f));
    batch.flush(EffectTextures::get().glow, BlendMode::Additive);

    // soft diagonal streak that follows the sun
    const float offset = std::max(-0.6f, std::min(0.6f, sx * 0.8f));
    style = {color(0.85f, 0.92f, 1.0f, 0.16f + 0.35f * glare), color(0.85f, 0.92f, 1.0f, 0.16f + 0.35f * glare), 0.0f, 2, 0.35f, offset, 0.12f};
    this->addDome(batch, v, style);
    batch.flush(streak, BlendMode::Additive);

    // scratches and dust: faint, lit up when looking towards the sun
    style = {color(1.0f, 0.97f, 0.9f, 0.14f + 0.85f * glare), color(1.0f, 0.97f, 0.9f, 0.14f + 0.85f * glare), 0.0f, 1, 0, 0, 0};
    this->addDome(batch, v, style);
    batch.flush(scratches, BlendMode::Additive);

    // glare: veil over the glass and a bright halo at the sun position
    if (glare > 0.001f) {
        style = {color(1.0f, 0.95f, 0.85f, 0.30f * glare), color(1.0f, 0.95f, 0.85f, 0.30f * glare), 0.0f, 0, 0, 0, 0};
        this->addDome(batch, v, style);
        batch.flush(white, BlendMode::Additive);
        float hx = std::max(-1.5f, std::min(1.5f, proj[0] * sx / sz));
        float hy = std::max(-1.5f, std::min(1.5f, proj[5] * sy / sz));
        this->addScreenQuad(batch, v, hx, hy, 0.7f, 0.7f, 0.0f, color(1.0f, 0.93f, 0.8f, 0.40f * glare));
        batch.flush(EffectTextures::get().glow, BlendMode::Additive);
    }

    // impact cracks: a dark shadow slightly offset, then the bright crack lines
    for (const Impact &im : impacts) {
        this->addScreenQuad(batch, v, (im.x + 0.004f) * v.aspectFix, im.y - 0.008f, im.size, im.size, im.rotation,
                            color(0.02f, 0.03f, 0.04f, 0.55f));
    }
    batch.flush(cracks, BlendMode::Alpha);
    for (const Impact &im : impacts) {
        this->addScreenQuad(batch, v, im.x * v.aspectFix, im.y, im.size, im.size, im.rotation,
                            color(0.88f, 0.94f, 1.0f, 0.80f + 0.20f * glare));
    }
    batch.flush(cracks, BlendMode::Alpha);
}

}  // namespace enhanced
