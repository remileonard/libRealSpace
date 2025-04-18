#pragma once
#include "precomp.h"

struct texture {
    GLuint texture_id{0};
    uint8_t *data{nullptr};
    bool initialized{false};
    int width{0};
    int height{0};
};

class SCSmokeSet {
    RSSmokeSet *smoke_set;
    VGAPalette palette;
public: 
    std::vector<texture *> textures;
    SCSmokeSet();
    ~SCSmokeSet();
    void init();
};