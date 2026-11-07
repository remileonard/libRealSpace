//
//  File.cpp
//  libRealSpace
//
//  Created by fabien sanglard on 1/28/2014.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//
//

#include "precomp.h"

// Table des avions de Training_AircraftScreen_56B04 (bouton n = entrée n − 1). Fichier objet et emport : mots 2 et 4 de
// chaque enregistrement de 15 octets, écrits ici en majuscules comme après la lecture d'un PART ; drapeau : octet 14
// (cmp byte ptr [bx+0AA2h], 0).
const MISN_TRAINING_PLANE SCTrainingMenu::training_planes[16] = {
    {
        "F-15",
        "F-15",
        "F15LOADA",
        true,
    },
    {
        "F-16",
        "F-16GRAY",
        "F16LOADA",
        true,
    },
    {
        "F/A-18",
        "F-18",
        "F18LOADA",
        true,
    },
    {
        "C-130",
        "C130DES",
        "",
        false,
    },
    {
        "F-22",
        "F-22",
        "F22LOAD",
        true,
    },
    {
        "YF-23",
        "YF23",
        "YF23LOAD",
        true,
    },
    {
        "A-10",
        "A-10",
        "",
        true,
    },
    {
        "E-3",
        "AWACS",
        "",
        false,
    },
    {
        "Tornado",
        "TORNCG",
        "TORNLOAD",
        true,
    },
    {
        "Mirage 2000",
        "MIRAGE",
        "MIRLOADA",
        true,
    },
    {
        "Lear Jet",
        "LEARJET",
        "",
        false,
    },
    {
        "747",
        "747",
        "",
        false,
    },
    {
        "MiG-21",
        "MIG21",
        "M21LOADA",
        true,
    },
    {
        "MiG-29",
        "MIG29",
        "M29LOADA",
        true,
    },
    {
        "Su-27",
        "SU27",
        "SUMLOAD",
        true,
    },
    {
        "TU-95",
        "TU-20",
        "TU20LOAD",
        false,
    },
};

// Sphère de 49 octets ajoutée au chunk AREA : centre x = −66000, z = −128000, rayon 45000 ; son altitude est le nombre
// de pieds recopié tel quel (`shl eax, 8` sur la valeur en pieds).
AREA *SCTrainingMenu::trainingArea(const MISN_TRAINING &training) {
    if (training.type != 1) {
        return nullptr;
    }
    AREA *zone = new AREA();
    zone->AreaType = 'S';
    memset(zone->AreaName, 0, sizeof(zone->AreaName));
    strncpy(zone->AreaName, "DESTROY BOGEYS", 32);
    zone->position.x = -66000.0f * BLOCK_COORD_SCALE;
    zone->position.y = (float)training.altitude_feet * HEIGH_MAP_SCALE;
    zone->position.z = -128000.0f * BLOCK_COORD_SCALE;
    zone->radius = 45000.0f * BLOCK_COORD_SCALE;
    zone->AreaWidth = (unsigned int)zone->radius;
    zone->AreaHeight = zone->AreaWidth;
    return zone;
}

