#include "EnhancedRenderer.h"
#include "../engine/Config.hpp"
#include "../realspace/RSArea.h"


#define SC_TEXTURE_MAX_ANISOTROPY 0x84FE
#define SC_MAX_TEXTURE_MAX_ANISOTROPY 0x84FF

// One world unit of the detail texture repeats every DETAIL_TILE_SIZE units on the ground (planar mapping)
static const float DETAIL_TILE_SIZE = 2500.0f;
typedef void (*TerrainQuadFn)(const MapVertex *cur, const MapVertex *right, const MapVertex *bottomRight, const MapVertex *bottom);

// Calls fn for every terrain quad of the given blocks, including the quads that reach into the neighbouring blocks.
static void eachTerrainQuad(RSArea *area, int LOD, const std::vector<int> &blocks, TerrainQuadFn fn) {
    for (int i : blocks) {
        AreaBlock *block = area->GetAreaBlockByID(LOD, i);
        if (block == nullptr || block->sideSize < 2) {
            continue;
        }
        const int s = block->sideSize;
        const bool hasRight = i % BLOCK_PER_MAP_SIDE != BLOCK_PER_MAP_SIDE - 1;
        const bool hasBottom = i / BLOCK_PER_MAP_SIDE != BLOCK_PER_MAP_SIDE - 1;
        AreaBlock *rightBlock = hasRight ? area->GetAreaBlockByID(LOD, i + 1) : nullptr;
        AreaBlock *bottomBlock = hasBottom ? area->GetAreaBlockByID(LOD, i + BLOCK_PER_MAP_SIDE) : nullptr;
        AreaBlock *cornerBlock = (hasRight && hasBottom) ? area->GetAreaBlockByID(LOD, i + 1 + BLOCK_PER_MAP_SIDE) : nullptr;
        for (int y = 0; y < s - 1; y++) {
            for (int x = 0; x < s - 1; x++) {
                fn(block->GetVertice(x, y), block->GetVertice(x + 1, y), block->GetVertice(x + 1, y + 1),
                   block->GetVertice(x, y + 1));
            }
        }
        if (rightBlock != nullptr) {
            for (int y = 0; y < s - 1; y++) {
                fn(block->GetVertice(s - 1, y), rightBlock->GetVertice(0, y), rightBlock->GetVertice(0, y + 1),
                   block->GetVertice(s - 1, y + 1));
            }
        }
        if (bottomBlock != nullptr) {
            for (int x = 0; x < s - 1; x++) {
                fn(block->GetVertice(x, s - 1), block->GetVertice(x + 1, s - 1), bottomBlock->GetVertice(x + 1, 0),
                   bottomBlock->GetVertice(x, 0));
            }
        }
        if (cornerBlock != nullptr) {
            fn(block->GetVertice(s - 1, s - 1), rightBlock->GetVertice(0, s - 1), cornerBlock->GetVertice(0, 0),
               bottomBlock->GetVertice(s - 1, 0));
        }
    }
}

static void detailVertex(const MapVertex *v) {
    glTexCoord2f(v->v.x / DETAIL_TILE_SIZE, v->v.z / DETAIL_TILE_SIZE);
    glVertex3f(v->v.x, v->v.y, v->v.z);
}

static void detailQuad(const MapVertex *cur, const MapVertex *right, const MapVertex *bottomRight, const MapVertex *bottom) {
    detailVertex(cur);
    detailVertex(bottomRight);
    detailVertex(bottom);
    detailVertex(cur);
    detailVertex(right);
    detailVertex(bottomRight);
}

