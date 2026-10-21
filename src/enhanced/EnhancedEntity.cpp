//
//  EnhancedEntity.cpp
//
//  RSEntity with optional replacement geometry loaded from an OBJ file.
//
//  Convention: for an entity loaded from "<...>/F-16DES.IFF" the file "<models dir>/F-16DES.obj" is used when it
//  exists (default models dir: ./assets/models). Nothing needs to be configured, any game model can be replaced
//  by dropping a file with the same base name in that folder. Flight model, hard points and child objects keep
//  coming from the IFF, only the airframe geometry (and its LOD levels) is replaced.
//

#include <cfloat>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <array>
#include <algorithm>
#include <unordered_map>
#include <vector>
#include <string>
#include "EnhancedEntity.h"
#include "../engine/Config.hpp"
#include "../engine/SCRenderer.h"

std::string EnhancedEntity::models_dir = "./assets/models";
bool EnhancedEntity::wanted = true;
std::vector<EnhancedEntity *> EnhancedEntity::replacedEntities;

void EnhancedEntity::InitFromRAM(uint8_t *data, size_t size, std::string name) {
    RSEntity::InitFromRAM(data, size, name);
    // only aircraft (entities with a flight model) are replaced
    if (this->jdyn == nullptr) {
        return;
    }
    wanted = Config::getInstance().getString("Video", "enhanced_f16_model", "1") != "0";
    const std::string path = replacementPathFor(name);
    if (FILE *f = fopen(path.c_str(), "r")) {
        fclose(f);
        this->takeGeometry(this->original);
        if (this->replaceGeometryFromOBJ(path)) {
            this->takeGeometry(this->replacement);
            this->replaced = true;
            this->active = true;
            replacedEntities.push_back(this);
            this->useReplacement(wanted);
        } else {
            this->putGeometry(this->original);
        }
    }
}

void EnhancedEntity::takeGeometry(Geometry &geometry) const {
    geometry.vertices = this->vertices;
    geometry.triangles = this->triangles;
    geometry.lods = this->lods;
    geometry.attrs.clear();
    for (const auto &kv : this->attrs) {
        geometry.attrs[kv.first] = new Attr(*kv.second);
    }
    geometry.uvs = this->uvs;
    geometry.quads = this->quads;
    geometry.qmapuvs = this->qmapuvs;
    geometry.childY.clear();
    for (const CHLD *c : this->chld) {
        geometry.childY.push_back(c != nullptr ? c->y : 0);
    }
}

void EnhancedEntity::putGeometry(const Geometry &geometry) {
    this->vertices = geometry.vertices;
    this->triangles = geometry.triangles;
    this->lods = geometry.lods;
    for (auto &kv : this->attrs) {
        delete kv.second;
    }
    this->attrs.clear();
    for (const auto &kv : geometry.attrs) {
        this->attrs[kv.first] = new Attr(*kv.second);
    }
    this->uvs = geometry.uvs;
    this->quads = geometry.quads;
    this->qmapuvs = geometry.qmapuvs;
    for (size_t i = 0; i < this->chld.size() && i < geometry.childY.size(); i++) {
        if (this->chld[i] != nullptr) {
            this->chld[i]->y = geometry.childY[i];
        }
    }
}

void EnhancedEntity::useReplacement(bool active) {
    if (!this->replaced || active == this->active) {
        return;
    }
    this->putGeometry(active ? this->replacement : this->original);
    this->active = active;
}

void EnhancedEntity::setReplacementsActive(bool active) {
    wanted = active;
    for (EnhancedEntity *entity : replacedEntities) {
        entity->useReplacement(active);
    }
    printf("[MODEL] replacement models %s (%zu aircraft models)\n", active ? "on" : "off", replacedEntities.size());
}

std::string EnhancedEntity::replacementPathFor(const std::string &entity_name) {
    size_t start = entity_name.find_last_of("/\\");
    start = start == std::string::npos ? 0 : start + 1;
    size_t end = entity_name.find_last_of('.');
    if (end == std::string::npos || end < start) {
        end = entity_name.size();
    }
    return models_dir + "/" + entity_name.substr(start, end - start) + ".obj";
}

