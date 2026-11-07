#include "TOGameflow.h"
#include "TOConvPlayer.h"
#include "TOF22LoadoutScene.h"

TOGameFlow::TOGameFlow() {
    TreEntry *shapes = Assets.GetEntryByName(Assets.gameflow_root_path + "OPTSHPS2.PAK");
    this->optShps2.InitFromRAM("OPTSHPS2.PAK", shapes->data, shapes->size);
    TreEntry *palettes = Assets.GetEntryByName(Assets.gameflow_root_path + "OPTPALS2.PAK");
    this->optPals2.InitFromRAM("OPTPALS2.PAK", palettes->data, palettes->size);
    this->initInstructions();
}

TOGameFlow::~TOGameFlow() {
}

void TOGameFlow::initInstructions() {
    SCGameFlow::initInstructions();
    this->instructions[EFECT_OPT_ALT_LOADOUT_SCREEN] = std::bind(&TOGameFlow::opWeaponLoadoutAlt, this, std::placeholders::_1);
}
void TOGameFlow::opWeaponLoadout(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        this->openTest(false, "WEAPON_LOADOUT");
        return;
    }
    this->weapon_loadout_id = instruction->value;
    this->playEndMission();
}
void TOGameFlow::playConv(uint8_t convId) {
    TOConvPlayer *conv = new TOConvPlayer();
    conv->init();
    conv->SetID(convId);
    this->pending_activities.push(conv);
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

void TOGameFlow::opWeaponLoadoutAlt(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        this->openTest(false, "WEAPON_LOADOUT_ALT");
        return;
    }
    this->weapon_loadout_id = instruction->value;
    printf("GAMEFLOW: WEAPON_LOADOUT_ALT %d, ecran d'armement du F-22 pour %s\n", this->weapon_loadout_id, this->missionToFly != nullptr ? this->missionToFly : "(aucune mission)");
    if (this->scen != nullptr) {
        delete this->scen;
        this->scen = nullptr;
        this->zones = nullptr;
    }
    this->next_miss = 0;
    if (this->gameFlowParser.game.game[this->current_miss]->scen.size() == 0) {
        this->openTest(true, "WEAPON_LOADOUT_ALT");
        this->flyMission(0);
        return;
    }
    TOF22LoadoutScene *loadout = new TOF22LoadoutScene(&this->optShps2, &this->optPals2);
    this->scen = loadout;
    this->zones = loadout->init(this->gameFlowParser.game.game[this->current_miss]->scen[this->current_scen], this->optionParser.opts[TOF22LoadoutScene::OPTIONS_SCREEN], std::bind(&TOGameFlow::loadoutExit, this, std::placeholders::_1));
    if (this->zones != nullptr) {
        loadout->applyDefaultLoadout(this->gameFlowParser.game.load, this->weapon_loadout_id);
    }
    this->script_waiting = true;
}
