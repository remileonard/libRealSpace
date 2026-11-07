//
//  SCFlightOptions.cpp
//  libRealSpace
//
//  Panneau CONFIGURATION du vol (ALT+O) : écrans, boutons et navigation (SCFlightOptions.h).
//

#include "precomp.h"

SCFlightOptions::SCFlightOptions() {
}

SCFlightOptions::~SCFlightOptions() {
}

// Options_DrawPanel_8FA4C : titre en (5, 4), fond en (5, 39).
void SCFlightOptions::loadScreen(Screen screen, const char *file) {
    OptionsScreen &target = this->screens[screen];
    std::string path = std::string("..\\..\\DATA\\FONTS\\") + file;
    TreEntry *entry = Assets.GetEntryByName(path.c_str());
    if (entry == nullptr) {
        printf("OPTIONS: %s absent\n", path.c_str());
        return;
    }
    target.pack.InitFromRAM(file, entry->data, entry->size);
    PakEntry *title = target.pack.GetEntry(IMAGE_TITLE);
    target.title.init(title->data, title->size);
    Point2D title_position;
    title_position.x = 5;
    title_position.y = 4;
    target.title.SetPosition(&title_position);
    Point2D panel_position;
    panel_position.x = 5;
    panel_position.y = 39;
    target.panel.init(target.pack.GetEntry(IMAGE_BACKGROUND), panel_position);
    target.loaded = true;
}

// Distances de vue des cinq cases HAZING, de FAR à NEAR (m).
const int SCFlightOptions::HAZING_DISTANCES[5] = {
    160000,
    130000,
    100000,
    70000,
    40000,
};

const int SCFlightOptions::LOD_UP_IMAGES[3] = {
    DETAIL_LOD_HI_UP,
    DETAIL_LOD_MEDIUM_UP,
    DETAIL_LOD_LO_UP,
};

const int SCFlightOptions::LOD_DOWN_IMAGES[3] = {
    DETAIL_LOD_HI_DOWN,
    DETAIL_LOD_MEDIUM_DOWN,
    DETAIL_LOD_LO_DOWN,
};

const int SCFlightOptions::HAZING_UP_IMAGES[5] = {
    DETAIL_HAZING_FAR_UP,
    DETAIL_HAZING_2_UP,
    DETAIL_HAZING_3_UP,
    DETAIL_HAZING_4_UP,
    DETAIL_HAZING_NEAR_UP,
};

const int SCFlightOptions::HAZING_DOWN_IMAGES[5] = {
    DETAIL_HAZING_FAR_DOWN,
    DETAIL_HAZING_2_DOWN,
    DETAIL_HAZING_3_DOWN,
    DETAIL_HAZING_4_DOWN,
    DETAIL_HAZING_NEAR_DOWN,
};

// Position (x, y) à l'écran, comme dans le code du jeu : ramenée au coin du panneau (5, 39).
SCButton *SCFlightOptions::addButton(Screen screen, int x, int y, int width, int height, int up, int down,
                                     std::function<void()> action) {
    OptionsScreen &target = this->screens[screen];
    if (!target.loaded) {
        return nullptr;
    }
    return target.panel.addButton(x - target.panel.position.x, y - target.panel.position.y, width, height,
                                  target.pack.GetEntry(up), target.pack.GetEntry(down), action);
}

SCToggleButton *SCFlightOptions::addToggleButton(Screen screen, int x, int y, int width, int height, int up, int down,
                                                 std::function<void()> action) {
    OptionsScreen &target = this->screens[screen];
    if (!target.loaded) {
        return nullptr;
    }
    return target.panel.addToggleButton(x - target.panel.position.x, y - target.panel.position.y, width, height,
                                        target.pack.GetEntry(up), target.pack.GetEntry(down), action);
}

