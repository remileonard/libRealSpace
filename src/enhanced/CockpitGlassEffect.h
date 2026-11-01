#pragma once
#include <vector>
#include "EffectCommon.h"

namespace enhanced {

/** Canopy glass seen from the 3D cockpit: reflection, sun glare, fine scratches and impact cracks that follow the damage. */
class CockpitGlassEffect {
public:
    /**
     * @param view   modelview matrix (world -> eye)
     * @param proj   projection matrix
     * @param sunDir world space direction towards the sun
     * @param damage 0 (intact) .. 1 (destroyed)
     */
    void draw(const float *view, const float *proj, Vector3D sunDir, float damage);

private:
    struct Impact {
        float x;
        float y;
        float size;
        float rotation;
    };

    void createTextures();
    void updateImpacts(float damage);
    struct ViewContext {
        CameraBasis cam;
        Vector3D forward;
        const float *proj;
        float distance;
        float aspectFix;
    };

    struct DomeStyle {
        Color bottom;
        Color top;
        float edgeAlpha;
        int uvMode;           // 0 constant, 1 texture over the whole dome, 2 band across the dome
        float bandAngle;
        float bandOffset;
        float bandWidth;
    };

    Vector3D screenPoint(const ViewContext &v, float nx, float ny) const;
    void addScreenQuad(Batch &batch, const ViewContext &v, float cx, float cy, float hx, float hy, float rotation, Color c) const;
    void addDome(Batch &batch, const ViewContext &v, const DomeStyle &style) const;

    unsigned int white{0};
    unsigned int scratches{0};
    unsigned int cracks{0};
    unsigned int streak{0};
    int impactCount{0};
    std::vector<Impact> impacts;
};

}  // namespace enhanced
