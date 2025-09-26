//
//  Texture.h
//  iff
//
//  Created by Fabien Sanglard on 12/20/2013.
//  Copyright (c) 2013 Fabien Sanglard. All rights reserved.
//

#pragma once

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#include <stdlib.h>
#include <stdio.h>

#ifdef __cplusplus
}
#endif
#include "../commons/ByteStream.h"

class RSPalette;
uint8_t convertFrom6To8(uint8_t value);
typedef struct Texel{
    uint8_t r{0};
    uint8_t g{0};
    uint8_t b{0};
    uint8_t a{0};
} Texel;

typedef struct VGAPalette{
    
    Texel colors[256];

    void SetColor(uint8_t value,Texel* texel){
        
        Texel* paletteColor ;
        paletteColor = &colors[value] ;
        *paletteColor = *texel;
    }
    
    Texel* GetRGBColor(uint8_t value){
        return &colors[value];
    }
    
    Texel* GetRGBColorSetAlpha(uint8_t value, bool setAlpha){
        if (setAlpha){
            colors[value].a = 0;
        }
        return &colors[value];
    }
    
    void Diff(VGAPalette* other);
    void ReadPatch(ByteStream* s);
    void CopyFromOtherPalette(VGAPalette* other, bool filter=true);
    
} VGAPalette ;

class RSImage;

class Texture{
    
public:
    enum Location{ DISK=0x1,RAM=0x2,VRAM=0x4};

    size_t width;
    size_t height;
    char name[9];
    uint8_t* data;
    bool initialized{false};
    bool animated{false};
    bool needAphaFix{false};
    uint8_t locFlag;
    uint32_t id;

    Texture();
    ~Texture();
    
    void set(RSImage* image );
    void updateContent(RSImage* image);
    uint32_t getTextureID(void);
    
};