// Boutons d'Options_ConfigurationScreen_90D20 et des sept sous-écrans (OPTIONS_SCREEN.md § 5.2). Les boutons à image
// dynamique prennent leur paire par défaut (§ 5.4).
void SCFlightOptions::init() {
    this->loadScreen(SCREEN_MAIN, "MAINOPT.SHP");
    this->loadScreen(SCREEN_DETAIL, "DETLOPT.SHP");
    this->loadScreen(SCREEN_FLIGHT, "FLITOPT.SHP");
    this->loadScreen(SCREEN_GAMEPLAY, "GAMEOPT.SHP");
    this->loadScreen(SCREEN_COCKPIT, "COCKOPT.SHP");
    this->loadScreen(SCREEN_CAMERAS, "CAMROPT.SHP");
    this->loadScreen(SCREEN_AUDIO, "AUDIOPT.SHP");
    this->loadScreen(SCREEN_EXIT_GAME, "RESTART.SHP");

    std::function<void()> none = std::bind(&SCFlightOptions::noAction, this);
    std::function<void()> back = std::bind(&SCFlightOptions::goTo, this, SCREEN_MAIN);

    // CONFIGURATION (MAINOPT.SHP)
    this->addButton(SCREEN_MAIN, 36, 61, 66, 11, MAIN_DETAIL_UP, MAIN_DETAIL_DOWN,
                    std::bind(&SCFlightOptions::goTo, this, SCREEN_DETAIL));
    this->addButton(SCREEN_MAIN, 106, 61, 66, 11, MAIN_FLIGHT_UP, MAIN_FLIGHT_DOWN,
                    std::bind(&SCFlightOptions::goTo, this, SCREEN_FLIGHT));
    this->addButton(SCREEN_MAIN, 36, 74, 66, 11, MAIN_GAMEPLAY_UP, MAIN_GAMEPLAY_DOWN,
                    std::bind(&SCFlightOptions::goTo, this, SCREEN_GAMEPLAY));
    this->addButton(SCREEN_MAIN, 106, 74, 66, 11, MAIN_COCKPIT_UP, MAIN_COCKPIT_DOWN,
                    std::bind(&SCFlightOptions::goTo, this, SCREEN_COCKPIT));
    this->addButton(SCREEN_MAIN, 36, 87, 66, 11, MAIN_CAMERAS_UP, MAIN_CAMERAS_DOWN,
                    std::bind(&SCFlightOptions::goTo, this, SCREEN_CAMERAS));
    this->addButton(SCREEN_MAIN, 106, 87, 66, 11, MAIN_AUDIO_UP, MAIN_AUDIO_DOWN,
                    std::bind(&SCFlightOptions::goTo, this, SCREEN_AUDIO));
    this->addButton(SCREEN_MAIN, 211, 61, 66, 11, MAIN_ROOKIE_UP, MAIN_ROOKIE_DOWN, none);
    this->addButton(SCREEN_MAIN, 211, 74, 66, 11, MAIN_VETERAN_UP, MAIN_VETERAN_DOWN, none);
    this->addButton(SCREEN_MAIN, 211, 87, 66, 11, MAIN_ACE_UP, MAIN_ACE_DOWN, none);
    this->addButton(SCREEN_MAIN, 18, 114, 48, 11, MAIN_SAVE_UP, MAIN_SAVE_DOWN,
                    std::bind(&SCFlightOptions::onSave, this));
    this->addButton(SCREEN_MAIN, 73, 114, 59, 11, MAIN_DEFAULT_UP, MAIN_DEFAULT_DOWN, none);
    this->addButton(SCREEN_MAIN, 139, 114, 49, 11, MAIN_EXIT_UP, MAIN_EXIT_DOWN,
                    std::bind(&SCFlightOptions::requestClose, this));
    this->addButton(SCREEN_MAIN, 211, 114, 66, 11, MAIN_EXIT_GAME_UP, MAIN_EXIT_GAME_DOWN,
                    std::bind(&SCFlightOptions::goTo, this, SCREEN_EXIT_GAME));

    // DETAIL (DETLOPT.SHP)
    this->terrain_gouraud_button =
        this->addToggleButton(SCREEN_DETAIL, 22, 67, 73, 11, DETAIL_TERRAIN_GOURAUD_UP, DETAIL_TERRAIN_GOURAUD_DOWN,
                              std::bind(&SCFlightOptions::onTerrainGouraud, this));
    this->addButton(SCREEN_DETAIL, 22, 80, 73, 11, DETAIL_TRANSITION_UP, DETAIL_TRANSITION_DOWN, none);
    this->addButton(SCREEN_DETAIL, 22, 93, 73, 11, DETAIL_INTEREST_UP, DETAIL_INTEREST_DOWN, none);
    this->addButton(SCREEN_DETAIL, 22, 106, 73, 11, DETAIL_RIVERS_UP, DETAIL_RIVERS_DOWN, none);
    this->addButton(SCREEN_DETAIL, 22, 119, 73, 11, DETAIL_FIELDS_UP, DETAIL_FIELDS_DOWN, none);
    this->addButton(SCREEN_DETAIL, 22, 132, 73, 11, DETAIL_CITIES_UP, DETAIL_CITIES_DOWN, none);
    this->addButton(SCREEN_DETAIL, 22, 145, 73, 11, DETAIL_RUNWAYS_UP, DETAIL_RUNWAYS_DOWN, none);
    this->object_gouraud_button =
        this->addToggleButton(SCREEN_DETAIL, 226, 67, 73, 11, DETAIL_OBJECT_GOURAUD_UP, DETAIL_OBJECT_GOURAUD_DOWN,
                              std::bind(&SCFlightOptions::onObjectGouraud, this));
    this->texture_button = this->addToggleButton(SCREEN_DETAIL, 226, 80, 73, 11, DETAIL_TEXTURE_UP, DETAIL_TEXTURE_DOWN,
                                                 std::bind(&SCFlightOptions::onTexture, this));
    // Cases HI à LO en (243 + 14 × k, 93) ; FAR à NEAR en (127 + 14 × k, 159).
    for (int k = 0; k < 3; k++) {
        this->lod_buttons.push_back(this->addToggleButton(SCREEN_DETAIL, 243 + 14 * k, 93, 13, 12, LOD_UP_IMAGES[k],
                                                          LOD_DOWN_IMAGES[k], std::bind(&SCFlightOptions::onLod, this, k)));
    }
    for (int k = 0; k < 5; k++) {
        this->hazing_buttons.push_back(this->addToggleButton(SCREEN_DETAIL, 127 + 14 * k, 159, 13, 12, HAZING_UP_IMAGES[k],
                                                             HAZING_DOWN_IMAGES[k],
                                                             std::bind(&SCFlightOptions::onHazing, this, k)));
    }
    this->addButton(SCREEN_DETAIL, 244, 132, 38, 11, DETAIL_EXIT_UP, DETAIL_EXIT_DOWN, back);

    // FLIGHT (FLITOPT.SHP) : CALIBRATE JOYSTICK prend sa paire par défaut.
    this->addButton(SCREEN_FLIGHT, 121, 62, 76, 17, FLIGHT_MIDAIR_COLLISIONS_UP, FLIGHT_MIDAIR_COLLISIONS_DOWN, none);
    this->addButton(SCREEN_FLIGHT, 121, 81, 76, 17, FLIGHT_EASY_LANDINGS_UP, FLIGHT_EASY_LANDINGS_DOWN, none);
    this->addButton(SCREEN_FLIGHT, 121, 100, 76, 17, FLIGHT_SHOW_WEAPONS_UP, FLIGHT_SHOW_WEAPONS_DOWN, none);
    this->addButton(SCREEN_FLIGHT, 121, 119, 76, 11, FLIGHT_STALLS_UP, FLIGHT_STALLS_DOWN, none);
    this->addButton(SCREEN_FLIGHT, 93, 146, 136, 11, FLIGHT_CALIBRATE_JOYSTICK_UP, FLIGHT_CALIBRATE_JOYSTICK_DOWN,
                    none);
    this->addButton(SCREEN_FLIGHT, 131, 176, 59, 11, FLIGHT_EXIT_UP, FLIGHT_EXIT_DOWN, back);

    // GAMEPLAY (GAMEOPT.SHP)
    this->addButton(SCREEN_GAMEPLAY, 48, 70, 73, 17, GAMEPLAY_UNLIMITED_AMMO_UP, GAMEPLAY_UNLIMITED_AMMO_DOWN, none);
    this->addButton(SCREEN_GAMEPLAY, 48, 90, 73, 17, GAMEPLAY_EASY_GUN_HITS_UP, GAMEPLAY_EASY_GUN_HITS_DOWN, none);
    this->addButton(SCREEN_GAMEPLAY, 129, 122, 59, 11, GAMEPLAY_EXIT_UP, GAMEPLAY_EXIT_DOWN, back);

    // COCKPIT (COCKOPT.SHP)
    this->addButton(SCREEN_COCKPIT, 45, 62, 111, 11, COCKPIT_LADDER_UP, COCKPIT_LADDER_DOWN, none);
    this->addButton(SCREEN_COCKPIT, 45, 75, 111, 11, COCKPIT_TAPES_UP, COCKPIT_TAPES_DOWN, none);
    this->addButton(SCREEN_COCKPIT, 45, 88, 111, 11, COCKPIT_AUTO_TARGET_UP, COCKPIT_AUTO_TARGET_DOWN, none);
    this->addButton(SCREEN_COCKPIT, 45, 101, 111, 11, COCKPIT_360_TARGET_LOCK_UP, COCKPIT_360_TARGET_LOCK_DOWN, none);
    this->addButton(SCREEN_COCKPIT, 45, 114, 111, 11, COCKPIT_SMART_TARGETING_UP, COCKPIT_SMART_TARGETING_DOWN, none);
    this->addButton(SCREEN_COCKPIT, 162, 62, 111, 11, COCKPIT_SUN_GLARE_UP, COCKPIT_SUN_GLARE_DOWN, none);
    this->addButton(SCREEN_COCKPIT, 162, 75, 111, 11, COCKPIT_G_EFFECTS_UP, COCKPIT_G_EFFECTS_DOWN, none);
    this->addButton(SCREEN_COCKPIT, 162, 88, 111, 11, COCKPIT_SMART_RADAR_UP, COCKPIT_SMART_RADAR_DOWN, none);
    this->addButton(SCREEN_COCKPIT, 162, 101, 111, 11, COCKPIT_360_AA_RADAR_UP, COCKPIT_360_AA_RADAR_DOWN, none);
    this->addButton(SCREEN_COCKPIT, 162, 114, 111, 11, COCKPIT_SMART_RAW_SCOPE_UP, COCKPIT_SMART_RAW_SCOPE_DOWN, none);
    this->addButton(SCREEN_COCKPIT, 129, 142, 59, 11, COCKPIT_EXIT_UP, COCKPIT_EXIT_DOWN, back);

    // CAMERAS (CAMROPT.SHP) : TERRAIN IN GUN CAMERA prend sa paire par défaut.
    this->addButton(SCREEN_CAMERAS, 22, 60, 136, 11, CAMERAS_AUTO_SWITCHING_UP, CAMERAS_AUTO_SWITCHING_DOWN, none);
    this->addButton(SCREEN_CAMERAS, 164, 60, 136, 11, CAMERAS_VICTIM_VIEW_UP, CAMERAS_VICTIM_VIEW_DOWN, none);
    this->addButton(SCREEN_CAMERAS, 22, 73, 136, 11, CAMERAS_WEAPON_CAMERA_UP, CAMERAS_WEAPON_CAMERA_DOWN, none);
    this->addButton(SCREEN_CAMERAS, 164, 73, 136, 11, CAMERAS_TERRAIN_IN_GUN_CAMERA_UP,
                    CAMERAS_TERRAIN_IN_GUN_CAMERA_DOWN, none);
    this->addButton(SCREEN_CAMERAS, 129, 124, 59, 11, CAMERAS_EXIT_UP, CAMERAS_EXIT_DOWN, back);

    // AUDIO (AUDIOPT.SHP) : paires par défaut.
    this->addButton(SCREEN_AUDIO, 88, 63, 72, 11, AUDIO_MUSIC_UP, AUDIO_MUSIC_DOWN, none);
    this->addButton(SCREEN_AUDIO, 88, 76, 72, 11, AUDIO_SOUND_FX_UP, AUDIO_SOUND_FX_DOWN, none);
    this->addButton(SCREEN_AUDIO, 165, 63, 72, 11, AUDIO_SPEECH_UP, AUDIO_SPEECH_DOWN, none);
    this->addButton(SCREEN_AUDIO, 165, 76, 72, 11, AUDIO_DIGITAL_FX_UP, AUDIO_DIGITAL_FX_DOWN, none);
    this->addButton(SCREEN_AUDIO, 129, 101, 59, 11, AUDIO_EXIT_UP, AUDIO_EXIT_DOWN, back);

    // EXIT GAME (RESTART.SHP) : élément vertical en mode 0.
    this->addButton(SCREEN_EXIT_GAME, 54, 80, 66, 11, EXIT_GAME_RESTART_UP, EXIT_GAME_RESTART_DOWN, none);
    this->addButton(SCREEN_EXIT_GAME, 126, 80, 66, 11, EXIT_GAME_ABORT_UP, EXIT_GAME_ABORT_DOWN, none);
    this->addButton(SCREEN_EXIT_GAME, 198, 80, 69, 11, EXIT_GAME_CONTINUE_UP, EXIT_GAME_CONTINUE_DOWN, back);
    this->addButton(SCREEN_EXIT_GAME, 290, 55, 11, 29, EXIT_GAME_VERTICAL_UP, EXIT_GAME_VERTICAL_DOWN, none);
}