// Joueur (1er objet du modèle) : x = −73000, z = −128000, cap 90, altitude = pieds × 256 / 839 ((pieds << 16) / 839 en
// 24.8 : `idiv` par dword_46456 = 839 / 256, pieds par mètre). FLAG : nombre total d'avions ajoutés (FLAG[1], décompté
// par le programme 3 du modèle). DOGFIGHT : un avion par appareil des groupes, rang compté sur tous les groupes ;
// x = −59000 + 500 × groupe, z = −(128000 + 300 × rang), cap 270, même altitude que le joueur ; avion armé : pilote au
// niveau choisi, vitesse 200, programme de mise à jour 1 ; sinon « TARGET », vitesse 100, programme 2 ; destruction :
// programme 3.
void SCTrainingMenu::applyTraining(RSMission *mission, const MISN_TRAINING &training) {
    int64_t altitude_raw = ((int64_t)training.altitude_feet * 65536) / 839;
    float altitude = (float)altitude_raw / 256.0f * HEIGH_MAP_SCALE;
    Vector3D player_position = Vector3D(-73000.0f * BLOCK_COORD_SCALE, altitude, -128000.0f * BLOCK_COORD_SCALE);
    const char *levels[3] = {
        "ROOKIE",
        "VETERAN",
        "ACE",
    };
    std::vector<MISN_ADDED_PART> parts;
    if (training.type == 1) {
        int rank = 0;
        for (int g = 0; g < 3; g++) {
            const MISN_TRAINING_PLANE &plane = training_planes[training.group_plane[g] % 16];
            for (int k = 0; k < training.group_count[g]; k++) {
                MISN_PART *part = new MISN_PART();
                part->member_name = plane.object;
                part->member_name_destroyed = "";
                part->weapon_load = plane.loadout;
                part->area_id = 255;
                part->placement = 0;
                part->position = Vector3D((-59000.0f + 500.0f * g) * BLOCK_COORD_SCALE, altitude,
                                          -(128000.0f + 300.0f * rank) * BLOCK_COORD_SCALE);
                part->azymuth = 270;
                part->relative_position = part->position;
                part->relative_azymuth = part->azymuth;
                part->speed = plane.armed ? 200 : 100;
                part->team = 255;
                part->follow_ally = 255;
                part->unknown_byte_47 = 255;
                part->formation_side = 0;
                part->formation_ahead = 0;
                part->formation_up = 0;
                part->progs_id.push_back(255);
                part->progs_id.push_back(plane.armed ? 1 : 2);
                part->progs_id.push_back(3);
                part->progs_id.push_back(255);
                part->on_is_activated = part->progs_id[0];
                part->on_mission_update = part->progs_id[1];
                part->on_is_destroyed = part->progs_id[2];
                part->on_missions_init = part->progs_id[3];
                part->entity = nullptr;
                MISN_ADDED_PART added;
                added.cast = plane.armed ? levels[training.pilot_level % 3] : "TARGET";
                added.part = part;
                parts.push_back(added);
                rank++;
            }
        }
    }
    mission->updateMission(player_position, 90, (int16_t)parts.size(), trainingArea(training), parts);
}

static const Point2D PANEL_LOW = {
    6,
    153,
};
static const Point2D PANEL_HIGH = {
    6,
    116,
};
static const int TEXT_LETTER_GAP = 2;

static const int16_t TRAINING_STOCK[9] = {
    6,
    6,
    12,
    12,
    24,
    24,
    4,
    8,
    6,
};

static const uint8_t LOADOUT_OPTIONS_SCREEN = 15;
static const uint8_t LOADOUT_SCENE = 11;
static const uint8_t LOADOUT_DEFAULT_LOAD = 0;
static const uint8_t LOADOUT_SPRITE_FLY = 12;

SCTrainingMenu::SCTrainingMenu() {
    this->music_playing = false;
}

SCTrainingMenu::~SCTrainingMenu() {
    this->closeLoadout();
    this->buttons.clear();
    if (this->font != nullptr) {
        delete this->font;
        this->font = nullptr;
    }
}

void SCTrainingMenu::focus(void) {
    this->focused = true;
    if (!this->music_playing) {
        Mixer.switchBank(0);
        Mixer.playMusic(28);
        this->music_playing = true;
    }
    if (this->flight_started) {
        this->flight_started = false;
        GameState.training_active = false;
        this->restoreInventory();
        this->settings = MISN_TRAINING();
        this->group = 0;
        this->texts.clear();
        this->history.clear();
        this->showScreen(SCREEN_TYPE);
    }
}

void SCTrainingMenu::unFocus(void) {
    this->focused = false;
    if (this->music_playing) {
        Mixer.stopMusic();
        this->music_playing = false;
    }
}

// Départ du vol, pour les deux types : MISSIONS\TEMPLATE.IFF, complété par SCTrainingMenu::applyTraining dans
// SCMission::loadMission. Le nom est gardé : EnhancedTrainingMenu le redéfinit pour lancer EnhancedStrike.
void SCTrainingMenu::OnSearchAndDestroy() {
    SCStrike *strike = new SCStrike();
    strike->init();
    strike->player_loadout = this->player_loadout_pending;
    strike->setMission("TEMPLATE.IFF");
    Game->addActivity(strike);
}

void SCTrainingMenu::OnExitTraining(void) {
    GameState.training_active = false;
    this->restoreInventory();
    Game->stopTopActivity();
}