static Vector3D closestPointOnTriangle(const Vector3D &p, const Vector3D &a, const Vector3D &b, const Vector3D &c) {
    auto sub = [](const Vector3D &u, const Vector3D &v) { return Vector3D{u.x - v.x, u.y - v.y, u.z - v.z}; };
    auto dot = [](const Vector3D &u, const Vector3D &v) { return u.x * v.x + u.y * v.y + u.z * v.z; };
    auto madd = [](const Vector3D &o, const Vector3D &d, float t) { return Vector3D{o.x + d.x * t, o.y + d.y * t, o.z + d.z * t}; };
    Vector3D ab = sub(b, a), ac = sub(c, a), ap = sub(p, a);
    float d1 = dot(ab, ap), d2 = dot(ac, ap);
    if (d1 <= 0 && d2 <= 0) return a;
    Vector3D bp = sub(p, b);
    float d3 = dot(ab, bp), d4 = dot(ac, bp);
    if (d3 >= 0 && d4 <= d3) return b;
    float vc = d1 * d4 - d3 * d2;
    if (vc <= 0 && d1 >= 0 && d3 <= 0) return madd(a, ab, d1 / (d1 - d3));
    Vector3D cp = sub(p, c);
    float d5 = dot(ab, cp), d6 = dot(ac, cp);
    if (d6 >= 0 && d5 <= d6) return c;
    float vb = d5 * d2 - d1 * d6;
    if (vb <= 0 && d2 >= 0 && d6 <= 0) return madd(a, ac, d2 / (d2 - d6));
    float va = d3 * d6 - d5 * d4;
    if (va <= 0 && (d4 - d3) >= 0 && (d5 - d6) >= 0) return madd(b, sub(c, b), (d4 - d3) / ((d4 - d3) + (d5 - d6)));
    float denom = 1.0f / (va + vb + vc);
    return madd(madd(a, ab, vb * denom), ac, vc * denom);
}

