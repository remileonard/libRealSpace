#include "../strike_commander/precomp.h"
#include "RunwayTexture.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <unordered_map>
#include <vector>
#include "EnhancedRenderer.h"
#include "../engine/Texture.h"
#include "../realspace/RSArea.h"
#include "../realspace/RSImage.h"

#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif

namespace enhanced {

namespace {

float hashNoise(int x, int y, int seed) {
    uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u + (uint32_t)seed * 2147483647u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (float)(h & 0xffff) / 65535.0f;
}

// smooth value noise that repeats after cells lattice cells, so neighbouring tiles join without a seam
float valueNoise(float u, float v, int cells, int seed) {
    const float fx = u * cells;
    const float fy = v * cells;
    const int ix = (int)std::floor(fx);
    const int iy = (int)std::floor(fy);
    float tx = fx - ix;
    float ty = fy - iy;
    tx = tx * tx * (3.0f - 2.0f * tx);
    ty = ty * ty * (3.0f - 2.0f * ty);
    const int x0 = ix % cells;
    const int y0 = iy % cells;
    const int x1 = (ix + 1) % cells;
    const int y1 = (iy + 1) % cells;
    const float a = hashNoise(x0, y0, seed);
    const float b = hashNoise(x1, y0, seed);
    const float c = hashNoise(x0, y1, seed);
    const float d = hashNoise(x1, y1, seed);
    return (a * (1.0f - tx) + b * tx) * (1.0f - ty) + (c * (1.0f - tx) + d * tx) * ty;
}

// 3x3 blur of an RGB float picture, edges are clamped
void blur(std::vector<float> &pixels, int w, int h) {
    std::vector<float> out(pixels.size());
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            for (int c = 0; c < 3; c++) {
                float sum = 0.0f;
                float weight = 0.0f;
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        const int xx = (std::min)(w - 1, (std::max)(0, x + dx));
                        const int yy = (std::min)(h - 1, (std::max)(0, y + dy));
                        const float k = (dx == 0 ? 2.0f : 1.0f) * (dy == 0 ? 2.0f : 1.0f);
                        sum += pixels[((size_t)yy * w + xx) * 3 + c] * k;
                        weight += k;
                    }
                }
                out[((size_t)y * w + x) * 3 + c] = sum / weight;
            }
        }
    }
    pixels.swap(out);
}

// The tile is read back from the texture the game uploaded, so the colours are exactly what the original shows.
// Tiles with large flat areas (painted numbers) keep their sharp edges, noisy asphalt tiles are smoothed first.
GLuint upgradeImage(RSImage *image, int size, int seed) {
    Texture *original = image->GetTexture();
    glBindTexture(GL_TEXTURE_2D, original->id);
    GLint tw = 0;
    GLint th = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &tw);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &th);
    const int w = (int)tw;
    const int h = (int)th;
    if (w < 16 || h < 16 || w > 4096 || h > 4096) {
        return 0;
    }
    std::vector<uint8_t> pixels((size_t)w * h * 4);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    std::vector<float> source((size_t)w * h * 3);
    std::vector<float> alphaSource((size_t)w * h);
    std::unordered_map<uint32_t, int> colors;
    for (int i = 0; i < w * h; i++) {
        const uint8_t r = pixels[(size_t)i * 4];
        const uint8_t g = pixels[(size_t)i * 4 + 1];
        const uint8_t b = pixels[(size_t)i * 4 + 2];
        alphaSource[(size_t)i] = pixels[(size_t)i * 4 + 3] / 255.0f;
        source[(size_t)i * 3] = r / 255.0f;
        source[(size_t)i * 3 + 1] = g / 255.0f;
        source[(size_t)i * 3 + 2] = b / 255.0f;
        colors[((uint32_t)(r >> 2) << 12) | ((uint32_t)(g >> 2) << 6) | (uint32_t)(b >> 2)]++;
    }
    int modeCount = 0;
    for (const auto &entry : colors) {
        modeCount = (std::max)(modeCount, entry.second);
    }
    const bool detail = (float)modeCount / (float)(w * h) >= 0.30f;
    if (!detail) {
        blur(source, w, h);
        blur(source, w, h);
    }

    const int outW = size;
    const int outH = (std::max)(2, (int)((long long)size * h / w));
    std::unique_ptr<uint8_t[]> out(new uint8_t[(size_t)outW * outH * 4]);
    for (int y = 0; y < outH; y++) {
        const float sy = ((y + 0.5f) / outH) * h - 0.5f;
        const int y0 = (std::min)(h - 1, (std::max)(0, (int)std::floor(sy)));
        const int y1 = (std::min)(h - 1, y0 + 1);
        const float fy = (std::min)(1.0f, (std::max)(0.0f, sy - y0));
        for (int x = 0; x < outW; x++) {
            const float sx = ((x + 0.5f) / outW) * w - 0.5f;
            const int x0 = (std::min)(w - 1, (std::max)(0, (int)std::floor(sx)));
            const int x1 = (std::min)(w - 1, x0 + 1);
            const float fx = (std::min)(1.0f, (std::max)(0.0f, sx - x0));
            float rgb[3];
            float luma = 0.0f;
            for (int c = 0; c < 3; c++) {
                const float top = source[((size_t)y0 * w + x0) * 3 + c] * (1.0f - fx) + source[((size_t)y0 * w + x1) * 3 + c] * fx;
                const float bottom = source[((size_t)y1 * w + x0) * 3 + c] * (1.0f - fx) + source[((size_t)y1 * w + x1) * 3 + c] * fx;
                rgb[c] = top * (1.0f - fy) + bottom * fy;
                luma += rgb[c] / 3.0f;
            }
            // tileable grain of three sizes and a fine aggregate speckle
            const float u = (x + 0.5f) / outW;
            const float v = (y + 0.5f) / outH;
            float grain = 0.0f;
            grain += (valueNoise(u, v, 21, seed) - 0.5f) * 0.10f;
            grain += (valueNoise(u, v, 85, seed + 1) - 0.5f) * 0.14f;
            grain += (valueNoise(u, v, 341, seed + 2) - 0.5f) * 0.20f;
            grain += (hashNoise(x, y, seed + 3) - 0.5f) * 0.22f;
            // rubber covered areas (dark) show less aggregate than the bare light asphalt
            const float strength = 0.35f + 1.1f * (std::min)(1.0f, luma * 1.6f);
            const float factor = 1.0f + grain * strength;
            const float aTop = alphaSource[(size_t)y0 * w + x0] * (1.0f - fx) + alphaSource[(size_t)y0 * w + x1] * fx;
            const float aBottom = alphaSource[(size_t)y1 * w + x0] * (1.0f - fx) + alphaSource[(size_t)y1 * w + x1] * fx;
            const float alpha = aTop * (1.0f - fy) + aBottom * fy;
            uint8_t *d = &out[((size_t)y * outW + x) * 4];
            for (int c = 0; c < 3; c++) {
                d[c] = (uint8_t)(std::min)(255.0f, (std::max)(0.0f, rgb[c] * factor * 255.0f));
            }
            // keep the alpha of the original tile
            d[3] = (uint8_t)(std::min(1.0f, (std::max)(0.0f, alpha)) * 255.0f);
        }
    }
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8.0f);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, outW, outH, 0, GL_RGBA, GL_UNSIGNED_BYTE, out.get());
    return texture;
}

}  // namespace

