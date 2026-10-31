#pragma once

#include <unordered_map>
#include <vector>
#include <cstring>

#include "../commons/IFFSaxLexer.h"
#include "RLEShape.h"
#include "../commons/ByteStream.h"
#include "RSFont.h"
#include "PakArchive.h"
#include "block_def.h"

class RSNavMap {
    std::vector<uint8_t> texts;

    void parseNMAP(uint8_t* data, size_t size);
    void parseNMAP_INFO(uint8_t *data, size_t size);
    void parseNMAP_MAPS(uint8_t* data, size_t size);
    void parseNMAP_FONT(uint8_t* data, size_t size);
    void parseNMAP_TEXT(uint8_t* data, size_t size);
    void parseNMAP_SHAP(uint8_t* data, size_t size);

public:
    std::unordered_map<std::string, RLEShape*> maps;
    RLEShape *background;
    RSFont *font;
    // Chunk INFO (Font_LoadDefinitionFromIFF_7E1B4) : fenêtre de la carte à l'écran et demi-taille du monde.
    int16_t map_left{ 0 };
    int16_t map_top{ 0 };
    int16_t map_width{ 0 };
    int16_t map_height{ 0 };
    float world_half_size{ 0.0f };
    RSNavMap();
	~RSNavMap();
	void InitFromRam(uint8_t* data, size_t size);
};