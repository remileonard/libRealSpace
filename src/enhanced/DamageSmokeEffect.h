#pragma once
#include <vector>
#include "EffectCommon.h"

namespace enhanced {

/** Soft, ageing smoke puffs with flames and embers for burning aircraft. */
class DamageSmokeEffect {
public:
    /**
     * @param points    puff positions in world space, newest first
     * @param intensity 0.2 (light damage, thin grey wisp) .. 1 (thick black)
     * @param burning   aircraft is on fire: flames and embers along the first part of the trail
     * @param view      modelview matrix (world -> eye)
     */
    void draw(const std::vector<Vector3D> &points, float intensity, bool burning, const float *view);

private:
    void addBillboard(Batch &batch, Vector3D center, float size, float rotation, Color c) const;
    void addSmoke(const std::vector<Vector3D> &points, float intensity);
    void addFire(const std::vector<Vector3D> &points, float intensity);

    Vector3D eye;
    float time{0.0f};
};

}  // namespace enhanced
