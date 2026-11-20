//
//  R4Bridge.cpp
//  libRealSpace
//
//  RSEntity as r4::assets::Model.
//
#include "R4Data.h"
#include "R4Bridge.h"

#include "r4/axes.h"

#include "../realspace/RSEntity.h"
#include "../realspace/RSImage.h"
#include "../engine/Texture.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <string>

namespace r4bridge {

namespace {

// SCRenderer::drawModel shows the next frame every 10 draws of the object: 6 a second at 60 Hz
constexpr float kAnimationFramesPerSecond = 6;

// a palette color as R4nkine's RGBA (R in the low byte)
uint32_t rgba(const Texel &t, uint8_t alpha = 255) {
    return (uint32_t)t.r | (uint32_t)t.g << 8 | (uint32_t)t.b << 16 | (uint32_t)alpha << 24;
}

// SCRenderer::shadeAlongRamp, as a color
uint32_t shade(const VGAPalette &palette, const std::vector<PaletteRange> &ranges, int colour, float intensity, RampFirst first) {
    const PaletteRange *range = nullptr;
    for (const PaletteRange &r : ranges) {
        if (r.first <= colour && colour <= r.first + r.fog) {
            range = &r;
            break;
        }
    }
    auto pack = [](float r, float g, float b) {
        auto c = [](float v) {
            return (uint32_t)std::max(0.0f, std::min(255.0f, v + 0.5f));
        };
        return c(r) | c(g) << 8 | c(b) << 16 | 0xff000000u;
    };
    const Texel &c = palette.colors[colour & 0xff];
    if (!range) {
        return pack(c.r * intensity, c.g * intensity, c.b * intensity);
    }
    int n = colour - range->first;
    if (n == 0 && first == RampFirst::ITSELF) {
        return pack(c.r, c.g, c.b);
    }
    if (n == 0) {
        n = range->shades;
    }
    if (n <= 0) {
        return pack(c.r * intensity, c.g * intensity, c.b * intensity);
    }
    float t = intensity * n;
    int k0 = std::min((int)t, n - 1);
    float f = t - k0;
    const Texel &a = palette.colors[(range->first + k0) & 0xff];
    const Texel &b = palette.colors[(range->first + k0 + 1) & 0xff];
    return pack(a.r + (b.r - a.r) * f, a.g + (b.g - a.g) * f, a.b + (b.b - a.b) * f);
}

// R4nkine's Z up by a turn of 90 degrees about X, which keeps the winding, or libRealSpace's Y up
r4::Vec3 toAxes(const Vector3D &v, Axes axes) {
    return axes == Axes::R4 ? r4::axes::fromYUp({v.x, v.y, v.z}) : r4::Vec3{v.x, v.y, v.z};
}

} // namespace

r4::assets::Image rampImage(const VGAPalette &palette, const std::vector<PaletteRange> &ranges, RampFirst first) {
    r4::assets::Image img;
    img.name = "palette ramps";
    img.width = 256;
    img.height = 32;
    img.channels = 4;
    img.clampS = img.clampT = true;
    img.pixels.resize(256 * 32 * 4);
    for (int k = 0; k < 32; k++) {
        for (int i = 0; i < 256; i++) {
            uint32_t c = shade(palette, ranges, i, k / 31.0f, first);
            std::memcpy(&img.pixels[((size_t)k * 256 + i) * 4], &c, 4);
        }
    }
    return img;
}

void bleedClearTexels(r4::assets::Image &img) {
    if (img.channels != 4 || img.width <= 0 || img.height <= 0) {
        return;
    }
    const int w = img.width;
    const int h = img.height;
    std::vector<uint8_t> known((size_t)w * h);
    bool any = false;
    for (size_t i = 0; i < known.size(); i++) {
        any |= (known[i] = img.pixels[i * 4 + 3] != 0) != 0;
    }
    if (!any) {
        return;
    }
    for (bool changed = true; changed;) {
        changed = false;
        std::vector<uint8_t> next = known;
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                size_t i = (size_t)y * w + x;
                if (known[i]) {
                    continue;
                }
                int sum[3] = {0, 0, 0};
                int n = 0;
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        int nx = x + dx;
                        int ny = y + dy;
                        if (nx < 0 || ny < 0 || nx >= w || ny >= h || !known[(size_t)ny * w + nx]) {
                            continue;
                        }
                        const uint8_t *c = &img.pixels[((size_t)ny * w + nx) * 4];
                        sum[0] += c[0];
                        sum[1] += c[1];
                        sum[2] += c[2];
                        n++;
                    }
                }
                if (!n) {
                    continue;
                }
                uint8_t *c = &img.pixels[i * 4];
                c[0] = (uint8_t)(sum[0] / n);
                c[1] = (uint8_t)(sum[1] / n);
                c[2] = (uint8_t)(sum[2] / n);
                next[i] = 1;
                changed = true;
            }
        }
        known.swap(next);
    }
}

