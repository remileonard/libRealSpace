#include "../strike_commander/precomp.h"
#include "EnhancedCockpit.h"
#include <cstdio>
#include <filesystem>
#include <string>
#include "../engine/Config.hpp"
#include "../engine/SCRenderer.h"
#include "../engine/Texture.h"
#include "../realspace/RSEntity.h"
#include "../realspace/RSImage.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "third_party/stb_image.h"

static const char *HD_DIR = "./assets/hdtex/";

void EnhancedCockpit::init() {
    SCCockpit::init();
    if (!Config::getInstance().getBool("Video", "hd_textures", false)) {
        return;
    }
    const int replaced = this->replaceTextures();
    printf("[enhanced] HD textures: %d replaced (%s)\n", replaced, HD_DIR);
}

int EnhancedCockpit::replaceTextures() {
    if (this->cockpit == nullptr || this->cockpit->REAL.OBJS == nullptr) {
        return 0;
    }
    int replaced = 0;
    for (RSImage *image : this->cockpit->REAL.OBJS->images) {
        if (image != nullptr && this->uploadReplacement(image)) {
            replaced++;
        }
    }
    return replaced;
}

bool EnhancedCockpit::uploadReplacement(RSImage *image) {
    char name[9] = {0};
    for (int i = 0; i < 8; i++) {
        const char c = image->name[i];
        if (c == 0) {
            break;
        }
        const bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_';
        name[i] = ok ? c : '_';
    }
    if (name[0] == 0) {
        return false;
    }
    const std::string path = std::string(HD_DIR) + name + "_" + std::to_string(image->width) + "x" +
                             std::to_string(image->height) + ".png";
    if (!std::filesystem::exists(path)) {
        return false;
    }
    int w = 0;
    int h = 0;
    int comp = 0;
    unsigned char *pixels = stbi_load(path.c_str(), &w, &h, &comp, 4);
    if (pixels == nullptr || w <= 0 || h <= 0) {
        printf("[enhanced] HD textures: could not load %s\n", path.c_str());
        if (pixels != nullptr) {
            stbi_image_free(pixels);
        }
        return false;
    }
    Texture *texture = image->GetTexture();
    glBindTexture(GL_TEXTURE_2D, texture->id);
    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    stbi_image_free(pixels);
    printf("[enhanced] HD textures: %s -> %dx%d\n", path.c_str(), w, h);
    return true;
}