void SCTrainingMenu::init() {

    TreEntry *objViewPAK = Assets.GetEntryByName("..\\..\\DATA\\GAMEFLOW\\OBJVIEW.PAK");
    PakArchive assets;
    assets.InitFromRAM("OBJVIEW.PAK", objViewPAK->data, objViewPAK->size);

    // OBJVIEW.PAK : entrée 7 = palette, 6 = fond plein écran en (0, 0), 1 = bandeau « TRAINING MISSION » en (4, 3)
    // (Training_TypeScreen_5547D : `push 3` / `push 4` avant Shape_DrawSpans_32CD8).
    ByteStream paletteReader;
    paletteReader.Set(assets.GetEntry(7)->data, assets.GetEntry(7)->size);
    this->palette.ReadPatch(&paletteReader);

    PakEntry *backgroundPakEntry = assets.GetEntry(6);
    PakArchive bgPack;
    bgPack.InitFromRAM("OBJVIEW.PAK: file 6", backgroundPakEntry->data, backgroundPakEntry->size);
    background.init(bgPack.GetEntry(0)->data, bgPack.GetEntry(0)->size);

    PakEntry *titlePackEntry = assets.GetEntry(1);
    PakArchive titlePack;
    titlePack.InitFromRAM("", titlePackEntry->data, titlePackEntry->size);
    title.init(titlePack.GetEntry(0)->data, titlePack.GetEntry(0)->size);
    Point2D positionTitle = {
        4,
        3,
    };
    title.SetPosition(&positionTitle);

    // TM.SHP : les panneaux et les boutons de tous les écrans.
    TreEntry *trButtonsEntry = Assets.GetEntryByName("..\\..\\DATA\\GAMEFLOW\\TM.SHP");
    this->tm_pack.InitFromRAM("TM.SHP", trButtonsEntry->data, trButtonsEntry->size);

    // BWFONT.SHP : police chargée par Training_TypeScreen_5547D (absente de RSFontManager).
    TreEntry *fontEntry = Assets.GetEntryByName("..\\..\\DATA\\GAMEFLOW\\BWFONT.SHP");
    if (fontEntry != nullptr) {
        PakArchive fontPack;
        fontPack.InitFromRAM("BWFONT.SHP", fontEntry->data, fontEntry->size);
        this->font = new RSFont();
        this->font->InitFromPAK(&fontPack);
    }

    TreEntry *optionIFF = Assets.GetEntryByName(Assets.option_filename);
    TreEntry *gameflowIFF = Assets.GetEntryByName(Assets.gameflow_filename);
    TreEntry *optShapEntry = Assets.GetEntryByName(Assets.optshps_filename);
    TreEntry *optPalettesEntry = Assets.GetEntryByName(Assets.optpals_filename);
    
    this->options.InitFromRam(optionIFF->data, optionIFF->size);
    this->gameflow.InitFromRam(gameflowIFF->data, gameflowIFF->size);
    this->opt_shps.InitFromRAM("OPTSHPS.PAK", optShapEntry->data, optShapEntry->size);
    this->opt_pals.InitFromRAM("OPTPALS.PAK", optPalettesEntry->data, optPalettesEntry->size);
    this->options_loaded = true;

    this->buildPanels();
    this->showScreen(SCREEN_TYPE);
}

// Un bouton : rectangle relatif au panneau, image normale `image`, image enfoncée `image + 1` (toutes les paires de
// TM.SHP se suivent).
void SCTrainingMenu::addButton(SCPanel *panel, int x, int y, int width, int height, int image,
                               std::function<void()> action) {
    panel->addButton(x, y, width, height, this->tm_pack.GetEntry(image), this->tm_pack.GetEntry(image + 1), action);
}

// BACKUP en (87, y) et CANCEL en (155, y), images `image` et `image + 2` : présents sur tous les écrans sauf le premier.
void SCTrainingMenu::addBackupCancel(SCPanel *panel, int y, int image) {
    this->addButton(panel, 87, y, 65, 12, image, std::bind(&SCTrainingMenu::backup, this));
    this->addButton(panel, 155, y, 65, 12, image + 2, std::bind(&SCTrainingMenu::OnExitTraining, this));
}

