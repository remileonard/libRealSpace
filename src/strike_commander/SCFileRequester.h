#pragma once
#include "precomp.h"
#include <functional>
#include <string>
#include <vector>
#include "../engine/keyboard.h"  // Inclure notre nouveau gestionnaire d'événements


class SCFileRequester {
private:
    AssetManager &Assets = AssetManager::getInstance();
    GameEngine *Game{nullptr};
    RSVGA &VGA = RSVGA::getInstance();
    RSFontManager &FontManager = RSFontManager::getInstance();
    SCMouse &Mouse = SCMouse::getInstance();
public:
    SCFileRequester(std::function<void(std::string)> callback, uint8_t shape_id_offset = 42);
    SCFileRequester(std::function<void(std::string)> callback, uint8_t shape_id_offset, bool save);
    ~SCFileRequester();
    
    void draw(FrameBuffer *fb);
    void checkevents();
    void cancel();
    void quitToDos();
    void loadFiles();
    void fileUp();
    void fileDown();
    void loadFile();

    SCButton* checkButtons(void);
    SCZone* checkZones();
    void selectFile(void *unused, int index);

    std::function<void(std::string)> callback;
    void activateTextEditor(void* unused, int id);
    void deactivateTextEditor();
    // Coin haut gauche du cadre (image 0 du jeu, 182 x 131), centré à l'écran.
    Point2D frameCorner();
    // Ascenseur : curseur (image 14 du jeu) sur le rail entre les deux flèches.
    void scrollFromMouse(int mouse_y);
    static const int VISIBLE_FILES = 6;
    static const int ROW_HEIGHT = 8;
    int first_file = 0;
    bool dragging_thumb = false;
    // Sauvegarde sur un fichier de la liste : dialogue « OVERWRITE / ARE YOU SURE » (images 15 et 16 du jeu), YES
    // (17, 18) et NO (19, 20), centré à l'écran.
    bool confirm_overwrite = false;
    std::vector<SCButton *> confirm_buttons;
    Point2D dialogCorner();
    // Palette_FindNearest_FA5F d'OPTEST : première couleur de la palette en place dont |dr| + |dg| + |db| est le plus
    // petit (r, g, b de 0 à 63).
    uint8_t nearestColor(int r, int g, int b);
    bool fileExists(const std::string &name);
    void overwriteYes();
    void overwriteNo();
    void saveRequested();
    SCZone* m_textInputZone = nullptr;
    bool m_isEditingText = false;
    RSImageSet *uiImageSet;
    bool opened = true;
    bool save_mode = false;
    std::string current_file;
    std::string requested_file;
    std::vector<std::string> files;
    std::vector<SCButton*> buttons;
    std::vector<SCZone*> zones;
    int selectd_file_index = -1;
    uint8_t shape_id_offset = 42;
    RSFont *font = nullptr;
    PakArchive optPals;
    uint8_t *rawpal;
    uint8_t color_offset = 49;
    uint8_t palette_index = 4;
    
    // Nouveaux membres pour la gestion des événements
    Keyboard* m_keyboard;
    std::shared_ptr<Keyboard::TextEditor> m_textEditor;
};