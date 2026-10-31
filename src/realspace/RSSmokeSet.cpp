#include "RSSmokeSet.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>

#include "../commons/ByteStream.h"
#include "../commons/LZBuffer.h"
#include "PakArchive.h"
#include "RLEShape.h"
#include "RSPalette.h"

typedef std::unordered_map<std::string, std::function<void(uint8_t *data, size_t size)>> IFFHandlers;

float RSSmokeModel::sizeRate() const {
    return (sizeEnd - sizeStart) / life;
}

float RSSmokeModel::shadeRate() const {
    return (shadeEnd - shadeStart) / life;
}

RSSmokeSet::RSSmokeSet() {
}

RSSmokeSet::~RSSmokeSet() {
    for (auto &shape : shapes) {
        for (auto image : shape.images) {
            delete image;
        }
    }
    for (auto &entry : fade_textures) {
        delete entry.second;
    }
}

RSSmokeSet &RSSmokeSet::getInstance() {
    if (!s_instance) {
        s_instance = std::make_unique<RSSmokeSet>();
        s_instance->load();
    }
    return *s_instance;
}

void RSSmokeSet::load() {
    AssetManager &assets = AssetManager::getInstance();
    TreEntry *smk = assets.GetEntryByName("..\\..\\DATA\\OBJECTS\\SMOKESET.IFF");
    InitFromRam(smk->data, smk->size);

    RSPalette palette;
    TreEntry *pal = assets.GetEntryByName("..\\..\\DATA\\PALETTE\\PALETTE.IFF");
    if (pal != nullptr) {
        palette.initFromFileRam(pal->data, pal->size);
    } else {
        FileData *f = assets.GetFileData("PALETTE.IFF");
        if (f != nullptr) {
            palette.initFromFileData(f);
        }
    }
    TreEntry *fade = assets.GetEntryByName("..\\..\\DATA\\PALETTE\\PALFADE.IFF");
    buildTextures(palette.GetColorPalette(), fade->data, fade->size);
    printf("[SMOKE] jeu de fumées : %zu modèles, %zu formes, %u particules au plus, %zu tables FADE, %zu textures\n",
           models.size(), shapes.size(), maxParticles, fade_blends.size(), fade_textures.size());
    for (size_t i = 0; i < models.size(); i++) {
        const RSSmokeModel &m = models[i];
        printf("[SMOKE]   modèle %zu : mode %u, espacement %.2f m, taille %.2f -> %.2f m, vie %.2f s, dispersion %.2f m, "
               "teinte %.0f -> %.0f, A %u B %u, formes %u (%u %%) / %u, anim %c %d img/s pas %d\n",
               i, m.renderMode, m.spacing, m.sizeStart, m.sizeEnd, m.life, m.jitter, m.shadeStart, m.shadeEnd,
               m.translucentFirst, m.translucentCount, m.shapeA, m.chanceA, m.shapeB, m.animMode, m.framesPerSecond,
               m.frameStep);
    }
}

