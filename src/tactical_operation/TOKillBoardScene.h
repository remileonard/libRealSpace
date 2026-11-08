#pragma once
//
//  TOKillBoardScene.h
//  libRealSpace
//
//  Tableau des victoires de Tactical Operations (Gameflow_KillBoard_5779B) : celui de Strike Commander, avec les
//  indicatifs de TOPTTST1.EXE (table de 8 pointeurs en DS:08B0h, pilote 6 TEFLON au lieu de VIXEN).
//

#include "../strike_commander/precomp.h"

class TOKillBoardScene : public KillBoardScene {
private:
    static bool moreKills(const std::pair<uint8_t, std::unordered_map<uint8_t, int16_t>> &a, const std::pair<uint8_t, std::unordered_map<uint8_t, int16_t>> &b);
public:
    TOKillBoardScene(PakArchive *optShps, PakArchive *optPals) : KillBoardScene(optShps, optPals) {};
    void Render() override;
    std::vector<SCZone *> * init(GAMEFLOW_SCEN *gf, SCEN *sc_opts, std::function<void(std::vector<EFCT *> *script, uint8_t id)> onclick) override;
};