bool EnhancedEntity::replaceGeometryFromOBJ(const std::string &path) {
    FILE *f = fopen(path.c_str(), "r");
    if (!f) {
        printf("[MODEL] cannot open %s, keeping original geometry\n", path.c_str());
        return false;
    }
    std::vector<Vector3D> nv;
    std::vector<std::vector<std::array<uint32_t, 4>>> groups;
    uint32_t curTag = 0;  // 0 = airframe (colour from the original model), 1 = glass, 0x1000000|RGB = fixed colour
    float lift = 0.0f;  // "# lift <units>": how far the airframe was raised relative to the original model
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#') {
            sscanf(line, "# lift %f", &lift);
        } else if (line[0] == 'v' && line[1] == ' ') {
            Vector3D v;
            if (sscanf(line + 2, "%f %f %f", &v.x, &v.y, &v.z) == 3) nv.push_back(v);
        } else if (line[0] == 'g') {
            groups.emplace_back();
            curTag = 0;
        } else if (strncmp(line, "usemtl ", 7) == 0) {
            unsigned rgb = 0;
            if (strncmp(line + 7, "glass", 5) == 0) curTag = 1;
            else if (sscanf(line + 7, "rgb_%x", &rgb) == 1) curTag = 0x1000000u | (rgb & 0xFFFFFFu);
            else curTag = 0;
        } else if (line[0] == 'f' && line[1] == ' ') {
            int a, b, c;
            if (sscanf(line + 2, "%d %d %d", &a, &b, &c) == 3) {
                if (groups.empty()) groups.emplace_back();
                groups.back().push_back({(uint32_t)(a - 1), (uint32_t)(b - 1), (uint32_t)(c - 1), curTag});
            }
        }
    }
    fclose(f);
    if (nv.empty() || groups.empty() || nv.size() > 65535) {
        printf("[MODEL] %s: unusable (%zu vertices, %zu groups), keeping original geometry\n", path.c_str(), nv.size(), groups.size());
        return false;
    }

    // Colour of every original LOD0 triangle (textured triangles use their first texel)
    struct Src { Vector3D a, b, c, n; uint8_t color; };
    std::vector<Src> src;
    if (!this->lods.empty()) {
        for (int i = 0; i < this->lods[LOD_LEVEL_MAX].numTriangles; i++) {
            uint16_t tid = this->lods[LOD_LEVEL_MAX].triangleIDs[i];
            if (this->attrs.size() > 0) {
                if (this->attrs[tid] == nullptr || this->attrs[tid]->type != 'T') continue;
                tid = this->attrs[tid]->id;
            }
            if (tid >= this->triangles.size()) continue;
            Triangle &t = this->triangles[tid];
            if (t.property == RSEntity::SC_TRANSPARENT) continue;
            Src s;
            s.a = this->vertices[t.ids[0]]; s.b = this->vertices[t.ids[1]]; s.c = this->vertices[t.ids[2]];
            Vector3D e1 = {s.b.x - s.a.x, s.b.y - s.a.y, s.b.z - s.a.z}, e2 = {s.c.x - s.a.x, s.c.y - s.a.y, s.c.z - s.a.z};
            Vector3D n = {e1.y * e2.z - e1.z * e2.y, e1.z * e2.x - e1.x * e2.z, e1.x * e2.y - e1.y * e2.x};
            float l = sqrtf(n.x * n.x + n.y * n.y + n.z * n.z);
            if (l < 1e-6f) continue;
            s.n = {n.x / l, n.y / l, n.z / l};
            s.color = t.color;
            if (t.property == 6) {
                for (auto &uv : this->uvs) {
                    if (uv.triangleID == tid && uv.textureID < this->images.size()) { s.color = this->images[uv.textureID]->data[0]; break; }
                }
            }
            src.push_back(s);
        }
    }

    std::vector<Triangle> ntris;
    std::vector<Lod> nlods;
    std::vector<std::pair<size_t, size_t>> granges;
    std::vector<Vector3D> tnorm, tcen;
    int agree = 0, disagree = 0;
    std::vector<float> dots;
    std::vector<uint8_t> fixedTri;  // glass and fixed-colour triangles: no colour rule, no smoothing
    for (size_t g = 0; g < groups.size(); g++) {
        Lod lod;
        size_t gstart = ntris.size();
        lod.dist = (g < this->lods.size()) ? this->lods[g].dist : (uint32_t)(g * 500);
        lod.numTriangles = 0;
        for (auto &tri : groups[g]) {
            if (tri[0] >= nv.size() || tri[1] >= nv.size() || tri[2] >= nv.size()) continue;
            if (lod.numTriangles >= 16336) break;
            Vector3D a = nv[tri[0]], b = nv[tri[1]], c = nv[tri[2]];
            Vector3D e1 = {b.x - a.x, b.y - a.y, b.z - a.z}, e2 = {c.x - a.x, c.y - a.y, c.z - a.z};
            Vector3D n = {e1.y * e2.z - e1.z * e2.y, e1.z * e2.x - e1.x * e2.z, e1.x * e2.y - e1.y * e2.x};
            float l = sqrtf(n.x * n.x + n.y * n.y + n.z * n.z);
            if (l > 1e-9f) n = {n.x / l, n.y / l, n.z / l};
            Vector3D cen = {(a.x + b.x + c.x) / 3, (a.y + b.y + c.y) / 3, (a.z + b.z + c.z) / 3};
            uint8_t colour = 0;
            float best = FLT_MAX, bestDot = 0;
            const uint32_t tag = tri[3];
            for (auto &s : src) {
                if (tag != 0) break;
                Vector3D q = closestPointOnTriangle(cen, s.a, s.b, s.c);
                float dx = q.x - cen.x, dy = q.y - cen.y, dz = q.z - cen.z;
                float d = sqrtf(dx * dx + dy * dy + dz * dz);
                float dp = n.x * s.n.x + n.y * s.n.y + n.z * s.n.z;
                float score = d + 4.0f * (1.0f - fabsf(dp));
                if (score < best) { best = score; colour = s.color; bestDot = dp; }
            }
            if (g == 0 && tag == 0) { if (bestDot > 0.5f) agree++; else if (bestDot < -0.5f) disagree++; }
            Triangle t;
            t.property = 0;
            t.ids[0] = (uint16_t)tri[0]; t.ids[1] = (uint16_t)tri[1]; t.ids[2] = (uint16_t)tri[2];
            t.color = colour;
            t.flags[0] = t.flags[1] = t.flags[2] = 0;
            if (tag == 1) {
                t.property = RSEntity::SC_TRANSPARENT;  // canopy glass: drawn additively with a sun glint
                t.flags[1] = 0xA7;
            } else if (tag != 0) {
                t.color = (uint8_t)SCRenderer::getInstance().nearestPaletteColour((tag >> 16) & 0xFF, (tag >> 8) & 0xFF, tag & 0xFF);
            }
            fixedTri.push_back(tag != 0 ? 1 : 0);
            lod.triangleIDs[lod.numTriangles++] = (uint16_t)ntris.size();
            ntris.push_back(t);
            tnorm.push_back(n);
            tcen.push_back(cen);
        }
        nlods.push_back(lod);
        granges.push_back({gstart, ntris.size()});
    }
    // Colour by direction of the (outward) face normal instead of the nearest original face:
    // top surfaces use the camouflage colour, undersides and fuselage sides the grey, the nose keeps
    // the colour of the nearest original face. The two colours are taken from the nearest-face result.
    if (!granges.empty() && granges[0].second > granges[0].first) {
        std::unordered_map<uint8_t, int> topVotes, sideVotes;
        for (size_t i = granges[0].first; i < granges[0].second; i++) {
            if (fixedTri[i]) continue;
            const Vector3D &n = tnorm[i], &c = tcen[i];
            if (n.y > 0.7f && c.x < 11.0f) topVotes[ntris[i].color]++;
            if (fabsf(n.y) < 0.3f && fabsf(c.z) < 1.3f && c.x > -6.0f && c.x < 8.0f && c.y < 1.5f) sideVotes[ntris[i].color]++;
        }
        auto best = [](std::unordered_map<uint8_t, int> &m, int &out) {
            int bv = -1;
            for (auto &kv : m) if (kv.second > bv) { bv = kv.second; out = kv.first; }
            return bv > 0;
        };
        int topC = 0, sideC = 0;
        if (best(topVotes, topC) && best(sideVotes, sideC) && topC != sideC) {
            for (size_t i = 0; i < ntris.size(); i++) {
                if (fixedTri[i]) continue;
                const Vector3D &n = tnorm[i], &c = tcen[i];
                if (c.x >= 11.0f) continue;
                bool fin = (c.y > 3.0f && fabsf(n.y) <= 0.25f && fabsf(c.z) < 0.9f);
                ntris[i].color = (n.y > 0.25f || fin) ? (uint8_t)topC : (uint8_t)sideC;
            }
            printf("[MODEL] colours: top=%d side=%d\n", topC, sideC);
        }
    }
    // The nearest original face flips between neighbouring triangles: majority vote over the
    // triangles sharing a vertex removes the speckles (a few passes).
    for (auto &gr : granges) {
        size_t n = gr.second - gr.first;
        if (n == 0) continue;
        // neighbours are found by vertex position, so hard-edge duplicates still count as neighbours
        auto posKey = [&](uint16_t id) {
            const Vector3D &p = nv[id];
            auto q = [](float x) { return (int64_t)llroundf(x * 200.0f) & 0x1FFFFF; };
            return ((uint64_t)q(p.x) << 42) | ((uint64_t)q(p.y) << 21) | (uint64_t)q(p.z);
        };
        std::unordered_map<uint64_t, std::vector<uint32_t>> vt;
        for (size_t i = 0; i < n; i++)
            if (!fixedTri[gr.first + i])
                for (int k = 0; k < 3; k++) vt[posKey(ntris[gr.first + i].ids[k])].push_back((uint32_t)i);
        for (int pass = 0; pass < 16; pass++) {
            std::vector<uint8_t> nc(n);
            for (size_t i = 0; i < n; i++) {
                if (fixedTri[gr.first + i]) { nc[i] = ntris[gr.first + i].color; continue; }
                std::unordered_map<uint8_t, int> votes;
                votes[ntris[gr.first + i].color] += 1;
                for (int k = 0; k < 3; k++)
                    for (uint32_t j : vt[posKey(ntris[gr.first + i].ids[k])]) votes[ntris[gr.first + j].color]++;
                uint8_t bestc = ntris[gr.first + i].color;
                int bestv = votes[bestc];
                for (auto &kv : votes)
                    if (kv.second > bestv) { bestv = kv.second; bestc = kv.first; }
                nc[i] = bestc;
            }
            for (size_t i = 0; i < n; i++) ntris[gr.first + i].color = nc[i];
        }
    }
    // Same winding convention as the original model: flip all if most triangles disagree
    if (disagree > agree) {
        for (auto &t : ntris) std::swap(t.ids[1], t.ids[2]);
    }
    if (ntris.size() > 65535) return false;
    this->vertices = nv;
    this->triangles = ntris;
    this->lods = nlods;
    for (auto &kv : this->attrs) delete kv.second;
    this->attrs.clear();
    this->uvs.clear();
    this->quads.clear();
    this->qmapuvs.clear();
    // The afterburner flame is a child object with a fixed offset: move it with the airframe
    if (lift != 0.0f) {
        for (auto *c : this->chld) {
            if (c && c->name.find("F16-AB") != std::string::npos) c->y += (int32_t)lroundf(lift * 256.0f);
        }
    }
    printf("[MODEL] F-16 airframe replaced from %s: %zu vertices, %zu triangles, %zu LODs (winding %s)\n", path.c_str(), nv.size(), ntris.size(),
           nlods.size(), disagree > agree ? "flipped" : "kept");
    return true;
}
