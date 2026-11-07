#pragma once
//
//  TOConvPlayer.h
//  libRealSpace
//
//  Conversations de Tactical Operations : celles de Strike Commander, avec un 6e pilote (Ari) et deux autres écrans de
//  choix de l'équipier (strike_commander_re/analysis/topt/README.md § 7).
//

#include "../strike_commander/precomp.h"

class TOConvPlayer : public SCConvPlayer {
public:
    enum TOConvCommand : uint8_t {
        TO_CHOOSE_WINGMAN_NO_LYLE = 16,
        TO_CHOOSE_WINGMAN_ORIGINAL = 17
    };
    enum TOPilot : uint8_t {
        TO_PILOT_LYLE = 3,
        TO_PILOT_ARI = 6
    };
    // Figure d'Ari : élément 6 de p_wing, image 95 (CONVSHP2.PAK), construite par TOConvAssetManager.
    static const std::string ARI_FIGURE;

    TOConvPlayer();
    ~TOConvPlayer();

protected:
    std::vector<uint8_t> wingmanCandidates(uint8_t command) override;
    ConvFrame *buildOtherCommand(const ConvRecord &record) override;
    const std::vector<std::string> &pilotNames(void) override;
    const std::vector<std::string> &pilotFigures(void) override;
    uint8_t pilotSlot(uint8_t pilot) override;
};
