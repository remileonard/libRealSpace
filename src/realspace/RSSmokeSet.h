#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include "../engine/Texture.h"
#include "AssetManager.h"
#include "../commons/IFFSaxLexer.h"

class RLEShape;

// Modèle de fumée : FORM SMOK de FORM TYPE (SmokeSet_LoadSmokeModel_A61D0).
struct RSSmokeModel {
    uint8_t renderMode{ 0 }; // 0 disque, 1 sprite opaque, 2 sprite translucide
    float spacing{ 1.0f };   // m entre deux particules d'une traînée
    float sizeStart{ 0.0f }; // m
    float sizeEnd{ 0.0f };   // m
    float life{ 1.0f };      // s
    float jitter{ 0.0f };    // m
    float shadeStart{ 0.0f };
    float shadeEnd{ 0.0f };
    uint8_t translucentFirst{ 0 };
    uint8_t translucentCount{ 0 };
    uint8_t shapeA{ 255 };
    uint8_t chanceA{ 100 };
    uint8_t shapeB{ 255 };
    char animMode{ 'T' };
    int16_t framesPerSecond{ 0 };
    int16_t frameStep{ 0 };
    std::vector<uint8_t> frameOrder; // chunk SCRI

    float sizeRate() const;
    float shadeRate() const;
};

// Forme : FORM SHAP de FORM TYPE (SmokeSet_LoadShape_A60AC).
struct RSSmokeShape {
    std::vector<uint8_t> data;
    std::vector<RLEShape *> images;
    int halfSize{ 1 }; // pixels : (plus grande hauteur + plus grande largeur) / 4
};

// Table FADE approchée par un mélange : palette[FADE[c]] ≈ (1 − alpha) × palette[c] + alpha × (r, g, b).
struct FadeBlend {
    float alpha;
    float r;
    float g;
    float b;
};

// Chunk SMOK d'un objet : point d'émission converti comme les sommets du chunk VERT (x = c1, y = c2, z = c0).
struct RSObjectSmoke {
    bool present{ false };
    uint8_t type{ 0 };
    float x{ 0.0f };
    float y{ 0.0f };
    float z{ 0.0f };
};

// Chunk INFO d'un objet SMKG (SmokeGen_LoadFromIFF_A5F89).
struct RSSmokeGeneratorInfo {
    uint8_t type{ 0 };
    float duration{ 0.0f }; // s
    float rise{ 0.0f };     // m/s
    float interval{ 1.0f }; // s
    uint16_t chance{ 100 };
    uint8_t removeAtEnd{ 0 };
};

class RSSmokeSet {
    void parseSMOK(uint8_t *data, size_t size);
    void parseSMOK_INFO(uint8_t *data, size_t size);
    void parseSMOK_TYPE(uint8_t *data, size_t size);
    void parseSMOK_TYPE_SMOK(uint8_t *data, size_t size);
    void parseSMOK_TYPE_SMOK_INFO(uint8_t *data, size_t size);
    void parseSMOK_TYPE_SMOK_ANIM(uint8_t *data, size_t size);
    void parseSMOK_TYPE_SMOK_SCRI(uint8_t *data, size_t size);
    void parseSMOK_TYPE_SHAP(uint8_t *data, size_t size);
    void parseSMOK_TYPE_SHAP_SHAP(uint8_t *data, size_t size);

    std::unordered_map<uint64_t, Texture *> fade_textures;
    std::vector<uint8_t> palette_rgb;

    static uint64_t textureKey(int model, int shape, int image, int shade);
    Texture *buildTexture(const RSSmokeModel &m, RLEShape *rle, int shade);
    void load();

    inline static std::unique_ptr<RSSmokeSet> s_instance{};

public:
    RSSmokeSet();
    ~RSSmokeSet();
    // Jeu de fumées partagé : OBJECTS\SMOKESET.IFF, PALETTE\PALETTE.IFF et PALETTE\PALFADE.IFF, lus au premier appel.
    static RSSmokeSet &getInstance();
    void InitFromRam(uint8_t *data, size_t size);

    uint16_t declaredModels{ 0 };
    uint16_t declaredShapes{ 0 };
    uint16_t maxParticles{ 0 };
    std::vector<RSSmokeModel> models;
    std::vector<RSSmokeShape> shapes;
    std::vector<FadeBlend> fade_blends;

    void buildTextures(VGAPalette *palette, uint8_t *palfade, size_t size);
    Texture *getTexture(int model, int shape, int image, int shade);
    size_t textureCount() const;
};

RSObjectSmoke readObjectSmoke(uint8_t *data, size_t size);
uint8_t readJinfFallSmoke(uint8_t *data, size_t size);
RSSmokeGeneratorInfo readSmokeGeneratorInfo(uint8_t *data, size_t size);
std::vector<uint8_t> loadFadeTables(uint8_t *data, size_t size);
FadeBlend fitFadeTable(const uint8_t *palette, const uint8_t *table);
std::vector<FadeBlend> fitFadeTables(const uint8_t *palette, const uint8_t *fade, size_t fadeSize);
