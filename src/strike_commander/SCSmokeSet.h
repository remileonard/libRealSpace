#pragma once
#include "precomp.h"

class SCSmokeSet {
private:
    RSSmokeSet *smoke_set;
    VGAPalette palette;
    AssetManager &Assets = AssetManager::getInstance();
public: 
    std::vector<Texture *> textures;
    std::vector<std::vector<Texture *>> smoke_textures;
    SCSmokeSet();
    ~SCSmokeSet();
    void init();
};