#pragma once
//
//  TOScene.h
//  libRealSpace
//
//  Écran du déroulement de Tactical Operations : celui de Strike Commander (SCScene), avec les images et palettes
//  d'OPTSHPS2.PAK et OPTPALS2.PAK selon les règles d'OptionScreen_Load_870B (TOPTTST1.EXE).
//

#include "../strike_commander/precomp.h"

class TOScene : public SCScene {
public:
    TOScene(PakArchive *optShps, PakArchive *optPals, PakArchive *optShps2, PakArchive *optPals2);
    std::vector<SCZone *> *init(GAMEFLOW_SCEN *gf, SCEN *sc_opts, std::function<void(std::vector<EFCT *> *script, uint8_t id)> onclick) override;

private:
    static constexpr uint8_t LAST_SC_SCREEN = 136;
    static constexpr uint16_t LAST_SC_PALETTE = 48;
    PakArchive *optShps2{ nullptr };
    PakArchive *optPals2{ nullptr };

    RSImageSet *screenShape(uint8_t shpid, bool second);
    uint8_t *screenPalette(uint16_t id);
};
