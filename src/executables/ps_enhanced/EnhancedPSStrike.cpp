#include "EnhancedPSStrike.h"

void EnhancedPSStrike::init() {
    PSStrike::init();
    this->orbitInit();
    this->engineVolume = (std::min)(128, Config::getInstance().getInt("Sound", "engine_volume", 100));
    if (this->engineVolume >= 0) {
        this->engine_channel_volume = this->engineVolume;
        printf("[enhanced] engine sound volume: %d of 128\n", this->engineVolume);
    }
}

void EnhancedPSStrike::afterInput() {
    this->orbitAfterInput(false);
}

void EnhancedPSStrike::afterCameraPlaced() {
    this->orbitAfterCameraPlaced();
}

void EnhancedPSGameFlow::flyMission(uint8_t missionid) {
    EnhancedPSStrike *fly = new EnhancedPSStrike();
    fly->init();
    fly->player_loadout = this->player_loadout_pending;
    this->player_loadout_pending = false;
    fly->setMission(this->missionToFly);
    this->missionToFly = nullptr;
    fly_mission.push(fly);
    this->next_miss = GameState.mission_id;
}