void SCFlightOptions::open() {
    this->refreshDetail();
    this->current = SCREEN_MAIN;
    this->screen_pending = false;
    this->close_pending = false;
    this->is_open = true;
    printf("OPTIONS: panneau ouvert\n");
}

void SCFlightOptions::checkButtons() {
    this->screens[this->current].panel.checkButtons();
}

void SCFlightOptions::draw(FrameBuffer *frameBuffer) {
    OptionsScreen &target = this->screens[this->current];
    if (!target.loaded) {
        return;
    }
    frameBuffer->drawShape(&target.title);
    target.panel.draw(frameBuffer);
    if (this->current == SCREEN_DETAIL) {
        this->drawDetailPreview(frameBuffer);
    }
}

// Image de DETLOPT.SHP en coordonnées locales, origine en (114, 69) : coin de la zone vide au centre du fond.
void SCFlightOptions::drawPreviewImage(FrameBuffer *frameBuffer, int image) {
    PakEntry *entry = this->screens[SCREEN_DETAIL].pack.GetEntry(image);
    if (entry == nullptr) {
        return;
    }
    RLEShape shape;
    shape.init(entry->data, entry->size);
    Point2D position;
    position.x = 114 - shape.leftDist;
    position.y = 69 - shape.topDist;
    shape.SetPosition(&position);
    frameBuffer->drawShape(&shape);
}

