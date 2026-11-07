//
//  SCGameFlow.cpp
//  libRealSpace
//
//  Created by Rémi LEONARD on 16/08/2024.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//

#include "precomp.h"

#include <imgui.h>
#include <imgui_impl_opengl2.h>
#include <imgui_impl_sdl2.h>
#include "SCGameFlow.h"


#define GAMOVER_NO_INSURANCE 254
#define GAMEOVER_NO_MONEY 255
#define LOST_PLAYER_PLANE 252
#define LOST_WINGMAN_PLANE 253

/**
 * SCGameFlow constructor.
 *
 * Initializes the SCGameFlow object with default values.
 *
 * @return None
 *
 * @throws None
 */
SCGameFlow::SCGameFlow() {
    this->current_miss = 0;
    this->current_scen = 0;
    this->efect = nullptr;
    this->currentOptCode = 0;
    this->fps = SDL_GetTicks() / 10;
    this->zones = new std::vector<SCZone *>();
    this->frequest = nullptr;
    if (Game == nullptr) {
        this->Game = &GameEngine::getInstance();
    }
    this->m_keyboard = Game->getKeyboard();
    this->initInstructions();
}

SCGameFlow::~SCGameFlow() {}

/**
 * Handles a click event on a sprite.
 *
 * @param id The ID of the sprite that was clicked.
 *
 * @return None
 */
void SCGameFlow::clicked(std::vector<EFCT *> *script, uint8_t id) {
    this->efect = script;
    this->currentSpriteId = id;
    this->currentOptCode = 0;
    VGA.fadeOut(30, 6);
    if (this->scen->sceneOpts->foreground->sprites[id]->tune != nullptr) {        
        Mixer.switchBank(1);
        Mixer.playMusic(this->scen->sceneOpts->foreground->sprites[id]->tune->ID, 1);
        
    }
    this->runEffect();
}
/**
 * Return from a scene and go back to the scenario.
 *
 * @param script The script of the scene.
 * @param id The ID of the scene.
 *
 * @return None
 *
 * @throws None
 */

