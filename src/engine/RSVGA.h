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
#include "Texture.h"
#include "FrameBuffer.h"

#define sgn(x) ((x<0)?-1:((x>0)?1:0))

struct VGAPalette;

class RSVGA{
private:
    int width;
    int height;
    AssetManager* assets;
    VGAPalette palette;
    FrameBuffer* frameBuffer;
    uint32_t *upscaled_framebuffer{nullptr};
    uint32_t textureID;
    Texel data[320 * 200];
    void displayBuffer(uint32_t* buffer, int width, int height);

public:
    bool upscale{false};
    
    static RSVGA& getInstance() {
        static RSVGA instance; 
        return instance;
    };    
    RSVGA();
    ~RSVGA();
    
    
    void init(int width, int height, AssetManager* amana);
    
    void activate(void);
    void setPalette(VGAPalette* newPalette);
    VGAPalette* getPalette(void);
    void vSync(void);
    void fadeOut(int steps = 10, int delayMs = 50);
    FrameBuffer* getFrameBuffer(void){ return frameBuffer;};

};