// Procedural, seamlessly tiling value noise (four octaves). The texture has a mean of 0.5 and is applied with
// a colour scale of 2, so the terrain keeps its brightness and only gets texture around it.
void EnhancedRenderer::ensureDetailTexture() {
    if (this->detail_tex != 0) {
        return;
    }
    const int N = 256;
    const int octaves[4] = {8, 16, 32, 64};
    const float weights[4] = {0.35f, 0.30f, 0.20f, 0.15f};
    const float amplitude = 0.5f * (this->terrain_detail / 100.0f); // 100 = about +-50% brightness at one sigma
    auto lattice = [](int x, int y, int period, int seed) {
        x = ((x % period) + period) % period;
        y = ((y % period) + period) % period;
        uint32_t h = (uint32_t)(x * 374761393 + y * 668265263 + seed * 2147483647);
        h = (h ^ (h >> 13)) * 1274126177u;
        h ^= h >> 16;
        return (float)(h & 0xffff) / 65535.0f;
    };
    std::vector<uint8_t> px((size_t)N * N * 4);
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            float n = 0.0f;
            for (int o = 0; o < 4; o++) {
                const int period = octaves[o];
                const float fx = (float)x / N * period;
                const float fy = (float)y / N * period;
                const int ix = (int)fx;
                const int iy = (int)fy;
                float tx = fx - ix;
                float ty = fy - iy;
                tx = tx * tx * (3.0f - 2.0f * tx);
                ty = ty * ty * (3.0f - 2.0f * ty);
                const float a = lattice(ix, iy, period, o);
                const float b = lattice(ix + 1, iy, period, o);
                const float c = lattice(ix, iy + 1, period, o);
                const float d = lattice(ix + 1, iy + 1, period, o);
                n += weights[o] * ((a + (b - a) * tx) * (1.0f - ty) + (c + (d - c) * tx) * ty);
            }
            // the octave sum has a standard deviation of about 0.12, normalise it to about 1
            const float z = (n - 0.5f) / 0.12f;
            float v = 0.5f * (1.0f + amplitude * z);
            if (v < 0.0f) v = 0.0f;
            if (v > 1.0f) v = 1.0f;
            const uint8_t c = (uint8_t)(v * 255.0f);
            const size_t i = ((size_t)y * N + x) * 4;
            px[i] = px[i + 1] = px[i + 2] = c;
            px[i + 3] = 255;
        }
    }
    glGenTextures(1, &this->detail_tex);
    glBindTexture(GL_TEXTURE_2D, this->detail_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, N, N, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    printf("[Renderer] terrain detail texture %d%%\n", this->terrain_detail);
}

// Detail pass over the whole terrain, after the colour and texture passes: the noise texture (mean 0.5) is
// multiplied onto what is already in the framebuffer (result = 2 * src * dst), so grass tiles, rock and
// coloured terrain all get fine structure and keep their brightness.
void EnhancedRenderer::renderTerrainDetail(RSArea *area, int LOD, const std::vector<int> &blocks) {
    this->ensureDetailTexture();
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, this->detail_tex);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);
    glDisable(GL_FOG);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-2.0f, -2.0f);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBegin(GL_TRIANGLES);
    eachTerrainQuad(area, LOD, blocks, detailQuad);
    glEnd();
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glDisable(GL_BLEND);
    if (this->show_fog) {
        glEnable(GL_FOG);
    }
    // the rest of the renderer expects the decal mode
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    glDisable(GL_TEXTURE_2D);
}

// Water is terrain of land type 0. Only triangles whose three vertices are water and that have no tile texture count.
static const float WAVE_TILE_SIZE = 1500.0f;

// Camera position for the distance fade of the waves, set before the water is drawn
static Vector3D waveCamera;
static const float WAVE_FADE_START = 8000.0f;
static const float WAVE_FADE_END = 30000.0f;

// Strength of the waves at a vertex: fades out towards the shore and with the distance to the camera
static float waveStrength(const MapVertex *v) {
    float shore = (v->waterShare - 0.4f) / 0.5f;
    shore = shore < 0.0f ? 0.0f : (shore > 1.0f ? 1.0f : shore);
    const float dx = v->v.x - waveCamera.x;
    const float dz = v->v.z - waveCamera.z;
    const float d = sqrtf(dx * dx + dz * dz);
    float fade = 1.0f - (d - WAVE_FADE_START) / (WAVE_FADE_END - WAVE_FADE_START);
    fade = fade < 0.0f ? 0.0f : (fade > 1.0f ? 1.0f : fade);
    return shore * fade;
}

static void waveVertex(const MapVertex *v) {
    glColor4f(1.0f, 1.0f, 1.0f, waveStrength(v));
    glTexCoord2f(v->v.x / WAVE_TILE_SIZE, v->v.z / WAVE_TILE_SIZE);
    glVertex3f(v->v.x, v->v.y, v->v.z);
}

static void waterQuad(const MapVertex *cur, const MapVertex *right, const MapVertex *bottomRight, const MapVertex *bottom) {
    if (cur->lowerImageID == 0xFF && cur->type == 0 && bottomRight->type == 0 && bottom->type == 0) {
        waveVertex(cur);
        waveVertex(bottomRight);
        waveVertex(bottom);
    }
    if (cur->upperImageID == 0xFF && cur->type == 0 && right->type == 0 && bottomRight->type == 0) {
        waveVertex(cur);
        waveVertex(right);
        waveVertex(bottomRight);
    }
}