void SCGameFlow::loadGame(std::string filename) {
    GameState.Load(filename);
    this->frequest->requested_file = "";
    this->efect = nullptr;
    this->current_miss = GameState.current_mission;
    this->next_miss = GameState.next_mission;
    int i=0;
    for (auto sc: this->gameFlowParser.game.game[this->current_miss]->scen) {
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

void SCGameFlow::saveGame(std::string filename) {
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

    GameState.Save(filename);
    this->frequest->requested_file = "";
}


void SCGameFlow::returnFromScene(std::vector<EFCT *> *script, uint8_t id) { this->createScen(); }
// Sortie de l'écran d'armement : le test WEAPON_LOADOUT réussit si le joueur part en vol, et le script reprend. Le vol
// part quand le script est fini (runFrame) ; au retour, la scène du déroulement est rétablie.
void SCGameFlow::loadoutExit(bool fly) {
    this->script_waiting = false;
    this->openTest(fly, "WEAPON_LOADOUT");
    if (fly) {
        // Départ de l'écran d'armement : OPTEST écrit le chunk PLYR ([partie+238] = 1).
        this->player_loadout_pending = true;
        this->flyMission(0);
    } else {
        this->createScen();
    }
}
void SCGameFlow::flyMission(uint8_t missionid) {
    SCStrike *fly = new SCStrike();
    fly->init();
    fly->player_loadout = this->player_loadout_pending;
    this->player_loadout_pending = false;
    fly->setMission(this->missionToFly);
    this->missionToFly = nullptr;
    fly_mission.push(fly);
    this->next_miss = GameState.mission_id;
}
void SCGameFlow::checkKeyboard(void) {
    IActivity::checkKeyboard();
}
SCZone *SCGameFlow::CheckZones(void) {
    static uint8_t color = 0;
    RSFont *fnt = FontManager.GetFont("..\\..\\DATA\\FONTS\\OPTFONT.SHP");
    for (auto zone : *this->zones) {
        if (zone->active) {
            if (zone->quad != nullptr) {
                if (isPointInQuad(Mouse.getPosition(), zone->quad)) {
                    Mouse.setMode(SCMouse::VISOR);

                    // If the mouse button has just been released: trigger action.
                    if (Mouse.buttons[MouseButton::LEFT].event == MouseButton::RELEASED) {
                        Mouse.buttons[MouseButton::LEFT].event = MouseButton::NONE;
                        zone->onAction();
                    }
                    Point2D p = {160 - static_cast<int32_t>(zone->label->length() / 2) * 8, 180};
                    VGA.getFrameBuffer()->printText(fnt, &p, (char *)zone->label->c_str(), color, 0,
                                                    static_cast<uint32_t>(zone->label->length()), 3, 5);
                    return zone;
                }
            }
            if (Mouse.getPosition().x > zone->position.x &&
                Mouse.getPosition().x < zone->position.x + zone->dimension.x &&
                Mouse.getPosition().y > zone->position.y &&
                Mouse.getPosition().y < zone->position.y + zone->dimension.y) {
                // HIT !
                Mouse.setMode(SCMouse::VISOR);

                if (Mouse.buttons[MouseButton::LEFT].event == MouseButton::RELEASED) {
                    Mouse.buttons[MouseButton::LEFT].event = MouseButton::NONE;
                    zone->onAction();
                }
                Point2D p = {160 - ((int32_t)(zone->label->length() / 2) * 8), 180};
                VGA.getFrameBuffer()->printText(fnt, &p, (char *)zone->label->c_str(), color, 0,
                                                static_cast<uint32_t>(zone->label->length()), 3, 5);

                return zone;
            }
        }
    }

    Mouse.setMode(SCMouse::CURSOR);
    return nullptr;
}
void SCGameFlow::playMapShot(std::vector<MAP_POINT *> *points) {
    MapShot *map = new MapShot();
    map->init();
    map->SetPoints(points);
    this->pending_activities.push(map);
}
// Table des instructions du script de Strike Commander : numéro de commande → fonction. Un autre jeu redéfinit cette
// méthode : il appelle celle-ci, puis ajoute, remplace ou retire des instructions.
void SCGameFlow::initInstructions() {
    this->instructions.clear();
    this->instructions[EFECT_OPT_CONV] = std::bind(&SCGameFlow::opConv, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_SCEN] = std::bind(&SCGameFlow::opScen, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_PLAY_MIDGAME] = std::bind(&SCGameFlow::opPlayMidgame, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_FLYM2] = std::bind(&SCGameFlow::opFlym2, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_NOP_4] = std::bind(&SCGameFlow::opNop, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_SELECT_MISSION_ALT] = std::bind(&SCGameFlow::opSelectMissionAlt, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_SETFLAG_TRUE] = std::bind(&SCGameFlow::opSetFlagTrue, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_SETFLAG_FALSE] = std::bind(&SCGameFlow::opSetFlagFalse, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_SHOT] = std::bind(&SCGameFlow::opShot, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_IF_NOT_FLAG] = std::bind(&SCGameFlow::opIfNotFlag, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_IF_FLAG] = std::bind(&SCGameFlow::opIfFlag, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_NOP_11] = std::bind(&SCGameFlow::opNop, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_FLYM] = std::bind(&SCGameFlow::opFlym, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_MIS2] = std::bind(&SCGameFlow::opMis2, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_START_FLIGHT] = std::bind(&SCGameFlow::opStartFlight, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_GO] = std::bind(&SCGameFlow::opGo, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_SAVE_GAME] = std::bind(&SCGameFlow::opSaveGame, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_LOAD_GAME] = std::bind(&SCGameFlow::opLoadGame, this, std::placeholders::_1);
    this->instructions[EFFCT_OPT_SHOW_MAP] = std::bind(&SCGameFlow::opShowMap, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_LOOK_AT_KILLBOARD] = std::bind(&SCGameFlow::opLookAtKillboard, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_MISS_ACCEPTED] = std::bind(&SCGameFlow::opMissAccepted, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_MISS_REJECTED] = std::bind(&SCGameFlow::opMissRejected, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_WEAPON_LOADOUT] = std::bind(&SCGameFlow::opWeaponLoadout, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_VIEW_CATALOG] = std::bind(&SCGameFlow::opViewCatalog, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_SET_MUSIC] = std::bind(&SCGameFlow::opSetMusic, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_LOOK_AT_LEDGER] = std::bind(&SCGameFlow::opLookAtLedger, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_SHOW_KILL_MARKS] = std::bind(&SCGameFlow::opShowKillMarks, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_REMOVE_PILOT] = std::bind(&SCGameFlow::opRemovePilot, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_IF_TRUE] = std::bind(&SCGameFlow::opIfTrue, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_CHECK_BALANCE] = std::bind(&SCGameFlow::opCheckBalance, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_MISS_ELSE] = std::bind(&SCGameFlow::opElse, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_MISS_ENDIF] = std::bind(&SCGameFlow::opEndif, this, std::placeholders::_1);
    this->instructions[EFFCT_OPT_IF_MISS_SUCCESS] = std::bind(&SCGameFlow::opIfMissSuccess, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_OR_GROUP] = std::bind(&SCGameFlow::opOrGroup, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_APPLY_CHANGE] = std::bind(&SCGameFlow::opApplyChange, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_IF_PLANES_LOST] = std::bind(&SCGameFlow::opIfPlanesLost, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_PLANES_LOST_CONV] = std::bind(&SCGameFlow::opPlanesLostConv, this, std::placeholders::_1);
    this->instructions[EFECT_IF_LAST_MISS_SUCCESS] = std::bind(&SCGameFlow::opIfLastMissSuccess, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_TUNE_MODIFIER] = std::bind(&SCGameFlow::opTuneModifier, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_IF_RESULT_4] = std::bind(&SCGameFlow::opIfResult4, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_IF_RESULT_5] = std::bind(&SCGameFlow::opIfResult5, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_IF_RESULT_6] = std::bind(&SCGameFlow::opIfResult6, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_IF_LOADED_FLAG] = std::bind(&SCGameFlow::opIfLoadedFlag, this, std::placeholders::_1);
}

/**
 * Runs the current effect based on the efect vector and currentOptCode.
 *
 * Chaque instruction est passée à sa fonction (table instructions) ; l'état des tests est gardé dans SCGameFlow, et
 * chaque fonction en tient compte. Un script suspendu par une activité reprend à l'instruction suivante.
 */
void SCGameFlow::runEffect() {
    if (this->efect == nullptr) {
        return;
    }
    if (this->currentOptCode == 0) {
        for (int depth = 0; depth < MAX_IF_DEPTH; depth++) {
            this->script_execute[depth] = false;
            this->script_else_seen[depth] = false;
        }
        this->script_execute[0] = true;
        this->script_if_depth = 0;
        this->script_waiting = false;
    }
    for (size_t i = this->currentOptCode; i < this->efect->size(); i++) {
        EFCT *instruction = this->efect->at(i);
        auto found = this->instructions.find(instruction->opcode);
        if (found == this->instructions.end()) {
            printf("Unkown opcode :%d, %d\n", instruction->opcode, instruction->value);
        } else {
            found->second(instruction);
        }
        // Une commande jouée (plan, carte, cinématique, conversation, écran d'armement) suspend le script jusqu'à sa fin.
        if (!this->pending_activities.empty() || this->script_waiting) {
            this->currentOptCode = (uint8_t)(i + 1);
            printf("GAMEFLOW: script suspendu avant l'instruction %zu\n", i + 1);
            return;
        }
    }
    this->efect = nullptr;
    this->currentOptCode = 0;
}

// Vrai si tous les tests ouverts ont réussi : les commandes sont jouées, sinon sautées.
bool SCGameFlow::scriptExecuting() {
    for (int depth = 0; depth <= this->script_if_depth; depth++) {
        if (!this->script_execute[depth]) {
            return false;
        }
    }
    return true;
}

// Ouvre un test du script : le bloc qui suit est joué si result est vrai (GAMEFLOW_SYSTEM.md § 5.1). Dans un bloc
// sauté, le test ouvre quand même un niveau, refermé par son ENDIF.
void SCGameFlow::openTest(bool result, const char *name) {
    bool executing = this->scriptExecuting();
    if (executing) {
        printf("GAMEFLOW: %s -> %d\n", name, result);
    }
    this->script_if_depth++;
    if (this->script_if_depth < MAX_IF_DEPTH) {
        this->script_execute[this->script_if_depth] = executing && result;
        this->script_else_seen[this->script_if_depth] = false;
    }
}

void SCGameFlow::opElse(EFCT *instruction) {
    if (this->script_if_depth >= 0 && !this->script_else_seen[this->script_if_depth]) {
        this->script_execute[this->script_if_depth] = !this->script_execute[this->script_if_depth];
        this->script_else_seen[this->script_if_depth] = true;
    }
}

void SCGameFlow::opEndif(EFCT *instruction) {
    if (this->script_if_depth > 0) {
        this->script_if_depth--;
    }
}

void SCGameFlow::opIfTrue(EFCT *instruction) {
    this->openTest(true, "IF_TRUE");
}

void SCGameFlow::opIfPlanesLost(EFCT *instruction) {
    this->openTest(GameState.mission_report.planes_lost != 0, "IF_PLANES_LOST");
}

void SCGameFlow::opIfResult4(EFCT *instruction) {
    this->openTest(GameState.mission_report.results[3] != 0, "IF_RESULT_4");
}

void SCGameFlow::opIfResult5(EFCT *instruction) {
    this->openTest(GameState.mission_report.results[4] != 0, "IF_RESULT_5");
}

void SCGameFlow::opIfResult6(EFCT *instruction) {
    this->openTest(GameState.mission_report.results[5] != 0, "IF_RESULT_6");
}

// Octet posé par le chargement d'une partie dans la version CD, rôle non établi : test considéré faux.
void SCGameFlow::opIfLoadedFlag(EFCT *instruction) {
    this->openTest(false, "IF_LOADED_FLAG");
}

void SCGameFlow::opMissAccepted(EFCT *instruction) {
    this->openTest(GameState.mission_accepted == true, "MISS_ACCEPTED");
}

void SCGameFlow::opMissRejected(EFCT *instruction) {
    this->openTest(GameState.mission_accepted == false, "MISS_REJECTED");
}

void SCGameFlow::opIfFlag(EFCT *instruction) {
    this->openTest(GameState.requierd_flags[instruction->value] == true, "IF_FLAG");
}

void SCGameFlow::opIfNotFlag(EFCT *instruction) {
    this->openTest(GameState.requierd_flags[instruction->value] == false, "IF_NOT_FLAG");
}

void SCGameFlow::opIfMissSuccess(EFCT *instruction) {
    this->openTest(GameState.mission_flyed_success[instruction->value], "IF_MISS_SUCCESS");
}

void SCGameFlow::opIfLastMissSuccess(EFCT *instruction) {
    this->openTest(GameState.mission_flyed_success[GameState.mission_flyed], "IF_LAST_MISS_SUCCESS");
}

void SCGameFlow::opShowMap(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    if (instruction->value < this->gameFlowParser.game.wrld.size()) {
        this->playMapShot(&this->gameFlowParser.game.wrld[instruction->value]->data->points);
    }
}

void SCGameFlow::opConv(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    this->playConv(instruction->value);
}

// SHOW_KILL_MARKS : marques de victoires si le joueur en a eu pendant le dernier vol et n'est pas mort.
void SCGameFlow::opShowKillMarks(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    MissionReport &report = GameState.mission_report;
    int kills = report.kills[PilotsId::PLAYER][KillBoardType::AIR_KILL] +
                report.kills[PilotsId::PLAYER][KillBoardType::GROUND_KILL];
    printf("GAMEFLOW: SHOW_KILL_MARKS, %d victoires au dernier vol, mort %d\n", kills, GameState.player_dead);
    if (kills > 0 && !GameState.player_dead) {
        EndMissionScene *end = new EndMissionScene(&this->optShps, &this->optPals);
        end->init();
        this->pending_activities.push(end);
    }
}

void SCGameFlow::opSetMusic(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    this->setMusic(instruction->value);
}

void SCGameFlow::opNop(EFCT *instruction) {
}

void SCGameFlow::opSelectMissionAlt(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    printf("GAMEFLOW: SELECT_MISSION_ALT %d\n", instruction->value);
    this->playFlyMission(instruction->value);
}

void SCGameFlow::opStartFlight(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    printf("GAMEFLOW: START_FLIGHT, mission %s\n", this->missionToFly != nullptr ? this->missionToFly : "(aucune)");
    if (this->missionToFly != nullptr) {
        this->flyMission(0);
    }
}

void SCGameFlow::opRemovePilot(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    printf("GAMEFLOW: REMOVE_PILOT %d\n", instruction->value);
    if (instruction->value < 8) {
        GameState.pilot_roaster[instruction->value] = 0;
    }
}

// Absent des données ; l'effet exact n'est pas un « ou » (GAMEFLOW_SYSTEM.md § 5.1) : pas porté.
void SCGameFlow::opOrGroup(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    printf("GAMEFLOW: OR_GROUP %d ignore\n", instruction->value);
}

// PLANES_LOST_CONV : avion perdu au dernier vol, joueur éjecté (mort : cinématique 33 au retour).
void SCGameFlow::opPlanesLostConv(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    printf("GAMEFLOW: PLANES_LOST_CONV, %d avions perdus au dernier vol, mort %d\n",
           GameState.mission_report.planes_lost, GameState.player_dead);
    if (GameState.mission_report.planes_lost > 0 && !GameState.player_dead) {
        this->setMusic(8);
        this->playConv(LOST_PLAYER_PLANE);
    }
}

void SCGameFlow::opScen(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    for (int j = 0; j < this->gameFlowParser.game.game[this->current_miss]->scen.size(); j++) {
        if (this->gameFlowParser.game.game[this->current_miss]->scen.at(j)->info.ID == instruction->value) {
            this->current_scen = j;
            this->currentSpriteId = 0;
            this->createScen();
        }
    }
}

// WEAPON_LOADOUT : écran d'armement, et test ouvert à sa sortie (loadoutExit) ; dans un bloc sauté, test ouvert à faux.
void SCGameFlow::opWeaponLoadout(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        this->openTest(false, "WEAPON_LOADOUT");
        return;
    }
    this->weapon_loadout_id = instruction->value;
    this->playEndMission();
}

void SCGameFlow::opMis2(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    GameState.mission_id = instruction->value;
}

void SCGameFlow::opShot(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    this->playShot(instruction->value);
}

// SELECT_MISSION : choix de la mission seulement.
void SCGameFlow::opFlym(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    this->playFlyMission(instruction->value);
}

// FLY_MISSION_NOW : choix de la mission et départ en vol, sans écran d'armement.
void SCGameFlow::opFlym2(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    this->playFlyMission2(instruction->value);
}

void SCGameFlow::opSetFlagTrue(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    GameState.requierd_flags[instruction->value] = true;
}

void SCGameFlow::opSetFlagFalse(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    GameState.requierd_flags[instruction->value] = false;
}

void SCGameFlow::opLoadGame(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    if (instruction->value != 0) {
        return;
    }
    if (this->frequest == nullptr) {
        uint8_t soffset = 0;
        if (this->gameFlowParser.game.game[this->current_miss]->scen[this->current_scen]->info.ID == 29) {
            soffset = 21;
        } else {
            soffset = 0;
        }
        this->frequest = new SCFileRequester(std::bind(&SCGameFlow::loadGame, this, std::placeholders::_1), soffset);
    }
    this->frequest->opened = true;
    this->frequest->loadFiles();
}

void SCGameFlow::opSaveGame(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    if (instruction->value != 0) {
        return;
    }
    if (this->frequest == nullptr) {
        uint8_t soffset = 0;
        if (this->gameFlowParser.game.game[this->current_miss]->scen[this->current_scen]->info.ID == 29) {
            soffset = 21;
        } else {
            soffset = 0;
        }
        this->frequest =
            new SCFileRequester(std::bind(&SCGameFlow::saveGame, this, std::placeholders::_1), soffset, true);
    }
    this->frequest->opened = true;
    this->frequest->loadFiles();
}

void SCGameFlow::opLookAtLedger(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    this->lookAtLedger();
}

void SCGameFlow::opLookAtKillboard(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    this->lookAtKillBoard();
}

void SCGameFlow::opPlayMidgame(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    this->playMidGame(instruction->value);
    if (instruction->value == 36) {
        this->playCredits(false);
        this->isGameOver = true;
    }
}

// CHECK_GAME_OVER (Gameflow_CheckLossConversations_50645) : plus de 16 avions perdus, puis argent < 0.
void SCGameFlow::opCheckBalance(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    printf("GAMEFLOW: CHECK_GAME_OVER, avions perdus %d, argent %d\n", GameState.lost_aircraft, GameState.proj_cash);
    // Conversation de fin, puis « THE END » (Credits_Play_BDE10 avec 1).
    if (GameState.lost_aircraft > 16) {
        this->playConv(GAMOVER_NO_INSURANCE);
        this->playCredits(true);
        this->isGameOver = true;
    } else if (GameState.proj_cash < 0) {
        this->playConv(GAMEOVER_NO_MONEY);
        this->playCredits(true);
        this->isGameOver = true;
    }
}

void SCGameFlow::opViewCatalog(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    this->viewCatalog();
}

void SCGameFlow::opGo(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    this->next_miss = GameState.mission_id;
}

void SCGameFlow::opTuneModifier(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    GameState.tune_modifier = instruction->value;
    Mixer.music->SelectSet(1, instruction->value);
}

void SCGameFlow::opApplyChange(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    if (this->gameFlowParser.game.stat[instruction->value] == nullptr) {
        return;
    }
    CHNG *chng = this->gameFlowParser.game.stat[instruction->value];
    if (chng->cash != nullptr) {
        switch (chng->cash->op) {
        case 2:
            GameState.proj_cash = chng->cash->value * 1000;
            break;
        case 1:
            GameState.proj_cash -= chng->cash->value * 1000;
            break;
        case 0:
            GameState.proj_cash += chng->cash->value * 1000;
            break;
        }
        if (GameState.cash == 0) {
            GameState.cash = GameState.proj_cash;
        }
    }
    if (chng->over != nullptr) {
        switch (chng->over->op) {
        case 2:
            GameState.over_head = chng->over->value * 1000;
            break;
        case 1:
            GameState.over_head -= chng->over->value * 1000;
            break;
        case 0:
            GameState.over_head += chng->over->value * 1000;
            break;
        }
    }
    if (chng->weap != nullptr) {
        for (auto weapon : *chng->weap) {
            const std::vector<int> weapon_ids_mapping = {
                ID_AIM9J,
                ID_AIM9M,
                ID_AGM65D,
                ID_LAU3,
                ID_MK20,
                ID_MK82,
                ID_DURANDAL,
                ID_GBU15,
                ID_AIM120
            };
            switch (weapon->op) {
            case 2:
                GameState.weapon_inventory[weapon_ids_mapping[weapon->weap_id]] = weapon->value;
                break;
            case 1:
                GameState.weapon_inventory[weapon_ids_mapping[weapon->weap_id]] -= weapon->value;
                break;
            case 0:
                GameState.weapon_inventory[weapon_ids_mapping[weapon->weap_id]] += weapon->value;
                break;
            }
        }
    }
    if (chng->pilt != nullptr) {
        for (auto pil : *chng->pilt) {
            switch (pil->op) {
            case 2:
                GameState.kill_board[pil->pilot_id][pil->air] = pil->value;
                break;
            case 1:
                GameState.kill_board[pil->pilot_id][pil->air] -= pil->value;
                break;
            case 0:
                GameState.kill_board[pil->pilot_id][pil->air] += pil->value;
                break;
            }
        }
    }
}

/**
 * Runs the current effect based on the efect vector and currentOptCode.
 *
 * Iterates through the efect vector, executing different actions based on the opcode.
 * Actions include playing conversations, scenes, and missions, as well as handling unknown opcodes.
 *
 * @return None
 */
void SCGameFlow::runEffectAfterLoad() {
    for (auto instruction : *this->efect) {
        switch (instruction->opcode) {
        case EFECT_OPT_TUNE_MODIFIER:
            GameState.tune_modifier = instruction->value;
            Mixer.music->SelectSet(1, instruction->value);
            break;
        default:
            break;
        }
    }
}
/**
 * Initializes the SCGameFlow object by loading the necessary IFF files and initializing the optionParser and
 * gameFlowParser. It also initializes the optShps and optPals objects by loading the OPTSHPS.PAK and OPTPALS.PAK files
 * respectively. Finally, it sets the efect pointer to nullptr and calls the createMiss() function.
 *
 * @throws None
 */
void SCGameFlow::init() {

    TreEntry *gameflowIFF = Assets.GetEntryByName(Assets.gameflow_filename);
    TreEntry *optionIFF = Assets.GetEntryByName(Assets.option_filename);
    this->optionParser.InitFromRam(optionIFF->data, optionIFF->size);
    this->gameFlowParser.InitFromRam(gameflowIFF->data, gameflowIFF->size);

    TreEntry *optShapEntry = Assets.GetEntryByName(Assets.optshps_filename);
    this->optShps.InitFromRAM("OPTSHPS.PAK", optShapEntry->data, optShapEntry->size);
    TreEntry *optPalettesEntry = Assets.GetEntryByName(Assets.optpals_filename);
    this->optPals.InitFromRAM("OPTPALS.PAK", optPalettesEntry->data, optPalettesEntry->size);
    this->efect = nullptr;
    this->createMiss();
}

void SCGameFlow::InitFromGameState() {

    TreEntry *gameflowIFF = Assets.GetEntryByName(Assets.gameflow_filename);
    TreEntry *optionIFF = Assets.GetEntryByName(Assets.option_filename);
    this->optionParser.InitFromRam(optionIFF->data, optionIFF->size);
    this->gameFlowParser.InitFromRam(gameflowIFF->data, gameflowIFF->size);

    TreEntry *optShapEntry = Assets.GetEntryByName(Assets.optshps_filename);
    this->optShps.InitFromRAM("OPTSHPS.PAK", optShapEntry->data, optShapEntry->size);
    TreEntry *optPalettesEntry = Assets.GetEntryByName(Assets.optpals_filename);
    this->optPals.InitFromRAM("OPTPALS.PAK", optPalettesEntry->data, optPalettesEntry->size);
    this->efect = nullptr;
    this->current_miss = GameState.current_mission;
    this->next_miss = GameState.next_mission;
    int i=0;
    for (auto sc: this->gameFlowParser.game.game[this->current_miss]->scen) {
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
/**
 * Creates a new miss in the game flow.
 *
 * This function initializes the efect, sprites, and zones for the current miss.
 * It also sets up the layer, raw palette, and foreground palette.
 *
 * @return None
 *
 * @throws None
 */
void SCGameFlow::createMiss() {
    this->missionToFly = nullptr;
    this->next_miss = 0;
    if (this->zones != nullptr) {
        this->zones->clear();
    }
    this->layers.clear();
    this->pending_activities = std::queue<IActivity *>();
    this->fly_mission = std::queue<SCStrike *>();
    this->currentOptCode = 0;
    if (this->gameFlowParser.game.game[this->current_miss]->efct != nullptr) {
        this->efect = this->gameFlowParser.game.game[this->current_miss]->efct;
    }

    this->runEffect();
    this->createScen();
}

void SCGameFlow::loadMiss() {
    this->missionToFly = nullptr;
    this->next_miss = 0;
    if (this->zones != nullptr) {
        this->zones->clear();
    }
    this->layers.clear();
    this->pending_activities = std::queue<IActivity *>();
    this->fly_mission = std::queue<SCStrike *>();
    this->currentOptCode = 0;
    this->createScen();
}

/**
 * Creates a new scene in the current miss.
 *
 * This function initializes the layers, sprites, and zones for the current scene.
 * It also sets up the raw palette and foreground palette.
 *
 * @return None
 *
 * @throws None
 */
void SCGameFlow::createScen() {
    if (this->zones != nullptr) {
        this->zones->clear();
    }
    
    if (this->gameFlowParser.game.game[this->current_miss]->scen.size() > 0) {
        uint8_t optionScenID = this->gameFlowParser.game.game[this->current_miss]->scen[this->current_scen]->info.ID;
        if (this->scen != nullptr) {
            // delete this->scen;
        }
        this->scen = new SCScene(&this->optShps, &this->optPals);
        this->zones = this->scen->init(
            this->gameFlowParser.game.game[this->current_miss]->scen[this->current_scen],
            this->optionParser.opts[optionScenID],
            std::bind(&SCGameFlow::clicked, this, std::placeholders::_1, std::placeholders::_2)
        );
    }
}

/**
 * Retrieves an RSImageSet object for the specified shape ID.
 *
 * @param shpid The ID of the shape to retrieve.
 *
 * @return A pointer to the RSImageSet object containing the shape data.
 *
 * @throws None.
 */
RSImageSet *SCGameFlow::getShape(uint8_t shpid) {
    if (this->shape_cache.find(shpid) != this->shape_cache.end()) {
        return this->shape_cache[shpid];
    }
    PakEntry *shapeEntry = this->optShps.GetEntry(shpid);
    PakArchive subPAK;

    subPAK.InitFromRAM("", shapeEntry->data, shapeEntry->size);
    RSImageSet *img = new RSImageSet();
    if (!subPAK.IsReady()) {
        img->InitFromPakEntry(this->optShps.GetEntry(shpid));
    } else {
        img->InitFromSubPakEntry(&subPAK);
    }
    this->shape_cache[shpid] = img;
    return (img);
}

/**
 * Runs a single frame of the game, updating the game state and rendering the graphics.
 *
 * @param None
 *
 * @return None
 *
 * @throws None
 */
// SET_MUSIC : piste n du jeu de musique du déroulement (banque 1), jouée par le séquenceur avec ses liaisons ; 255 = rien.
void SCGameFlow::setMusic(uint8_t track) {
    printf("GAMEFLOW: SET_MUSIC %d\n", track);
    if (track == 255) {
        return;
    }
    Mixer.switchBank(1);
    Mixer.playMusic(track);
}

// Retour de vol : le compte rendu de SCStrike est appliqué à la partie (Gameflow_ReadMissionResult_4B544 du jeu).
void SCGameFlow::applyMissionReport(void) {
    MissionReport &report = GameState.mission_report;
    GameState.missions_flags.assign(report.results.begin(), report.results.end());
    // WPNS : le stock d'armes est remplacé par le compte rendu (Gameflow_ReadMissionResult_4B544).
    if (report.weapons_reported) {
        for (auto &weapon : report.weapons_left) {
            printf("GAMEFLOW: stock arme %d : %d -> %d\n", weapon.first, GameState.weapon_inventory[weapon.first],
                   weapon.second);
            GameState.weapon_inventory[weapon.first] = weapon.second;
        }
    }
    GameState.mission_flyed_success[GameState.mission_flyed] = report.results[0] > 0;
    GameState.air_kills += report.kills[PilotsId::PLAYER][KillBoardType::AIR_KILL];
    GameState.ground_kills += report.kills[PilotsId::PLAYER][KillBoardType::GROUND_KILL];
    GameState.kill_board[PilotsId::PLAYER][KillBoardType::AIR_KILL] = GameState.air_kills;
    GameState.kill_board[PilotsId::PLAYER][KillBoardType::GROUND_KILL] = GameState.ground_kills;
    if (report.exit_code == 2) {
        GameState.player_dead = true;
    }
    if (report.planes_lost > 0) {
        GameState.player_ejected = true;
        GameState.f16_replacements += 500000 * report.planes_lost;
        GameState.lost_aircraft += report.planes_lost;
    }
    GameState.cash = GameState.proj_cash - GameState.over_head;
    GameState.proj_cash = GameState.cash + report.results[1] * 1000 - GameState.weapons_costs - GameState.f16_replacements;
    GameState.weapons_costs = 0;
    for (auto &pilot : report.kills) {
        if (pilot.first == PilotsId::PLAYER) {
            continue;
        }
        GameState.kill_board[pilot.first][KillBoardType::AIR_KILL] += pilot.second[KillBoardType::AIR_KILL];
        GameState.kill_board[pilot.first][KillBoardType::GROUND_KILL] += pilot.second[KillBoardType::GROUND_KILL];
    }
    printf("GAMEFLOW: compte rendu applique, mission %d %s, succes %d, gain %d milliers, mort %d, ejecte %d, argent %d\n",
           GameState.mission_flyed, report.mission_name.c_str(), report.results[0] > 0, report.results[1],
           GameState.player_dead, GameState.player_ejected, GameState.cash);
    // Joueur mort (code de sortie 2) : cinématique 33 puis fin du déroulement, sans script (runFrame, file pending_activities).
    if (GameState.player_dead) {
        this->playMidGame(33);
        this->playCredits(true, true);
        this->isGameOver = true;
    }
}

void SCGameFlow::runFrame(void) {
    
    if (this->frequest != nullptr && !this->frequest->opened) {
        delete this->frequest;
        this->frequest = nullptr;
    }
    if (this->mission_report_pending) {
        this->mission_report_pending = false;
        this->applyMissionReport();
    }
    if (!this->pending_activities.empty()) {
        IActivity *c = this->pending_activities.front();
        this->pending_activities.pop();
        Game->addActivity(c);
        return;
    } else if (isGameOver) {
        this->stop();
    }
    if (this->currentOptCode > 0 && this->efect != nullptr && !this->script_waiting) {
        this->runEffect();
        return;
    }
    if (this->fly_mission.size() > 0) {
        SCStrike *c = this->fly_mission.front();
        this->fly_mission.pop();
        Game->addActivity(c);
        this->mission_report_pending = true;
        return;
    }
    if (this->next_miss > 0) {
        this->current_miss = this->next_miss;
        this->next_miss = 0;
        this->current_scen = 0;
        this->createMiss();
        return;
    }
    int fpsupdate = 0;
    fpsupdate = (SDL_GetTicks() / 10) - this->fps > 12;
    if (fpsupdate) {
        this->fps = (SDL_GetTicks() / 10);
    }
    if (this->frequest != nullptr && this->frequest->opened) {
        this->frequest->checkevents();
    }
    VGA.activate();
    VGA.getFrameBuffer()->fillWithColor(0);
    
    if (this->scen != nullptr) {
        this->scen->Render();
        if (this->test_shape != nullptr) {
            VGA.getFrameBuffer()->drawShape(this->test_shape);
        }
    }
    if (this->frequest == nullptr || !this->frequest->opened) {
        this->CheckZones();
    }
    if (this->frequest != nullptr && this->frequest->opened) {
        this->frequest->draw(VGA.getFrameBuffer());
    }
    Mouse.draw();
    VGA.vSync();
}
void SCGameFlow::playShot(uint8_t shotId) {
    SCShot *sht = new SCShot();
    sht->init();
    sht->SetShotId(shotId);
    this->pending_activities.push(sht);
}
void SCGameFlow::playConv(uint8_t convId) {
    SCConvPlayer *conv = new SCConvPlayer();
    conv->init();
    conv->SetID(convId);
    this->pending_activities.push(conv);
}

void SCGameFlow::playMidGame(uint8_t midGameId) {
    SCAnimationPlayer *midgame = new SCAnimationPlayer();
    midgame->init();
    SCAnimationArchive archive;
    std::vector<MIDGAME_SHOT *> shots;
    std::unordered_map<uint8_t, std::string> midgame_file_map = {
        {1, "./assets/MID_1.IFF"},
        {2, "./assets/MID_2.IFF"},
        {3, "./assets/MID_3.IFF"},
        {34, "./assets/MID_4.IFF"},
        {5, "./assets/MID_5.IFF"},
        {12, "./assets/MID_12.IFF"},
        {14, "./assets/MID_14.IFF"},
        {15, "./assets/MID_15.IFF"},
        {16, "./assets/MID_16.IFF"},
        {17, "./assets/MID_17.IFF"},
        {20, "./assets/MID_20.IFF"},
        {33, "./assets/MID_33.IFF"},
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

void SCGameFlow::playCredits(bool the_end_only, bool wait_music_end) {
    SCCredit *credits = new SCCredit();
    credits->the_end_only = the_end_only;
    credits->wait_music_end = wait_music_end;
    credits->init();
    this->pending_activities.push(credits);
    printf("GAMEFLOW: générique en file (%s)\n", the_end_only ? "THE END seul" : "déroulant");
}

void SCGameFlow::playShowMap(uint8_t mapId) {}

// Écran d'armement du chargement weapon_loadout_id ; le script attend sa sortie. Sans scène, départ en vol direct.
void SCGameFlow::playEndMission() {
    printf("GAMEFLOW: WEAPON_LOADOUT %d, ecran d'armement pour %s\n", this->weapon_loadout_id, this->missionToFly != nullptr ? this->missionToFly : "(aucune mission)");
    if (this->scen != nullptr) {
        delete this->scen;
        this->scen = nullptr;
        this->zones = nullptr;
    }
    this->next_miss = 0;
    if (this->gameFlowParser.game.game[this->current_miss]->scen.size() == 0) {
        this->openTest(true, "WEAPON_LOADOUT");
        this->flyMission(0);
        return;
    }
    WeaponLoadoutScene *loadout = new WeaponLoadoutScene(&this->optShps, &this->optPals);
    this->scen = loadout;
    this->zones = loadout->init(this->gameFlowParser.game.game[this->current_miss]->scen[this->current_scen], this->optionParser.opts[15], std::bind(&SCGameFlow::loadoutExit, this, std::placeholders::_1));
    if (this->zones != nullptr) {
        loadout->applyDefaultLoadout(this->gameFlowParser.game.load, this->weapon_loadout_id);
    }
    this->script_waiting = true;
}

bool SCGameFlow::playFlyMission(uint8_t flyMissionId) {
    uint8_t flymID = flyMissionId;
    GameState.mission_flyed = flymID;
    const std::string extension = ".IFF";
    std::string missionFilePath = *this->gameFlowParser.game.mlst->data[flymID] + extension;
    std::transform(missionFilePath.begin(), missionFilePath.end(), missionFilePath.begin(), ::toupper);
    if (this->missionToFly != nullptr) {
        free(this->missionToFly);
    }
    this->missionToFly = (char *)malloc(missionFilePath.size() + 1);
    std::strcpy(this->missionToFly, missionFilePath.c_str());
    return true;
}

bool SCGameFlow::playFlyMission2(uint8_t flyMissionId) {
    this->playFlyMission(flyMissionId);
    printf("GAMEFLOW: FLY_MISSION_NOW, depart en vol sans ecran d'armement : %s\n", this->missionToFly);
    this->flyMission(0);
    return true;
}

void SCGameFlow::lookAtLedger() {
    this->zones->clear();
    if (this->scen != nullptr) {
        delete this->scen;
    }
    this->scen = new LedgerScene(&this->optShps, &this->optPals);

    this->zones = this->scen->init(
        this->gameFlowParser.game.game[this->current_miss]->scen[this->current_scen],
        this->optionParser.opts[128],
        std::bind(&SCGameFlow::returnFromScene, this, std::placeholders::_1, std::placeholders::_2)
    );
}

void SCGameFlow::lookAtKillBoard() {
    this->zones->clear();
    if (this->scen != nullptr) {
        delete this->scen;
    }
    this->scen = new KillBoardScene(&this->optShps, &this->optPals);

    this->zones = this->scen->init(
        this->gameFlowParser.game.game[this->current_miss]->scen[this->current_scen],
        this->optionParser.opts[30],
        std::bind(&SCGameFlow::returnFromScene, this, std::placeholders::_1, std::placeholders::_2)
    );
}

void SCGameFlow::viewCatalog() {
    this->zones->clear();
    if (this->scen != nullptr) {
        delete this->scen;
    }
    this->scen = new CatalogueScene(&this->optShps, &this->optPals);

    this->zones = this->scen->init(
        this->gameFlowParser.game.game[this->current_miss]->scen[this->current_scen],
        this->optionParser.opts[30],
        std::bind(&SCGameFlow::returnFromScene, this, std::placeholders::_1, std::placeholders::_2)
    );
}
