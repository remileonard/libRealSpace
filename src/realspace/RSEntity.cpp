//
//  RSEntity.cpp
//  libRealSpace
//
//  Created by fabien sanglard on 12/29/2013.
//  Copyright (c) 2013 Fabien Sanglard. All rights reserved.
//


#include <cfloat>
#include <cstdio>
#include <cmath>
#include <array>
#include <algorithm>
#include "RSEntity.h"
#include "RLEShape.h"
#include "RSImageSet.h"
#include "../commons/RLEBuffer.h"


RSEntity::~RSEntity() {
    while (!images.empty()) {
        RSImage *image = images.back();
        images.pop_back();
        delete image;
    }
}

void RSEntity::InitFromRAM(uint8_t *data, size_t size, std::string name) {
    this->name = name;
    IFFSaxLexer lexer;
    std::unordered_map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["REAL"] = std::bind(&RSEntity::parseREAL, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
    CalcBoundingBox();
    calcWingArea();
    if (!RSEntity::airframe_obj_path.empty() && this->jdyn != nullptr && this->name.find("F-16DES.IFF") != std::string::npos) {
        this->replaceGeometryFromOBJ(RSEntity::airframe_obj_path);
    }
}

std::string RSEntity::airframe_obj_path;

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

bool RSEntity::replaceGeometryFromOBJ(const std::string &path) {
    FILE *f = fopen(path.c_str(), "r");
    if (!f) {
        printf("[MODEL] cannot open %s, keeping original geometry\n", path.c_str());
        return false;
    }
    std::vector<Vector3D> nv;
    std::vector<std::vector<std::array<uint32_t, 3>>> groups;
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
        } else if (line[0] == 'f' && line[1] == ' ') {
            int a, b, c;
            if (sscanf(line + 2, "%d %d %d", &a, &b, &c) == 3) {
                if (groups.empty()) groups.emplace_back();
                groups.back().push_back({(uint32_t)(a - 1), (uint32_t)(b - 1), (uint32_t)(c - 1)});
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
            for (auto &s : src) {
                Vector3D q = closestPointOnTriangle(cen, s.a, s.b, s.c);
                float dx = q.x - cen.x, dy = q.y - cen.y, dz = q.z - cen.z;
                float d = sqrtf(dx * dx + dy * dy + dz * dz);
                float dp = n.x * s.n.x + n.y * s.n.y + n.z * s.n.z;
                float score = d + 4.0f * (1.0f - fabsf(dp));
                if (score < best) { best = score; colour = s.color; bestDot = dp; }
            }
            if (g == 0) { if (bestDot > 0.5f) agree++; else if (bestDot < -0.5f) disagree++; }
            Triangle t;
            t.property = 0;
            t.ids[0] = (uint16_t)tri[0]; t.ids[1] = (uint16_t)tri[1]; t.ids[2] = (uint16_t)tri[2];
            t.color = colour;
            t.flags[0] = t.flags[1] = t.flags[2] = 0;
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
            for (int k = 0; k < 3; k++) vt[posKey(ntris[gr.first + i].ids[k])].push_back((uint32_t)i);
        for (int pass = 0; pass < 16; pass++) {
            std::vector<uint8_t> nc(n);
            for (size_t i = 0; i < n; i++) {
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

BoudingBox *RSEntity::GetBoudingBpx(void) { return &this->bb; }

void RSEntity::CalcBoundingBox(void) {

    this->bb.min.x = FLT_MAX;
    this->bb.min.y = FLT_MAX;
    this->bb.min.z = FLT_MAX;

    this->bb.max.x = FLT_MIN;
    this->bb.max.y = FLT_MIN;
    this->bb.max.z = FLT_MIN;
    if (this->vertices.size() == 0)
        return;
    for (int i=0; i < this->lods[LOD_LEVEL_MAX].numTriangles; i++) {
        int triangle_id = this->lods[LOD_LEVEL_MAX].triangleIDs[i];
        char attribute = 'T';
        if (this->attrs.size() > 0) {
            if (this->attrs[triangle_id] != nullptr) {
                attribute = this->attrs[triangle_id]->type;
                triangle_id = this->attrs[triangle_id]->id;
            }
        }
        
        if (attribute == 'T') {
            for (int j = 0; j < 3; j++) {
                if (triangle_id >= this->triangles.size()) {
                    continue;
                }
                Point3D vertex = this->vertices[this->triangles[triangle_id].ids[j]];

                if (bb.min.x > vertex.x)
                    bb.min.x = vertex.x;
                if (bb.min.y > vertex.y)
                    bb.min.y = vertex.y;
                if (bb.min.z > vertex.z)
                    bb.min.z = vertex.z;

                if (bb.max.x < vertex.x)
                    bb.max.x = vertex.x;
                if (bb.max.y < vertex.y)
                    bb.max.y = vertex.y;
                if (bb.max.z < vertex.z)
                    bb.max.z = vertex.z;
            }
        } else if (attribute == 'Q') {
            for (int j = 0; j < 4; j++) {
                Point3D vertex = this->vertices[this->quads[triangle_id]->ids[j]];

                if (bb.min.x > vertex.x)
                    bb.min.x = vertex.x;
                if (bb.min.y > vertex.y)
                    bb.min.y = vertex.y;
                if (bb.min.z > vertex.z)
                    bb.min.z = vertex.z;

                if (bb.max.x < vertex.x)
                    bb.max.x = vertex.x;
                if (bb.max.y < vertex.y)
                    bb.max.y = vertex.y;
                if (bb.max.z < vertex.z)
                    bb.max.z = vertex.z;
            }
        }
        
    }
}

void RSEntity::calcWingArea(void) {
    if (this->vertices.size() == 0)
        return;
    if (this->jdyn == NULL)
        return;
    std::vector<int> wing_ids;
    int vert_id = 0;
    for (auto vertex: this->vertices) {
        if (vertex.z == bb.min.z) {
            wing_ids.push_back(vert_id);
        }
        if (vertex.z == bb.max.z) {
            wing_ids.push_back(vert_id);
        }
        vert_id++;
    }

    std::vector<Point2Df> wing_surface_points;
    for (int i=0; i < this->lods[LOD_LEVEL_MAX].numTriangles; i++) {
        int triangle_id = this->lods[LOD_LEVEL_MAX].triangleIDs[i];
        char attribute = 'T';
        
        if (this->attrs.size() > 0) {
            if (this->attrs[triangle_id] != nullptr) {
                attribute = this->attrs[triangle_id]->type;
                triangle_id = this->attrs[triangle_id]->id;

            }
        }
        if (attribute == 'T') {
            if (triangle_id >= this->triangles.size()) {
                continue;
            }
            Triangle triangle = this->triangles[triangle_id];
            for (auto id : triangle.ids) {
                if (std::find(wing_ids.begin(), wing_ids.end(), id) != wing_ids.end()) {
                    for (int j = 0; j < 3; j++) {
                        Vector3D v = this->vertices[triangle.ids[j]];
                        Point2Df p = {v.x*0.5f, v.z * 0.5f};
                        if (std::find(wing_surface_points.begin(), wing_surface_points.end(), p) == wing_surface_points.end()) {
                            wing_surface_points.push_back(p);
                        }
                    }
                }
            }
        } else {
            Quads *quad = this->quads[triangle_id];
            for (auto id : quad->ids) {
                if (std::find(wing_ids.begin(), wing_ids.end(), id) != wing_ids.end()) {
                    for (int j = 0; j < 4; j++) {
                        Vector3D v = this->vertices[quad->ids[j]];
                        Point2Df p = {v.x*0.5f, v.z * 0.5f};
                        if (std::find(wing_surface_points.begin(), wing_surface_points.end(), p) == wing_surface_points.end()) {
                            wing_surface_points.push_back(p);
                        }
                    }
                }
            }
        }
        
    }

    if (!wing_surface_points.empty()) {
        float area = 0.0f;
        
        // Compute the convex hull of wing_surface_points using the monotone chain algorithm.
        auto cross = [](const Point2Df &O, const Point2Df &A, const Point2Df &B) -> float {
            return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
        };

        // Tri des points par coordonnées (x, puis y)
        std::sort(wing_surface_points.begin(), wing_surface_points.end(), [](const Point2Df &a, const Point2Df &b) {
            return (a.x == b.x) ? (a.y < b.y) : (a.x < b.x);
        });

        std::vector<Point2Df> hull;
        // Construction de la chaîne inférieure
        for (const auto &p : wing_surface_points) {
            while (hull.size() >= 2 && cross(hull[hull.size()-2], hull.back(), p) <= 0)
                hull.pop_back();
            hull.push_back(p);
        }
        // Construction de la chaîne supérieure
        for (int i = wing_surface_points.size() - 2, t = hull.size() + 1; i >= 0; i--) {
            while (hull.size() >= t && cross(hull[hull.size()-2], hull.back(), wing_surface_points[i]) <= 0)
                hull.pop_back();
            hull.push_back(wing_surface_points[i]);
        }
        // Retirer le dernier point car il est identique au premier
        if (!hull.empty())
            hull.pop_back();
        
        
        wing_surface_points = hull;
        int n = wing_surface_points.size();
        // Calcul de la surface selon la formule du polygone (formule de shoelace)
        for (int i = 0; i < n; i++) {
            Vector3D sp1 = {wing_surface_points[i].x*2, 0.0f , wing_surface_points[i].y*2};
            Vector3D sp2 = {wing_surface_points[(i + 1) % n].x*2, 0.0f , wing_surface_points[(i + 1) % n].y*2};
        }
        for (int i = 0; i < n; i++) {
            int next = (i + 1) % n;
            area += wing_surface_points[i].x * wing_surface_points[next].y - wing_surface_points[i].y * wing_surface_points[next].x;
        }
        area = std::fabs(area) / 2.0f;
        this->wing_area = area;
    }
}
size_t RSEntity::NumImages(void) { return this->images.size(); }

size_t RSEntity::NumVertice(void) { return this->vertices.size(); }

size_t RSEntity::NumUVs(void) { return this->uvs.size(); }

size_t RSEntity::NumLods(void) { return this->lods.size(); }

size_t RSEntity::NumTriangles(void) { return this->triangles.size(); }

void RSEntity::AddImage(RSImage *image) { this->images.push_back(image); }

void RSEntity::AddVertex(Point3D *vertex) { this->vertices.push_back(*vertex); }

void RSEntity::AddUV(uvxyEntry *uv) { this->uvs.push_back(*uv); }

void RSEntity::AddLod(Lod *lod) { this->lods.push_back(*lod); }

void RSEntity::AddTriangle(Triangle *triangle) { this->triangles.push_back(*triangle); }

void RSEntity::parseREAL(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;

    std::unordered_map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["OBJT"] = std::bind(&RSEntity::parseREAL_OBJT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["APPR"] = std::bind(&RSEntity::parseREAL_APPR, this, std::placeholders::_1, std::placeholders::_2);
    handlers["INFO"] = std::bind(&RSEntity::parseREAL_INFO, this, std::placeholders::_1, std::placeholders::_2);
    
    lexer.InitFromRAM(data, size, handlers);
}
void RSEntity::parseREAL_INFO(uint8_t *data, size_t size) {}
void RSEntity::parseREAL_OBJT(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;

    std::unordered_map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["INFO"] = std::bind(&RSEntity::parseREAL_OBJT_INFO, this, std::placeholders::_1, std::placeholders::_2);
    handlers["AFTB"] = std::bind(&RSEntity::parseREAL_OBJT_AFTB, this, std::placeholders::_1, std::placeholders::_2);
    handlers["JETP"] = std::bind(&RSEntity::parseREAL_OBJT_JETP, this, std::placeholders::_1, std::placeholders::_2);
    handlers["GRND"] = std::bind(&RSEntity::parseREAL_OBJT_GRND, this, std::placeholders::_1, std::placeholders::_2);
    handlers["ORNT"] = std::bind(&RSEntity::parseREAL_OBJT_ORNT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["EXTE"] = std::bind(&RSEntity::parseREAL_OBJT_EXTE, this, std::placeholders::_1, std::placeholders::_2);
    handlers["RNWY"] = std::bind(&RSEntity::parseREAL_OBJT_RNWY, this, std::placeholders::_1, std::placeholders::_2);
    handlers["MISS"] = std::bind(&RSEntity::parseREAL_OBJT_MISS, this, std::placeholders::_1, std::placeholders::_2);
    handlers["BOMB"] = std::bind(&RSEntity::parseREAL_OBJT_BOMB, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TRCR"] = std::bind(&RSEntity::parseREAL_OBJT_TRCR, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SWPN"] = std::bind(&RSEntity::parseREAL_OBJT_SWPN, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SMKG"] = std::bind(&RSEntity::parseREAL_OBJT_SMKG, this, std::placeholders::_1, std::placeholders::_2);
    handlers["EXPL"] = std::bind(&RSEntity::parseREAL_OBJT_EXPL, this, std::placeholders::_1, std::placeholders::_2);
    handlers["OMOB"] = std::bind(&RSEntity::parseREAL_OBJT_OMOB, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DEBR"] = std::bind(&RSEntity::parseREAL_OBJT_DEBR, this, std::placeholders::_1, std::placeholders::_2);
    handlers["PODR"] = std::bind(&RSEntity::parseREAL_OBJT_PODR, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DECY"] = std::bind(&RSEntity::parseREAL_OBJT_DECY, this, std::placeholders::_1, std::placeholders::_2);
    handlers["XMIT"] = std::bind(&RSEntity::parseREAL_OBJT_XMIT, this, std::placeholders::_1, std::placeholders::_2);
    
    lexer.InitFromRAM(data, size, handlers);
}
void RSEntity::parseREAL_OBJT_AFTB(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    this->entity_type = EntityType::aftb;
    std::unordered_map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["EXTE"] = std::bind(&RSEntity::parseREAL_OBJT_EXTE, this, std::placeholders::_1, std::placeholders::_2);
    handlers["APPR"] = std::bind(&RSEntity::parseREAL_OBJT_AFTB_APPR, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

void RSEntity::parseREAL_OBJT_AFTB_APPR(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;

    std::unordered_map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["AFTB"] = std::bind(&RSEntity::parseREAL_APPR_POLY, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

void RSEntity::parseREAL_OBJT_MISS(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    this->entity_type = EntityType::missiles;
    std::unordered_map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["EXPL"] = std::bind(&RSEntity::parseREAL_OBJT_JETP_EXPL, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SIGN"] = std::bind(&RSEntity::parseREAL_OBJT_MISS_SIGN, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TRGT"] = std::bind(&RSEntity::parseREAL_OBJT_MISS_TRGT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SMOK"] = std::bind(&RSEntity::parseREAL_OBJT_MISS_SMOK, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DAMG"] = std::bind(&RSEntity::parseREAL_OBJT_MISS_DAMG, this, std::placeholders::_1, std::placeholders::_2);
    handlers["WDAT"] = std::bind(&RSEntity::parseREAL_OBJT_MISS_WDAT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DATA"] = std::bind(&RSEntity::parseREAL_OBJT_MISS_DATA, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DYNM"] = std::bind(&RSEntity::parseREAL_OBJT_MISS_DYNM, this, std::placeholders::_1, std::placeholders::_2);


    lexer.InitFromRAM(data, size, handlers);
}
void RSEntity::parseREAL_OBJT_BOMB(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    this->entity_type = EntityType::bomb;
    std::unordered_map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["EXPL"] = std::bind(&RSEntity::parseREAL_OBJT_JETP_EXPL, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SIGN"] = std::bind(&RSEntity::parseREAL_OBJT_MISS_SIGN, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TRGT"] = std::bind(&RSEntity::parseREAL_OBJT_MISS_TRGT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SMOK"] = std::bind(&RSEntity::parseREAL_OBJT_MISS_SMOK, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DAMG"] = std::bind(&RSEntity::parseREAL_OBJT_MISS_DAMG, this, std::placeholders::_1, std::placeholders::_2);
    handlers["WDAT"] = std::bind(&RSEntity::parseREAL_OBJT_MISS_WDAT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DATA"] = std::bind(&RSEntity::parseREAL_OBJT_BOMB_DATA, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DYNM"] = std::bind(&RSEntity::parseREAL_OBJT_MISS_DYNM, this, std::placeholders::_1, std::placeholders::_2);


    lexer.InitFromRAM(data, size, handlers);
}
void RSEntity::parseREAL_OBJT_TRCR(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    this->entity_type = EntityType::tracer;
    std::unordered_map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["INFO"] = std::bind(&RSEntity::parseREAL_OBJT_TRCR_INFO, this, std::placeholders::_1, std::placeholders::_2);
    handlers["EXPL"] = std::bind(&RSEntity::parseREAL_OBJT_JETP_EXPL, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SIGN"] = std::bind(&RSEntity::parseREAL_OBJT_MISS_SIGN, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TRGT"] = std::bind(&RSEntity::parseREAL_OBJT_MISS_TRGT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SMOK"] = std::bind(&RSEntity::parseREAL_OBJT_MISS_SMOK, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DAMG"] = std::bind(&RSEntity::parseREAL_OBJT_MISS_DAMG, this, std::placeholders::_1, std::placeholders::_2);
    handlers["WDAT"] = std::bind(&RSEntity::parseREAL_OBJT_MISS_WDAT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DATA"] = std::bind(&RSEntity::parseREAL_OBJT_MISS_DATA, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DYNM"] = std::bind(&RSEntity::parseREAL_OBJT_MISS_DYNM, this, std::placeholders::_1, std::placeholders::_2);


    lexer.InitFromRAM(data, size, handlers);
}
void RSEntity::parseREAL_OBJT_PODR(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    this->entity_type = EntityType::podr;
    std::unordered_map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["EXPL"] = std::bind(&RSEntity::parseREAL_OBJT_JETP_EXPL, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SIGN"] = std::bind(&RSEntity::parseREAL_OBJT_MISS_SIGN, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TRGT"] = std::bind(&RSEntity::parseREAL_OBJT_MISS_TRGT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SMOK"] = std::bind(&RSEntity::parseREAL_OBJT_MISS_SMOK, this, std::placeholders::_1, std::placeholders::_2);
    handlers["WDAT"] = std::bind(&RSEntity::parseREAL_OBJT_MISS_WDAT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DATA"] = std::bind(&RSEntity::parseREAL_OBJT_PODR_DATA, this, std::placeholders::_1, std::placeholders::_2);

    lexer.InitFromRAM(data, size, handlers);
}
void RSEntity::parseREAL_OBJT_PODR_DATA(uint8_t *data, size_t size){
    ByteStream bs(data, size);
    std::string podr_subweapon_name = bs.ReadString(8);
    uint8_t podr_subweapon_number = bs.ReadByte();
    WEAPS *podr_data = new WEAPS();
    podr_data->name = podr_subweapon_name;
    podr_data->nb_weap = podr_subweapon_number;
    std::transform(podr_data->name.begin(), podr_data->name.end(), podr_data->name.begin(), ::toupper);
    std::string tmpname = assetsManager.object_root_path + podr_data->name + ".IFF";
    RSEntity *objct = new RSEntity();
    TreEntry *entry = assetsManager.GetEntryByName(tmpname);
    if (entry != nullptr) {
        objct->InitFromRAM(entry->data, entry->size, tmpname);
        podr_data->objct = objct;
    }
    this->weaps.push_back(podr_data);
}
void RSEntity::parseREAL_OBJT_JETP_INFO(uint8_t *data, size_t size){}
void RSEntity::parseREAL_OBJT_TRCR_INFO(uint8_t *data, size_t size) {
    ByteStream bs(data, size);
    TRCR_INFO *info = new TRCR_INFO();
    info->muzzle_speed = bs.ReadFixedFloatLE();
    info->unknown2 = bs.ReadFixedFloatLE();
    info->start_jitter = bs.ReadFixedFloatLE();
    info->end_jitter = bs.ReadFixedFloatLE();
    info->lifetime = bs.ReadFixedFloatLE();
    info->speed_jitter = bs.ReadFixedFloatLE();
    info->shooter_speed_part = bs.ReadFixedFloatLE();
    info->unknown8 = bs.ReadUShort();
    info->damage_factor = bs.ReadUShort();
    this->tracer_info = info;
}
void RSEntity::parseREAL_OBJT_MISS_EXPL(uint8_t *data, size_t size){}
void RSEntity::parseREAL_OBJT_MISS_SIGN(uint8_t *data, size_t size){}
void RSEntity::parseREAL_OBJT_MISS_TRGT(uint8_t *data, size_t size){}
void RSEntity::parseREAL_OBJT_MISS_SMOK(uint8_t *data, size_t size){
    
}
void RSEntity::parseREAL_OBJT_MISS_DAMG(uint8_t *data, size_t size){}
void RSEntity::parseREAL_OBJT_MISS_WDAT(uint8_t *data, size_t size){
    WDAT *wdat = new WDAT();
    ByteStream bs(data, size);
    wdat->damage = bs.ReadShort();
    wdat->radius = bs.ReadShort();
    wdat->launch_class = bs.ReadByte();
    wdat->weapon_id = bs.ReadByte();
    
    wdat->weapon_category = bs.ReadByte();
    wdat->target_domain = bs.ReadByte();
    wdat->weapon_aspec = bs.ReadByte();
    wdat->target_range = bs.ReadInt32LE();
    wdat->tracking_cone = bs.ReadByte();
    wdat->effective_range = bs.ReadInt32LE();
    wdat->fire_interval = (float) bs.ReadInt32LE() / 256.0f;
    this->wdat = wdat;
}
void RSEntity::parseREAL_OBJT_MISS_DATA(uint8_t *data, size_t size){
    ByteStream bs(data, size);
    this->missile_flight_time = bs.ReadInt32LE();
    this->missile_data_unknown = bs.ReadByte();
    this->missile_fuse_radius = bs.ReadUShort();
}
void RSEntity::parseREAL_OBJT_BOMB_DATA(uint8_t *data, size_t size) {
    // PlayerComponent_LoadFieldsWithRetry_9FAD0 lit 5 octets sans borne de chunk : le DATA de la GBU-15 n'en a que 3
    ByteStream bs(data, 5);
    this->bomb_guided = bs.ReadByte();
    this->bomb_arming_delay = bs.ReadByte();
    bs.ReadByte();
    this->bomb_lock_cone_rate = bs.ReadShort();
}
void RSEntity::parseREAL_OBJT_DECY(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    std::unordered_map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["DATA"] = std::bind(&RSEntity::parseREAL_OBJT_DECY_DATA, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SIGN"] = std::bind(&RSEntity::parseREAL_OBJT_JETP_SIGN, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TRGT"] = std::bind(&RSEntity::parseREAL_OBJT_JETP_TRGT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DYNM"] = std::bind(&RSEntity::parseREAL_OBJT_MISS_DYNM, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}
void RSEntity::parseREAL_OBJT_DECY_DATA(uint8_t *data, size_t size) {
    ByteStream bs(data, size);
    this->decoy_lifetime = (float) bs.ReadInt32LE();
}
void RSEntity::parseREAL_OBJT_MISS_DYNM(uint8_t *data, size_t size){
    IFFSaxLexer lexer;

    std::unordered_map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["DYNM"] = std::bind(&RSEntity::parseREAL_OBJT_JETP_DYNM_DYNM, this, std::placeholders::_1, std::placeholders::_2);
    handlers["MISS"] = std::bind(&RSEntity::parseREAL_OBJT_MISS_DYNM_MISS, this, std::placeholders::_1, std::placeholders::_2);
    handlers["ATMO"] = std::bind(&RSEntity::parseREAL_OBJT_MISS_DYNM_ATMO, this, std::placeholders::_1, std::placeholders::_2);
    handlers["GRAV"] = std::bind(&RSEntity::parseREAL_OBJT_MISS_DYNM_GRAV, this, std::placeholders::_1, std::placeholders::_2);
    handlers["AGRV"] = std::bind(&RSEntity::parseREAL_OBJT_MISS_DYNM_AGRV, this, std::placeholders::_1, std::placeholders::_2);
    handlers["GBMB"] = std::bind(&RSEntity::parseREAL_OBJT_MISS_DYNM_GBMB, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}
void RSEntity::parseREAL_OBJT_MISS_DYNM_AGRV(uint8_t *data, size_t size){}
void RSEntity::parseREAL_OBJT_MISS_DYNM_GRAV(uint8_t *data, size_t size){}
void RSEntity::parseREAL_OBJT_MISS_DYNM_ATMO(uint8_t *data, size_t size){
    ByteStream bs(data, size);
    this->drag_coefficient = bs.ReadFixedFloatLE();
}
void RSEntity::parseREAL_OBJT_MISS_DYNM_GBMB(uint8_t *data, size_t size){
    ByteStream bs(data, size);
    this->bomb_turn_rate = bs.ReadFixedFloatLE();
}
void RSEntity::parseREAL_OBJT_MISS_DYNM_MISS(uint8_t *data, size_t size){
    ByteStream bs(data, size);
    DYNN_MISS *dynn_miss = new DYNN_MISS();
    dynn_miss->turn_degre_per_sec = bs.ReadFixedFloatLE();
    dynn_miss->velovity_m_per_sec = bs.ReadFixedFloatLE();
    dynn_miss->acceleration = bs.ReadFixedFloatLE();
    dynn_miss->cruise_lateral = bs.ReadFixedFloatLE();
    dynn_miss->cruise_normal = bs.ReadFixedFloatLE();
    this->dynn_miss = dynn_miss;
}
void RSEntity::parseREAL_OBJT_INFO(uint8_t *data, size_t size) {}
void RSEntity::parseREAL_OBJT_JETP(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    this->entity_type = EntityType::jet;
    std::unordered_map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["EXPL"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_EXPL, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DEBR"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_DEBR, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DEST"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_DEST, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SMOK"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_SMOK, this, std::placeholders::_1, std::placeholders::_2);

    handlers["CHLD"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_CHLD, this, std::placeholders::_1, std::placeholders::_2);
    handlers["JINF"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_JINF, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DAMG"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_DAMG, this, std::placeholders::_1, std::placeholders::_2);
    handlers["EJEC"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_EJEC, this, std::placeholders::_1, std::placeholders::_2);

    handlers["SIGN"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_SIGN, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TRGT"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_TRGT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["CTRL"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_CTRL, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TOFF"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_TOFF, this, std::placeholders::_1, std::placeholders::_2);

    handlers["LAND"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_LAND, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DYNM"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_DYNM, this, std::placeholders::_1, std::placeholders::_2);
    handlers["WEAP"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_WEAP, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DAMG"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_DAMG, this, std::placeholders::_1, std::placeholders::_2);
    handlers["CKPT"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_CKPT, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}
void RSEntity::parseREAL_OBJT_ORNT(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    this->entity_type = EntityType::ornt;
    std::unordered_map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["EXPL"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_EXPL, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DEBR"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_DEBR, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DEST"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_DEST, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DAMG"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_DAMG, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TRGT"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_TRGT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DAMG"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_DAMG, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SIGN"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_SIGN, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}
void RSEntity::parseREAL_OBJT_XMIT(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    this->entity_type = EntityType::xmit;
    std::unordered_map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["EXPL"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_EXPL, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DEBR"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_DEBR, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DEST"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_DEST, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DAMG"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_DAMG, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TRGT"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_TRGT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DAMG"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_DAMG, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SIGN"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_SIGN, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}
void RSEntity::parseREAL_OBJT_GRND(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    this->entity_type = EntityType::ground;
    std::unordered_map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["EXPL"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_EXPL, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DEBR"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_DEBR, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DEST"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_DEST, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DAMG"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_DAMG, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TRGT"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_TRGT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DYNM"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_DYNM, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DAMG"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_DAMG, this, std::placeholders::_1, std::placeholders::_2);

    lexer.InitFromRAM(data, size, handlers);
}

void RSEntity::parseREAL_OBJT_SWPN(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    this->entity_type = EntityType::swpn;
    std::unordered_map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["EXPL"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_EXPL, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DEBR"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_DEBR, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DEST"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_DEST, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DAMG"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_DAMG, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SIGN"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_SIGN, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TRGT"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_TRGT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DYNM"] =
        std::bind(&RSEntity::parseREAL_OBJT_SWPN_DYNM, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DATA"] =
        std::bind(&RSEntity::parseREAL_OBJT_SWPN_DATA, this, std::placeholders::_1, std::placeholders::_2);
    handlers["ALGN"] =
        std::bind(&RSEntity::parseREAL_OBJT_SWPN_ALGN, this, std::placeholders::_1, std::placeholders::_2);
        
    lexer.InitFromRAM(data, size, handlers);
}
void RSEntity::parseREAL_OBJT_SWPN_DYNM(uint8_t *data, size_t size) {}
void RSEntity::parseREAL_OBJT_SWPN_DATA(uint8_t *data, size_t size) {
    ByteStream bs(data, size);
    SWPN_DATA *swpn_data = new SWPN_DATA();
    swpn_data->weapons_round = bs.ReadInt32LE();
    swpn_data->detection_range = bs.ReadInt32LE();
    swpn_data->effective_range = bs.ReadInt32LE();
    swpn_data->dispersion = bs.ReadUShort();
    swpn_data->fire_interval = bs.ReadFixedFloatLE();
    swpn_data->missile_launcher = bs.ReadByte();
    swpn_data->weapon_quantity = bs.ReadUShort();
    swpn_data->weapon_name = bs.ReadString(8);
    std::transform(swpn_data->weapon_name.begin(), swpn_data->weapon_name.end(), swpn_data->weapon_name.begin(), ::toupper);
    swpn_data->weapon_name = assetsManager.object_root_path + swpn_data->weapon_name + ".IFF";
    TreEntry *entry = assetsManager.GetEntryByName(swpn_data->weapon_name);
    if (entry != nullptr) {
        RSEntity *objct = new RSEntity();
        objct->InitFromRAM(entry->data, entry->size, swpn_data->weapon_name);
        swpn_data->weapon_entity = objct;
    } else {
        swpn_data->weapon_entity = nullptr;
    }
    swpn_data->launch_offset = bs.ReadInt32LE();
    this->swpn_data = swpn_data;
}
void RSEntity::parseREAL_OBJT_SWPN_ALGN(uint8_t *data, size_t size) {}
void RSEntity::parseREAL_OBJT_JETP_EXPL(uint8_t *data, size_t size) {
    EXPL *expl = new EXPL();
    
    std::string tmpname;
    ByteStream bs(data, size);
    expl->name = bs.ReadString(8);
    tmpname = assetsManager.object_root_path + expl->name + ".IFF";
    expl->x = bs.ReadShort();
    expl->y = bs.ReadShort();
    std::transform (tmpname.begin(), tmpname.end(), tmpname.begin(), ::toupper);
    expl->objct = new RSEntity();
    TreEntry *entry = assetsManager.GetEntryByName(tmpname);
    if (entry != nullptr) {
        expl->objct->InitFromRAM(entry->data, entry->size, tmpname);
    }
    this->explos = expl;
}
void RSEntity::parseREAL_OBJT_JETP_DEBR(uint8_t *data, size_t size) {}
void RSEntity::parseREAL_OBJT_JETP_DEST(uint8_t *data, size_t size) {
    ByteStream bs(data, size);
    std::string tmpname = bs.ReadString(8);
    std::transform(tmpname.begin(), tmpname.end(), tmpname.begin(), ::toupper);
    this->destroyed_object_name = assetsManager.object_root_path + tmpname + ".IFF";
    RSEntity *objct = new RSEntity();
    TreEntry *entry = assetsManager.GetEntryByName(this->destroyed_object_name);
    if (this->destroyed_object_name == name) {
        entry = nullptr;
    }
    if (entry != nullptr) { 
        objct->InitFromRAM(entry->data, entry->size, this->destroyed_object_name);
        this->destroyed_object = objct;
    } else {
        this->destroyed_object = nullptr;
    }
    
}
void RSEntity::parseREAL_OBJT_JETP_SMOK(uint8_t *data, size_t size) {
    
}
void RSEntity::parseREAL_OBJT_JETP_CHLD(uint8_t *data, size_t size) {
    size_t nb_child = size /32;
    ByteStream bs(data, size);
    for (size_t i = 0; i < nb_child; i++) {
        CHLD *chld = new CHLD();
        
        std::string tmpname = bs.ReadString(8);
        std::transform(tmpname.begin(), tmpname.end(), tmpname.begin(), ::toupper);
        chld->name = assetsManager.object_root_path+tmpname+".IFF";
        chld->x = bs.ReadInt32LE();
        chld->z = bs.ReadInt32LE();
        chld->y = bs.ReadInt32LE();
        
        for (size_t j = 0; j < 12; j++) {
            chld->data.push_back(bs.ReadByte());
        }
        RSEntity *objct = new RSEntity();
        
        TreEntry *entry = assetsManager.GetEntryByName(chld->name);
        if (entry != nullptr) {
            objct->InitFromRAM(entry->data, entry->size, chld->name);
            chld->objct = objct;
            this->chld.push_back(chld);
        }
    }
}
void RSEntity::parseREAL_OBJT_JETP_JINF(uint8_t *data, size_t size) {
    // MissionRecord_LoadStringFields_9D4F0 : octet +0x53, mot +0x54, mot +0x56, octet +0x52
    ByteStream bs(data, size);
    bs.ReadByte();
    bs.ReadShort();
    bs.ReadShort();
    this->combat_class = bs.ReadByte();
}
void RSEntity::parseREAL_OBJT_JETP_DAMG(uint8_t *data, size_t size) {
    if (size > 2) {
        IFFSaxLexer lexer;

        std::unordered_map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
        handlers["SYSM"] = std::bind(&RSEntity::parseREAL_OBJT_JETP_WEAP_DAMG_SYSM, this, std::placeholders::_1, std::placeholders::_2);

        lexer.InitFromRAM(data, size, handlers);
    } else {
        ByteStream bs(data, size);
        this->health = bs.ReadByte();
    }
}
void RSEntity::parseREAL_OBJT_JETP_EJEC(uint8_t *data, size_t size) {}
void RSEntity::parseREAL_OBJT_JETP_SIGN(uint8_t *data, size_t size) {
    if (size < 3)
        return;
    ByteStream bs(data, size);
    RADAR_SIGN *radar_signature = new RADAR_SIGN();
    radar_signature->unknown1 = bs.ReadByte();
    radar_signature->unknown2 = bs.ReadByte();  
    radar_signature->unknown3 = bs.ReadByte();
    this->radar_signature = radar_signature;
}
void RSEntity::parseREAL_OBJT_JETP_TRGT(uint8_t *data, size_t size) {
    if (size >= 1)
        this->target_type = data[0];
}
void RSEntity::parseREAL_OBJT_JETP_CTRL(uint8_t *data, size_t size) {}
void RSEntity::parseREAL_OBJT_JETP_CKPT(uint8_t *data, size_t size) {
    if (size < 26)
        return;
    ByteStream bs(data, size);
    std::string str1 = bs.ReadString(10);
    std::string str2 = bs.ReadString(8);
    std::string str3 = bs.ReadString(8);
    std::transform(str3.begin(), str3.end(), str3.begin(), ::toupper);
    this->cockpit_name = str3;
}
void RSEntity::parseREAL_OBJT_JETP_TOFF(uint8_t *data, size_t size) {
    if (size < 8) {
        return;
    }
    ByteStream bs(data, size);
    this->takeoff_roll_accel = bs.ReadShort();
    this->takeoff_rotate_speed = bs.ReadShort();
    this->takeoff_climb_pitch = bs.ReadShort();
    this->takeoff_pitch_gain = bs.ReadShort();
}
void RSEntity::parseREAL_OBJT_JETP_LAND(uint8_t *data, size_t size) {
    if (size < 10) {
        return;
    }
    ByteStream bs(data, size);
    this->landing_speed = bs.ReadShort();
    this->landing_unused = (int32_t) bs.ReadUInt32LE();
    this->landing_aim_height = bs.ReadShort();
    this->landing_pitch_steps = bs.ReadShort();
}
void RSEntity::parseREAL_OBJT_JETP_DYNM(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;

    std::unordered_map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["DYNM"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_DYNM_DYNM, this, std::placeholders::_1, std::placeholders::_2);
    handlers["ORDY"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_DYNM_ORDY, this, std::placeholders::_1, std::placeholders::_2);
    handlers["STBL"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_DYNM_STBL, this, std::placeholders::_1, std::placeholders::_2);
    handlers["ATMO"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_DYNM_ATMO, this, std::placeholders::_1, std::placeholders::_2);
    handlers["GRAV"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_DYNM_GRAV, this, std::placeholders::_1, std::placeholders::_2);
    handlers["THRS"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_DYNM_THRS, this, std::placeholders::_1, std::placeholders::_2);
    handlers["JDYN"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_DYNM_JDYN, this, std::placeholders::_1, std::placeholders::_2);

    lexer.InitFromRAM(data, size, handlers);
}
void RSEntity::parseREAL_OBJT_JETP_DYNM_DYNM(uint8_t *data, size_t size) {
    ByteStream bs(data, size);
    if (size >= 4) {
        bs.ReadByte();
        this->weight_in_kg = bs.ReadInt24LEByte3();
    }
}
void RSEntity::parseREAL_OBJT_JETP_DYNM_ORDY(uint8_t *data, size_t size) {}
void RSEntity::parseREAL_OBJT_JETP_DYNM_STBL(uint8_t *data, size_t size) {
    if (size < 4)
        return;
    ByteStream bs(data, size);
    this->stability_gain = bs.ReadFixedFloatLE();
}
void RSEntity::parseREAL_OBJT_JETP_DYNM_ATMO(uint8_t *data, size_t size) {
    if (size < 2)
        return;
    ByteStream bs(data, size);
    this->drag = bs.ReadUShort();
    if (size >= 4) {
        ByteStream fixed(data, size);
        this->parasite_drag = fixed.ReadFixedFloatLE();
    }
}
void RSEntity::parseREAL_OBJT_JETP_DYNM_GRAV(uint8_t *data, size_t size) {
    this->gravity = true;
}
void RSEntity::parseREAL_OBJT_JETP_DYNM_THRS(uint8_t *data, size_t size) {
    if (size < 6)
        return;
    ByteStream bs(data, size);
    if (size == 6 || size == 7 || size == 8) {
        bs.ReadByte();
        this->thrust_in_newton = bs.ReadInt24LEByte3();
        /* octets restants (decodes cette session, cf. DATA_MODEL.md §6.2 "Poussee") :
           fraction MIL, fraction a l'altitude de reference, echelle de l'altitude de coupure */
        size_t remaining = size - 4;
        if (remaining >= 1)
            this->thrust_mil_fraction = bs.ReadByte() / 256.0f;
        if (remaining >= 2)
            this->thrust_ref_alt_fraction = bs.ReadByte() / 256.0f;
        if (remaining >= 3)
            this->thrust_cutoff_alt_raw = bs.ReadByte();
    } else if (size == 19 || size == 20) {
        bs.ReadByte();
        bs.ReadByte();
        bs.ReadByte();
        bs.ReadByte();
        bs.ReadByte();
        this->thrust_in_newton = bs.ReadInt24LEByte3()*15;
    }
    
}
void RSEntity::parseREAL_OBJT_JETP_DYNM_JDYN(uint8_t *data, size_t size) {
    if (size < 73)
        return;
    ByteStream bs(data, size);
    JDYN *dyn = new JDYN();
    dyn->fuel_capacity = (int)bs.ReadFixedFloatLE();
    dyn->sfc = bs.ReadFixedFloatLE();
    dyn->drag_airbrake = bs.ReadFixedFloatLE();
    dyn->drag_gear = bs.ReadFixedFloatLE();
    dyn->ground_moment_1 = bs.ReadFixedFloatLE();
    dyn->ground_moment_2 = bs.ReadFixedFloatLE();
    dyn->rate_limit_dps = bs.ReadFixedFloatLE();
    dyn->max_turn_rate_dps = bs.ReadFixedFloatLE();
    dyn->stall_alpha_deg = bs.ReadByte();
    dyn->wing_incidence_deg = bs.ReadByte();
    dyn->flap_lift_increment_deg = bs.ReadByte();
    dyn->stall_speed_ms = bs.ReadFixedFloatLE();
    dyn->max_speed_ms = bs.ReadFixedFloatLE();
    dyn->max_bank_deg = bs.ReadByte();
    dyn->pitch_rate_limit_dps = bs.ReadByte();
    dyn->pitch_margin_deg = bs.ReadByte();
    dyn->control_speed_ms = bs.ReadFixedFloatLE();
    dyn->induced_drag_k = bs.ReadFixedFloatLE();
    dyn->lift_gain = bs.ReadFixedFloatLE();
    dyn->pitch_stick_gain = bs.ReadByte();
    dyn->yaw_authority = bs.ReadByte();
    dyn->max_g = bs.ReadByte();
    dyn->ai_speed_max = bs.ReadShort();
    dyn->ai_speed_min = bs.ReadShort();
    dyn->ai_speed_cruise = bs.ReadShort();
    dyn->ai_engage_range = bs.ReadFixedFloatLE();
    dyn->ai_unknown_8a = bs.ReadByte();
    dyn->ai_decision_weight = bs.ReadByte();
    this->jdyn = dyn;
}
void RSEntity::parseREAL_OBJT_JETP_WEAP(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;

    std::unordered_map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["INFO"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_WEAP_INFO, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DCOY"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_WEAP_DCOY, this, std::placeholders::_1, std::placeholders::_2);
    handlers["WPNS"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_WEAP_WPNS, this, std::placeholders::_1, std::placeholders::_2);
    handlers["HPTS"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_WEAP_HPTS, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DAMG"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_WEAP_DAMG, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}
void RSEntity::parseREAL_OBJT_JETP_WEAP_INFO(uint8_t *data, size_t size) {}
void RSEntity::parseREAL_OBJT_JETP_WEAP_DCOY(uint8_t *data, size_t size) {}
void RSEntity::parseREAL_OBJT_JETP_WEAP_WPNS(uint8_t *data, size_t size) {
    if (size < 10)
        return;
    size_t nb_weap = 0;
    nb_weap = size / 10;
    ByteStream bs(data, size);  

    for (size_t i = 0; i < nb_weap; i++) {
        WEAPS *htps = new WEAPS();
        htps->nb_weap = bs.ReadShort();
        htps->name = bs.ReadString(8);
        std::transform(htps->name.begin(), htps->name.end(), htps->name.begin(), ::toupper);
        std::string tmpname = assetsManager.object_root_path + htps->name + ".IFF";
        RSEntity *objct = new RSEntity();
        TreEntry *entry = assetsManager.GetEntryByName(tmpname);
        if (entry != nullptr) {
            objct->InitFromRAM(entry->data, entry->size, tmpname);
            htps->objct = objct;
        }
        this->weaps.push_back(htps);
    }
}
void RSEntity::parseREAL_OBJT_JETP_WEAP_HPTS(uint8_t *data, size_t size) {
    if (size < 13)
        return;
    size_t nb_hpts = size / 13;
    ByteStream bs(data, size);
    for (size_t i = 0; i < nb_hpts; i++) {
        HPTS *hpts = new HPTS();
        hpts->id = bs.ReadByte();
        hpts->x = bs.ReadInt32LE();
        hpts->z = bs.ReadInt32LE();
        hpts->y = bs.ReadInt32LE();
        this->hpts.push_back(hpts);
    }
}
void RSEntity::parseREAL_OBJT_JETP_WEAP_DAMG(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;

    std::unordered_map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["SYSM"] =
        std::bind(&RSEntity::parseREAL_OBJT_JETP_WEAP_DAMG_SYSM, this, std::placeholders::_1, std::placeholders::_2);
}
void RSEntity::parseREAL_OBJT_JETP_WEAP_DAMG_SYSM(uint8_t *data, size_t size) {
    if (size < 2)
        return;
    ByteStream bs(data, size);

    this->life = bs.ReadShort();
    size_t nb_sysm = (size - 2) / 18;
    
    for (size_t i=0; i<nb_sysm;i++) {
        uint16_t pv = bs.ReadShort();
        std::string sub_system = bs.ReadString(8);
        std::string main_system = bs.ReadString(8);
        this->sysm[main_system][sub_system] = pv;
    }
}
void RSEntity::parseREAL_OBJT_EXTE(uint8_t *data, size_t size) {
    
}
void RSEntity::parseREAL_OBJT_RNWY(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    std::unordered_map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    this->entity_type = EntityType::rnwy;
    handlers["TRGT"] = std::bind(&RSEntity::parseREAL_OBJT_JETP_TRGT, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}
void RSEntity::parseREAL_APPR(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;

    std::unordered_map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["POLY"] = std::bind(&RSEntity::parseREAL_APPR_POLY, this, std::placeholders::_1, std::placeholders::_2);
    handlers["AFTB"] = std::bind(&RSEntity::parseREAL_APPR_POLY, this, std::placeholders::_1, std::placeholders::_2);
    handlers["ANIM"] = std::bind(&RSEntity::parseREAL_APPR_ANIM, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}
void RSEntity::parseREAL_APPR_POLY(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;

    std::unordered_map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["INFO"] =
        std::bind(&RSEntity::parseREAL_APPR_POLY_INFO, this, std::placeholders::_1, std::placeholders::_2);
    handlers["VERT"] =
        std::bind(&RSEntity::parseREAL_APPR_POLY_VERT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DETA"] =
        std::bind(&RSEntity::parseREAL_APPR_POLY_DETA, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TRIS"] =
        std::bind(&RSEntity::parseREAL_APPR_POLY_TRIS, this, std::placeholders::_1, std::placeholders::_2);
    handlers["QUAD"] =
        std::bind(&RSEntity::parseREAL_APPR_POLY_QUAD, this, std::placeholders::_1, std::placeholders::_2);
    handlers["ATTR"] =
        std::bind(&RSEntity::parseREAL_APPR_POLY_ATTR, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TXMS"] =
        std::bind(&RSEntity::parseREAL_APPR_POLY_TRIS_TXMS, this, std::placeholders::_1, std::placeholders::_2);
    handlers["LNTH"] =
        std::bind(&RSEntity::parseREAL_APPR_POLY_TRIS_LNTH, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}
void RSEntity::parseREAL_APPR_POLY_TRIS_LNTH(uint8_t *data, size_t size) {}
void RSEntity::parseREAL_APPR_POLY_INFO(uint8_t *data, size_t size) {}
void RSEntity::parseREAL_APPR_POLY_VERT(uint8_t *data, size_t size) {
    if (size < 12)
        return;
    ByteStream stream(data, size);
    size_t numVertice = size / 12;
    for (int i = 0; i < numVertice; i++) {
        Point3D vertex;

        vertex.z = stream.ReadFixedFloatLE();
        vertex.x = stream.ReadFixedFloatLE();
        vertex.y = stream.ReadFixedFloatLE();

        AddVertex(&vertex);
    }
}
void RSEntity::parseREAL_APPR_POLY_DETA(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;

    std::unordered_map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["LVL0"] =
        std::bind(&RSEntity::parseREAL_APPR_POLY_DETA_LVLX, this, std::placeholders::_1, std::placeholders::_2);
    handlers["LVL1"] =
        std::bind(&RSEntity::parseREAL_APPR_POLY_DETA_LVLX, this, std::placeholders::_1, std::placeholders::_2);
    handlers["LVL2"] =
        std::bind(&RSEntity::parseREAL_APPR_POLY_DETA_LVLX, this, std::placeholders::_1, std::placeholders::_2);
    handlers["LVL3"] =
        std::bind(&RSEntity::parseREAL_APPR_POLY_DETA_LVLX, this, std::placeholders::_1, std::placeholders::_2);

    lexer.InitFromRAM(data, size, handlers);
}
void RSEntity::parseREAL_APPR_POLY_DETA_LVLX(uint8_t *data, size_t size) {
    if (size < 4)
        return;
    ByteStream stream(data, size);
    Lod lod;

    lod.numTriangles = static_cast<uint16_t>((size - 4) / 2);
    lod.dist = stream.ReadUInt32LE();

    for (int i = 0; i < lod.numTriangles; i++) {
        lod.triangleIDs[i] = stream.ReadUShort();
    }

    AddLod(&lod);
}
void RSEntity::parseREAL_APPR_POLY_ATTR(uint8_t *data, size_t size) {
    if (size < 9)
        return;
    size_t num_attr = size / 9;
    ByteStream stream(data, size);
    std::unordered_map<char, uint16_t> compteurs = {{'L', 0}, {'T', 0}, {'Q', 0}};
    for (size_t i = 0; i < num_attr; i++) {
        Attr *attr = new Attr();
        uint16_t id = stream.ReadUShort();
        attr->type = stream.ReadByte();
        attr->id = compteurs[attr->type]++;
        attr->props1 = stream.ReadByte();
        attr->props2 = stream.ReadByte();
        this->attrs[id] = attr;
        stream.ReadByte();
        stream.ReadByte();
        stream.ReadByte();
        stream.ReadByte();
    }
}
void RSEntity::parseREAL_APPR_POLY_TRIS(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;

    std::unordered_map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["VTRI"] =
        std::bind(&RSEntity::parseREAL_APPR_POLY_TRIS_VTRI, this, std::placeholders::_1, std::placeholders::_2);
    handlers["FACE"] =
        std::bind(&RSEntity::parseREAL_APPR_POLY_TRIS_FACE, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TXMS"] =
        std::bind(&RSEntity::parseREAL_APPR_POLY_TRIS_TXMS, this, std::placeholders::_1, std::placeholders::_2);
    handlers["UVXY"] =
        std::bind(&RSEntity::parseREAL_APPR_POLY_TRIS_UVXY, this, std::placeholders::_1, std::placeholders::_2);
    handlers["MAPS"] =
        std::bind(&RSEntity::parseREAL_APPR_POLY_TRIS_MAPS, this, std::placeholders::_1, std::placeholders::_2);

    lexer.InitFromRAM(data, size, handlers);
}

void RSEntity::parseREAL_APPR_POLY_TRIS_VTRI(uint8_t *data, size_t size) {
    if (size < 8)
        return;
    size_t numTriangle = size / 8;
    ByteStream stream(data, size);

    Triangle triangle;
    for (int i = 0; i < numTriangle; i++) {

        triangle.property = stream.ReadByte();

        triangle.ids[0] = stream.ReadByte();
        triangle.ids[1] = stream.ReadByte();
        triangle.ids[2] = stream.ReadByte();

        triangle.color = stream.ReadByte();

        triangle.flags[0] = stream.ReadByte();
        triangle.flags[1] = stream.ReadByte();
        triangle.flags[2] = stream.ReadByte();

        AddTriangle(&triangle);
    }
}
void RSEntity::parseREAL_APPR_POLY_TRIS_FACE(uint8_t *data, size_t size) {
    if (size < 8)
        return;
    size_t numTriangle = size / 8;
    ByteStream stream(data, size);

    Triangle triangle;
    for (int i = 0; i < numTriangle; i++) {
        triangle.property = stream.ReadByte();
        triangle.color = stream.ReadByte();
        triangle.ids[0] = stream.ReadUShort();
        triangle.ids[1] = stream.ReadUShort();
        triangle.ids[2] = stream.ReadUShort();
        

        //triangle.flags[0] = stream.ReadByte();
        //triangle.flags[1] = stream.ReadByte();
        //triangle.flags[2] = stream.ReadByte();

        AddTriangle(&triangle);
    }
}
void RSEntity::parseREAL_APPR_POLY_TRIS_TXMS(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;

    std::unordered_map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["INFO"] =
        std::bind(&RSEntity::parseREAL_APPR_POLY_TRIS_TXMS_INFO, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TXMP"] =
        std::bind(&RSEntity::parseREAL_APPR_POLY_TRIS_TXMS_TXMP, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TXMA"] =
        std::bind(&RSEntity::parseREAL_APPR_POLY_TRIS_TXMS_TXMA, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}
void RSEntity::parseREAL_APPR_POLY_TRIS_TXMS_INFO(uint8_t *data, size_t size) {}
void RSEntity::parseREAL_APPR_POLY_TRIS_TXMS_TXMP(uint8_t *data, size_t size) {
    if (size < 14)
        return;
    ByteStream stream(data, size);

    RSImage *image = new RSImage();

    char name[9];

    for (int i = 0; i < 8; i++)
        name[i] = stream.ReadByte();

    uint32_t width = stream.ReadShort();
    uint32_t height = stream.ReadShort();

    image->Create(name, width, height, 0);
    uint8_t *src = stream.GetPosition();
    uint8_t *pic_data = nullptr;
    size_t csize = 0;
    if (src[0]=='L' && src[1]=='Z'){
        LZBuffer lzbuffer;
        pic_data = lzbuffer.DecodeLZW(src+2,size-14,csize);
        src = pic_data;
    } else if (src[0]=='P' && src[1]=='+'){
        PKWareDecompressor pkware;
        size_t remain = size - ((src+2) - data);
        size_t byte_read = 0;
        uint8_t *temp_data = pkware.DecompressPKWare(src+2,remain,csize);
        
        // Doubler chaque ligne
        size_t source_height = csize / width;
        size_t new_size = width * source_height * 2;
        size_t real_size = width * height;
        pic_data = (uint8_t*)malloc(width * height);
        size_t line = 0;
        for (line = 0; line < source_height; line++) {
            size_t src_offset = line * width;
            size_t dst_offset = line * width * 2;
            
            // Copier la ligne deux fois
            memcpy(pic_data + dst_offset, temp_data + src_offset, width);
            memcpy(pic_data + dst_offset + width, temp_data + src_offset, width);
        }
        if (line*2 < height) {
            // Si la hauteur cible est plus grande, remplir le reste avec des zéros
            memset(pic_data + line*2 * width, 0, (height - line*2) * width);
        }
        free(temp_data);
        csize = real_size;
        src = pic_data;
        image->flags = 1;
    }
    
    if (csize == width * height || pic_data == nullptr) {
        image->UpdateContent(src);
    } else {
        image->UpdateContent(src, csize);
    }
    
    AddImage(image);
}
void RSEntity::parseREAL_APPR_POLY_TRIS_TXMS_TXMA(uint8_t *data, size_t size) {
    if (size < 14)
        return;
    ByteStream stream(data, size);

    RSImage *image = new RSImage();

    char name[9];

    for (int i = 0; i < 8; i++)
        name[i] = stream.ReadByte();

    uint32_t width = stream.ReadShort();
    uint32_t height = stream.ReadShort();
    uint16_t nbframe = stream.ReadShort();
    
    
    uint8_t *src = stream.GetPosition();
    
    uint8_t *pic_data = nullptr;
    size_t csize = 0;
    if (src[0]=='L' && src[1]=='Z'){
        image->Create(name, width, height*nbframe, 0);
        image->nbframes = nbframe;
        LZBuffer lzbuffer;
        size_t remain = size - ((src+4) - data);
        size_t byte_read = 0;
        pic_data = lzbuffer.DecodeLZW(src+4,remain,csize);
        if (pic_data == nullptr) {
            printf("Error decoding image %s\n", name);
            return;
        }
        src = pic_data;
        image->UpdateContent(src);
        image->width = width;
        image->height = height;
        AddImage(image);
    }  else if (src[0]=='P' && src[1]=='+'){
        stream.ReadByte();
        stream.ReadByte();
        size_t next_frame_offset = 0;
        uint8_t *frame_data = nullptr;
        frame_data = (uint8_t*)malloc(width * height * nbframe);
        size_t remaind_bytes = size - 16;
        int current_frame_index = 0;
        while (remaind_bytes > 0) {
            uint16_t next_frame_offset = stream.ReadShort();
            PKWareDecompressor pkware;
            size_t remain = size - ((src+2) - data);
            std::vector<uint8_t> picture_pkware = stream.ReadBytes(next_frame_offset);
            size_t byte_read = 0;
            uint8_t *temp_data = pkware.DecompressPKWare(picture_pkware.data(),next_frame_offset,csize);
            size_t source_height = csize / width;
            size_t new_size = width * source_height * 2;
            size_t real_size = width * height;
            pic_data = (uint8_t*)malloc(width * height);
            size_t line = 0;
            for (line = 0; line < source_height; line++) {
                size_t src_offset = line * width;
                size_t dst_offset = line * width * 2;
                
                // Copier la ligne deux fois
                memcpy(pic_data + dst_offset, temp_data + src_offset, width);
                memcpy(pic_data + dst_offset + width, temp_data + src_offset, width);
            }
            if (line*2 < height) {
                // Si la hauteur cible est plus grande, remplir le reste avec des zéros
                memset(pic_data + line*2 * width, 0, (height - line*2) * width);
            }
            memcpy(frame_data + current_frame_index * width * height, pic_data, width * height);
            free(temp_data);
            char a = stream.ReadByte();
            char b = stream.ReadByte();
            remaind_bytes -= (next_frame_offset + 4);
            if (a == 'P' && b == '+') {
                current_frame_index++;
            } else {
                // On a atteint la fin des frames
                remaind_bytes = 0;
            }
            image->flags = 1;
        }
        image->Create(name, width, height*nbframe, 0);
        image->nbframes = nbframe;
        image->UpdateContent(frame_data);
        image->width = width;
        image->height = height;
        AddImage(image);
        free(frame_data);
        
    } else {
        image->Create(name, width, height*nbframe, 0);
        image->nbframes = nbframe;
        image->UpdateContent(src);
        image->width = width;
        image->height = height;
        AddImage(image);
    }
    
    
}
void RSEntity::parseREAL_APPR_POLY_TRIS_UVXY(uint8_t *data, size_t size) {
    if (size < 8)
        return;
    ByteStream stream(data, size);

    size_t numEntries = size / 8;

    uvxyEntry uvEntry;
    for (size_t i = 0; i < numEntries; i++) {

        uvEntry.triangleID = stream.ReadByte();
        uvEntry.textureID = stream.ReadByte();

        uvEntry.uvs[0].u = stream.ReadByte();
        uvEntry.uvs[0].v = stream.ReadByte();

        uvEntry.uvs[1].u = stream.ReadByte();
        uvEntry.uvs[1].v = stream.ReadByte();

        uvEntry.uvs[2].u = stream.ReadByte();
        uvEntry.uvs[2].v = stream.ReadByte();

        AddUV(&uvEntry);
    }
}
void RSEntity::parseREAL_APPR_POLY_TRIS_MAPS(uint8_t *data, size_t size) {
    if (size < 16)
        return;
    ByteStream stream(data, size);

    size_t numEntries = size / 16;

    uvxyEntry uvEntry;
    for (size_t i = 0; i < numEntries; i++) {

        uvEntry.triangleID = stream.ReadUShort();
        uvEntry.textureID = stream.ReadUShort();

        uvEntry.uvs[0].u = stream.ReadByte();
        stream.ReadByte();
        uvEntry.uvs[0].v = stream.ReadByte();
        stream.ReadByte();
        uvEntry.uvs[1].u = stream.ReadByte();
        stream.ReadByte();
        uvEntry.uvs[1].v = stream.ReadByte();
        stream.ReadByte();

        uvEntry.uvs[2].u = stream.ReadByte();
        stream.ReadByte();
        uvEntry.uvs[2].v = stream.ReadByte();
        stream.ReadByte();

        AddUV(&uvEntry);
    }
}
void RSEntity::parseREAL_APPR_POLY_QUAD(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;

    std::unordered_map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    
    handlers["FACE"] =
        std::bind(&RSEntity::parseREAL_APPR_POLY_QUAD_FACE, this, std::placeholders::_1, std::placeholders::_2);
    handlers["MAPS"] =
        std::bind(&RSEntity::parseREAL_APPR_POLY_QUAD_MAPS, this, std::placeholders::_1, std::placeholders::_2);

    lexer.InitFromRAM(data, size, handlers);
}

void RSEntity::parseREAL_APPR_POLY_QUAD_FACE(uint8_t *data, size_t size) {
    if (size < 10)
        return;
    size_t numQuad = size / 10;
    ByteStream stream(data, size);
    
    Quads *q;
    for (int i = 0; i < numQuad; i++) {
        uint8_t property = stream.ReadByte();
        uint8_t color = stream.ReadByte();
        uint16_t verts[4];
        verts[0] = stream.ReadUShort();
        verts[1] = stream.ReadUShort();
        verts[2] = stream.ReadUShort();
        verts[3] = stream.ReadUShort();

        q = new Quads();
        q->property = property;
        q->color = color;
        q->ids[0] = verts[0];
        q->ids[1] = verts[1];
        q->ids[2] = verts[2];
        q->ids[3] = verts[3];
        quads.push_back(q);
    }
}
void RSEntity::parseREAL_APPR_POLY_QUAD_MAPS(uint8_t *data, size_t size) {
    if (size < 20)
        return;
    ByteStream stream(data, size);

    size_t numEntries = size / 20;

    qmapuvxyEntry *uvEntry;
    for (size_t i = 0; i < numEntries; i++) {
        uvEntry = new qmapuvxyEntry();
        uvEntry->triangleID = stream.ReadUShort();
        uvEntry->textureID = stream.ReadUShort();

        uvEntry->uvs[0].u = stream.ReadByte();
        stream.ReadByte();
        uvEntry->uvs[0].v = stream.ReadByte();
        stream.ReadByte();
        uvEntry->uvs[1].u = stream.ReadByte();
        stream.ReadByte();
        uvEntry->uvs[1].v = stream.ReadByte();
        stream.ReadByte();

        uvEntry->uvs[2].u = stream.ReadByte();
        stream.ReadByte();
        uvEntry->uvs[2].v = stream.ReadByte();
        stream.ReadByte();

        uvEntry->uvs[3].u = stream.ReadByte();
        stream.ReadByte();
        uvEntry->uvs[3].v = stream.ReadByte();
        stream.ReadByte();
        this->qmapuvs.push_back(uvEntry);
    }
}
void RSEntity::parseREAL_APPR_ANIM(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;

    std::unordered_map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    
    handlers["INFO"] =
        std::bind(&RSEntity::parseREAL_APPR_ANIM_INFO, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SEQU"] =
        std::bind(&RSEntity::parseREAL_APPR_ANIM_SEQU, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SHAP"] =
        std::bind(&RSEntity::parseREAL_APPR_ANIM_SHAP, this, std::placeholders::_1, std::placeholders::_2);

    lexer.InitFromRAM(data, size, handlers);
}
void RSEntity::parseREAL_APPR_ANIM_INFO(uint8_t *data, size_t size) {}
void RSEntity::parseREAL_APPR_ANIM_SEQU(uint8_t *data, size_t size) {}
void RSEntity::parseREAL_APPR_ANIM_SHAP(uint8_t *data, size_t size) {
    if (size < 1)
        return;
    RSImageSet *img_set = new RSImageSet();
    PakArchive pak;
    uint8_t* data2 = (uint8_t*) malloc(size);
    memcpy(data2, data, size);
    if (data2[0]=='L' && data2[1]=='Z'){
        pak.InitFromRAM("SHAPE", data2, size);
        img_set->InitFromSubPakEntry(&pak);
    } else {
        pak.InitFromRAM("SHAPE", data2, size);
        img_set->InitFromSubPakEntry(&pak);    
    }
    
    RSPalette palette;
    
    TreEntry *entries = (TreEntry *)assetsManager.GetEntryByName("..\\..\\DATA\\PALETTE\\PALETTE.IFF");
    if (entries != nullptr) {
        palette.initFromFileRam(entries->data, entries->size);
    } else {
        FileData *f = assetsManager.GetFileData("PALETTE.IFF");
        if (f != nullptr) {
            palette.initFromFileData(f);
        }
    }

    for (auto img : img_set->shapes) {
        Texture *tex = new Texture();
        tex->width = img->GetWidth();
        tex->height = img->GetHeight();
        img->position.x = 0;
        img->position.y = 0;
        img->buffer_size.x = (int32_t) tex->width;
        img->buffer_size.y = (int32_t) tex->height;
        size_t imgsize = tex->width*tex->height;
        uint8_t *imgdata = (uint8_t *)malloc(imgsize);
        memset(imgdata, 255, imgsize);
        size_t byteRead = 0;
        img->Expand(imgdata, &byteRead);
        if (byteRead > imgsize) {
            printf("RLEShape::Expand failed\n");
        }
        
        tex->data = (uint8_t *)malloc(imgsize*4);
        memset(tex->data, 255, imgsize*4);
        uint8_t *dst = tex->data;
        long checksum = 0;
        for (size_t j = 0; j < imgsize; j++) {
            Texel *rgba = palette.GetColorPalette()->GetRGBColor(imgdata[j]);
            if (imgdata[j] == 255) {
                rgba->a = 0;
            }
            if (rgba->a == 255 && rgba->r == 211 && rgba->g == 211 && rgba->b == 211) {
                rgba->a = 0;
            }
            dst[0] = rgba->r;
            dst[1] = rgba->g;
            dst[2] = rgba->b;
            dst[3] = rgba->a;
            dst += 4;
        }
        tex->initialized = false;
        this->animations.push_back(tex);
    }
    this->images_set.push_back(img_set);
}
void RSEntity::parseREAL_OBJT_EXPL(uint8_t *data, size_t size) {
    this->entity_type = EntityType::explosion;
}
void RSEntity::parseREAL_OBJT_OMOB(uint8_t *data, size_t size) {
    this->entity_type = EntityType::object_mobile;
}
void RSEntity::parseREAL_OBJT_DEBR(uint8_t *data, size_t size) {
    this->entity_type = EntityType::debris;
}
void RSEntity::parseREAL_OBJT_SMKG(uint8_t *data, size_t size) {
    this->entity_type = EntityType::destroyed_object;
}