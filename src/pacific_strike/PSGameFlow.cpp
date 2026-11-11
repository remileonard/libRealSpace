//
//  PSGameFlow.cpp
//  libRealSpace
//

#include "PSGameFlow.h"
#include "PSScene.h"

PSGameFlow::PSGameFlow() {
    this->initInstructions();
}

PSGameFlow::~PSGameFlow() {
}

// Table de Gameflow_RunScript_604D4 (§ 3.3). 27, 28, 33 et 39 sont des erreurs dans le jeu : absentes de la table.
void PSGameFlow::initInstructions() {
    this->instructions.clear();
    this->instructions[EFECT_OPT_CONV] = std::bind(&PSGameFlow::opConv, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_SCEN] = std::bind(&PSGameFlow::opPlayScene, this, std::placeholders::_1);
    this->instructions[PS_PLAY_ENDING] = std::bind(&PSGameFlow::opPlayEnding, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_FLYM2] = std::bind(&PSGameFlow::opFlym2, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_NOP_4] = std::bind(&PSGameFlow::opNop, this, std::placeholders::_1);
    this->instructions[PS_CAMPAIGN_RESULT] = std::bind(&PSGameFlow::opCampaignResult, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_SETFLAG_TRUE] = std::bind(&PSGameFlow::opSetFlag, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_SETFLAG_FALSE] = std::bind(&PSGameFlow::opClearFlag, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_SHOT] = std::bind(&PSGameFlow::opShot, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_IF_NOT_FLAG] = std::bind(&PSGameFlow::opIfNotGameFlag, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_IF_FLAG] = std::bind(&PSGameFlow::opIfGameFlag, this, std::placeholders::_1);
    this->instructions[PS_RETURN_TO_MENU] = std::bind(&PSGameFlow::opReturnToMenu, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_FLYM] = std::bind(&PSGameFlow::opFlym, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_MIS2] = std::bind(&PSGameFlow::opMis2, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_START_FLIGHT] = std::bind(&PSGameFlow::opStartFlight, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_GO] = std::bind(&PSGameFlow::opGotoNextMission, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_SAVE_GAME] = std::bind(&PSGameFlow::opSaveGame, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_LOAD_GAME] = std::bind(&PSGameFlow::opLoadGame, this, std::placeholders::_1);
    this->instructions[PS_TRAVEL] = std::bind(&PSGameFlow::opTravel, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_LOOK_AT_KILLBOARD] = std::bind(&PSGameFlow::opLookAtKillboard, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_MISS_ACCEPTED] = std::bind(&PSGameFlow::opMissAccepted, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_MISS_REJECTED] = std::bind(&PSGameFlow::opMissRejected, this, std::placeholders::_1);
    this->instructions[PS_LAUNCH_MISSION] = std::bind(&PSGameFlow::opLaunchMission, this, std::placeholders::_1);
    this->instructions[PS_MEDIT_SCREEN] = std::bind(&PSGameFlow::opMeditScreen, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_SET_MUSIC] = std::bind(&PSGameFlow::opSetMusic, this, std::placeholders::_1);
    this->instructions[PS_SET_SCENE] = std::bind(&PSGameFlow::opSetScene, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_SHOW_KILL_MARKS] = std::bind(&PSGameFlow::opNop, this, std::placeholders::_1);
    this->instructions[PS_SHOW_DOSSIER] = std::bind(&PSGameFlow::opShowDossier, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_MISS_ELSE] = std::bind(&PSGameFlow::opElse, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_MISS_ENDIF] = std::bind(&PSGameFlow::opEndif, this, std::placeholders::_1);
    this->instructions[EFFCT_OPT_IF_MISS_SUCCESS] = std::bind(&PSGameFlow::opIfMissSuccess, this, std::placeholders::_1);
    this->instructions[EFECT_OPT_APPLY_CHANGE] = std::bind(&PSGameFlow::opApplyChange, this, std::placeholders::_1);
    this->instructions[PS_SET_VARIABLE] = std::bind(&PSGameFlow::opSetVariable, this, std::placeholders::_1);
    this->instructions[PS_SET_DATE] = std::bind(&PSGameFlow::opSetDate, this, std::placeholders::_1);
    this->instructions[PS_ADVANCE_MONTHS] = std::bind(&PSGameFlow::opAdvanceMonths, this, std::placeholders::_1);
    this->instructions[PS_SET_BYTE_1382] = std::bind(&PSGameFlow::opSetByte1382, this, std::placeholders::_1);
    this->instructions[PS_SAVE_MISSION_INDEX] = std::bind(&PSGameFlow::opSaveMissionIndex, this, std::placeholders::_1);
    this->instructions[PS_RESTORE_MISSION_INDEX] = std::bind(&PSGameFlow::opRestoreMissionIndex, this, std::placeholders::_1);
}

