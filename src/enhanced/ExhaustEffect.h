#pragma once
#include <unordered_map>
#include "EffectCommon.h"

namespace enhanced {

/** Thrust-dependent engine exhaust: hot gas with Mach diamonds at military power, long turbulent plume in afterburner. */
class ExhaustEffect {
public:
    /**
     * @param key     identifies the aircraft (for the debug traces)
     * @param model   aircraft model -> world
     * @param nozzle  nozzle exit in model space; the plume points to model -x
     * @param thrust  throttle 0..100
     * @param view    modelview matrix (world -> eye) of the current camera
     */
    void draw(const void *key, const Mat4 &model, Vec3 nozzle, int thrust, const float *view);

private:
    void addStrip(float length, float width, float swell, Color c, float repeat, float speed, float phase);
    void addSprite(float x, float radius, Color c);
    void addDiamond(float x, float halfLength, float halfWidth, float alpha);
    void addHotGas();
    void addAfterburner();
    Vec3 toWorld(float x, float y, float z) const;
    Vec3 sideAxis(float x, float *facing) const;
    void traceStage(const void *key, int thrust);

    static const float NOZZLE_RADIUS;

    Mat4 model;
    Vec3 nozzle;
    Vec3 eye;
    Vec3 camRight;
    Vec3 camUp;
    float time{0.0f};
    float flicker{1.0f};
    float dry{0.0f};
    float ab{0.0f};
    float abp{0.0f};
    float t{0.0f};
    float endon{0.0f};
    Batch strips;
    Batch glows;
    std::unordered_map<const void *, int> stages;
};

}  // namespace enhanced
