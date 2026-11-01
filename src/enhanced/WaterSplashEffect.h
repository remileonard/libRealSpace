#pragma once
#include <unordered_set>
#include <vector>
#include "EffectCommon.h"

class SCMission;
class SCExplosion;
class RSArea;

namespace enhanced {

/** Water splashes for shots and bombs that hit the sea: a ring wave, a spray fountain and droplets. */
class WaterSplashEffect {
public:
    /** Looks for new explosions over open water, replaces them by splashes. Call once per frame before the scene is drawn. */
    void scan(SCMission *mission);
    void draw(const float *view);

private:
    struct Splash {
        Vector3D position;
        float start;
        float scale;
        uint32_t seed;
    };

    static const size_t MAX_SPLASHES;
    static const float LIFE;

    bool isOpenWater(RSArea *area, float x, float z) const;
    void addBillboard(Batch &batch, Vector3D center, float size, Color c) const;
    void addRing(Batch &batch, const Splash &splash, float age) const;
    void addSpray(Batch &batch, const Splash &splash, float age) const;
    void addDroplets(Batch &batch, const Splash &splash, float age) const;
    unsigned int ringTexture();
    unsigned int puffTexture();

    std::vector<Splash> splashes;
    std::unordered_set<SCExplosion *> seen;
    unsigned int ring{0};
    unsigned int puff{0};
    Vector3D eye{0.0f, 0.0f, 0.0f};
};

}  // namespace enhanced