// Panneaux et boutons des 7 écrans (positions et images de TRAINING_MISSION.md § 3), construits une fois.
void SCTrainingMenu::buildPanels() {
    SCPanel *panel = &this->panels[SCREEN_TYPE];
    panel->init(this->tm_pack.GetEntry(0), PANEL_LOW);
    this->addButton(panel, 16, 9, 136, 12, 1, std::bind(&SCTrainingMenu::onTypeChosen, this, 2));
    this->addButton(panel, 155, 9, 136, 12, 3, std::bind(&SCTrainingMenu::onTypeChosen, this, 1));
    this->addButton(panel, 155, 24, 65, 12, 5, std::bind(&SCTrainingMenu::OnExitTraining, this));

    panel = &this->panels[SCREEN_COUNT];
    panel->init(this->tm_pack.GetEntry(7), PANEL_LOW);
    for (int n = 0; n < 5; n++) {
        this->addButton(panel, 113 + 17 * n, 9, 14, 12, 8 + 2 * n, std::bind(&SCTrainingMenu::onCountChosen, this, n + 1));
    }
    this->addBackupCancel(panel, 24, 18);

    panel = &this->panels[SCREEN_PLANE];
    panel->init(this->tm_pack.GetEntry(22), PANEL_HIGH);
    const int columns[4] = {
        9,
        77,
        145,
        235,
    };
    for (int n = 0; n < 16; n++) {
        this->addButton(panel, columns[n / 4], 7 + 13 * (n % 4), 65, 12, 23 + 2 * n,
                        std::bind(&SCTrainingMenu::onPlaneChosen, this, n + 1));
    }
    this->addBackupCancel(panel, 61, 55);

    panel = &this->panels[SCREEN_MORE];
    panel->init(this->tm_pack.GetEntry(59), PANEL_LOW);
    this->addButton(panel, 87, 9, 65, 12, 60, std::bind(&SCTrainingMenu::onMore, this));
    this->addButton(panel, 155, 9, 65, 12, 62, std::bind(&SCTrainingMenu::onEnough, this));
    this->addBackupCancel(panel, 24, 64);

    panel = &this->panels[SCREEN_ALTITUDE];
    panel->init(this->tm_pack.GetEntry(68), PANEL_LOW);
    for (int n = 0; n < 4; n++) {
        this->addButton(panel, 19 + 68 * n, 9, 65, 12, 69 + 2 * n,
                        std::bind(&SCTrainingMenu::onAltitudeChosen, this, n + 1));
    }
    this->addBackupCancel(panel, 24, 77);

    panel = &this->panels[SCREEN_LEVEL];
    panel->init(this->tm_pack.GetEntry(81), PANEL_LOW);
    for (int n = 0; n < 3; n++) {
        this->addButton(panel, 54 + 68 * n, 9, 65, 12, 82 + 2 * n,
                        std::bind(&SCTrainingMenu::onLevelChosen, this, n + 1));
    }
    this->addBackupCancel(panel, 24, 88);

    panel = &this->panels[SCREEN_ACCEPT];
    panel->init(this->tm_pack.GetEntry(92), PANEL_LOW);
    this->addButton(panel, 121, 9, 65, 12, 93, std::bind(&SCTrainingMenu::onAccept, this));
    this->addBackupCancel(panel, 24, 95);
}

// Changement d'écran : la liste des boutons de l'activité devient celle du panneau. Appelé depuis l'action d'un bouton
// pendant checkButtons, qui s'arrête juste après l'action : les boutons ne sont pas détruits, rien n'est invalidé.
void SCTrainingMenu::showScreen(TrainingScreen next) {
    this->screen = next;
    this->buttons = this->panels[next].buttons;
}

// Largeur d'un texte écrit par FrameBuffer::printText, espace qui suit compris : x du texte écrit à sa suite.
int SCTrainingMenu::textWidth(const std::string &text) {
    if (this->font == nullptr || text.empty()) {
        return 0;
    }
    return VGA.getFrameBuffer()->getJustifiedTextWidth(this->font, text, TEXT_LETTER_GAP) + TEXT_LETTER_GAP;
}

// Training_PrintText_55BDC : texte écrit en (x, y), coin haut gauche. Renvoie le x qui suit le texte.
int SCTrainingMenu::addText(int x, int y, const std::string &text) {
    TrainingText line;
    line.position = {
        x,
        y,
    };
    line.text = text;
    this->texts.push_back(line);
    return x + this->textWidth(text);
}

