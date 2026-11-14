#pragma once
#include "../strike_commander/precomp.h"

// GAMEFLOW\PTR.IFF (FORM CPTR) : points de la main du commandant à la carte, une paire x, y (mots) par image, en
// coordonnées de l'image hors écran (PointerLayer_LoadHandPoints_62F7E, strike_commander_re/analysis/pacific/README.md § 4).
class PSConvCommanderPointer {
private:
    void parseCPTR(uint8_t *data, size_t size);
    void parseCPTR_DATA(uint8_t *data, size_t size);

public:
    std::vector<Point2D> hand_points;
    void InitFromRam(uint8_t *data, size_t size);
};
