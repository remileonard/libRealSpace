#pragma once
#include "../realspace/RSArea.h"

// Terrain de Pacific Strike : même PAK que Strike Commander, sauf les régions de pistes
// (TRI_ParseOverlayGeometry_A7710 de PACIFIC.EXE, strike_commander_re/analysis/pacific/README.md § 7).
class PSArea : public RSArea {
protected:
    void ParseTriFile(PakEntry *entry) override;
};