TextureQuality &textureQuality() {
    static TextureQuality quality;
    return quality;
}

void finishTexture(r4::assets::Image &img) {
    img.clampS = img.clampT = true;
    bleedClearTexels(img);
    const TextureQuality &q = textureQuality();
    if (q.upscale != r4::assets::Upscale::NONE) {
        img = r4::assets::upscale(img, q.upscale);
        bleedClearTexels(img);
    }
    img.mipmaps = q.mipmaps;
}

r4::Mat4 realSpaceToR4() {
    return r4::axes::fromYUpMatrix();
}

std::shared_ptr<r4::assets::Model> toModel(RSEntity *e, const VGAPalette &palette, const std::vector<PaletteRange> &ranges, Lighting lighting, Axes axes) {
    using namespace r4::assets;
    if (!e || e->vertices.empty()) {
        return nullptr;
    }
    auto model = std::make_shared<Model>();
    model->source = e->name;
    const size_t vertexCount = e->vertices.size();
    auto inRange = [&](const uint16_t *ids, int n) {
        for (int k = 0; k < n; k++) {
            if (ids[k] >= vertexCount) {
                return false;
            }
        }
        return true;
    };

    std::vector<r4::Vec3> normals(vertexCount, r4::Vec3(0));
    auto faceNormal = [&](uint16_t a, uint16_t b, uint16_t c) {
        r4::Vec3 A = toAxes(e->vertices[a], axes);
        r4::Vec3 B = toAxes(e->vertices[b], axes);
        r4::Vec3 C = toAxes(e->vertices[c], axes);
        return r4::normalize(r4::cross(A - B, C - B));
    };
    for (const Triangle &t : e->triangles) {
        if (inRange(t.ids, 3)) {
            r4::Vec3 n = faceNormal(t.ids[0], t.ids[1], t.ids[2]);
            for (int k = 0; k < 3; k++) {
                normals[t.ids[k]] += n;
            }
        }
    }
    for (const Quads *q : e->quads) {
        if (q && inRange(q->ids, 4)) {
            r4::Vec3 n1 = faceNormal(q->ids[0], q->ids[1], q->ids[2]);
            r4::Vec3 n2 = faceNormal(q->ids[0], q->ids[2], q->ids[3]);
            for (int k : {0, 1, 2}) {
                normals[q->ids[k]] += n1;
            }
            for (int k : {0, 2, 3}) {
                normals[q->ids[k]] += n2;
            }
        }
    }
    for (r4::Vec3 &n : normals) {
        if (r4::length(n) > 0) {
            n = r4::normalize(n);
        }
    }

    model->images.push_back(rampImage(palette, ranges));
    model->images.push_back(rampImage(palette, {}));
    model->images[1].name = "palette, no ramps";
    std::map<std::string, int> materials;
    auto material = [&](const std::string &key, Material m) {
        auto it = materials.find(key);
        if (it != materials.end()) {
            return it->second;
        }
        m.shading = Shading::PALETTE_RAMP;
        m.rampAmbient = lighting.ambient;
        m.rampLight = lighting.light;
        model->materials.push_back(m);
        return materials[key] = (int)model->materials.size() - 1;
    };
    auto colorMaterial = [&](bool twoSided) {
        Material m;
        m.name = twoSided ? "palette ramp, two-sided" : "palette ramp";
        m.diffuseImage = 0;
        m.doubleSided = twoSided;
        return material(twoSided ? "color2" : "color1", m);
    };
    auto clearMaterial = [&](bool twoSided) {
        Material m;
        m.name = twoSided ? "clear, two-sided" : "clear";
        m.diffuseImage = 1;
        m.doubleSided = twoSided;
        m.alpha = AlphaMode::ADDITIVE;
        return material(twoSided ? "clear2" : "clear1", m);
    };
    std::vector<int> textureImage(e->images.size(), -1);
    for (size_t i = 0; i < e->images.size(); i++) {
        RSImage *src = e->images[i];
        if (!src || !src->data || src->width == 0 || src->height == 0) {
            continue;
        }
        Image img;
        img.name = src->name;
        img.width = (int)src->width;
        img.height = (int)src->height * std::max<int>(1, src->nbframes);
        img.channels = 4;
        img.pixels.resize((size_t)img.width * img.height * 4);
        for (int y = 0; y < img.height; y++) {
            for (int x = 0; x < img.width; x++) {
                uint8_t index = src->data[(size_t)y * img.width + x];
                uint32_t c = rgba(palette.colors[index], index == PALETTE_CLEAR ? 0 : 255);
                std::memcpy(&img.pixels[((size_t)(img.height - 1 - y) * img.width + x) * 4], &c, 4);
            }
        }
        finishTexture(img);
        model->images.push_back(std::move(img));
        textureImage[i] = (int)model->images.size() - 1;
    }
    auto textureMaterial = [&](int texture, bool replaces, bool twoSided) {
        Material m;
        m.name = std::string(e->images[texture]->name) + (replaces ? "" : " (decal)") + (twoSided ? ", two-sided" : "");
        m.diffuseImage = textureImage[texture];
        m.frames = std::max<int>(1, e->images[texture]->nbframes);
        m.framesPerSecond = kAnimationFramesPerSecond;
        m.rampTextured = true;
        m.alpha = AlphaMode::BLEND;
        m.alphaTest = 0.1f;
        m.doubleSided = twoSided;
        if (replaces) {
            m.blendDepthWrite = true;
        } else {
            m.depthEqual = true;
            m.depthWrite = false;
        }
        return material("tex" + std::to_string(texture) + (replaces ? "r" : "d") + (twoSided ? "2" : "1"), m);
    };

    struct Level {
        std::map<int, Mesh> meshes;
        Axes axes;
        void add(const RSEntity *e, const std::vector<r4::Vec3> &normals, int material, const uint16_t *ids, int n, uint32_t color, const UV *uvs = nullptr, float w = 1, float h = 1) {
            Mesh &m = meshes[material];
            for (int k = 1; k + 1 < n; k++) {
                for (int j : {0, k + 1, k}) {
                    Vertex v;
                    v.position = toAxes(e->vertices[ids[j]], axes);
                    v.normal = normals[ids[j]];
                    v.color = color;
                    if (uvs) {
                        v.uv = {uvs[j].u / w, 1 - uvs[j].v / h};
                    }
                    m.vertices.push_back(v);
                    m.indices.push_back((uint32_t)m.vertices.size() - 1);
                }
            }
        }
    };
    auto paletteColor = [](int index) {
        return (uint32_t)(index & 0xff) | 0xff000000u;
    };

    std::vector<std::pair<float, std::vector<uint16_t>>> lods;
    for (const Lod &lod : e->lods) {
        lods.push_back({(float)lod.dist, std::vector<uint16_t>(lod.triangleIDs, lod.triangleIDs + lod.numTriangles)});
    }
    if (lods.empty()) {
        std::vector<uint16_t> all;
        for (size_t i = 0; i < e->triangles.size(); i++) {
            all.push_back((uint16_t)i);
        }
        lods.push_back({0.0f, all});
    }

    std::vector<Node> levelNodes;
    for (size_t level = 0; level < lods.size(); level++) {
        Level out;
        out.axes = axes;
        for (uint16_t id : lods[level].second) {
            bool isTriangle = true;
            bool isQuad = true;
            bool quadTwoSided = false;
            uint16_t triangleID = id;
            uint16_t quadID = id;
            if (!e->attrs.empty()) {
                auto it = e->attrs.find(id);
                if (it == e->attrs.end() || !it->second) {
                    continue;
                }
                const Attr *a = it->second;
                isTriangle = a->type == 'T';
                isQuad = a->type == 'Q';
                triangleID = quadID = a->id;
                quadTwoSided = a->props2 == FACE_TWO_SIDED;
            }
            if (isTriangle && triangleID < e->triangles.size()) {
                const Triangle &t = e->triangles[triangleID];
                bool twoSided = t.flags[2] == FACE_TWO_SIDED;
                if (inRange(t.ids, 3) && t.property == RSEntity::SC_TRANSPARENT) {
                    out.add(e, normals, clearMaterial(twoSided), t.ids, 3, paletteColor(t.color - 1));
                } else if (inRange(t.ids, 3) && t.property != FACE_TEXTURE_SOLID && t.property != FACE_TEXTURE_SOLID_ALT) {
                    out.add(e, normals, colorMaterial(twoSided), t.ids, 3, paletteColor(t.color));
                }
            }
            if (isQuad && quadID < e->quads.size() && e->quads[quadID]) {
                const Quads &q = *e->quads[quadID];
                if (inRange(q.ids, 4) && q.property == RSEntity::SC_TRANSPARENT) {
                    out.add(e, normals, clearMaterial(quadTwoSided), q.ids, 4, paletteColor(q.color - 1));
                } else if (inRange(q.ids, 4) && q.property != FACE_TEXTURE_SOLID && q.property != FACE_TEXTURE_SOLID_ALT) {
                    out.add(e, normals, colorMaterial(quadTwoSided), q.ids, 4, paletteColor(q.color - 1));
                }
            }
        }
        if (level == 0) {
            for (const uvxyEntry &u : e->uvs) {
                if (u.textureID >= e->images.size() || textureImage[u.textureID] < 0 || u.triangleID >= e->triangles.size()) {
                    continue;
                }
                const Triangle &t = e->triangles[u.triangleID];
                if (!inRange(t.ids, 3)) {
                    continue;
                }
                RSImage *img = e->images[u.textureID];
                bool replaces = t.property == FACE_TEXTURE_SOLID || t.property == FACE_TEXTURE_SOLID_ALT;
                out.add(e, normals, textureMaterial(u.textureID, replaces, t.flags[2] == FACE_TWO_SIDED), t.ids, 3, 0xffffffffu, u.uvs, (float)img->width, (float)img->height);
            }
            for (const qmapuvxyEntry *u : e->qmapuvs) {
                if (!u || u->textureID >= e->images.size() || textureImage[u->textureID] < 0 || u->triangleID >= e->quads.size() || !e->quads[u->triangleID]) {
                    continue;
                }
                const Quads &q = *e->quads[u->triangleID];
                if (!inRange(q.ids, 4)) {
                    continue;
                }
                bool twoSided = false;
                for (const auto &[key, a] : e->attrs) {
                    if (a && a->id == u->triangleID && a->type == 'Q') {
                        twoSided = a->props2 == FACE_TWO_SIDED;
                        break;
                    }
                }
                RSImage *img = e->images[u->textureID];
                bool replaces = q.property == FACE_TEXTURE_SOLID || q.property == FACE_TEXTURE_SOLID_ALT;
                out.add(e, normals, textureMaterial(u->textureID, replaces, twoSided), q.ids, 4, 0xffffffffu, u->uvs, (float)img->width, (float)img->height);
            }
        }
        Node node;
        node.name = "LVL" + std::to_string(level);
        for (auto &[mat, mesh] : out.meshes) {
            mesh.name = model->materials[mat].name;
            mesh.material = mat;
            mesh.computeBounds();
            model->meshes.push_back(std::move(mesh));
            node.meshes.push_back((int)model->meshes.size() - 1);
        }
        levelNodes.push_back(node);
    }

    Node root;
    root.name = e->name;
    model->nodes.push_back(root);
    if (levelNodes.size() == 1) {
        model->nodes[0].meshes = levelNodes[0].meshes;
    } else {
        Node lod;
        lod.name = "levels of detail";
        lod.kind = NodeKind::LOD;
        for (size_t i = 0; i < lods.size(); i++) {
            lod.lodRanges.push_back(i == 0 ? 0.0f : lods[i].first);
        }
        lod.lodRanges.push_back(1e30f);
        model->nodes.push_back(lod);
        model->nodes[0].children.push_back(1);
        for (Node &n : levelNodes) {
            model->nodes.push_back(n);
            model->nodes[1].children.push_back((int)model->nodes.size() - 1);
        }
    }
    return model;
}

} // namespace r4bridge
