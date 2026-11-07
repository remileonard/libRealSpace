//
//  File.h
//  libRealSpace
//
//  Created by fabien sanglard on 1/28/2014.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//
//  TRAINING MISSION (OPTEST.EXE, ovr190) : écrans de réglage, puis vol de MISSIONS\TEMPLATE.IFF complété par
//  SCTrainingMenu::applyTraining. strike_commander_re/analysis/TRAINING_MISSION.md.
//

#ifndef __libRealSpace__File__
#define __libRealSpace__File__

#include "../engine/SCPanel.h"

// Déclarée dans SCScene.h, inclus après ce fichier par precomp.h.
class WeaponLoadoutScene;

// Mission d'entraînement (TRAINING MISSION d'OPTEST.EXE) : un avion de l'écran de choix, table du segment de données
// d'OPTEST lue par Training_AircraftScreen_56B04 et Training_BuildInstantMission_58EEA (15 octets par avion : nom affiché,
// fichier objet, emport, deux dwords non lus par ce code, drapeau). strike_commander_re/analysis/TRAINING_MISSION.md § 5.
struct MISN_TRAINING_PLANE {
    const char *name;
    const char *object;
    const char *loadout;
    // Drapeau : pilote CAST au niveau choisi, programme de mise à jour 1, vitesse 200 ; sinon « TARGET », programme 2,
    // vitesse 100.
    bool armed;
};

class SCTrainingMenu : public IActivity {

public:
    SCTrainingMenu();
    ~SCTrainingMenu();

    void init();
    void unFocus(void);
    void focus(void);
    void runFrame(void);
    // Les 16 avions de l'écran de choix, dans l'ordre des boutons.
    static const MISN_TRAINING_PLANE training_planes[16];
    // Training_BuildInstantMission_58EEA : complète la mission lue depuis MISSIONS\TEMPLATE.IFF avec les réglages des
    // écrans (position du joueur, zone, avions et pilotes ajoutés), au lieu d'écrire instant.iff.
    static void applyTraining(RSMission *mission, const MISN_TRAINING &training);

protected:
    // Départ du vol après l'écran d'armement : SCStrike::player_loadout (emport choisi à l'écran, GameState.weapon_load_out).
    bool player_loadout_pending{ false };
    virtual void OnSearchAndDestroy();

private:
    // Un écran = un panneau de TM.SHP et ses boutons (§ 3 du document), rangé dans panels à son indice.
    enum TrainingScreen {
        SCREEN_TYPE,
        SCREEN_COUNT,
        SCREEN_PLANE,
        SCREEN_MORE,
        SCREEN_ALTITUDE,
        SCREEN_LEVEL,
        SCREEN_ACCEPT,
        SCREEN_TOTAL
    };

    // Ligne de texte écrite au-dessus du panneau (Training_PrintText_55BDC).
    struct TrainingText {
        Point2D position;
        std::string text;
    };
    // État avant un choix : BACKUP le rétablit (efface les textes écrits depuis, comme Training_EraseLastText_55C7B).
    struct TrainingStep {
        TrainingScreen screen;
        MISN_TRAINING settings;
        int group;
        size_t text_count;
    };

    SCState &GameState = SCState::getInstance();
    RLEShape title;
    RLEShape background;
    PakArchive tm_pack;
    // Les 7 écrans, construits une fois par init ; seul l'écran affiché suit la souris et se dessine.
    SCPanel panels[SCREEN_TOTAL];
    RSFont *font{ nullptr };

    TrainingScreen screen{ SCREEN_TYPE };
    MISN_TRAINING settings;
    int group{ 0 };
    std::vector<TrainingText> texts;
    std::vector<TrainingStep> history;

    // Écran d'armement de la campagne (Gameflow_WeaponLoadout_51AB3 dans OPTEST), ouvert après ACCEPT.
    RSOption options;
    RSGameFlow gameflow;
    PakArchive opt_shps;
    PakArchive opt_pals;
    bool options_loaded{ false };
    WeaponLoadoutScene *loadout{ nullptr };
    std::vector<SCZone *> *zones{ nullptr };
    GAMEFLOW_SCEN loadout_scene;
    // Stock de la partie avant l'entraînement, rétabli au retour.
    std::unordered_map<uint8_t, int16_t> saved_inventory;
    bool inventory_saved{ false };
    bool zone_pending{ false };
    bool flight_pending{ false };
    bool flight_started{ false };
    static AREA *trainingArea(const MISN_TRAINING &training); // vol lancé : focus() remet le menu à zéro au retour
    uint8_t zone_choice{ 0 };

    void OnExitTraining();
    void buildPanels();
    void addButton(SCPanel *panel, int x, int y, int width, int height, int image, std::function<void()> action);
    void addBackupCancel(SCPanel *panel, int y, int image);
    void showScreen(TrainingScreen next);
    int addText(int x, int y, const std::string &text);
    int textWidth(const std::string &text);
    // Actions des boutons, une par écran (valeur du bouton en argument).
    void onTypeChosen(int type);
    void onCountChosen(int count);
    void onPlaneChosen(int plane);
    void onMore();
    void onEnough();
    void onAltitudeChosen(int choice);
    void onLevelChosen(int level);
    void onAccept();
    void pushStep();
    void backup();
    void startFlight(bool with_loadout);
    bool openLoadout();
    void closeLoadout();
    void restoreInventory();
    void onLoadoutClick(std::vector<EFCT *> *script, uint8_t id);
};

#endif /* defined(__libRealSpace__File__) */
