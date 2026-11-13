//
//  gfx.h
//  iff
//
//  Created by Fabien Sanglard on 12/21/2013.
//  Copyright (c) 2013 Fabien Sanglard. All rights reserved.
//

#pragma once

#include "../engine/SCRenderer.h"

class EnhancedRenderer : public SCRenderer {
public:
    void init(int width, int height) override;
    void ensureDetailTexture() override;
    void renderTerrainDetail(RSArea *area, int LOD, const std::vector<int> &blocks) override;
    void renderWaterWaves(RSArea *area, int LOD, const std::vector<int> &blocks) override;
    void ensureWaveTexture() override;
    void createTextureInGPU(Texture *texture) override;
    void drawTexturedQuad(Vector3D pos, Vector3D orientation, std::vector<Vector3D> quad, Texture *tex) override;
    void drawBillboard(Vector3D pos, Texture *tex, float size, float alpha) override;
    void applyTextureFilter(const Texture *texture) override;
    void renderMapOverlay(RSArea *area) override;

    // [Video] runway_texture: 0 = off (default, the original overlay), 1024 / 2048 / 4096 = size of the procedural high
    // resolution tiles of the map overlay (runway, taxiways); the flat painted markings get a grain texture
    int runway_texture{0};
    // high resolution tiles by texture index and the grain of the markings (0 = none), made by enhanced::RunwayTexture
    GLuint overlay_hd_tile[256]{};
    GLuint overlay_grain_tex{0};
    // the map and the original texture the tiles were made from, new tiles are made when one of them changes
    RSArea *overlay_hd_area{nullptr};
    GLuint overlay_hd_source{0};
};
