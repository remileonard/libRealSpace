//
//  SCFlightOptions.h
//  libRealSpace
//
//  Panneau CONFIGURATION du vol (ALT+O, Options_ConfigurationScreen_90D20 de STRIKE.EXE) et ses sept sous-écrans,
//  dessinés par SCStrike par-dessus l'image figée du vol. Chaque écran est un fichier FONTS\*.SHP : image 0 (bandeau du
//  titre) en (5, 4), image 1 (fond, avec tous les libellés) en (5, 39), puis les boutons, image normale et image
//  enfoncée. strike_commander_re/analysis/OPTIONS_SCREEN.md § 2 et § 5.
//

#pragma once
#include "precomp.h"
#include "../engine/SCPanel.h"

class SCFlightOptions {
public:
    enum Screen {
        SCREEN_MAIN,
        SCREEN_DETAIL,
        SCREEN_FLIGHT,
        SCREEN_GAMEPLAY,
        SCREEN_COCKPIT,
        SCREEN_CAMERAS,
        SCREEN_AUDIO,
        SCREEN_EXIT_GAME,
        SCREEN_TOTAL
    };

    bool is_open{ false };
    // Terrain du vol en cours (TERRAIN GOURAUD), donné par SCStrike à l'ouverture.
    RSArea *area{ nullptr };

    // Aperçu de l'écran DETAIL : un drapeau par bouton, qui choisit les images dessinées (à relier à Config).
    struct DetailPreview {
        bool terrain_gouraud{ true };
        bool transition{ true };
        bool interest{ true };
        bool rivers{ true };
        bool fields{ true };
        bool cities{ true };
        bool runways{ true };
        bool object_gouraud{ true };
        bool texture{ true };
    };
    DetailPreview detail_preview;

