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
};
