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
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
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
    
    void Diff(VGAPalette* other){
        for (int i=0  ;i <256 ; i++){
            if(colors[i].r != other->colors[i].r ||
               colors[i].g != other->colors[i].g ||
               colors[i].b != other->colors[i].b ||
               colors[i].a != other->colors[i].a
               ) {}
        }
            
    }
    
    void ReadPatch(ByteStream* s){
        
        int16_t offset = s->ReadShort();
        int16_t numColors = s->ReadShort();
        
        if (offset + numColors > 256){
            return;
        }
        
        for (uint16_t i= 0 ; i < numColors ; i++){
            colors[offset+i].r = s->ReadByte() * (uint8_t) (255.0f/63.0f);
            colors[offset+i].g = s->ReadByte() * (uint8_t) (255.0f/63.0f);
            colors[offset+i].b = s->ReadByte() * (uint8_t) (255.0f/63.0f);
            colors[offset+i].a = 255 ;
        }
        
    }
    void ReadPatch(VGAPalette* other){
        int i=-1;
        for (auto c : other->colors){
            i++;
            if (c.r == 0 && c.g == 255 && c.b == 0){
                continue;
            }
            if (c.r == 255 && c.g == 0 && c.b == 255){
                continue;
            }
            colors[i] = c;
        }
    }
    
} VGAPalette ;

class RSImage;

class Texture{
    
public:
    Texture();
    ~Texture();
    
    void Set(RSImage* image );
    size_t width;
    size_t height;
    char name[9];
    uint8_t* data;
    bool initialized{false};
    
    
    enum Location{ DISK=0x1,RAM=0x2,VRAM=0x4};
    uint8_t locFlag;
    
    //GPU stuff
    uint32_t id;
    uint32_t GetTextureID(void);
    

    
    void UpdateContent(RSImage* image);

private:
    
};
