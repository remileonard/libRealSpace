#pragma once
#include <deque>
#include <unordered_map>
#include <vector>
#include "EffectCommon.h"

class RSEntity;

namespace enhanced {

/** Condensation trails behind the wing tips while pulling G. Keeps the sampled tip positions per aircraft. */
class WingVortexEffect {
public:
    /**
     * @param key    identifies the aircraft (any stable pointer)
     * @param entity its model; the wing tips are derived from the vertices
     * @param model  model -> world matrix
     * @param view   modelview matrix (world -> eye)
     */
    void draw(const void *key, const RSEntity *entity, const Mat4 &model, const float *view, float gLoad);

private:
    struct Sample {
        Vector3D p;
        float t;
        float s;
    };
    struct Trail {
        std::deque<Sample> side[2];
        float lastSample{-1.0f};
        float lastSeen{0.0f};
        bool active{false};
    };
    struct WingTips {
        Vector3D p[2];
        bool valid{false};
    };
    struct Frame {
        Vector3D b1;
        Vector3D b2;
        Vector3D s;
    };
    struct RibbonStyle {
        float widthStart;
        float widthEnd;
        float alpha;
        float helixRadius;
        float helixPhase;
        float helixTurns;
        Color tint;
    };

    static RibbonStyle makeStyle(float widthStart, float widthEnd, float alpha, float helixRadius, float helixPhase,
                                 float helixTurns, Color tint);
    const WingTips &wingTips(const RSEntity *entity);
    void forgetOldTrails(float now);
    void sample(Trail &trail, const Vector3D *tips, float now, float strength);
    bool hasVisibleSamples(const Trail &trail, float strength) const;
    void buildFrames(const Vector3D &eye);
    void addRibbon(int side, const RibbonStyle &style, float now);
    void drawSide(int side, const Trail &trail, const Vector3D &tip, const Vector3D &eye, float now, float strength);

    static const float LIFE;

    std::unordered_map<const void *, Trail> trails;
    std::unordered_map<const RSEntity *, WingTips> tips;
    std::vector<Sample> points;
    std::vector<float> distance;
    std::vector<Frame> frames;
    Batch batch;
    unsigned int noiseTexture{0};
};

}  // namespace enhanced
