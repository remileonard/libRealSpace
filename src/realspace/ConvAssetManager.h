//
//  ConAssetManager.h
//  libRealSpace
//
//  Created by Fabien Sanglard on 2/1/2014.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//
#pragma once

#define CONV_TOP_BAR_HEIGHT 23
#define CONV_BOTTOM_BAR_HEIGHT 48

#define CONV_BORDER_MARGIN 69
#define CONV_INTERLETTER_SPACE 3
#define CONV_SPACE_SIZE 5

#include "AssetManager.h"
#include "RSImageSet.h"
#include "../commons/IFFSaxLexer.h"

typedef struct CharFace {

    char name[9];
    RSImageSet *appearances;
    // size_t paletteID;
} CharFace;

typedef struct FacePalette {

    char name[9];
    uint8_t index;

} FacePalette;

typedef struct CharFigure {

    char name[9];
    RSImageSet *appearances;
    size_t paletteID;
    uint16_t x{0}, y{0};

} CharFigure;

typedef struct ConvBackGround {
    std::vector<RLEShape *> layers;
    std::vector<uint8_t *> palettes;
    char name[9];
} ConvBackGround;

class ConvAssetManager {
private:
    void BuildDB(void);


    std::map<char *, CharFace *, Char_String_Comparator> faces;
    std::map<char *, FacePalette *, Char_String_Comparator> facePalettes;
    std::map<char *, ConvBackGround *, Char_String_Comparator> backgrounds;
    std::map<char *, CharFigure *, Char_String_Comparator> figures;

    PakArchive convShps;
    PakArchive convPals;
    PakArchive optShps;
    PakArchive optPals;
    ConvBackGround *tmp_conv_bg{nullptr};
    
    void ParseBGLayer(uint8_t *data, size_t layerID, ConvBackGround *back);
    void parseIFF(uint8_t *data, size_t size);
    void parseBCKS(uint8_t *data, size_t size);
    void parseBCKS_BACK(uint8_t *data, size_t size);
    void parseBCKS_BACK_INFO(uint8_t *data, size_t size);
    void parseBCKS_BACK_DATA(uint8_t *data, size_t size);
    void parseFACE(uint8_t *data, size_t size);
    void parseFACE_DATA(uint8_t *data, size_t size);
    void parseFIGR(uint8_t *data, size_t size);
    void parseFIGR_DATA(uint8_t *data, size_t size);
    void parsePFIG(uint8_t *data, size_t size);
    void parsePFIG_DATA(uint8_t *data, size_t size);
    void parseFCPL(uint8_t *data, size_t size);
    void parseFCPL_DATA(uint8_t *data, size_t size);
    void parseFGPL(uint8_t *data, size_t size);
    void parseFGPL_DATA(uint8_t *data, size_t size);
    inline static std::unique_ptr<ConvAssetManager> s_instance{};
    AssetManager &Assets = AssetManager::getInstance();
public:
    static ConvAssetManager& getInstance() {
        if (!ConvAssetManager::hasInstance()) {
            ConvAssetManager::setInstance(std::make_unique<ConvAssetManager>());
        }
        ConvAssetManager& instance = ConvAssetManager::instance();
        return instance;
    };
    static ConvAssetManager& instance() {
        return *s_instance; 
    }
    static void setInstance(std::unique_ptr<ConvAssetManager> inst) {
        s_instance = std::move(inst);
    }
    static bool hasInstance() { return (bool)s_instance; }
    ConvAssetManager();
    ~ConvAssetManager();

    void init(void);

    CharFace *GetCharFace(char *name);
    ConvBackGround *GetBackGround(char *name);
    CharFigure *GetFigure(char *name);

    uint8_t GetFacePaletteID(char *name);
    std::string conv_file_name;

};