    SCFlightOptions();
    ~SCFlightOptions();
    void init();
    // Ouvre le panneau sur l'écran principal.
    void open();
    // Souris sur les boutons de l'écran affiché (SCPanel::checkButtons).
    void checkButtons();
    // Bandeau, fond et boutons de l'écran affiché.
    void draw(FrameBuffer *frameBuffer);
    // Changement d'écran ou fermeture demandés par un clic, appliqués après checkButtons (un bouton ne
    // change pas l'écran pendant sa propre action).
    void applyPending();

private:
    // Numéros d'image dans les fichiers FONTS\*.SHP. Les images 0 et 1 sont les mêmes pour tous les écrans ; un bouton
    // a une image relevée (UP) et une image enfoncée (DOWN).
    enum ScreenImage {
        IMAGE_TITLE = 0,
        IMAGE_BACKGROUND = 1,
    };
    // MAINOPT.SHP
    enum MainImage {
        MAIN_DETAIL_UP = 2,
        MAIN_DETAIL_DOWN = 3,
        MAIN_GAMEPLAY_UP = 4,
        MAIN_GAMEPLAY_DOWN = 5,
        MAIN_CAMERAS_UP = 6,
        MAIN_CAMERAS_DOWN = 7,
        MAIN_FLIGHT_UP = 8,
        MAIN_FLIGHT_DOWN = 9,
        MAIN_COCKPIT_UP = 10,
        MAIN_COCKPIT_DOWN = 11,
        MAIN_AUDIO_UP = 12,
        MAIN_AUDIO_DOWN = 13,
        MAIN_ROOKIE_UP = 14,
        MAIN_ROOKIE_DOWN = 15,
        MAIN_VETERAN_UP = 16,
        MAIN_VETERAN_DOWN = 17,
        MAIN_ACE_UP = 18,
        MAIN_ACE_DOWN = 19,
        MAIN_SAVE_UP = 20,
        MAIN_SAVE_DOWN = 21,
        MAIN_DEFAULT_UP = 22,
        MAIN_DEFAULT_DOWN = 23,
        MAIN_EXIT_UP = 24,
        MAIN_EXIT_DOWN = 25,
        MAIN_EXIT_GAME_UP = 26,
        MAIN_EXIT_GAME_DOWN = 27,
    };
    // DETLOPT.SHP : cases HI à LO et FAR à NEAR de gauche à droite ; images de l'aperçu, origine en (114, 69).
    enum DetailImage {
        DETAIL_TERRAIN_GOURAUD_UP = 2,
        DETAIL_TERRAIN_GOURAUD_DOWN = 3,
        DETAIL_TRANSITION_UP = 4,
        DETAIL_TRANSITION_DOWN = 5,
        DETAIL_INTEREST_UP = 6,
        DETAIL_INTEREST_DOWN = 7,
        DETAIL_RIVERS_UP = 8,
        DETAIL_RIVERS_DOWN = 9,
        DETAIL_FIELDS_UP = 10,
        DETAIL_FIELDS_DOWN = 11,
        DETAIL_CITIES_UP = 12,
        DETAIL_CITIES_DOWN = 13,
        DETAIL_RUNWAYS_UP = 14,
        DETAIL_RUNWAYS_DOWN = 15,
        DETAIL_HAZING_FAR_UP = 16,
        DETAIL_HAZING_FAR_DOWN = 17,
        DETAIL_HAZING_2_UP = 18,
        DETAIL_HAZING_2_DOWN = 19,
        DETAIL_HAZING_3_UP = 20,
        DETAIL_HAZING_3_DOWN = 21,
        DETAIL_HAZING_4_UP = 22,
        DETAIL_HAZING_4_DOWN = 23,
        DETAIL_HAZING_NEAR_UP = 24,
        DETAIL_HAZING_NEAR_DOWN = 25,
        DETAIL_OBJECT_GOURAUD_UP = 26,
        DETAIL_OBJECT_GOURAUD_DOWN = 27,
        DETAIL_TEXTURE_UP = 28,
        DETAIL_TEXTURE_DOWN = 29,
        DETAIL_LOD_HI_UP = 30,
        DETAIL_LOD_HI_DOWN = 31,
        DETAIL_LOD_MEDIUM_UP = 32,
        DETAIL_LOD_MEDIUM_DOWN = 33,
        DETAIL_LOD_LO_UP = 34,
        DETAIL_LOD_LO_DOWN = 35,
        DETAIL_EXIT_UP = 36,
        DETAIL_EXIT_DOWN = 37,
        DETAIL_PREVIEW_TERRAIN_FLAT = 38,
        DETAIL_PREVIEW_TERRAIN_GOURAUD = 39,
        DETAIL_PREVIEW_TRANSITION = 40,
        DETAIL_PREVIEW_RIVERS = 41,
        DETAIL_PREVIEW_INTEREST = 42,
        DETAIL_PREVIEW_FIELDS = 43,
        DETAIL_PREVIEW_CITIES = 44,
        DETAIL_PREVIEW_NO_RUNWAYS = 45,
        DETAIL_PREVIEW_RUNWAYS = 46,
        DETAIL_PREVIEW_UNTEXTURED = 47,
        DETAIL_PREVIEW_TEXTURED = 48,
        DETAIL_PREVIEW_OBJECT_GOURAUD = 49,
        DETAIL_PREVIEW_OBJECT_FLAT = 50,
        DETAIL_PREVIEW_OBJECT_TEXTURE = 51,
    };
    // FLITOPT.SHP
    enum FlightImage {
        FLIGHT_MIDAIR_COLLISIONS_UP = 29,
        FLIGHT_MIDAIR_COLLISIONS_DOWN = 30,
        FLIGHT_EASY_LANDINGS_UP = 31,
        FLIGHT_EASY_LANDINGS_DOWN = 32,
        FLIGHT_SHOW_WEAPONS_UP = 33,
        FLIGHT_SHOW_WEAPONS_DOWN = 34,
        FLIGHT_STALLS_UP = 36,
        FLIGHT_STALLS_DOWN = 37,
        FLIGHT_EXIT_UP = 38,
        FLIGHT_EXIT_DOWN = 39,
        FLIGHT_CALIBRATE_JOYSTICK_UP = 40,
        FLIGHT_CALIBRATE_JOYSTICK_DOWN = 41,
    };
    // GAMEOPT.SHP
    enum GameplayImage {
        GAMEPLAY_UNLIMITED_AMMO_UP = 2,
        GAMEPLAY_UNLIMITED_AMMO_DOWN = 3,
        GAMEPLAY_EASY_GUN_HITS_UP = 4,
        GAMEPLAY_EASY_GUN_HITS_DOWN = 5,
        GAMEPLAY_EXIT_UP = 12,
        GAMEPLAY_EXIT_DOWN = 13,
    };
    // COCKOPT.SHP
    enum CockpitImage {
        COCKPIT_LADDER_UP = 2,
        COCKPIT_LADDER_DOWN = 3,
        COCKPIT_TAPES_UP = 4,
        COCKPIT_TAPES_DOWN = 5,
        COCKPIT_AUTO_TARGET_UP = 6,
        COCKPIT_AUTO_TARGET_DOWN = 7,
        COCKPIT_360_TARGET_LOCK_UP = 8,
        COCKPIT_360_TARGET_LOCK_DOWN = 9,
        COCKPIT_SMART_TARGETING_UP = 10,
        COCKPIT_SMART_TARGETING_DOWN = 11,
        COCKPIT_SUN_GLARE_UP = 12,
        COCKPIT_SUN_GLARE_DOWN = 13,
        COCKPIT_G_EFFECTS_UP = 14,
        COCKPIT_G_EFFECTS_DOWN = 15,
        COCKPIT_SMART_RADAR_UP = 16,
        COCKPIT_SMART_RADAR_DOWN = 17,
        COCKPIT_360_AA_RADAR_UP = 18,
        COCKPIT_360_AA_RADAR_DOWN = 19,
        COCKPIT_SMART_RAW_SCOPE_UP = 20,
        COCKPIT_SMART_RAW_SCOPE_DOWN = 21,
        COCKPIT_EXIT_UP = 22,
        COCKPIT_EXIT_DOWN = 23,
    };
    // CAMROPT.SHP
    enum CamerasImage {
        CAMERAS_AUTO_SWITCHING_UP = 2,
        CAMERAS_AUTO_SWITCHING_DOWN = 3,
        CAMERAS_VICTIM_VIEW_UP = 4,
        CAMERAS_VICTIM_VIEW_DOWN = 5,
        CAMERAS_WEAPON_CAMERA_UP = 6,
        CAMERAS_WEAPON_CAMERA_DOWN = 7,
        CAMERAS_TERRAIN_IN_GUN_CAMERA_UP = 8,
        CAMERAS_TERRAIN_IN_GUN_CAMERA_DOWN = 9,
        CAMERAS_EXIT_UP = 17,
        CAMERAS_EXIT_DOWN = 18,
    };
    // AUDIOPT.SHP
    enum AudioImage {
        AUDIO_MUSIC_UP = 2,
        AUDIO_MUSIC_DOWN = 3,
        AUDIO_SPEECH_UP = 5,
        AUDIO_SPEECH_DOWN = 6,
        AUDIO_SOUND_FX_UP = 8,
        AUDIO_SOUND_FX_DOWN = 9,
        AUDIO_DIGITAL_FX_UP = 11,
        AUDIO_DIGITAL_FX_DOWN = 12,
        AUDIO_EXIT_UP = 14,
        AUDIO_EXIT_DOWN = 15,
    };
    // RESTART.SHP : élément vertical à droite, rôle non établi.
    enum ExitGameImage {
        EXIT_GAME_RESTART_UP = 2,
        EXIT_GAME_RESTART_DOWN = 3,
        EXIT_GAME_ABORT_UP = 4,
        EXIT_GAME_ABORT_DOWN = 5,
        EXIT_GAME_CONTINUE_UP = 6,
        EXIT_GAME_CONTINUE_DOWN = 7,
        EXIT_GAME_VERTICAL_UP = 8,
        EXIT_GAME_VERTICAL_DOWN = 9,
    };

