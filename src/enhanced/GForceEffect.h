#pragma once
#include "EffectCommon.h"

namespace enhanced {

/**
 * G-force vision in the first person views: tunnel vision, grey-out and a (never complete) blackout at high positive G,
 * red-out at negative G. The G load is low-passed (the body reacts slowly), the effect is drawn as a screen overlay.
 */
class GForceEffect {
public:
    /**
     * @param threshold G load where the tunnel vision starts (full grey-out/blackout follows about 4 G above it)
     * @param strength  0..2, scales all effects
     */
    void configure(float threshold, float strength);

    /**
     * Call once per frame.
     * @param gLoad current G load of the player's aircraft
     * @param active false outside the first person views or when the pilot is gone: the effect fades out at once
     */
    void update(float gLoad, bool active);

    /** Draws the overlay over the finished picture, does nothing while no effect is visible. */
    void draw();

private:
    void createTextures();

    float threshold{4.5f};
    float strength{1.0f};
    float smoothedG{1.0f};
    float lastTime{-1.0f};
    float tunnel{0.0f};
    float grey{0.0f};
    float black{0.0f};
    float red{0.0f};
    unsigned int white{0};
};

}  // namespace enhanced
