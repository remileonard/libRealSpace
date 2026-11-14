#pragma once
//
//  PSGameFlow.h
//  libRealSpace
//
//  Déroulement de la campagne de Pacific Strike (Gameflow_RunScript_604D4 de PACIFIC.EXE, table de 42 commandes,
//  strike_commander_re/analysis/pacific/README.md § 3) : la boucle de SCGameFlow avec sa propre table d'instructions.
//

#include "../strike_commander/precomp.h"
#include "PSState.h"

class PSGameFlow : public virtual SCGameFlow {
protected:
    PSState &PSGameState = PSState::getInstance();

public:
    // Commandes propres à Pacific Strike (noms proposés dans l'analyse).
    enum PSGameflowCommand : uint8_t {
        PS_PLAY_ENDING = 2,
        PS_CAMPAIGN_RESULT = 5,
        PS_RETURN_TO_MENU = 11,
        PS_TRAVEL = 18,
        PS_LAUNCH_MISSION = 22,
        PS_MEDIT_SCREEN = 23,
        PS_SET_SCENE = 25,
        PS_SHOW_DOSSIER = 29,
        PS_SET_VARIABLE = 35,
        PS_SET_DATE = 36,
        PS_ADVANCE_MONTHS = 37,
        PS_SET_BYTE_1382 = 38,
        PS_SAVE_MISSION_INDEX = 40,
        PS_RESTORE_MISSION_INDEX = 41
    };
    // Drapeaux de PSState qui portent une valeur (§ 2 et § 3.4).
    static const uint16_t VAR_MONTH = 82;
    static const uint16_t VAR_YEAR = 83;
    static const uint16_t VAR_SAVED_MISSION = 130;

    PSGameFlow();
    ~PSGameFlow();
    void init() override;
    void initInstructions() override;
    void createScen() override;
    void playConv(uint8_t convId) override;
    void flyMission(uint8_t missionid) override;

protected:
    // [partie+1382], posé par SET_BYTE_1382 ; rôle non établi.
    uint8_t byte_1382{ 0 };

    void opPlayScene(EFCT *instruction);
    void opPlayEnding(EFCT *instruction);
    void opCampaignResult(EFCT *instruction);
    void opSetFlag(EFCT *instruction);
    void opClearFlag(EFCT *instruction);
    void opIfNotGameFlag(EFCT *instruction);
    void opIfGameFlag(EFCT *instruction);
    void opReturnToMenu(EFCT *instruction);
    void opGotoNextMission(EFCT *instruction);
    void opTravel(EFCT *instruction);
    void opLaunchMission(EFCT *instruction);
    void opMeditScreen(EFCT *instruction);
    void opSetScene(EFCT *instruction);
    void opShowDossier(EFCT *instruction);
    void opSetVariable(EFCT *instruction);
    void opSetDate(EFCT *instruction);
    void opAdvanceMonths(EFCT *instruction);
    void opSetByte1382(EFCT *instruction);
    void opSaveMissionIndex(EFCT *instruction);
    void opRestoreMissionIndex(EFCT *instruction);
    void setDate(int total_months);
};
