#pragma once
//
//  TOGameSelectMenu.h
//  libRealSpace
//
//  Menu de la version CD-ROM après l'animation THEFLIC.FLX (MENU.EXE, strike_commander_re/analysis/menu_cd/README.md) :
//  « PLAY STRIKE COMMANDER » ou « PLAY TACTICAL OPERATIONS ». Sans choix pendant 20 secondes, l'animation est rejouée.
//  Rangé dans tactical_operation parce qu'il lance Tactical Operations (la bibliothèque strike_commander ne la voit pas).
//

#include "../strike_commander/precomp.h"
#include "../engine/SCPanel.h"

class TOGameSelectMenu : public IActivity {
public:
    TOGameSelectMenu();
    ~TOGameSelectMenu();
    void init(void) override;
    void runFrame(void) override;
    void focus(void) override;
    void unFocus(void) override;

private:
    static const uint32_t IDLE_MS = 20000;
    PakArchive tomenu;
    PakArchive panel_images;
    PakArchive button_images;
    PakArchive mountain_images;
    RLEShape mountains;
    SCPanel panel;
    VGAPalette palette;
    uint32_t shown_at{ 0 };

    void applyPalette(PakEntry *entry);
    void onStrikeCommander();
    void onTacticalOperations();
    void loadTacticalOperationsData();
};
