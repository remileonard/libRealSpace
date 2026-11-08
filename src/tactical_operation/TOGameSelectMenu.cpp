//
//  TOGameSelectMenu.cpp
//  libRealSpace
//

#include "TOGameSelectMenu.h"
#include "TOConvAssetManager.h"
#include "TOMainMenu.h"

TOGameSelectMenu::TOGameSelectMenu() {
    this->music_playing = false;
    this->focused = false;
    if (Game == nullptr) {
        Game = &GameEngine::instance();
    }
}

TOGameSelectMenu::~TOGameSelectMenu() {
}

void TOGameSelectMenu::applyPalette(PakEntry *entry) {
    ByteStream reader;
    reader.Set(entry->data, entry->size);
    this->palette.ReadPatch(&reader);
}

// MENU.EXE (fonction en 18340) : palettes OPTPALS 21 et 24, MID1 59, TOMENU 2 ; fond OPTSHPS 44 ; panneau TOMENU 0
// en (45, 24) ; boutons, images de TOMENU 1 : Strike Commander en (44, 9), 166 x 26, Tactical Operations en (45, 47),
// 165 x 26 (images 0 et 1 relevées, 2 et 3 enfoncées, d'après les images).
void TOGameSelectMenu::init(void) {
    TreEntry *optpals = Assets.GetEntryByName(Assets.optpals_filename);
    PakArchive optpals_pak;
    optpals_pak.InitFromRAM("OPTPALS.PAK", optpals->data, optpals->size);
    this->applyPalette(optpals_pak.GetEntry(21));
    this->applyPalette(optpals_pak.GetEntry(24));
    TreEntry *mid1 = Assets.GetEntryByName("..\\..\\DATA\\MIDGAMES\\MID1.PAK");
    PakArchive mid1_pak;
    mid1_pak.InitFromRAM("MID1.PAK", mid1->data, mid1->size);
    this->applyPalette(mid1_pak.GetEntry(59));
    TreEntry *tomenu = Assets.GetEntryByName(Assets.gameflow_root_path + "TOMENU.PAK");
    this->tomenu.InitFromRAM("TOMENU.PAK", tomenu->data, tomenu->size);
    this->applyPalette(this->tomenu.GetEntry(2));

    TreEntry *optshps = Assets.GetEntryByName(Assets.optshps_filename);
    PakArchive optshps_pak;
    optshps_pak.InitFromRAM("OPTSHPS.PAK", optshps->data, optshps->size);
    PakEntry *mountain_entry = optshps_pak.GetEntry(44);
    this->mountain_images.InitFromRAM("OPTSHPS 44", mountain_entry->data, mountain_entry->size);
    this->mountains.init(this->mountain_images.GetEntry(0)->data, this->mountain_images.GetEntry(0)->size);
    Point2D origin = {
        0,
        0
    };
    this->mountains.SetPosition(&origin);

    PakEntry *panel_entry = this->tomenu.GetEntry(0);
    this->panel_images.InitFromRAM("TOMENU 0", panel_entry->data, panel_entry->size);
    PakEntry *button_entry = this->tomenu.GetEntry(1);
    this->button_images.InitFromRAM("TOMENU 1", button_entry->data, button_entry->size);
    Point2D panel_position = {
        45,
        24
    };
    this->panel.init(this->panel_images.GetEntry(0), panel_position);
    this->panel.addButton(44, 9, 166, 26, this->button_images.GetEntry(0), this->button_images.GetEntry(2), std::bind(&TOGameSelectMenu::onStrikeCommander, this));
    this->panel.addButton(45, 47, 165, 26, this->button_images.GetEntry(1), this->button_images.GetEntry(3), std::bind(&TOGameSelectMenu::onTacticalOperations, this));
    printf("CD MENU: menu de choix du jeu pret\n");
}

// Musique du menu (morceau 23, comme MENU.EXE et SCMainMenu) et minuteur, à chaque retour du menu en haut de la pile.
void TOGameSelectMenu::focus(void) {
    if (this->focused) {
        return;
    }
    this->focused = true;
    this->shown_at = SDL_GetTicks();
    if (!this->music_playing) {
        Mixer.switchBank(0);
        Mixer.playMusic(23);
        this->music_playing = true;
    }
}