// Copie de SCGameFlow::createScen : écran de Pacific Strike (PSScene).
void PSGameFlow::createScen() {
    if (this->zones != nullptr) {
        this->zones->clear();
    }
    if (this->gameFlowParser.game.game[this->current_miss]->scen.size() > 0) {
        uint8_t optionScenID = this->gameFlowParser.game.game[this->current_miss]->scen[this->current_scen]->info.ID;
        this->scen = new PSScene(&this->optShps, &this->optPals);
        this->zones = this->scen->init(this->gameFlowParser.game.game[this->current_miss]->scen[this->current_scen], this->optionParser.opts[optionScenID], std::bind(&PSGameFlow::clicked, this, std::placeholders::_1, std::placeholders::_2));
    }
}

// PLAY_SCENE (1) : scène n, puis arrêt du script.
void PSGameFlow::opPlayScene(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    this->opScen(instruction);
    this->script_stopped = true;
}

// PLAY_ENDING (2) : Gameflow_PlayEndingOrFuneral_C798D (texte de fin ENDTEXTn.IFF ou FUNERAL.PAK) ; écran pas encore porté.
void PSGameFlow::opPlayEnding(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    printf("PS GAMEFLOW: PLAY_ENDING %d, ecran pas encore porte\n", instruction->value);
}

// CAMPAIGN_RESULT (5) : calcul du résultat de la campagne, pas encore lu en détail ; pas porté.
void PSGameFlow::opCampaignResult(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    printf("PS GAMEFLOW: CAMPAIGN_RESULT, mission %d, pas encore porte\n", this->current_miss);
}

// SET_FLAG (6) : GameVar_SetFlag_5646D.
void PSGameFlow::opSetFlag(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    PSGameState.setFlag(instruction->value, 1);
    printf("PS GAMEFLOW: SET_FLAG %d\n", instruction->value);
}

// CLEAR_FLAG (7) : GameVar_ClearFlag_56520.
void PSGameFlow::opClearFlag(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    PSGameState.setFlag(instruction->value, 0);
    printf("PS GAMEFLOW: CLEAR_FLAG %d\n", instruction->value);
}

// IF_NOT_FLAG (9) : réussit si GameVar_Get_56414(n) = 0.
void PSGameFlow::opIfNotGameFlag(EFCT *instruction) {
    this->openTest(PSGameState.getFlag(instruction->value) == 0, "IF_NOT_FLAG");
}

// IF_FLAG (10) : réussit si GameVar_Get_56414(n) != 0.
void PSGameFlow::opIfGameFlag(EFCT *instruction) {
    this->openTest(PSGameState.getFlag(instruction->value) != 0, "IF_FLAG");
}

// RETURN_TO_MENU (11) : [partie+1254] = 0 et arrêt du script.
void PSGameFlow::opReturnToMenu(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    printf("PS GAMEFLOW: RETURN_TO_MENU\n");
    this->script_stopped = true;
}

// GOTO_NEXT_MISSION (15) : passage à la mission suivante et arrêt du script.
void PSGameFlow::opGotoNextMission(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    this->opGo(instruction);
    this->script_stopped = true;
}

// TRAVEL (18) : Gameflow_Travel_D3E8B (départ, carte, arrivée) ; écran pas encore porté.
void PSGameFlow::opTravel(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    printf("PS GAMEFLOW: TRAVEL %d, ecran pas encore porte\n", instruction->value);
}

// LAUNCH_MISSION (22) : Gameflow_LaunchMission_CAD9D ; compté comme un test pendant un saut. Écran du pont pas encore
// porté : le niveau est ouvert à vrai.
void PSGameFlow::opLaunchMission(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        this->openTest(false, "LAUNCH_MISSION");
        return;
    }
    printf("PS GAMEFLOW: LAUNCH_MISSION, mission %s, ecran pas encore porte\n", this->missionToFly != nullptr ? this->missionToFly : "(aucune)");
    this->openTest(true, "LAUNCH_MISSION");
}

