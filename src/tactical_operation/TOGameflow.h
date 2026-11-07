#pragma once
#include "../strike_commander/precomp.h"

class TOGameFlow : public virtual SCGameFlow {
protected:
    // Images et palettes du second écran d'armement (sub_870B avec 1 : OPTSHPS2.PAK, OPTPALS2.PAK).
    PakArchive optShps2;
    PakArchive optPals2;

public:
    TOGameFlow();
    ~TOGameFlow();
    void initInstructions() override;
    void playMidGame(uint8_t midGameId) override;
    void playConv(uint8_t convId) override;
    void opWeaponLoadout(EFCT *instruction) override;
    // WEAPON_LOADOUT_ALT (43) : écran d'armement du F-22, test comme WEAPON_LOADOUT.
    virtual void opWeaponLoadoutAlt(EFCT *instruction);
};
