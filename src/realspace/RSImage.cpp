//
//  RSImage.cpp
//  libRealSpace
//
//  Created by Fabien Sanglard on 12/31/2013.
//  Copyright (c) 2013 Fabien Sanglard. All rights reserved.
//

#include "precomp.h"
#include "RSImage.h"
#include "../engine/SCRenderer.h"

RSImage::RSImage() :
data(0),
dirty(false),
palette(NULL){
    
}

RSImage::~RSImage(){
    if (data)
        free(data);
}

void RSImage::Create(const char name[8],uint32_t width,uint32_t height, uint32_t flags){

    strncpy(this->name,name,8);
    this->flags = flags;
    this->width = width;
    this->height = height;
    this->data = (uint8_t*)malloc(this->width*this->height);
    SCRenderer &Renderer = SCRenderer::getInstance();
    this->palette = Renderer.getPalette();
    this->nbframes = 1;
    this->texture.set(this);
    dirty = true;
}

void RSImage::UpdateContent(uint8_t* src) {
    memcpy(this->data,src, width * height);
    this->dirty = true;
}
void RSImage::UpdateContent(uint8_t* src, size_t csize) {
    this->data = (uint8_t*)realloc(this->data, csize);
    memcpy(this->data,src, csize);
    this->dirty = true;
}


void RSImage::SyncTexture(void){
    SCRenderer &Renderer = SCRenderer::getInstance();
    //Check that we have a texture with an id on the GPU
    if ((texture.locFlag & Texture::VRAM) != Texture::VRAM){
        //Create texture in the GPU
        Renderer.createTextureInGPU(&texture);
        texture.locFlag |= Texture::VRAM;
    }
    
    //Check if we are synchornized with GPU
    if (this->dirty){
        texture.updateContent(this);
        Renderer.uploadTextureContentToGPU(&texture);
        dirty = false;
    }
    
    
}


uint8_t* RSImage::GetData(void){
    dirty = true;
    return data;
}

void RSImage::ClearContent(void){
    memset(this->data,0,this->width*this->height);
    dirty = true;
}

void RSImage::GetNextFrame() {
    SCRenderer &Renderer = SCRenderer::getInstance();
    if (this->sub_frame_buffer ==  nullptr) {
        this->sub_frame_buffer = (uint8_t*)malloc(this->width * this->height);
    }
    if (this->nbframes > 1) {
        this->fps++;
        if (this->fps < 10) {
            return; // Do not change frame too fast
        }
        this->fps = 0;
        size_t frame_size = this->width * this->height;
        memcpy(this->sub_frame_buffer, this->data+frame_size*this->current_frame, frame_size);
        RSImage subframe;
        subframe.Create(this->name, this->width, this->height, this->flags);
        subframe.UpdateContent(this->sub_frame_buffer);
        subframe.SetPalette(this->palette);
        //this->texture.Set(&subframe);
        this->texture.animated = true;
        this->texture.updateContent(&subframe);
        this->current_frame= (this->current_frame + 1) % this->nbframes;
        //texture.UpdateContent(this);
        Renderer.uploadTextureContentToGPU(&texture);
    }
}
void RSImage::SetPalette(VGAPalette* palette){
    this->palette = palette;
    dirty = true;
}

Texture* RSImage::GetTexture(void){
    SyncTexture();
    return &this->texture;
}
