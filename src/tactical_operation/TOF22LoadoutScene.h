#pragma once
//
//  TOF22LoadoutScene.h
//  libRealSpace
//
//  Écran d'armement du F-22 de Tactical Operations (commande 43 WEAPON_LOADOUT_ALT, Gameflow_WeaponLoadoutAlt_58DEB) :
//  écran 32 d'OPTIONS2.IFF, images d'OPTSHPS2.PAK et OPTPALS2.PAK (strike_commander_re/analysis/topt/README.md).
//  Deux points d'emport : les AIM-9 sur le point 0, les AIM-120 sur le point 1, une arme par côté.
//

#include "../strike_commander/precomp.h"

class TOF22LoadoutScene : public WeaponLoadoutScene {
public:
    static constexpr uint8_t OPTIONS_SCREEN = 32;

    TOF22LoadoutScene(PakArchive *optShps, PakArchive *optPals);
    std::vector<SCZone *> *init(GAMEFLOW_SCEN *gf, SCEN *sc_opts, std::function<void(bool fly)> onexit);

protected:
    void placeWeapon(uint8_t id) override;
    void loadoutRange(uint8_t weapon, int &first_hp, int &last_hp, int &max_per_side) override;
    RSImageSet *getShape(uint8_t shpid);
    void updateWeaponDisplay() override;

private:
    // SCZone::draw dessine l'image 0 : l'image vide mise en 0 par InitFromPakEntry est retirée.
    void dropEmptyFirstImage(RSImageSet *img);
    void addSpriteZone(uint8_t sprite, std::function<void(std::vector<EFCT *> *, uint8_t)> action);
};
