#include "WingVortexEffect.h"
#include <cstdio>
#include "../realspace/RSEntity.h"

namespace enhanced {

const float WingVortexEffect::LIFE = 3.0f;

WingVortexEffect::RibbonStyle WingVortexEffect::makeStyle(float widthStart, float widthEnd, float alpha, float helixRadius,
                                                          float helixPhase, float helixTurns, Color tint) {
    RibbonStyle st;
    st.widthStart = widthStart;
    st.widthEnd = widthEnd;
    st.alpha = alpha;
    st.helixRadius = helixRadius;
    st.helixPhase = helixPhase;
    st.helixTurns = helixTurns;
    st.tint = tint;
    return st;
}

const WingVortexEffect::WingTips &WingVortexEffect::wingTips(const RSEntity *entity) {
    auto found = tips.find(entity);
    if (found != tips.end()) {
        return found->second;
    }
    WingTips wt;
    float zmax = 0.0f;
    float zmin = 0.0f;
    for (const auto &v : entity->vertices) {
        zmax = std::max(zmax, (float)v.z);
        zmin = std::min(zmin, (float)v.z);
    }
    if (std::min(zmax, -zmin) > 1.0f) {
        for (int s = 0; s < 2; s++) {
            float ext = s == 0 ? zmax : zmin;
            double sx = 0;
            double sy = 0;
            int n = 0;
            for (const auto &v : entity->vertices) {
                if ((s == 0 ? v.z : -v.z) > 0.94f * fabsf(ext)) {
                    sx += v.x;
                    sy += v.y;
                    n++;
                }
            }
            if (n > 0) {
                wt.p[s] = Vector3D((float)(sx / n) - 0.4f, (float)(sy / n), ext);
                wt.valid = true;
            }
        }
    }
    printf("[enhanced] wing tips of entity %p: %s\n", (const void *)entity, wt.valid ? "found" : "not found");
    return tips.emplace(entity, wt).first->second;
}

void WingVortexEffect::forgetOldTrails(float now) {
    for (auto it = trails.begin(); it != trails.end();) {
        if (now - it->second.lastSeen > 6.0f) {
            it = trails.erase(it);
        } else {
            ++it;
        }
    }
}

void WingVortexEffect::sample(Trail &trail, const Vector3D *tipsWorld, float now, float strength) {
    if (now - trail.lastSample < 0.025f && trail.lastSample >= 0.0f) {
        return;
    }
    trail.lastSample = now;
    for (int k = 0; k < 2; k++) {
        auto &d = trail.side[k];
        if (!d.empty() && (d.back().p - tipsWorld[k]).Length() > 60.0f) {
            d.clear();
        }
        Sample s;
        s.p = tipsWorld[k];
        s.t = now;
        s.s = strength;
        d.push_back(s);
        while (!d.empty() && now - d.front().t > 3.2f) {
            d.pop_front();
        }
    }
}

bool WingVortexEffect::hasVisibleSamples(const Trail &trail, float strength) const {
    if (strength > 0.01f) {
        return true;
    }
    for (int k = 0; k < 2; k++) {
        for (const auto &s : trail.side[k]) {
            if (s.s > 0.01f) {
                return true;
            }
        }
    }
    return false;
}

void WingVortexEffect::buildFrames(const Vector3D &eye) {
    const size_t n = points.size();
    distance.assign(n, 0.0f);
    frames.resize(n);
    for (size_t i = 1; i < n; i++) {
        distance[i] = distance[i - 1] + (points[i].p - points[i - 1].p).Length();
    }
    for (size_t i = 0; i < n; i++) {
        size_t a = i > 0 ? i - 1 : i;
        size_t b = i + 1 < n ? i + 1 : i;
        Vector3D tangent = (points[b].p - points[a].p).Normalized(Vector3D(1.0f, 0.0f, 0.0f));
        frames[i].s = Vector3D::Cross(tangent, eye - points[i].p).Normalized(Vector3D(0.0f, 1.0f, 0.0f));
        frames[i].b1 = Vector3D::Cross(tangent, Vector3D(0.0f, 1.0f, 0.0f)).Normalized(Vector3D(0.0f, 0.0f, 1.0f));
        frames[i].b2 = Vector3D::Cross(tangent, frames[i].b1);
    }
}

void WingVortexEffect::addRibbon(int side, const RibbonStyle &st, float now) {
    const size_t n = points.size();
    EffectVertex prevLeft;
    EffectVertex prevRight;
    for (size_t i = 0; i < n; i++) {
        float age = now - points[i].t;
        float u = std::min(1.0f, age / LIFE);
        float a = st.alpha * points[i].s * powf(1.0f - u, 1.4f) * smooth(0.0f, 0.12f, u + 0.02f);
        float w = st.widthStart + (st.widthEnd - st.widthStart) * sqrtf(u);
        Vector3D c = points[i].p;
        if (st.helixRadius > 0.0f) {
            float ph = st.helixPhase + st.helixTurns * age * 6.2831853f;
            float hr = st.helixRadius * (0.4f + 1.6f * u);
            c = c + (frames[i].b1 * cosf(ph) + frames[i].b2 * sinf(ph)) * hr;
        }
        Color col = color(st.tint.r, st.tint.g, st.tint.b, std::min(1.0f, a));
        float tv = distance[i] * 0.22f + (side ? 0.37f : 0.0f);
        EffectVertex left = vert(c + frames[i].s * w, 0.0f, tv, col);
        EffectVertex right = vert(c - frames[i].s * w, 1.0f, tv, col);
        if (i > 0) {
            batch.stripSegment(prevLeft, prevRight, left, right);
        }
        prevLeft = left;
        prevRight = right;
    }
    batch.flush(noiseTexture, BlendMode::Alpha);
}

void WingVortexEffect::drawSide(int side, const Trail &trail, const Vector3D &tip, const Vector3D &eye, float now, float strength) {
    points.assign(trail.side[side].begin(), trail.side[side].end());
    Sample current;
    current.p = tip;
    current.t = now;
    current.s = strength;
    points.push_back(current);
    if (points.size() < 2) {
        return;
    }
    buildFrames(eye);
    float sg = side == 0 ? 1.0f : -1.0f;
    // shadow side (contrast against bright sky), diffuse condensation, two vortex strands
    addRibbon(side, makeStyle(0.30f, 1.9f, 0.34f, 0.0f, 0.0f, 0.0f, color(0.46f, 0.55f, 0.70f, 1.0f)), now);
    addRibbon(side, makeStyle(0.20f, 1.4f, 0.62f, 0.0f, 0.0f, 0.0f, color(0.97f, 0.98f, 1.0f, 1.0f)), now);
    addRibbon(side, makeStyle(0.09f, 0.30f, 1.0f, 0.14f, 0.0f, sg * 1.6f, color(1.0f, 1.0f, 1.0f, 1.0f)), now);
    addRibbon(side, makeStyle(0.07f, 0.26f, 0.80f, 0.14f, 3.14159f, sg * 1.6f, color(0.95f, 0.98f, 1.0f, 1.0f)), now);
}

void WingVortexEffect::draw(const void *key, const RSEntity *entity, const Mat4 &model, const float *view, float gLoad) {
    const float now = effectTime();
    forgetOldTrails(now);
    const WingTips &wt = wingTips(entity);
    if (!wt.valid) {
        return;
    }
    const float strength = smooth(2.2f, 6.0f, fabsf(gLoad));
    Trail &trail = trails[key];
    trail.lastSeen = now;

    Vector3D tipsWorld[2];
    tipsWorld[0] = model.point(wt.p[0]);
    tipsWorld[1] = model.point(wt.p[1]);
    sample(trail, tipsWorld, now, strength);

    bool visible = hasVisibleSamples(trail, strength);
    if (visible != trail.active) {
        printf("[enhanced] vortices %p: %s (g load %.1f)\n", key, visible ? "start" : "end", gLoad);
        trail.active = visible;
    }
    if (!visible) {
        return;
    }
    noiseTexture = EffectTextures::get().noise;
    Vector3D eye = CameraBasis::fromView(view).eye;
    for (int side = 0; side < 2; side++) {
        drawSide(side, trail, tipsWorld[side], eye, now, strength);
    }
}

}  // namespace enhanced