// Worn paint: tileable grey grain (about 0.7 .. 1.0) that modulates the flat colour of the markings
static void createMarkingGrain(EnhancedRenderer &renderer) {
    const int n = 512;
    std::vector<uint8_t> pixels((size_t)n * n * 4);
    for (int y = 0; y < n; y++) {
        for (int x = 0; x < n; x++) {
            const float u = (x + 0.5f) / n;
            const float v = (y + 0.5f) / n;
            float grain = 0.0f;
            grain += (valueNoise(u, v, 6, 11) - 0.5f) * 0.30f;
            grain += (valueNoise(u, v, 24, 12) - 0.5f) * 0.22f;
            grain += (valueNoise(u, v, 96, 13) - 0.5f) * 0.18f;
            grain += (hashNoise(x, y, 14) - 0.5f) * 0.20f;
            const float value = (std::min)(1.0f, (std::max)(0.0f, 0.90f + grain));
            uint8_t *d = &pixels[((size_t)y * n + x) * 4];
            d[0] = (uint8_t)(value * 255.0f);
            d[1] = d[0];
            d[2] = d[0];
            d[3] = 255;
        }
    }
    if (renderer.overlay_grain_tex == 0) {
        glGenTextures(1, &renderer.overlay_grain_tex);
    }
    glBindTexture(GL_TEXTURE_2D, renderer.overlay_grain_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8.0f);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, n, n, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
}

void RunwayTexture::clear(EnhancedRenderer &renderer) {
    for (int id = 0; id < 256; id++) {
        if (renderer.overlay_hd_tile[id] != 0) {
            glDeleteTextures(1, &renderer.overlay_hd_tile[id]);
            renderer.overlay_hd_tile[id] = 0;
        }
    }
}

int RunwayTexture::upgrade(EnhancedRenderer &renderer, RSArea *area, int size) {
    if (area == nullptr || size < 128) {
        return 0;
    }
    std::vector<int> polygons(256, 0);
    for (const AreaOverlay &ov : area->objectOverlay) {
        for (size_t k = 0; k < ov.drawList.size(); k++) {
            const AreaOverlayTriangles &tri = ov.trianles[ov.drawList[k]];
            if (tri.color < 255) {
                polygons[tri.color]++;
            }
        }
    }
    clear(renderer);
    int replaced = 0;
    for (int id = 0; id < 255; id++) {
        if (polygons[id] == 0) {
            continue;
        }
        RSImage *image = area->GetImageByID(id + 255);
        if (image == nullptr) {
            continue;
        }
        renderer.overlay_hd_tile[id] = upgradeImage(image, size, 100);
        if (renderer.overlay_hd_tile[id] != 0) {
            replaced++;
        }
    }
    createMarkingGrain(renderer);
    printf("[enhanced] runway texture: %d tiles enlarged to %dx%d\n", replaced, size, size);
    return replaced;
}

}  // namespace enhanced
