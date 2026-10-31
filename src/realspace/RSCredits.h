#pragma once

#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#include "../commons/ByteStream.h"
#include "../commons/IFFSaxLexer.h"
#include "../engine/Texture.h"
#include "PakArchive.h"
#include "RLEShape.h"
#include "RSFont.h"

// Chunk LINE : premier octet = type ; les champs suivants dépendent du type.
struct RSCreditsLine {
    uint8_t type{ 0 };          // 0 image, 1 texte réglé dans le chunk, 2 à 7 texte réglé par INFO, 8 fin
    uint16_t gap{ 0 };          // types 0, 1 et 8 : écart avec la ligne précédente (pixels)
    uint16_t height{ 0 };       // types 0 et 1 : hauteur de la ligne (pixels)
    uint16_t x{ 0 };            // types 0 et 1
    uint8_t justify{ 0 };       // type 1 : 0 centré sur x, 1 aligné à droite sur x, 2 aligné à gauche sur x
    uint8_t font{ 0 };          // type 1 : numéro de police du FORM FONT
    std::string text;           // types 1 à 7
    RLEShape *image{ nullptr }; // type 0
};

// OPTEST.EXE Credits_Play_BDE10 : MIDGAMES\CREDITS.IFF, FORM CRED.
class RSCredits {
    void parseCRED(uint8_t *data, size_t size);
    void parseCRED_INFO(uint8_t *data, size_t size);
    void parseCRED_PALT(uint8_t *data, size_t size);
    void parseCRED_PALT_INFO(uint8_t *data, size_t size);
    void parseCRED_PALT_DATA(uint8_t *data, size_t size);
    void parseCRED_FONT(uint8_t *data, size_t size);
    void parseCRED_FONT_INFO(uint8_t *data, size_t size);
    void parseCRED_FONT_DATA(uint8_t *data, size_t size);
    void parseCRED_LINE(uint8_t *data, size_t size);

public:
    // Chunk INFO, 10 uint16 puis 3 uint8 ; valeurs du jeu quand le chunk manque. « right » = types 2, 5 et 6 (alignés à
    // droite), « left » = types 3 et 4 (alignés à gauche), « title » = type 7 (centré).
    uint16_t frames_per_second{ 10 };
    uint16_t right_height{ 8 };
    uint16_t left_height{ 8 };
    uint16_t title_height{ 8 };
    uint16_t role_gap{ 10 };    // type 2
    uint16_t name_gap{ 10 };    // types 3 et 5
    uint16_t section_gap{ 15 }; // type 6
    uint16_t title_gap{ 10 };   // type 7
    uint16_t right_x{ 150 };
    uint16_t left_x{ 170 };
    uint8_t right_font{ 0 };
    uint8_t left_font{ 0 };
    uint8_t title_font{ 0 };

    uint8_t palette_count{ 0 };
    uint8_t font_count{ 0 };
    VGAPalette palette;
    std::vector<uint8_t> palette_rgb; // 768 octets, composantes de 0 à 63 comme dans le fichier
    std::vector<RSFont *> fonts;
    std::vector<RSCreditsLine> lines;

    RSCredits();
    ~RSCredits();
    void InitFromRam(uint8_t *data, size_t size);
};