// Ridged, seamlessly tiling noise (crest lines), mean 0.5, applied like the detail texture.
void EnhancedRenderer::ensureWaveTexture() {
    if (this->wave_tex != 0) {
        return;
    }
    const int N = 256;
    const int octaves[3] = {6, 12, 24};
    const float weights[3] = {0.5f, 0.3f, 0.2f};
    const float amplitude = 0.5f * (this->water_waves / 100.0f);
    auto lattice = [](int x, int y, int period, int seed) {
        x = ((x % period) + period) % period;
        y = ((y % period) + period) % period;
        uint32_t h = (uint32_t)(x * 374761393 + y * 668265263 + seed * 2147483647);
        h = (h ^ (h >> 13)) * 1274126177u;
        h ^= h >> 16;
        return (float)(h & 0xffff) / 65535.0f;
    };
    std::vector<uint8_t> px((size_t)N * N * 4);
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            float n = 0.0f;
            for (int o = 0; o < 3; o++) {
                const int period = octaves[o];
                const float fx = (float)x / N * period;
                const float fy = (float)y / N * period;
                const int ix = (int)fx;
                const int iy = (int)fy;
                float tx = fx - ix;
                float ty = fy - iy;
                tx = tx * tx * (3.0f - 2.0f * tx);
                ty = ty * ty * (3.0f - 2.0f * ty);
                const float a = lattice(ix, iy, period, 10 + o);
                const float b = lattice(ix + 1, iy, period, 10 + o);
                const float c = lattice(ix, iy + 1, period, 10 + o);
                const float d = lattice(ix + 1, iy + 1, period, 10 + o);
                const float v = (a + (b - a) * tx) * (1.0f - ty) + (c + (d - c) * tx) * ty;
                n += weights[o] * (1.0f - fabsf(2.0f * v - 1.0f)); // ridge
            }
            // ridge values are around 0.5 with a deviation of about 0.12
            const float z = (n - 0.5f) / 0.12f;
            float v = 0.5f * (1.0f + amplitude * z);
            if (v < 0.0f) v = 0.0f;
            if (v > 1.0f) v = 1.0f;
            const uint8_t c = (uint8_t)(v * 255.0f);
            const size_t i = ((size_t)y * N + x) * 4;
            px[i] = px[i + 1] = px[i + 2] = c;
            px[i + 3] = 255;
        }
    }
    glGenTextures(1, &this->wave_tex);
    glBindTexture(GL_TEXTURE_2D, this->wave_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, N, N, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    printf("[Renderer] water waves %d%%\n", this->water_waves);
}
// Two layers of the wave texture drift over the water in different directions and speeds.
void EnhancedRenderer::renderWaterWaves(RSArea *area, int LOD, const std::vector<int> &blocks) {
    this->ensureWaveTexture();
    const float t = (float)SDL_GetTicks() / 1000.0f;
    // scale, drift in tile fractions per second
    const float layers[2][3] = {{1.0f, 0.010f, 0.006f}, {1.7f, -0.007f, 0.012f}};
    waveCamera = this->camera.getPosition();
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, this->wave_tex);
    // result = mix(0.5, texture, vertex alpha): the waves fade to a neutral 0.5 that the blend leaves unchanged
    const GLfloat neutral[4] = {0.5f, 0.5f, 0.5f, 1.0f};
    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, neutral);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_CONSTANT);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_PRIMARY_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_ALPHA);
    glEnable(GL_BLEND);
    glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);
    glDisable(GL_FOG);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-2.0f, -2.0f);
    for (int layer = 0; layer < 2; layer++) {
        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();
        glTranslatef(t * layers[layer][1], t * layers[layer][2], 0.0f);
        glScalef(layers[layer][0], layers[layer][0], 1.0f);
        glMatrixMode(GL_MODELVIEW);
        glBegin(GL_TRIANGLES);
        eachTerrainQuad(area, LOD, blocks, waterQuad);
        glEnd();
    }
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glDisable(GL_BLEND);
    if (this->show_fog) {
        glEnable(GL_FOG);
    }
    // give the combiner back its defaults: other code (drawTexturedQuad) relies on them
    const GLfloat black[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, black);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PREVIOUS);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_CONSTANT);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_ALPHA);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    glDisable(GL_TEXTURE_2D);
}