void TOGameSelectMenu::unFocus(void) {
    this->focused = false;
    if (this->music_playing) {
        Mixer.stopMusic();
        this->music_playing = false;
    }
}

void TOGameSelectMenu::runFrame(void) {
    if (SDL_GetTicks() - this->shown_at > IDLE_MS && SCFlicPlayer::IsAvailable()) {
        printf("CD MENU: pas de choix depuis %u ms, l'animation est rejouee\n", IDLE_MS);
        SCFlicPlayer *flic = new SCFlicPlayer();
        flic->init();
        Game->addActivity(flic);
        return;
    }
    VGA.activate();
    VGA.getFrameBuffer()->clear();
    VGA.setPalette(&this->palette);
    VGA.getFrameBuffer()->drawShape(&this->mountains);
    this->panel.checkButtons();
    this->panel.draw(VGA.getFrameBuffer());
    Mouse.draw();
    VGA.vSync();
}

// PLAY STRIKE COMMANDER : intro puis menu principal, comme main.cpp sans le menu de choix.
void TOGameSelectMenu::onStrikeCommander() {
    printf("CD MENU: Strike Commander\n");
    this->unFocus();
    this->stop();
    SCMainMenu *main = new SCMainMenu();
    main->init();
    Game->addActivity(main);
    SCAnimationPlayer *intro = new SCAnimationPlayer();
    intro->init();
    Game->addActivity(intro);
}

// PLAY TACTICAL OPERATIONS : données de Tactical Operations, puis son menu principal.
void TOGameSelectMenu::onTacticalOperations() {
    printf("CD MENU: Tactical Operations\n");
    this->unFocus();
    this->stop();
    this->loadTacticalOperationsData();
    SCState::getInstance().Reset();
    TOMainMenu *main = new TOMainMenu();
    main->init();
    Game->addActivity(main);
}

// Copie du chargement de DebugGame::loadTO, sans la partie de débogage.
void TOGameSelectMenu::loadTacticalOperationsData() {
    ConvAssetManager::setInstance(std::make_unique<TOConvAssetManager>());
    std::vector<std::string> cdTreFiles = {
        "TOBIGTRE.TRE",
        "LILTRE.TRE",
        "TOVOCLST.TRE",
    };
    Assets.ReadISOImage("./SC.DAT");
    Assets.init(cdTreFiles);
    Assets.intel_root_path = "..\\..\\DATA\\INTEL\\";
    Assets.mission_root_path = "..\\..\\DATA\\MISSIONS\\";
    Assets.object_root_path = "..\\..\\DATA\\OBJECTS\\";
    Assets.sound_root_path = "..\\..\\DATA\\SOUND\\";
    Assets.texture_root_path = "..\\..\\DATA\\TXM\\";
    Assets.gameflow_root_path = "..\\..\\DATA\\GAMEFLOW\\";
    Assets.gameflow_filename = Assets.gameflow_root_path + "GAMEFLO2.IFF";
    Assets.optshps_filename = Assets.gameflow_root_path + "OPTSHPS.PAK";
    Assets.optpals_filename = Assets.gameflow_root_path + "OPTPALS.PAK";
    Assets.optfont_filename = Assets.gameflow_root_path + "OPTFONT.IFF";
    Assets.navmap_filename = "..\\..\\DATA\\COCKPITS\\NAVMAP2.IFF";
    Assets.conv_pak_filename = Assets.gameflow_root_path + "CONVSHPS.PAK";
    Assets.option_filename = Assets.gameflow_root_path + "OPTIONS2.IFF";
    Assets.conv_data_filename = Assets.gameflow_root_path + "CONVDAT2.IFF";
    Assets.conv_pal_filename = Assets.gameflow_root_path + "CONVPALS.PAK";
    Assets.txm_filename = Assets.texture_root_path + "TXMPACK.PAK";
    Assets.acc_filename = Assets.texture_root_path + "ACCPACK.PAK";
    Assets.convpak_filename = Assets.gameflow_root_path + "CONV2.PAK";
    SCMouse::getInstance().init();
    RSFontManager::getInstance().init();
    ConvAssetManager::getInstance().init();
    RSSound::getInstance().init();
    RSMixer::getInstance().init();
    printf("CD MENU: donnees de Tactical Operations chargees\n");
}
