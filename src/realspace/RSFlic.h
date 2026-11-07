//
//  RSFlic.h
//  libRealSpace
//
//  Animation d'introduction de la version CD-ROM (DATA\FLIC\THEFLIC.FLX), décodée comme MENU.EXE.
//  Format : strike_commander_re/analysis/menu_cd/FLX_FORMAT.md
//
#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

#include "../commons/LZBuffer.h"
#include "../engine/Texture.h"
#define FLC_WIDTH 320
#define FLC_HEIGHT 200
class RSFlic {
public:
    

    // Image courante (indices de palette) et palette courante, telles que l'écran du jeu les montre.
    uint8_t screen[FLC_WIDTH * FLC_HEIGHT]{};
    VGAPalette palette;
    // Vrai quand la dernière image décodée suit une nouvelle palette (le jeu éteint l'écran pendant
    // le changement de palette et le rallume après le dessin de cette image).
    bool paletteChanged{ false };
    // Nombre d'images déjà dessinées (compteur des sons de MENU.EXE, mot DGROUP:2690).
    int frameCount{ 0 };

    RSFlic();
    ~RSFlic();
    // Lit l'en-tête et la table des enregistrements ; data doit rester valide pendant la lecture.
    bool InitFromRAM(uint8_t *data, size_t size);
    // Traite les enregistrements jusqu'à la prochaine image et la dessine ; false à la fin du fichier.
    bool DecodeNextFrame(void);
    size_t GetNumRecords(void) const;

private:
    struct FlicRecord {
        uint8_t *data;
        size_t size;
    };
    std::vector<FlicRecord> records;
    size_t next{ 0 };
    LZBuffer lzw;

    void ApplyPalette(const FlicRecord &rec);
    void DrawFrame(const FlicRecord &rec);
    void DecodeBlocks(uint8_t *src, size_t size, int firstRow);
    void DrawRows(uint8_t *src, size_t size);
    void CopyRows(const uint8_t *src, size_t size, int firstRow);
    uint8_t DarkestColor(void) const;
};
