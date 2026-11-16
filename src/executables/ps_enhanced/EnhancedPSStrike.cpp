#include "EnhancedPSStrike.h"

void EnhancedPSStrike::init() {
    PSStrike::init();
    this->orbitInit();
    this->engineVolume = (std::min)(128, Config::getInstance().getInt("Sound", "engine_volume", 100));
    if (this->engineVolume >= 0) {
        printf("[enhanced] engine sound volume: %d of 128\n", this->engineVolume);
    }
}

void EnhancedPSStrike::afterInput() {
    if (this->engineVolume >= 0) {
        // SCStrike::runFrame() sets the engine channel (5) to volume 5 of 128 every frame
        Mixer.setVolume(this->engineVolume, 5);
    }
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
