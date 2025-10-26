//
//  VGA.h
//  libRealSpace
//
//  Created by Fabien Sanglard on 1/27/2014.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//

#pragma once
#ifdef _WIN32
#include <Windows.h>
#endif
#include <GL/gl.h>

#include "../realspace/AssetManager.h"
#include "../realspace/RLEShape.h"
#include "../realspace/RSFont.h"
#include "RSScreen.h"
#include "Texture.h"
#include "FrameBuffer.h"

#define sgn(x) ((x<0)?-1:((x>0)?1:0))

struct VGAPalette;

class RSVGA{
private:
    int width;
    int height;
    VGAPalette palette;
    FrameBuffer* frameBuffer;
    uint32_t *upscaled_framebuffer{nullptr};
    uint32_t textureID;
    Texel data[320 * 200];
    inline static std::unique_ptr<RSVGA> s_instance{};
    void displayBuffer(uint32_t* buffer, int width, int height);
    RSScreen *Screen = &RSScreen::instance();
public:
    bool upscale{false};
    
    static RSVGA& getInstance() {
        if (!RSVGA::hasInstance()) {
            RSVGA::setInstance(std::make_unique<RSVGA>());
        }
        RSVGA& instance = RSVGA::instance();
        return instance;
    };    
    static RSVGA& instance() {
        return *s_instance; 
    }
    static void setInstance(std::unique_ptr<RSVGA> inst) {
        s_instance = std::move(inst);
    }
    static bool hasInstance() { return (bool)s_instance; }

    RSVGA();
    ~RSVGA();
    void init(int width, int height);
    void activate(void);
    void setPalette(VGAPalette* newPalette);
    VGAPalette* getPalette(void);
    void vSync(void);
    void fadeOut(int steps = 10, int delayMs = 50);
    FrameBuffer* getFrameBuffer(void){ return frameBuffer;};

    void ajusterContraste(float facteur);
    void ajusterLuminosite(float facteur);
    void appliquerTeinte(uint8_t r, uint8_t g, uint8_t b, float intensite);
    void redistributionCouleurs();
    void restaurerPalette();
    void interpolerPalettes(VGAPalette* palette1, VGAPalette* palette2, float facteur);
    
    float contrastFactor = 1.0f;
    float brightnessFactor = 1.0f;
    uint8_t tintR = 0;
    uint8_t tintG = 0;
    uint8_t tintB = 0;
    float tintIntensity = 0.0f;

};