// MEDIT_SCREEN (23) : Gameflow_MeditScreen_E3D9D ; écran pas encore porté.
void PSGameFlow::opMeditScreen(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    printf("PS GAMEFLOW: MEDIT_SCREEN %d, ecran pas encore porte\n", instruction->value);
}

// SET_SCENE (25) : scène n, sans arrêt du script.
void PSGameFlow::opSetScene(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    this->opScen(instruction);
}

// SHOW_DOSSIER (29) : Gameflow_ShowDossier_D3F00, page n de DOSSIER.PAK ; écran pas encore porté.
void PSGameFlow::opShowDossier(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    printf("PS GAMEFLOW: SHOW_DOSSIER %d, ecran pas encore porte\n", instruction->value);
}

// SET_VARIABLE (35) : GameVar_Set_564BB(a, b), a = octet bas, b = octet haut de l'argument.
void PSGameFlow::opSetVariable(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    uint16_t number = instruction->value & 0xFF;
    int16_t value = (int16_t)(instruction->value >> 8);
    PSGameState.setFlag(number, value);
    printf("PS GAMEFLOW: SET_VARIABLE %d = %d\n", number, value);
}

void PSGameFlow::setDate(int total_months) {
    int year = total_months / 12;
    int month = total_months - year * 12;
    PSGameState.setFlag(VAR_MONTH, (int16_t)month);
    PSGameState.setFlag(VAR_YEAR, (int16_t)year);
    printf("PS GAMEFLOW: date, mois %d, annee %d\n", month, year);
}

// SET_DATE (36) : t = année b x 12 + mois a, puis drapeaux 401 à 400 + mission courante (rang pair : + 2 si non posé ;
// rang impair : - 1 si posé ; rang compté à partir de 1), variables 170 à 182 soustraites, 185 à 194 ajoutées deux
// fois ; t au moins date courante + 1 mois (§ 3.4).
void PSGameFlow::opSetDate(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    int month = instruction->value & 0xFF;
    int year = instruction->value >> 8;
    int total = year * 12 + month;
    for (int rank = 1; rank <= this->current_miss; rank++) {
        bool set = PSGameState.getFlag((uint16_t)(400 + rank)) != 0;
        if (rank % 2 == 0 && !set) {
            total += 2;
        } else if (rank % 2 == 1 && set) {
            total -= 1;
        }
    }
    for (uint16_t number = 170; number <= 182; number++) {
        total -= PSGameState.getFlag(number);
    }
    for (uint16_t number = 185; number <= 194; number++) {
        total += 2 * PSGameState.getFlag(number);
    }
    int current = PSGameState.getFlag(VAR_YEAR) * 12 + PSGameState.getFlag(VAR_MONTH);
    if (total < current + 1) {
        total = current + 1;
    }
    printf("PS GAMEFLOW: SET_DATE mois %d annee %d\n", month, year);
    this->setDate(total);
}

// ADVANCE_MONTHS (37) : date + n mois.
void PSGameFlow::opAdvanceMonths(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    int total = PSGameState.getFlag(VAR_YEAR) * 12 + PSGameState.getFlag(VAR_MONTH) + instruction->value;
    printf("PS GAMEFLOW: ADVANCE_MONTHS %d\n", instruction->value);
    this->setDate(total);
}

// SET_BYTE_1382 (38) : [partie+1382] = n ; rôle non établi.
void PSGameFlow::opSetByte1382(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    this->byte_1382 = (uint8_t)instruction->value;
    printf("PS GAMEFLOW: SET_BYTE_1382 %d\n", instruction->value);
}

// SAVE_MISSION_INDEX (40) : variable 130 = mission courante.
void PSGameFlow::opSaveMissionIndex(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    PSGameState.setFlag(VAR_SAVED_MISSION, (int16_t)this->current_miss);
    printf("PS GAMEFLOW: SAVE_MISSION_INDEX %d\n", this->current_miss);
}

// RESTORE_MISSION_INDEX (41) : mission courante = variable 130.
void PSGameFlow::opRestoreMissionIndex(EFCT *instruction) {
    if (!this->scriptExecuting()) {
        return;
    }
    this->current_miss = (uint8_t)PSGameState.getFlag(VAR_SAVED_MISSION);
    printf("PS GAMEFLOW: RESTORE_MISSION_INDEX %d\n", this->current_miss);
}
