#include "../strike_commander/precomp.h"
#include "EnhancedGameFlow.h"
#include "EnhancedStrike.h"

void EnhancedGameFlow::flyMission(uint8_t missionid) {
    EnhancedStrike *fly = new EnhancedStrike();
    fly->init();
    fly->setMission(this->missionToFly);
    this->missionToFly = nullptr;
    fly_mission.push(fly);
    this->next_miss = GameState.mission_id;
}
