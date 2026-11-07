#include "TOGameflow.h"

TOGameFlow::TOGameFlow() {
}

TOGameFlow::~TOGameFlow() {
}

void TOGameFlow::initInstructions() {
    SCGameFlow::initInstructions();
    this->instructions[EFECT_OPT_ALT_LOADOUT_SCREEN] = std::bind(&TOGameFlow::opWeaponLoadout, this, std::placeholders::_1);
}

void TOGameFlow::playMidGame(uint8_t midGameId) {
    SCAnimationPlayer *midgame = new SCAnimationPlayer();
    midgame->init();
    SCAnimationArchive archive;
    std::vector<MIDGAME_SHOT *> shots;
    std::unordered_map<uint8_t, std::string> midgame_file_map = {
        {1, "./assets/MID_1.IFF"},
        {2, "./assets/TO1MID1.IFF"},
        {3, "./assets/TO1MID2.IFF"},
        {5, "./assets/TO1MID3.IFF"},
        {12, "./assets/MID_12.IFF"},
        {14, "./assets/MID_14.IFF"},
        {15, "./assets/MID_15.IFF"},
        {16, "./assets/MID_16.IFF"},
        {17, "./assets/MID_17.IFF"},
        {20, "./assets/MID_20.IFF"},
        {33, "./assets/MID_33.IFF"},
        {34, "./assets/MID_4.IFF"},
        {36, "./assets/MID_36.IFF"},
    };
    if (midgame_file_map.find(midGameId) != midgame_file_map.end()) {
        std::string filepath = midgame_file_map[midGameId];
        if (archive.LoadFromFile(filepath.c_str(), shots)) {
            midgame->midgames_shots[1] = shots;
            for (auto shot : shots) {
                midgame->resetShot(shot);
            }
            this->pending_activities.push(midgame);
            printf("Animation loaded from %s\n", filepath.c_str());
        } else {
            printf("Failed to load animation from %s\n", filepath.c_str());
        }
    }
}