// Must run before glTexImage2D: GL_GENERATE_MIPMAP rebuilds the mip chain on every upload.
// Textures that are rebuilt every frame stay bilinear, mipmaps would be wasted work for them.
void EnhancedRenderer::applyTextureFilter(const Texture *texture) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (this->texture_filter <= 0) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        return;
    }
    if (texture->animated) {
        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        return;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    if (this->texture_filter >= 2 && this->max_anisotropy > 1.0f) {
        glTexParameterf(GL_TEXTURE_2D, SC_TEXTURE_MAX_ANISOTROPY, (std::min)(16.0f, this->max_anisotropy));
    }
}

void EnhancedRenderer::drawTexturedQuad(Vector3D pos, Vector3D orientation, std::vector<Vector3D> quad, Texture *tex) {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_ADD);
    glEnable(GL_BLEND);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1f);
    glDisable(GL_CULL_FACE);
    if (!tex->initialized) {
        glGenTextures(1, &tex->id);
        glBindTexture(GL_TEXTURE_2D, tex->id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        this->applyTextureFilter(tex);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)tex->width, (GLsizei)tex->height, 
                     0, GL_RGBA, GL_UNSIGNED_BYTE, tex->data);
        
        tex->initialized = true;
    } else {
        glBindTexture(GL_TEXTURE_2D, tex->id);
    }
    
    glPushMatrix();
    glTranslatef(pos.x, pos.y, pos.z);
    glRotatef(orientation.x, 0, 1, 0);
    glRotatef(orientation.y, 0, 0, 1);
    glRotatef(orientation.z, 1, 0, 0);
    glColor4f(1.0f, 1.0f, 1.0f, 0.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(quad[0].x, quad[0].y, quad[0].z);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(quad[1].x, quad[1].y, quad[1].z);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(quad[2].x, quad[2].y, quad[2].z);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(quad[3].x, quad[3].y, quad[3].z);
    glEnd();
    glPopMatrix();
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glEnable(GL_CULL_FACE);
}
void EnhancedRenderer::drawBillboard(Vector3D pos, Texture *tex, float size, float alpha) {
    if (!initialized || tex == nullptr)
        return;
    
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnable(GL_BLEND);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1f);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    
    // Initialize texture if needed
    if (!tex->initialized) {
        glGenTextures(1, &tex->id);
        glBindTexture(GL_TEXTURE_2D, tex->id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        this->applyTextureFilter(tex);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)tex->width, (GLsizei)tex->height, 
                     0, GL_RGBA, GL_UNSIGNED_BYTE, tex->data);
        
        tex->initialized = true;
    } else {
        glBindTexture(GL_TEXTURE_2D, tex->id);
    }
    
    // Get camera position
    Point3D cameraPos = camera.getPosition();
    
    // Calculate billboard vectors
    Vector3D look, right, up;
    
    // Look vector points from billboard to camera
    look.x = cameraPos.x - pos.x;
    look.y = cameraPos.y - pos.y;
    look.z = cameraPos.z - pos.z;
    look.Normalize();
    
    // Right vector is perpendicular to look and global up
    Vector3D globalUp = {0.0f, 1.0f, 0.0f};
    right = globalUp.CrossProduct(&look);
    right.Normalize();
    
    // Up vector completes the orthogonal basis
    up = look.CrossProduct(&right);
    
    // Calculate the corners of the billboard quad
    float halfSize = size * 0.5f;
    
    Vector3D bottomLeft, bottomRight, topRight, topLeft;
    
    bottomLeft.x = pos.x - right.x * halfSize - up.x * halfSize;
    bottomLeft.y = pos.y - right.y * halfSize - up.y * halfSize;
    bottomLeft.z = pos.z - right.z * halfSize - up.z * halfSize;
    
    bottomRight.x = pos.x + right.x * halfSize - up.x * halfSize;
    bottomRight.y = pos.y + right.y * halfSize - up.y * halfSize;
    bottomRight.z = pos.z + right.z * halfSize - up.z * halfSize;
    
    topRight.x = pos.x + right.x * halfSize + up.x * halfSize;
    topRight.y = pos.y + right.y * halfSize + up.y * halfSize;
    topRight.z = pos.z + right.z * halfSize + up.z * halfSize;
    
    topLeft.x = pos.x - right.x * halfSize + up.x * halfSize;
    topLeft.y = pos.y - right.y * halfSize + up.y * halfSize;
    topLeft.z = pos.z - right.z * halfSize + up.z * halfSize;
    
    // Draw the billboard quad
    glBegin(GL_QUADS);
    glColor4f(1.0f, 1.0f, 1.0f, alpha);
    
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(bottomLeft.x, bottomLeft.y, bottomLeft.z);
    
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(bottomRight.x, bottomRight.y, bottomRight.z);
    
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(topRight.x, topRight.y, topRight.z);
    
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(topLeft.x, topLeft.y, topLeft.z);
    glEnd();
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
}

