#include "../strike_commander/precomp.h"
#include "EnhancedTrainingMenu.h"
#include "EnhancedStrike.h"

void EnhancedTrainingMenu::OnSearchAndDestroy() {
    EnhancedStrike *strike = new EnhancedStrike();
    strike->init();
    strike->player_loadout = this->player_loadout_pending;
    strike->setMission("TEMPLATE.IFF");
    Game->addActivity(strike);
}