// Options_DetailPreview_Draw_91B70 : images dessinées l'une sur l'autre, dans cet ordre.
void SCFlightOptions::drawDetailPreview(FrameBuffer *frameBuffer) {
    if (!this->screens[SCREEN_DETAIL].loaded) {
        return;
    }
    const DetailPreview &p = this->detail_preview;
    this->drawPreviewImage(frameBuffer, p.terrain_gouraud ? DETAIL_PREVIEW_TERRAIN_GOURAUD : DETAIL_PREVIEW_TERRAIN_FLAT);
    if (p.transition) {
        this->drawPreviewImage(frameBuffer, DETAIL_PREVIEW_TRANSITION);
    }
    if (p.rivers) {
        this->drawPreviewImage(frameBuffer, DETAIL_PREVIEW_RIVERS);
    }
    if (p.interest) {
        this->drawPreviewImage(frameBuffer, DETAIL_PREVIEW_INTEREST);
    }
    if (p.fields) {
        this->drawPreviewImage(frameBuffer, DETAIL_PREVIEW_FIELDS);
    }
    if (p.cities) {
        this->drawPreviewImage(frameBuffer, DETAIL_PREVIEW_CITIES);
    }
    this->drawPreviewImage(frameBuffer, p.runways ? DETAIL_PREVIEW_RUNWAYS : DETAIL_PREVIEW_NO_RUNWAYS);
    this->drawPreviewImage(frameBuffer, p.object_gouraud ? DETAIL_PREVIEW_OBJECT_GOURAUD : DETAIL_PREVIEW_OBJECT_FLAT);
    if (p.texture) {
        this->drawPreviewImage(frameBuffer, DETAIL_PREVIEW_TEXTURED);
        this->drawPreviewImage(frameBuffer, DETAIL_PREVIEW_OBJECT_TEXTURE);
    } else {
        this->drawPreviewImage(frameBuffer, DETAIL_PREVIEW_UNTEXTURED);
    }
}