void EnhancedRenderer::createTextureInGPU(Texture *texture) {

    if (!initialized)
        return;
    
    glGenTextures(1, &texture->id);
    glBindTexture(GL_TEXTURE_2D, texture->id);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_ADD);

    this->applyTextureFilter(texture);
    glTexImage2D(GL_TEXTURE_2D, 0, 4, (GLsizei)texture->width, (GLsizei)texture->height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 texture->data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
}

void EnhancedRenderer::init(int width, int height) {
    SCRenderer::init(width, height);
    Config &config = Config::instance();
    this->water_waves = config.getInt("Video", "water_waves", 0);
    this->texture_filter = config.getInt("Video", "texture_filter", 0);

    if (this->texture_filter >= 2) {
        const char *ext = (const char *)glGetString(GL_EXTENSIONS);
        if (ext && strstr(ext, "texture_filter_anisotropic")) {
            glGetFloatv(SC_MAX_TEXTURE_MAX_ANISOTROPY, &this->max_anisotropy);
        }
    }
    if (this->texture_filter > 0) {
        printf("[Renderer] texture_filter %d, max anisotropy %.0f\n", this->texture_filter, this->max_anisotropy);
    }

}
// ---- Brushed metal detail for the 3D cockpit model -------------------------------------------------------------------

// Planar mapping along the dominant axis of the face normal, one texture tile per 4 model units
static void metalTexCoord(const Vector3D &v, const Vector3D &n) {
    const float k = 0.25f;
    const float ax = fabsf(n.x);
    const float ay = fabsf(n.y);
    const float az = fabsf(n.z);
    if (ay >= ax && ay >= az) {
        glTexCoord2f(v.x * k, v.z * k);
    } else if (ax >= az) {
        glTexCoord2f(v.z * k, v.y * k);
    } else {
        glTexCoord2f(v.x * k, v.y * k);
    }
}

static Vector3D metalFaceNormal(RSEntity *object, uint16_t a, uint16_t b, uint16_t c) {
    const Vector3D p0 = object->vertices[a];
    const Vector3D p1 = object->vertices[b];
    const Vector3D p2 = object->vertices[c];
    const Vector3D u(p1.x - p0.x, p1.y - p0.y, p1.z - p0.z);
    const Vector3D w(p2.x - p0.x, p2.y - p0.y, p2.z - p0.z);
    return Vector3D(u.y * w.z - u.z * w.y, u.z * w.x - u.x * w.z, u.x * w.y - u.y * w.x);
}

void EnhancedRenderer::setMetalDetail(bool on) {
    EnhancedRenderer *renderer = dynamic_cast<EnhancedRenderer *>(&SCRenderer::getInstance());
    if (renderer != nullptr) {
        renderer->metalDetail = on;
    }
}

// Procedural brushed metal: horizontal streaks, fine grain, panel seams and rivets. Tiles seamlessly.
void EnhancedRenderer::ensureMetalTexture() {
    if (this->metalTexture != 0) {
        return;
    }
    const int N = 128;
    std::vector<uint8_t> px((size_t)N * N * 4);
    uint32_t seed = 0x1234abcd;
    auto rnd = [&seed]() {
        seed = seed * 1664525u + 1013904223u;
        return (float)((seed >> 8) & 0xffff) / 65535.0f;
    };
    std::vector<float> row(N);
    for (int y = 0; y < N; y++) {
        row[y] = rnd();
    }
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            float v = 0.84f + 0.10f * row[y] + 0.06f * rnd();
            // panel seams (one per tile in both directions)
            if (x == 0 || y == 0) {
                v *= 0.55f;
            } else if (x == 1 || y == 1) {
                v *= 0.80f;
            }
            // rivets
            const int rc[4][2] = {{18, 18}, {110, 18}, {18, 110}, {110, 110}};
            for (const auto &r : rc) {
                const int dx = x - r[0];
                const int dy = y - r[1];
                const int d2 = dx * dx + dy * dy;
                if (d2 <= 2) {
                    v = 1.0f;
                } else if (d2 <= 7) {
                    v *= 0.6f;
                }
            }
            if (v > 1.0f) {
                v = 1.0f;
            }
            const uint8_t c = (uint8_t)(v * 255.0f);
            const size_t i = ((size_t)y * N + x) * 4;
            px[i] = px[i + 1] = px[i + 2] = c;
            px[i + 3] = 255;
        }
    }
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    if (this->max_anisotropy > 1.0f) {
        glTexParameterf(GL_TEXTURE_2D, SC_TEXTURE_MAX_ANISOTROPY, (std::min)(8.0f, this->max_anisotropy));
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, N, N, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    this->metalTexture = (unsigned int)tex;
}

