#include "TOGameflow.h"
#include "TOConvPlayer.h"
#include "TOF22LoadoutScene.h"
#include "TOKillMarksScene.h"
#include "TOScene.h"
#include "TOKillBoardScene.h"

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
    this->instructions[EFECT_OPT_IF_LOADED_FLAG] = std::bind(&TOGameFlow::opIfResult7, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_RESTORE_PILOT] = std::bind(&TOGameFlow::opRestorePilot, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_SHOW_KILL_MARKS_ALT] = std::bind(&TOGameFlow::opShowKillMarksAlt, this, std::placeholders::_1);
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

// Gameflow_RunScriptPairs_63B9, IF_MISSION_SUCCESS : cmp byte ptr [si+0F9h], 2 / cmp byte ptr [si+156h], 1 /
// mov byte ptr [si+3D7h], 1.
void TOGameFlow::opIfMissSuccess(EFCT *instruction) {
    if (this->scriptExecuting() && GameState.mission_report.exit_code == 2 && GameState.requierd_flags[88]) {
        GameState.mission_report.results[6] = 1;
        printf("GAMEFLOW: IF_MISS_SUCCESS, code de sortie 2 et drapeau 88 : 7e valeur de STAT a 1\n");
    }
    SCGameFlow::opIfMissSuccess(instruction);
}

void TOGameFlow::opIfResult7(EFCT *instruction) {
    this->openTest(GameState.mission_report.results[6] != 0, "IF_RESULT_7");
}

void TOGameFlow::opRestorePilot(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    printf("GAMEFLOW: RESTORE_PILOT %d\n", instruction->value);
    if (instruction->value < 8) {
        GameState.pilot_roaster[instruction->value] = 1;
    }
}

void TOGameFlow::opShowKillMarksAlt(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    MissionReport &report = GameState.mission_report;
    int kills = report.kills[PilotsId::PLAYER][KillBoardType::AIR_KILL] + report.kills[PilotsId::PLAYER][KillBoardType::GROUND_KILL];
    printf("GAMEFLOW: SHOW_KILL_MARKS_ALT, %d victoires au dernier vol, mort %d\n", kills, GameState.player_dead);
    if (kills > 0 && !GameState.player_dead) {
        TOKillMarksScene *end = new TOKillMarksScene(&this->optShps2, &this->optPals2);
        end->init();
        this->pending_activities.push(end);
    }
}

// Copie de SCGameFlow::loadGame : fichier de Tactical Operations (SCState::LoadTO).
void TOGameFlow::loadGame(std::string filename) {
    GameState.LoadTO(filename);
    this->frequest->requested_file = "";
    this->efect = nullptr;
    this->current_miss = GameState.current_mission;
    this->next_miss = GameState.next_mission;
    int i = 0;
    for (auto sc : this->gameFlowParser.game.game[this->current_miss]->scen) {
        if (sc->info.ID == GameState.current_scene) {
            this->current_scen = i;
            break;
        }
        i++;
    }
    if (this->gameFlowParser.game.game[this->current_miss]->efct != nullptr) {
        this->efect = this->gameFlowParser.game.game[this->current_miss]->efct;
    }
    this->runEffectAfterLoad();
    this->efect = nullptr;
    this->loadMiss();
}

// Copie de SCGameFlow::saveGame : fichier de Tactical Operations (SCState::SaveTO).
void TOGameFlow::saveGame(std::string filename) {
    if (filename.empty()) {
        this->frequest->requested_file = "";
        return;
    }
    // Der Lade-Dialog listet nur .sav/.SAV: Endung ergaenzen, falls sie fehlt
    if (filename.size() < 4 || (filename.substr(filename.size() - 4) != ".SAV" && filename.substr(filename.size() - 4) != ".sav")) {
        filename += ".SAV";
    }
    GameState.current_mission = this->current_miss;
    GameState.next_mission = this->next_miss;
    GameState.current_scene = this->gameFlowParser.game.game[this->current_miss]->scen[this->current_scen]->info.ID;

    GameState.SaveTO(filename);
    this->frequest->requested_file = "";
}

// Copie de SCGameFlow::createScen : écran de Tactical Operations (TOScene).
void TOGameFlow::createScen() {
    if (this->zones != nullptr) {
        this->zones->clear();
    }

    if (this->gameFlowParser.game.game[this->current_miss]->scen.size() > 0) {
        uint8_t optionScenID = this->gameFlowParser.game.game[this->current_miss]->scen[this->current_scen]->info.ID;
        if (this->scen != nullptr) {
            // delete this->scen;
        }
        this->scen = new TOScene(&this->optShps, &this->optPals, &this->optShps2, &this->optPals2);
        this->zones = this->scen->init(
            this->gameFlowParser.game.game[this->current_miss]->scen[this->current_scen],
            this->optionParser.opts[optionScenID],
            std::bind(&TOGameFlow::clicked, this, std::placeholders::_1, std::placeholders::_2));
    }
}

// Copie de SCGameFlow::lookAtKillBoard : indicatifs de Tactical Operations (TOKillBoardScene).
void TOGameFlow::lookAtKillBoard() {
    this->zones->clear();
    if (this->scen != nullptr) {
        delete this->scen;
    }
    this->scen = new TOKillBoardScene(&this->optShps, &this->optPals);
    this->zones = this->scen->init(this->gameFlowParser.game.game[this->current_miss]->scen[this->current_scen], this->optionParser.opts[30], std::bind(&TOGameFlow::returnFromScene, this, std::placeholders::_1, std::placeholders::_2));
}