void RSSmokeSet::InitFromRam(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    IFFHandlers handlers;
    handlers["SMOK"] = std::bind(&RSSmokeSet::parseSMOK, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

void RSSmokeSet::parseSMOK(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    IFFHandlers handlers;
    handlers["INFO"] = std::bind(&RSSmokeSet::parseSMOK_INFO, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TYPE"] = std::bind(&RSSmokeSet::parseSMOK_TYPE, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

void RSSmokeSet::parseSMOK_INFO(uint8_t *data, size_t size) {
    ByteStream bs(data, size);
    declaredModels = bs.ReadUShort();
    declaredShapes = bs.ReadUShort();
    maxParticles = bs.ReadUShort();
}

// Numéro de modèle = rang parmi les FORM SMOK, numéro de forme = rang parmi les FORM SHAP.
void RSSmokeSet::parseSMOK_TYPE(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    IFFHandlers handlers;
    handlers["SMOK"] = std::bind(&RSSmokeSet::parseSMOK_TYPE_SMOK, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SHAP"] = std::bind(&RSSmokeSet::parseSMOK_TYPE_SHAP, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

void RSSmokeSet::parseSMOK_TYPE_SMOK(uint8_t *data, size_t size) {
    models.push_back(RSSmokeModel());
    IFFSaxLexer lexer;
    IFFHandlers handlers;
    handlers["INFO"] =
        std::bind(&RSSmokeSet::parseSMOK_TYPE_SMOK_INFO, this, std::placeholders::_1, std::placeholders::_2);
    handlers["ANIM"] =
        std::bind(&RSSmokeSet::parseSMOK_TYPE_SMOK_ANIM, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SCRI"] =
        std::bind(&RSSmokeSet::parseSMOK_TYPE_SMOK_SCRI, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

void RSSmokeSet::parseSMOK_TYPE_SMOK_INFO(uint8_t *data, size_t size) {
    RSSmokeModel &m = models.back();
    ByteStream bs(data, size);
    m.renderMode = bs.ReadByte();
    m.spacing = bs.ReadInt32LE() / 256.0f;
    m.sizeStart = bs.ReadInt32LE() / 256.0f;
    m.sizeEnd = bs.ReadInt32LE() / 256.0f;
    m.life = bs.ReadInt32LE() / 256.0f;
    // Math_RandomJitter_54E1C ne prend que le mot bas.
    m.jitter = static_cast<int16_t>(bs.ReadInt32LE() & 0xFFFF) / 256.0f;
    m.shadeStart = bs.ReadByte();
    m.shadeEnd = bs.ReadByte();
    m.translucentFirst = bs.ReadByte();
    m.translucentCount = bs.ReadByte();
    if (m.life <= 0.0f) {
        m.life = 1.0f / 256.0f;
    }
}

void RSSmokeSet::parseSMOK_TYPE_SMOK_ANIM(uint8_t *data, size_t size) {
    RSSmokeModel &m = models.back();
    ByteStream bs(data, size);
    m.shapeA = bs.ReadByte();
    m.chanceA = bs.ReadByte();
    m.shapeB = bs.ReadByte();
    m.animMode = static_cast<char>(bs.ReadByte());
    m.framesPerSecond = bs.ReadShort();
    m.frameStep = bs.ReadShort();
}

void RSSmokeSet::parseSMOK_TYPE_SMOK_SCRI(uint8_t *data, size_t size) {
    RSSmokeModel &m = models.back();
    m.frameOrder.assign(data, data + size);
}

void RSSmokeSet::parseSMOK_TYPE_SHAP(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    IFFHandlers handlers;
    handlers["SHAP"] =
        std::bind(&RSSmokeSet::parseSMOK_TYPE_SHAP_SHAP, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

// Version CD : « LZ », uint32 taille, données LZW.
void RSSmokeSet::parseSMOK_TYPE_SHAP_SHAP(uint8_t *data, size_t size) {
    RSSmokeShape shape;
    if (size > 6 && data[0] == 'L' && data[1] == 'Z') {
        LZBuffer lz;
        size_t csize = 0;
        uint8_t *raw = lz.DecodeLZW(data + 6, size - 6, csize);
        shape.data.assign(raw, raw + csize);
    } else {
        shape.data.assign(data, data + size);
    }
    // Demi-taille (Text_ComputeMaxLineHeight_5E706, Text_ComputeMaxLineWidth_5E6BF, sar ax, 2).
    int maxWidth = 0;
    int maxHeight = 0;
    ByteStream header(shape.data.data(), shape.data.size());
    header.ReadUInt32LE();
    uint32_t first = header.ReadUInt32LE();
    size_t count = first / 4 - 1;
    for (size_t i = 0; i < count; i++) {
        uint32_t offset = 0;
        memcpy(&offset, &shape.data[4 + 4 * i], 4);
        ByteStream bs(&shape.data[offset], 8);
        int right = bs.ReadShort();
        int left = bs.ReadShort();
        int top = bs.ReadShort();
        int bottom = bs.ReadShort();
        maxWidth = (std::max)(maxWidth, right + left + 1);
        maxHeight = (std::max)(maxHeight, top + bottom + 1);
    }
    shape.halfSize = (std::max)(1, (maxHeight + maxWidth) / 4);
    shapes.push_back(std::move(shape));
    // PakArchive et RLEShape gardent un pointeur dans data, qui ne bouge pas quand shapes s'agrandit.
    RSSmokeShape &stored = shapes.back();
    PakArchive pak;
    pak.InitFromRAM("SHAPE", stored.data.data(), stored.data.size());
    for (size_t i = 0; i < pak.GetNumEntries(); i++) {
        RLEShape *image = new RLEShape();
        image->init(pak.GetEntry(i)->data, pak.GetEntry(i)->size);
        Point2D origin;
        origin.x = 0;
        origin.y = 0;
        image->SetPosition(&origin);
        stored.images.push_back(image);
    }
}

// uint8 type, puis 3 int32 24.8 : c0, c1 (axe du nez), c2 (altitude).
RSObjectSmoke readObjectSmoke(uint8_t *data, size_t size) {
    RSObjectSmoke smoke;
    ByteStream bs(data, size);
    smoke.present = true;
    smoke.type = bs.ReadByte();
    float c0 = bs.ReadInt32LE() / 256.0f;
    float c1 = bs.ReadInt32LE() / 256.0f;
    float c2 = bs.ReadInt32LE() / 256.0f;
    smoke.x = c1;
    smoke.y = c2;
    smoke.z = c0;
    return smoke;
}

// Octet 11 du JINF (Aircraft_LoadEJECAndJINF_9D4F0) ; 6 sans cet octet.
uint8_t readJinfFallSmoke(uint8_t *data, size_t size) {
    if (size < 12) {
        return 6;
    }
    return data[11];
}

RSSmokeGeneratorInfo readSmokeGeneratorInfo(uint8_t *data, size_t size) {
    RSSmokeGeneratorInfo info;
    ByteStream bs(data, size);
    info.type = bs.ReadByte();
    info.duration = bs.ReadInt32LE() / 256.0f;
    info.rise = bs.ReadInt32LE() / 256.0f;
    info.interval = bs.ReadInt32LE() / 256.0f;
    info.chance = bs.ReadUShort();
    info.removeAtEnd = bs.ReadByte();
    return info;
}

// Chunk FADE de FORM FTBL : tables de 256 octets.
std::vector<uint8_t> loadFadeTables(uint8_t *data, size_t size) {
    std::vector<uint8_t> tables;
    IFFSaxLexer lexer;
    IFFHandlers handlers;
    handlers["FTBL"] = [&tables](uint8_t *form, size_t formSize) {
        IFFSaxLexer inner;
        IFFHandlers chunks;
        chunks["FADE"] = [&tables](uint8_t *fade, size_t fadeSize) {
            tables.assign(fade, fade + fadeSize - fadeSize % 256);
        };
        inner.InitFromRAM(form, formSize, chunks);
    };
    lexer.InitFromRAM(data, size, handlers);
    return tables;
}

// Moindres carrés sur les 256 couleurs : alpha et cible (SMOKE_SYSTEM.md § 6.1, tools/smoke_fade_fit.py).
FadeBlend fitFadeTable(const uint8_t *palette, const uint8_t *table) {
    const int n = 256;
    double meanIn[3] = {
        0.0,
        0.0,
        0.0,
    };
    double meanDelta[3] = {
        0.0,
        0.0,
        0.0,
    };
    for (int c = 0; c < n; c++) {
        for (int ch = 0; ch < 3; ch++) {
            double in = palette[3 * c + ch];
            double out = palette[3 * table[c] + ch];
            meanIn[ch] += in / n;
            meanDelta[ch] += (out - in) / n;
        }
    }
    double num = 0.0;
    double den = 0.0;
    for (int c = 0; c < n; c++) {
        for (int ch = 0; ch < 3; ch++) {
            double in = palette[3 * c + ch];
            double out = palette[3 * table[c] + ch];
            double x = in - meanIn[ch];
            double y = out - in - meanDelta[ch];
            num += x * y;
            den += x * x;
        }
    }
    FadeBlend blend;
    blend.alpha = den > 0.0 ? static_cast<float>(-num / den) : 0.0f;
    float target[3];
    for (int ch = 0; ch < 3; ch++) {
        double b = meanDelta[ch] + blend.alpha * meanIn[ch];
        double t = std::fabs(blend.alpha) > 1e-6 ? b / blend.alpha : 0.0;
        if (t < 0.0) {
            t = 0.0;
        }
        if (t > 255.0) {
            t = 255.0;
        }
        target[ch] = static_cast<float>(t);
    }
    blend.r = target[0];
    blend.g = target[1];
    blend.b = target[2];
    if (blend.alpha < 0.0f) {
        blend.alpha = 0.0f;
    }
    if (blend.alpha > 1.0f) {
        blend.alpha = 1.0f;
    }
    return blend;
}

std::vector<FadeBlend> fitFadeTables(const uint8_t *palette, const uint8_t *fade, size_t fadeSize) {
    std::vector<FadeBlend> blends;
    for (size_t k = 0; k + 256 <= fadeSize; k += 256) {
        blends.push_back(fitFadeTable(palette, fade + k));
    }
    return blends;
}

uint64_t RSSmokeSet::textureKey(int model, int shape, int image, int shade) {
    uint64_t k = static_cast<uint64_t>(model & 0xFF);
    k = (k << 8) | static_cast<uint64_t>(shape & 0xFF);
    k = (k << 8) | static_cast<uint64_t>(image & 0xFF);
    return (k << 8) | static_cast<uint64_t>((shade + 128) & 0xFF);
}

// Texture carrée centrée sur le point d'accroche de l'image, lignes de bas en haut (v = 0 en bas dans
// drawBillboard). Couleur hors de A … A + B − 1 : opaque ; dans la plage : table teinte + c − A.
Texture *RSSmokeSet::buildTexture(const RSSmokeModel &m, RLEShape *rle, int shade) {
    int width = (std::max)(0, static_cast<int>(rle->GetWidth()));
    int height = (std::max)(0, static_cast<int>(rle->GetHeight()));
    if (width == 0 || height == 0) {
        return nullptr;
    }
    std::vector<uint8_t> pixels(static_cast<size_t>(width) * height, 255);
    Point2D origin;
    origin.x = 0;
    origin.y = 0;
    rle->SetPosition(&origin);
    rle->buffer_size.x = width;
    rle->buffer_size.y = height;
    size_t read = 0;
    rle->Expand(pixels.data(), &read);
    rle->buffer_size.x = 320;
    rle->buffer_size.y = 200;
    int first = m.translucentFirst;
    int count = m.renderMode == 1 ? 0 : m.translucentCount;

    int left = rle->GetLeft();
    int top = rle->GetTop();
    int half = (std::max)((std::max)(left, width - left), (std::max)(top, height - top));
    int side = 2 * half;
    Texture *tex = new Texture();
    tex->width = static_cast<size_t>(side);
    tex->height = static_cast<size_t>(side);
    tex->initialized = false;
    tex->data = static_cast<uint8_t *>(calloc(static_cast<size_t>(side) * side * 4, 1));
    for (int y = 0; y < height; y++) {
        int row = side - 1 - (half - top + y);
        for (int x = 0; x < width; x++) {
            int c = pixels[static_cast<size_t>(y) * width + x];
            if (c == 255) {
                continue;
            }
            uint8_t *dst = &tex->data[4 * (static_cast<size_t>(row) * side + half - left + x)];
            if (c < first || c >= first + count) {
                dst[0] = palette_rgb[3 * c];
                dst[1] = palette_rgb[3 * c + 1];
                dst[2] = palette_rgb[3 * c + 2];
                dst[3] = 255;
                continue;
            }
            int table = shade + c - first;
            if (table < 0 || table >= static_cast<int>(fade_blends.size())) {
                continue;
            }
            const FadeBlend &blend = fade_blends[table];
            dst[0] = static_cast<uint8_t>(std::lround(blend.r));
            dst[1] = static_cast<uint8_t>(std::lround(blend.g));
            dst[2] = static_cast<uint8_t>(std::lround(blend.b));
            dst[3] = static_cast<uint8_t>(std::lround(blend.alpha * 255.0f));
        }
    }
    return tex;
}

// Une texture par modèle à formes, forme A et B, image et teinte entière de sa vie.
void RSSmokeSet::buildTextures(VGAPalette *palette, uint8_t *palfade, size_t size) {
    palette_rgb.assign(768, 0);
    for (int c = 0; c < 256; c++) {
        Texel *t = palette->GetRGBColor(static_cast<uint8_t>(c));
        palette_rgb[3 * c] = t->r;
        palette_rgb[3 * c + 1] = t->g;
        palette_rgb[3 * c + 2] = t->b;
    }
    std::vector<uint8_t> fade = loadFadeTables(palfade, size);
    fade_blends = fitFadeTables(palette_rgb.data(), fade.data(), fade.size());
    for (size_t mi = 0; mi < models.size(); mi++) {
        const RSSmokeModel &m = models[mi];
        if (m.renderMode != 1 && m.renderMode != 2) {
            continue;
        }
        int low = static_cast<int>(std::floor((std::min)(m.shadeStart, m.shadeEnd)));
        int high = static_cast<int>(std::floor((std::max)(m.shadeStart, m.shadeEnd)));
        if (m.renderMode == 1) {
            low = 0;
            high = 0;
        }
        int candidates[2] = {
            m.shapeA,
            m.shapeB,
        };
        for (int shape : candidates) {
            if (shape == 255 || shape >= static_cast<int>(shapes.size())) {
                continue;
            }
            const RSSmokeShape &s = shapes[shape];
            for (size_t image = 0; image < s.images.size(); image++) {
                for (int shade = low; shade <= high; shade++) {
                    uint64_t k = textureKey(static_cast<int>(mi), shape, static_cast<int>(image), shade);
                    if (fade_textures.count(k)) {
                        continue;
                    }
                    Texture *tex = buildTexture(m, s.images[image], shade);
                    if (tex != nullptr) {
                        fade_textures[k] = tex;
                    }
                }
            }
        }
    }
}

Texture *RSSmokeSet::getTexture(int model, int shape, int image, int shade) {
    const RSSmokeModel &m = models[model];
    if (m.renderMode == 1) {
        shade = 0;
    }
    auto found = fade_textures.find(textureKey(model, shape, image, shade));
    if (found == fade_textures.end()) {
        return nullptr;
    }
    return found->second;
}

size_t RSSmokeSet::textureCount() const {
    return fade_textures.size();
}