void SCFlightOptions::applyPending() {
    if (this->close_pending) {
        this->close_pending = false;
        this->screen_pending = false;
        this->is_open = false;
        printf("OPTIONS: panneau fermé\n");
        return;
    }
    if (this->screen_pending) {
        this->screen_pending = false;
        this->current = this->pending_screen;
        printf("OPTIONS: écran %d\n", (int)this->current);
    }
}

void SCFlightOptions::goTo(Screen screen) {
    this->pending_screen = screen;
    this->screen_pending = true;
}

void SCFlightOptions::requestClose() {
    this->close_pending = true;
}

void SCFlightOptions::noAction() {
}

// Bouton qui porte l'état d'un réglage : enfoncé si le réglage est actif, relevé sinon.
void SCFlightOptions::showState(SCToggleButton *button, bool active) {
    if (button == nullptr) {
        return;
    }
    button->setActive(active);
}

void SCFlightOptions::showSelected(std::vector<SCToggleButton *> &boxes, int selected) {
    for (int k = 0; k < (int)boxes.size(); k++) {
        this->showState(boxes[k], k == selected);
    }
}

void SCFlightOptions::refreshDetail() {
    this->detail_preview.terrain_gouraud = !this->area->isTerrainFlat();
    this->showState(this->terrain_gouraud_button, !this->area->isTerrainFlat());
    this->detail_preview.object_gouraud = Renderer.object_gouraud;
    this->showState(this->object_gouraud_button, Renderer.object_gouraud);
    this->detail_preview.texture = Renderer.show_textured;
    this->showState(this->texture_button, Renderer.show_textured);
    int lod = Renderer.lodLevel;
    if (lod < 0) {
        lod = 0;
    }
    if (lod > 2) {
        lod = 2;
    }
    this->showSelected(this->lod_buttons, lod);
    int hazing = 0;
    for (int k = 1; k < 5; k++) {
        if (std::abs(HAZING_DISTANCES[k] - Renderer.max_view_distance) <
            std::abs(HAZING_DISTANCES[hazing] - Renderer.max_view_distance)) {
            hazing = k;
        }
    }
    this->showSelected(this->hazing_buttons, hazing);
}

