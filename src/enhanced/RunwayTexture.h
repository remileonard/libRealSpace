#pragma once

class RSArea;
class EnhancedRenderer;

namespace enhanced {

/**
 * Procedural high resolution version of the textures of the map overlay (runway, taxiways).
 * The original 126x126 tile gives the drawing (rubber zones, edges, stains, numbers, same colours and alpha); it is
 * enlarged to size x size pixels and fine grain and aggregate speckle are added. Noisy asphalt tiles are smoothed first,
 * tiles with large flat areas (painted numbers) keep their sharp edges. The flat painted markings get a tileable
 * worn-paint grain texture. No external image is used.
 * The textures are stored in the enhanced renderer beside the originals.
 */
class RunwayTexture {
public:
    /** @return number of textures replaced */
    static int upgrade(EnhancedRenderer &renderer, RSArea *area, int size);
    /** removes the high resolution tiles of a previous map */
    static void clear(EnhancedRenderer &renderer);
};

}  // namespace enhanced
