#pragma once
#include "../strike_commander/precomp.h"

class TOGameFlow : public virtual SCGameFlow {
protected:
public:
    TOGameFlow();
    ~TOGameFlow();
    void initInstructions() override;
    void playMidGame(uint8_t midGameId) override;
    void playConv(uint8_t convId) override;
    void opWeaponLoadout(EFCT *instruction) override;
};