// SAVE : écrit les réglages dans le fichier lu au démarrage par les main.cpp (config.load("./assets/config.ini")).
void SCFlightOptions::onSave() {
    config.save("./assets/config.ini");
    printf("OPTIONS: réglages enregistrés dans ./assets/config.ini\n");
}

// TERRAIN GOURAUD : teintes du terrain dégradées entre les sommets, ou une teinte par triangle (RSArea::terrain_flat,
// [Game] terrain_flat). Les couleurs des triangles sont recalculées tout de suite.
void SCFlightOptions::onTerrainGouraud() {
    this->area->setTerrainFlat(!this->area->isTerrainFlat());
    config.setBool("Game", "terrain_flat", this->area->isTerrainFlat());
    this->refreshDetail();
    printf("OPTIONS: terrain gouraud %s\n", this->area->isTerrainFlat() ? "non" : "oui");
}

// OBJECTS GOURAUD : objets éclairés par les normales des sommets, ou par celle de la face (SCRenderer::object_gouraud,
// [Game] object_gouraud).
void SCFlightOptions::onObjectGouraud() {
    Renderer.object_gouraud = !Renderer.object_gouraud;
    config.setBool("Game", "object_gouraud", Renderer.object_gouraud);
    this->refreshDetail();
    printf("OPTIONS: objets gouraud %s\n", Renderer.object_gouraud ? "oui" : "non");
}

// TEXTURE : textures des objets (SCRenderer::show_textured, [Game] show_texture).
void SCFlightOptions::onTexture() {
    Renderer.show_textured = !Renderer.show_textured;
    config.setBool("Game", "show_texture", Renderer.show_textured);
    this->refreshDetail();
    printf("OPTIONS: textures %s\n", Renderer.show_textured ? "oui" : "non");
}

// HI à LO : niveau de détail des objets (SCRenderer::lodLevel, [Game] object_detail ; 0 = le plus détaillé).
void SCFlightOptions::onLod(int level) {
    Renderer.lodLevel = level;
    config.setInt("Game", "object_detail", level);
    this->refreshDetail();
    printf("OPTIONS: niveau de détail %d\n", level);
}

// FAR à NEAR : distance de vue (SCRenderer::max_view_distance, [Game] max_view_distance).
void SCFlightOptions::onHazing(int box) {
    Renderer.max_view_distance = HAZING_DISTANCES[box];
    Renderer.resetCameraPerspective();
    config.setInt("Game", "max_view_distance", HAZING_DISTANCES[box]);
    this->refreshDetail();
    printf("OPTIONS: distance de vue %d m\n", HAZING_DISTANCES[box]);
}