// État avant un choix : rétabli par BACKUP sur l'écran suivant.
void SCTrainingMenu::pushStep() {
    TrainingStep step;
    step.screen = this->screen;
    step.settings = this->settings;
    step.group = this->group;
    step.text_count = this->texts.size();
    this->history.push_back(step);
}

// BACKUP : retour à l'écran précédent, avec les réglages et les textes d'avant son choix (dans le jeu, chaque écran
// est appelé par le précédent et rend 0 ; Training_EraseLastText_55C7B efface les textes de ce choix).
void SCTrainingMenu::backup() {
    if (this->history.empty()) {
        return;
    }
    TrainingStep step = this->history.back();
    this->history.pop_back();
    this->settings = step.settings;
    this->group = step.group;
    this->texts.resize(step.text_count);
    this->showScreen(step.screen);
}

void SCTrainingMenu::startFlight(bool with_loadout) {
    GameState.training = this->settings;
    GameState.training_active = true;
    this->player_loadout_pending = with_loadout;
    this->flight_pending = true;
}

void SCTrainingMenu::restoreInventory() {
    if (!this->inventory_saved) {
        return;
    }
    GameState.weapon_inventory = this->saved_inventory;
    this->saved_inventory.clear();
    this->inventory_saved = false;
}

// ACCEPT : stock de l'entraînement puis écran d'armement de la campagne (WeaponLoadoutScene, comme SCGameFlow pour la
// commande WEAPON_LOADOUT), avec la scène 11 et le chargement par défaut 0.
bool SCTrainingMenu::openLoadout() {
    if (this->options.opts.find(LOADOUT_OPTIONS_SCREEN) == this->options.opts.end()) {
        return false;
    }
    if (!this->inventory_saved) {
        this->saved_inventory = GameState.weapon_inventory;
        this->inventory_saved = true;
    }
    GameState.weapon_inventory.clear();
    for (int i = 0; i < 9; i++) {
        GameState.weapon_inventory[optest_weapon_ids[i]] = TRAINING_STOCK[i];
    }
    this->loadout_scene.info.ID = LOADOUT_SCENE;
    this->loadout_scene.info.UNKOWN = 0;
    this->loadout_scene.requ = nullptr;
    this->loadout = new WeaponLoadoutScene(&this->opt_shps, &this->opt_pals);
    this->loadout->keep_music = true;
    this->zones = this->loadout->init(
        &this->loadout_scene, this->options.opts[LOADOUT_OPTIONS_SCREEN],
        std::bind(&SCTrainingMenu::onLoadoutClick, this, std::placeholders::_1, std::placeholders::_2));
    if (this->zones == nullptr) {
        this->closeLoadout();
        return false;
    }
    this->loadout->applyDefaultLoadout(this->gameflow.game.load, LOADOUT_DEFAULT_LOAD);
    return true;
}

void SCTrainingMenu::closeLoadout() {
    if (this->loadout != nullptr) {
        delete this->loadout;
        this->loadout = nullptr;
    }
    this->zones = nullptr;
    this->zone_pending = false;
}

// Clic sur une zone de l'écran d'armement : noté, appliqué par runFrame (les zones appartiennent à la scène).
void SCTrainingMenu::onLoadoutClick(std::vector<EFCT *> *script, uint8_t id) {
    this->zone_pending = true;
    this->zone_choice = id;
}

// Premier écran : 1 = DOGFIGHT, 2 = SEARCH AND DESTROY.
void SCTrainingMenu::onTypeChosen(int type) {
    this->pushStep();
    this->settings = MISN_TRAINING();
    this->settings.type = (uint8_t)type;
    this->group = 0;
    if (type == 1) {
        // Training_DogfightBriefing_58E2A
        this->addText(5, 43, "Mission Briefing: Air Superiority");
        this->addText(5, 55, "Intercept and engage inbound bogeys:");
        this->showScreen(SCREEN_COUNT);
    } else {
        // Training_SearchBriefing_58E74 : aucun réglage, validation directe.
        this->addText(5, 43, "Mission Briefing: Ground Attack");
        this->addText(5, 55, "Search and destroy enemy targets at");
        this->addText(5, 67, "pilot's discretion.  Refer to area");
        this->addText(5, 79, "map for enemy positions.");
        this->showScreen(SCREEN_ACCEPT);
    }
}

