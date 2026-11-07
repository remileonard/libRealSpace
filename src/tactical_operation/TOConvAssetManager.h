#pragma once
//
//  TOConvAssetManager.h
//  libRealSpace
//
//  Images des conversations de Tactical Operations : CONVSHPS.PAK de Strike Commander, complété par CONVSHP2.PAK pour
//  les images de numéro supérieur à 92 (strike_commander_re/analysis/topt/README.md § 7).
//

#include "../realspace/ConvAssetManager.h"

class TOConvAssetManager : public ConvAssetManager {
protected:
    void BuildDB(void) override;
    PakEntry *convShape(uint8_t id) override;

private:
    static const uint8_t LAST_SC_SHAPE = 92;
    static const uint8_t ARI_IMAGE = 95;
    PakArchive convShps2;
};
