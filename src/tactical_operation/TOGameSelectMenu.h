#pragma once

#include "../strike_commander/precomp.h"
#include "../engine/SCPanel.h"

class TOGameSelectMenu : public IActivity, public Creatable<TOGameSelectMenu> {
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

public:
    TOGameSelectMenu();
    ~TOGameSelectMenu();
    void init(void) override;
    void runFrame(void) override;
    void focus(void) override;
    void unFocus(void) override;
};
