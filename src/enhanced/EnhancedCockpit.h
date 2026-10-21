#pragma once
#include "../strike_commander/precomp.h"
#include "../strike_commander/SCCockpit.h"

// Cockpit with optional HD replacement of the cockpit images.
// [Video] hd_textures = 1 uses ./assets/hdtex/NAME_WxH.png instead of the original image NAME with size WxH.
class EnhancedCockpit : public SCCockpit {
public:
    EnhancedCockpit() = default;
    void init() override;

private:
    int replaceTextures();
    bool uploadReplacement(RSImage *image);
};
