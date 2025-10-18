#include "precomp.h"
#include "SDL2/SDL_opengl_glext.h"
#include <random>
#include <cmath>
#include <algorithm>

SCSmokeSet::SCSmokeSet(){

}
SCSmokeSet::~SCSmokeSet(){
    for (auto tex : this->textures) {
        if (tex) {
            if (tex->data) {
                free(tex->data);
                tex->data = nullptr;
            }
            delete tex;
        }
    }
    this->textures.clear();

    this->smoke_textures.clear();

    for (auto tex : this->missile_smoke_textures) {
        if (tex) {
            if (tex->data) {
                free(tex->data);
                tex->data = nullptr;
            }
            delete tex;
        }
    }
    this->missile_smoke_textures.clear();

    if (this->smoke_set) {
        delete this->smoke_set;
        this->smoke_set = nullptr;
    }
}
void SCSmokeSet::generateMissileSmokeTextures(int frames, int size) {
    if (frames <= 0 || size <= 0) return;

    // Taille du “pixel” visible (4x4 sur une texture 64x64 => grille 16x16)
    const int cell = 4;
    const int gw = (std::max)(1, size / cell);
    const int gh = (std::max)(1, size / cell);

    std::mt19937 rng(1337u); // déterministe
    std::uniform_real_distribution<float> rnd(0.0f, 1.0f);

    const float cgx = gw * 0.5f;
    const float cgy = gh * 0.5f;

    // Rayon en “cellules” (grille basse résolution)
    const float r0 = (size * 0.46f) / float(cell);
    const float r1 = (size * 0.18f) / float(cell);

    for (int f = 0; f < frames; ++f) {
        const float t = (frames > 1) ? float(f) / float(frames - 1) : 1.0f; // 0..1
        const float r = r0 * (1.0f - t) + r1 * t;

        // Disparition globale avec le temps
        const float globalFade = std::clamp(1.0f - t * 0.7f, 0.0f, 1.0f);

        Texture* tex = new Texture();
        tex->width  = size;
        tex->height = size;
        tex->initialized = false;

        const size_t imgsize = size_t(size) * size_t(size);
        tex->data = (uint8_t*)malloc(imgsize * 4);
        if (!tex->data) {
            delete tex;
            continue;
        }

        // Calcule l’alpha par cellule (pixel art), puis étend à 4x4 pixels
        std::vector<uint8_t> cellAlpha(size_t(gw) * size_t(gh), 0);

        for (int gy = 0; gy < gh; ++gy) {
            for (int gx = 0; gx < gw; ++gx) {
                const float dx = (gx + 0.5f) - cgx;
                const float dy = (gy + 0.5f) - cgy;
                const float d  = std::sqrt(dx * dx + dy * dy);

                // Jitter radial pour casser la circularité parfaite (en unités “cellule”)
                const float jitter = (rnd(rng) - 0.5f) * 0.9f;
                const float rEff = (std::max)(0.0f, r + jitter);

                bool inside = (d <= rEff + 0.25f);

                // Bord “cassé” + trous qui augmentent dans le temps
                const float rim = std::clamp((d - (rEff - 1.0f)) / 1.5f, 0.0f, 1.0f);
                const float holeProb = std::clamp(t * 0.55f + rim * 0.25f, 0.0f, 0.9f);

                if (inside && rnd(rng) < holeProb) {
                    inside = false;
                }

                float aNorm = 0.0f;
                if (inside) {
                    // 3 niveaux “pixel art” selon la proximité du bord
                    if (d < rEff - 2.0f)      aNorm = 1.0f;
                    else if (d < rEff - 1.0f) aNorm = 0.6f;
                    else                      aNorm = 0.35f;

                    aNorm *= globalFade;

                    // Quantification à 4 niveaux (0, 1/3, 2/3, 1)
                    const float steps = 3.0f;
                    aNorm = std::round(aNorm * steps) / steps;
                }

                cellAlpha[size_t(gy) * size_t(gw) + size_t(gx)] =
                    static_cast<uint8_t>(std::round(std::clamp(aNorm, 0.0f, 1.0f) * 255.0f));
            }
        }

        // Écriture dans la texture (chaque cellule remplit un bloc 4x4)
        uint8_t* dst = tex->data;
        for (int y = 0; y < size; ++y) {
            const int gy = (std::min)(gh - 1, y / cell);
            for (int x = 0; x < size; ++x) {
                const int gx = (std::min)(gw - 1, x / cell);
                const uint8_t a = cellAlpha[size_t(gy) * size_t(gw) + size_t(gx)];
                dst[0] = 255; // blanc
                dst[1] = 255;
                dst[2] = 255;
                dst[3] = a;
                dst += 4;
            }
        }
        const unsigned denom = (unsigned)f + 1u;
        uint8_t* p = tex->data;
        for (size_t i = 0; i < imgsize; ++i) {
            const unsigned a0 = (unsigned)p[3];
            p[3] = (uint8_t)((a0 + denom / 2u) / denom);
            p += 4;
        }
        this->missile_smoke_textures.push_back(tex);
    }
}
void SCSmokeSet::init(){

    RSPalette palette;
    TreEntry *entries = (TreEntry *)Assets.GetEntryByName("..\\..\\DATA\\PALETTE\\PALETTE.IFF");
    
    if (entries != nullptr) {
        palette.initFromFileRam(entries->data, entries->size);
    } else {
        FileData *f = Assets.GetFileData("PALETTE.IFF");
        if (f != nullptr) {
            palette.initFromFileData(f);
        }
    }
    this->palette = *palette.GetColorPalette();

    std::string path = "..\\..\\DATA\\OBJECTS\\SMOKESET.IFF";
    TreEntry *smk = Assets.GetEntryByName(path.c_str());

    RSSmokeSet *smoke_set = new RSSmokeSet();
    smoke_set->InitFromRam(smk->data, smk->size);
    this->smoke_set = smoke_set;
    int qsdf = 0;
    for (auto img_set: this->smoke_set->images) {
        size_t numimages = img_set->GetNumImages();
        std::vector<Texture *> smk_set;

        for (size_t i=0; i<numimages; i++) {
            RLEShape *img = img_set->GetShape(i);
            Texture *tex = new Texture();
            tex->height = img->GetHeight();
            tex->width = img->GetWidth();
            img->position.x = 0;
            img->position.y = 0;
            img->buffer_size.x = (int32_t) tex->width;
            img->buffer_size.y = (int32_t) tex->height;
            size_t imgsize = tex->width*tex->height;
            uint8_t *imgdata = (uint8_t *)malloc(imgsize);
            memset(imgdata, 255, imgsize);
            size_t byteRead = 0;
            img->Expand(imgdata, &byteRead);
            
            tex->data = (uint8_t *)malloc(imgsize*4);
            memset(tex->data, 255, imgsize*4);
            uint8_t *dst = tex->data;
            long checksum = 0;
            for (size_t j = 0; j < imgsize; j++) {
                Texel *rgba = this->palette.GetRGBColor(imgdata[j]);
                if (imgdata[j] == 255) {
                    rgba->a = 0;
                }
                if (rgba->a == 255 && rgba->r == 211 && rgba->g == 211 && rgba->b == 211) {
                    rgba->a = 0;
                }
                dst[0] = rgba->r;
                dst[1] = rgba->g;
                dst[2] = rgba->b;
                dst[3] = rgba->a;
                dst += 4;
            }
            tex->initialized = false;
            
            this->textures.push_back(tex);
            smk_set.push_back(tex);
        }
        this->smoke_textures.push_back(smk_set);
    }
    generateMissileSmokeTextures(20, 64);
}