// Nombre d'avions du groupe (1 à 5).
void SCTrainingMenu::onCountChosen(int count) {
    this->pushStep();
    this->settings.group_count[this->group] = (uint8_t)count;
    this->addText(20, 67 + 12 * this->group, std::to_string(count));
    this->showScreen(SCREEN_PLANE);
}

// Type d'avion du groupe (1 à 16, rang dans SCTrainingMenu::training_planes + 1).
void SCTrainingMenu::onPlaneChosen(int plane) {
    this->pushStep();
    this->settings.group_plane[this->group] = (uint8_t)(plane - 1);
    std::string name = SCTrainingMenu::training_planes[plane - 1].name;
    if (this->group == 2) {
        name += ".";
    }
    this->addText(30, 67 + 12 * this->group, name);
    if (this->group < 2) {
        this->showScreen(SCREEN_MORE);
    } else {
        this->addText(5, 103, "Altitude: ");
        this->showScreen(SCREEN_ALTITUDE);
    }
}

// MORE : « , » après le 1er groupe, « , and » après le 2e, puis le groupe suivant.
void SCTrainingMenu::onMore() {
    this->pushStep();
    int line = 67 + 12 * this->group;
    int x = 30 + this->textWidth(SCTrainingMenu::training_planes[this->settings.group_plane[this->group]].name);
    if (this->group == 1) {
        this->addText(x, line, ", and");
    } else {
        this->addText(x, line, ",");
    }
    this->group++;
    this->showScreen(SCREEN_COUNT);
}

// ENOUGH : « . » après le dernier groupe, puis l'altitude.
void SCTrainingMenu::onEnough() {
    this->pushStep();
    int line = 67 + 12 * this->group;
    int x = 30 + this->textWidth(SCTrainingMenu::training_planes[this->settings.group_plane[this->group]].name);
    this->addText(x, line, ".");
    this->addText(5, 103, "Altitude: ");
    this->showScreen(SCREEN_ALTITUDE);
}

// Training_AltitudeScreen_55C92 : choix × 10000 − 5000 pieds (choix 1 à 4).
void SCTrainingMenu::onAltitudeChosen(int choice) {
    this->pushStep();
    this->settings.altitude_feet = choice * 10000 - 5000;
    this->addText(5 + this->textWidth("Altitude: "), 103, std::to_string(this->settings.altitude_feet) + " feet.");
    this->addText(5, 115, "Intelligence reports indicate ");
    this->showScreen(SCREEN_LEVEL);
}

// Niveau des pilotes ennemis (1 à 3).
void SCTrainingMenu::onLevelChosen(int level) {
    const char *levels[3] = {
        "rookie",
        "veteran",
        "ace",
    };
    this->pushStep();
    this->settings.pilot_level = (uint8_t)(level - 1);
    this->addText(5 + this->textWidth("Intelligence reports indicate "), 115,
                  std::string(levels[level - 1]) + " pilots.");
    this->showScreen(SCREEN_ACCEPT);
}

void SCTrainingMenu::onAccept() {
    this->openLoadout();
}

void SCTrainingMenu::runFrame(void) {
    if (this->flight_pending) {
        this->flight_pending = false;
        this->flight_started = true;
        this->OnSearchAndDestroy();
        return;
    }
    if (this->loadout != nullptr) {
        VGA.activate();
        VGA.getFrameBuffer()->fillWithColor(0);
        this->loadout->Render();
        this->checkZones(this->zones);
        Mouse.draw();
        VGA.vSync();
        if (this->zone_pending) {
            uint8_t id = this->zone_choice;
            this->closeLoadout();
            if (id == LOADOUT_SPRITE_FLY) {
                this->startFlight(true);
            } else {
                this->OnExitTraining();
            }
        }
        return;
    }
    checkButtons();

    VGA.activate();
    VGA.getFrameBuffer()->clear();

    VGA.setPalette(&this->palette);

    VGA.getFrameBuffer()->drawShape(&background);
    VGA.getFrameBuffer()->drawShape(&title);
    this->panels[this->screen].draw(VGA.getFrameBuffer());

    if (this->font != nullptr) {
        for (auto &line : this->texts) {
            VGA.getFrameBuffer()->printText(this->font, line.position, line.text, 0);
        }
    }

    drawButtons();

    Mouse.draw();

    VGA.vSync();
}