    struct OptionsScreen {
        PakArchive pack;
        RLEShape title;
        SCPanel panel;
        bool loaded{ false };
    };
    OptionsScreen screens[SCREEN_TOTAL];
    Screen current{ SCREEN_MAIN };
    Screen pending_screen{ SCREEN_MAIN };
    bool screen_pending{ false };
    bool close_pending{ false };
    AssetManager &Assets = AssetManager::getInstance();
    SCRenderer &Renderer = SCRenderer::getInstance();
    Config &config = Config::getInstance();
    // Cases de l'écran DETAIL : HI à LO (niveau de détail des objets), FAR à NEAR (distance de vue).
    SCToggleButton *terrain_gouraud_button{ nullptr };
    SCToggleButton *object_gouraud_button{ nullptr };
    SCToggleButton *texture_button{ nullptr };
    std::vector<SCToggleButton *> lod_buttons;
    std::vector<SCToggleButton *> hazing_buttons;
    static const int HAZING_DISTANCES[5];

    void onSave();
    void onTerrainGouraud();
    void onObjectGouraud();
    void onTexture();
    void onLod(int level);
    void onHazing(int box);
    // Aperçu et cases de DETAIL d'après les réglages actuels de SCRenderer.
    void refreshDetail();
    // La case choisie reste enfoncée, les autres sont relevées.
    void showSelected(std::vector<SCToggleButton *> &boxes, int selected);
    void showState(SCToggleButton *button, bool active);
    // Images des cases HI à LO et FAR à NEAR, de gauche à droite.
    static const int LOD_UP_IMAGES[3];
    static const int LOD_DOWN_IMAGES[3];
    static const int HAZING_UP_IMAGES[5];
    static const int HAZING_DOWN_IMAGES[5];

    void loadScreen(Screen screen, const char *file);
    void drawDetailPreview(FrameBuffer *frameBuffer);
    void drawPreviewImage(FrameBuffer *frameBuffer, int image);
    SCButton *addButton(Screen screen, int x, int y, int width, int height, int up, int down,
                        std::function<void()> action);
    SCToggleButton *addToggleButton(Screen screen, int x, int y, int width, int height, int up, int down,
                                    std::function<void()> action);
    void goTo(Screen screen);
    void requestClose();
    // Bouton posé, sans action pour l'instant.
    void noAction();
};