// Second pass over the flat coloured polygons of the colour pass: the metal texture is multiplied onto the picture.
// The polygon selection mirrors SCRenderer::drawModelColorPass.
void EnhancedRenderer::drawMetalPass(RSEntity *object, size_t lodLevel) {
    this->ensureMetalTexture();
    Lod *lod = &object->lods[lodLevel];
    const bool cullWasOn = glIsEnabled(GL_CULL_FACE) == GL_TRUE;
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_DST_COLOR, GL_ZERO);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, this->metalTexture);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    for (int i = 0; i < lod->numTriangles; i++) {
        uint16_t triangleID = lod->triangleIDs[i];
        if (!object->attrs.empty()) {
            auto attr = object->attrs.find(triangleID);
            if (attr == object->attrs.end() || attr->second == nullptr || attr->second->type == 'Q' ||
                attr->second->type == 'L') {
                continue;
            }
            triangleID = attr->second->id;
        }
        if (triangleID >= object->triangles.size()) {
            continue;
        }
        const Triangle *triangle = &object->triangles[triangleID];
        if (triangle->property == RSEntity::SC_TRANSPARENT ||
            ((triangle->property == 6 || triangle->property == 9) && this->show_textured)) {
            continue;
        }
        const bool twoSided = triangle->flags[2] == 1;
        if (twoSided || !cullWasOn) {
            glDisable(GL_CULL_FACE);
        } else {
            glEnable(GL_CULL_FACE);
        }
        const Vector3D normal = metalFaceNormal(object, triangle->ids[0], triangle->ids[1], triangle->ids[2]);
        glBegin(GL_TRIANGLES);
        for (int j = 0; j < 3; j++) {
            const Vector3D v = object->vertices[triangle->ids[j]];
            metalTexCoord(v, normal);
            glVertex3f(v.x, v.y, v.z);
        }
        glEnd();
    }
    if (!object->quads.empty()) {
        for (int i = 0; i < lod->numTriangles; i++) {
            uint16_t quadID = lod->triangleIDs[i];
            int prop2 = 0;
            if (!object->attrs.empty()) {
                auto attr = object->attrs.find(quadID);
                if (attr == object->attrs.end() || attr->second == nullptr || attr->second->type == 'T' ||
                    attr->second->type == 'L') {
                    continue;
                }
                prop2 = attr->second->props2;
                quadID = attr->second->id;
            }
            if (quadID >= object->quads.size() || object->quads[quadID] == nullptr) {
                continue;
            }
            const Quads *quad = object->quads[quadID];
            if (quad->property == RSEntity::SC_TRANSPARENT ||
                ((quad->property == 6 || quad->property == 9) && this->show_textured)) {
                continue;
            }
            if (prop2 == 1 || !cullWasOn) {
                glDisable(GL_CULL_FACE);
            } else {
                glEnable(GL_CULL_FACE);
            }
            const Vector3D normal = metalFaceNormal(object, quad->ids[0], quad->ids[1], quad->ids[2]);
            glBegin(GL_QUADS);
            for (int j = 0; j < 4; j++) {
                const Vector3D v = object->vertices[quad->ids[j]];
                metalTexCoord(v, normal);
                glVertex3f(v.x, v.y, v.z);
            }
            glEnd();
        }
    }
    glPopAttrib();
}

void EnhancedRenderer::drawModelColorPass(RSEntity *object, size_t lodLevel, std::vector<Vector3D> &vertexNormals,
                                          float ambientLamber, Vector3D lightEye, float *MV) {
    SCRenderer::drawModelColorPass(object, lodLevel, vertexNormals, ambientLamber, lightEye, MV);
    if (this->metalDetail) {
        this->drawMetalPass(object, lodLevel);
    }
}
