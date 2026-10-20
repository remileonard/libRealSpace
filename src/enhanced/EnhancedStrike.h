#pragma once
#include "../strike_commander/precomp.h"
#include <unordered_map>
#include <vector>
#include "../strike_commander/SCStrike.h"
#include "DamageSmokeEffect.h"
#include "ExhaustEffect.h"
#include "WingVortexEffect.h"

// Flight activity with visual effects (engine exhaust, wing tip vortices, damage smoke/fire) and mouse orbit for the
// external cameras.
class EnhancedStrike : public SCStrike {
public:
    EnhancedStrike() = default;
    void init() override;

protected:
    SCMission *createMission(char const *missionName) override;
    void afterPlaneRendered(SCPlane *plane, Vector3D position, int thrust) override;
    void afterInput() override;
    void afterCameraPlaced() override;

private:
    struct SmokeTrail {
        std::vector<enhanced::Vec3> points;  // newest first
        float lastSample{-1.0f};
        float lastSeen{0.0f};
        int stage{0};                        // 0 none, 1 smoke, 2 fire
    };

    static const size_t SMOKE_POINTS;
    static const float SMOKE_MIN_DAMAGE;
    static const float FIRE_MIN_DAMAGE;

    bool orbitAllowed() const;
    void updateOrbit();
    Vector3D orbitedPosition(Vector3D pivot, Vector3D position) const;
    float damageLevel(SCPlane *plane) const;
    enhanced::Mat4 modelMatrix(SCPlane *plane, Vector3D position) const;
    void forgetOldTrails(float now);
    void updateSmoke(SCPlane *plane, Vector3D position, float now, const float *view);

    bool orbitActive{false};
    bool orbitInvertY{true};
    float orbitYaw{0.0f};
    float orbitPitch{0.0f};
    float orbitZoom{1.0f};
    int orbitLastView{-1};
    int orbitTraceFrames{5};

    std::unordered_map<const SCPlane *, SmokeTrail> smoke;
    enhanced::ExhaustEffect exhaust;
    enhanced::WingVortexEffect vortices;
    enhanced::DamageSmokeEffect damageSmoke;
};
