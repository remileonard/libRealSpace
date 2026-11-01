#include "WaterSplashEffect.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <cstdio>
#include "../realspace/RSArea.h"
#include "../strike_commander/SCExplosion.h"
#include "../strike_commander/SCMission.h"

namespace enhanced {

const size_t WaterSplashEffect::MAX_SPLASHES = 64;
const float WaterSplashEffect::LIFE = 1.3f;

namespace {

float hash01(uint32_t h, uint32_t k) {
    h = (h ^ (k * 0x9e3779b1u)) * 0x85ebca6bu;
    h ^= h >> 13;
    h *= 0xc2b2ae35u;
    h ^= h >> 16;
    return (float)(h & 0xffff) / 65535.0f;
}

const float GRAVITY = 55.0f;

}  // namespace

bool WaterSplashEffect::isOpenWater(RSArea *area, float x, float z) const {
    const float center = (float)BLOCK_WIDTH * (float)BLOCK_PER_MAP_SIDE_DIV_2;
    const float fx = x + center;
    const float fz = z + center;
    if (fx < 0.0f || fz < 0.0f) {
        return false;
    }
    const int bx = (int)(fx / (float)BLOCK_WIDTH);
    const int bz = (int)(fz / (float)BLOCK_WIDTH);
    if (bx >= BLOCK_PER_MAP_SIDE || bz >= BLOCK_PER_MAP_SIDE) {
        return false;
    }
    AreaBlock *block = area->GetAreaBlockByID(0, bz * BLOCK_PER_MAP_SIDE + bx);
    if (block == nullptr || block->sideSize < 2) {
        return false;
    }
    const int s = block->sideSize;
    const float cell = (float)BLOCK_WIDTH / (float)s;
    int ix = (int)((fx - (float)bx * (float)BLOCK_WIDTH) / cell);
    int iz = (int)((fz - (float)bz * (float)BLOCK_WIDTH) / cell);
    if (ix > s - 2) {
        ix = s - 2;
    }
    if (iz > s - 2) {
        iz = s - 2;
    }
    // all four corners of the cell have to be sea, so shore cells keep the normal explosion
    return block->GetVertice(ix, iz)->type == 0 && block->GetVertice(ix + 1, iz)->type == 0 &&
           block->GetVertice(ix, iz + 1)->type == 0 && block->GetVertice(ix + 1, iz + 1)->type == 0;
}

void WaterSplashEffect::scan(SCMission *mission) {
    if (mission == nullptr || mission->area == nullptr) {
        this->seen.clear();
        return;
    }
    // forget explosions that are gone
    std::unordered_set<SCExplosion *> present;
    for (SCExplosion *explosion : mission->explosions) {
        present.insert(explosion);
    }
    for (auto it = this->seen.begin(); it != this->seen.end();) {
        it = present.count(*it) != 0 ? std::next(it) : this->seen.erase(it);
    }
    const float now = effectTime();
    for (SCExplosion *explosion : mission->explosions) {
        if (explosion == nullptr || explosion->is_finished || !this->seen.insert(explosion).second) {
            continue;
        }
        const Vector3D p = explosion->position;
        if (!this->isOpenWater(mission->area, p.x, p.z)) {
            continue;
        }
        explosion->is_finished = true;
        Splash splash;
        splash.position = Vector3D(p.x, mission->area->getY(p.x, p.z), p.z);
        splash.start = now;
        splash.scale = 1.0f;
        splash.seed = (uint32_t)(p.x * 31.0f) * 73856093u ^ (uint32_t)(p.z * 17.0f) * 19349663u ^ (uint32_t)(now * 1000.0f);
        if (this->splashes.size() >= MAX_SPLASHES) {
            this->splashes.erase(this->splashes.begin());
        }
        this->splashes.push_back(splash);
        printf("[enhanced] water splash at %.0f %.0f\n", p.x, p.z);
    }
}

unsigned int WaterSplashEffect::ringTexture() {
    if (this->ring == 0) {
        const int N = 64;
        std::vector<unsigned char> px((size_t)N * N * 4);
        for (int y = 0; y < N; y++) {
            for (int x = 0; x < N; x++) {
                const float dx = ((float)x + 0.5f) / N * 2.0f - 1.0f;
                const float dy = ((float)y + 0.5f) / N * 2.0f - 1.0f;
                const float r = sqrtf(dx * dx + dy * dy);
                // soft ring around r = 0.8
                const float a = smooth(0.55f, 0.78f, r) * (1.0f - smooth(0.82f, 1.0f, r));
                const size_t i = ((size_t)y * N + x) * 4;
                px[i] = px[i + 1] = px[i + 2] = 255;
                px[i + 3] = (unsigned char)(a * 255.0f);
            }
        }
        this->ring = SCRenderer::getInstance().createTexture(N, N, px.data(), false);
    }
    return this->ring;
}

// soft round blob with a smooth falloff, so magnified puffs never show hard pixel edges
unsigned int WaterSplashEffect::puffTexture() {
    if (this->puff == 0) {
        const int N = 64;
        std::vector<unsigned char> px((size_t)N * N * 4);
        for (int y = 0; y < N; y++) {
            for (int x = 0; x < N; x++) {
                const float dx = ((float)x + 0.5f) / N * 2.0f - 1.0f;
                const float dy = ((float)y + 0.5f) / N * 2.0f - 1.0f;
                const float r = std::min(1.0f, sqrtf(dx * dx + dy * dy));
                const float falloff = 1.0f - smooth(0.0f, 1.0f, r);
                const float a = falloff * falloff;
                const size_t i = ((size_t)y * N + x) * 4;
                px[i] = px[i + 1] = px[i + 2] = 255;
                px[i + 3] = (unsigned char)(a * 255.0f);
            }
        }
        this->puff = SCRenderer::getInstance().createTexture(N, N, px.data(), false);
    }
    return this->puff;
}

void WaterSplashEffect::addBillboard(Batch &batch, Vector3D center, float size, Color c) const {
    Vector3D look = this->eye - center;
    const float ll = look.Length();
    if (ll < 1e-4f) {
        return;
    }
    look = look * (1.0f / ll);
    const Vector3D right = Vector3D(look.z, 0.0f, -look.x).Normalized(Vector3D(1.0f, 0.0f, 0.0f));
    const Vector3D up = Vector3D::Cross(look, right);
    const Vector3D rx = right * size;
    const Vector3D ux = up * size;
    batch.quad(vert(center - rx - ux, 0.0f, 0.0f, c), vert(center + rx - ux, 1.0f, 0.0f, c),
               vert(center + rx + ux, 1.0f, 1.0f, c), vert(center - rx + ux, 0.0f, 1.0f, c));
}

void WaterSplashEffect::addRing(Batch &batch, const Splash &splash, float age) const {
    const float t = age / LIFE;
    const float radius = (3.0f + 26.0f * powf(t, 0.6f)) * splash.scale;
    const float alpha = 0.55f * (1.0f - smooth(0.0f, 1.0f, t)) * smooth(0.0f, 0.06f, t);
    const Color c = color(0.90f, 0.96f, 1.0f, alpha);
    const Vector3D p = Vector3D(splash.position.x, splash.position.y + 0.3f, splash.position.z);
    batch.quad(vert(Vector3D(p.x - radius, p.y, p.z - radius), 0.0f, 0.0f, c), vert(Vector3D(p.x + radius, p.y, p.z - radius), 1.0f, 0.0f, c),
               vert(Vector3D(p.x + radius, p.y, p.z + radius), 1.0f, 1.0f, c), vert(Vector3D(p.x - radius, p.y, p.z + radius), 0.0f, 1.0f, c));
}

// puffs of spray are thrown up, grow and fall back
void WaterSplashEffect::addSpray(Batch &batch, const Splash &splash, float age) const {
    const int count = 8;
    for (int k = 0; k < count; k++) {
        const float vy = (16.0f + 14.0f * hash01(splash.seed, 10 + k)) * splash.scale;
        const float angle = hash01(splash.seed, 30 + k) * 6.2831853f;
        const float speed = (2.0f + 6.0f * hash01(splash.seed, 50 + k)) * splash.scale;
        const float t = age;
        const float y = vy * t - 0.5f * GRAVITY * t * t;
        if (y < 0.0f && t > 0.1f) {
            continue;
        }
        const Vector3D pos = Vector3D(splash.position.x + cosf(angle) * speed * t, splash.position.y + std::max(0.0f, y),
                              splash.position.z + sinf(angle) * speed * t);
        const float life = age / LIFE;
        const float size = (1.6f + 4.2f * life) * splash.scale * (0.7f + 0.6f * hash01(splash.seed, 70 + k));
        const float alpha = 0.55f * (1.0f - smooth(0.25f, 1.0f, life)) * smooth(0.0f, 0.04f, life);
        this->addBillboard(batch, pos, size, color(0.93f, 0.97f, 1.0f, alpha));
    }
}

void WaterSplashEffect::addDroplets(Batch &batch, const Splash &splash, float age) const {
    const int count = 12;
    for (int k = 0; k < count; k++) {
        const float vy = (20.0f + 22.0f * hash01(splash.seed, 100 + k)) * splash.scale;
        const float angle = hash01(splash.seed, 120 + k) * 6.2831853f;
        const float speed = (4.0f + 12.0f * hash01(splash.seed, 140 + k)) * splash.scale;
        const float t = age;
        const float y = vy * t - 0.5f * GRAVITY * t * t;
        if (y < 0.0f && t > 0.1f) {
            continue;
        }
        const Vector3D pos = Vector3D(splash.position.x + cosf(angle) * speed * t, splash.position.y + std::max(0.0f, y),
                              splash.position.z + sinf(angle) * speed * t);
        this->addBillboard(batch, pos, 0.45f * splash.scale, color(0.85f, 0.94f, 1.0f, 0.9f));
    }
}

void WaterSplashEffect::draw(const float *view) {
    if (this->splashes.empty()) {
        return;
    }
    const float now = effectTime();
    this->splashes.erase(std::remove_if(this->splashes.begin(), this->splashes.end(),
                                        [now](const Splash &s) { return now - s.start > LIFE; }),
                         this->splashes.end());
    if (this->splashes.empty()) {
        return;
    }
    this->eye = CameraBasis::fromView(view).eye;

    Batch rings;
    for (const Splash &splash : this->splashes) {
        this->addRing(rings, splash, now - splash.start);
    }
    rings.flush(this->ringTexture(), BlendMode::Alpha);

    Batch spray;
    Batch drops;
    for (const Splash &splash : this->splashes) {
        this->addSpray(spray, splash, now - splash.start);
        this->addDroplets(drops, splash, now - splash.start);
    }
    spray.flush(this->puffTexture(), BlendMode::Alpha);
    drops.flush(EffectTextures::get().glow, BlendMode::Additive);
}

}  // namespace enhanced
