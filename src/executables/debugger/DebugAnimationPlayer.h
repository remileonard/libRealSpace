#pragma once
#include "../../strike_commander/precomp.h"

class DebugAnimationPlayer: public SCAnimationPlayer {
private:
    PakArchive *current_mid{nullptr};
    PakEntry *current_entry{nullptr};
    int current_entry_index{0};
    MIDGAME_SHOT_BG* p_bg_editor{nullptr};
    MIDGAME_SHOT_SPRITE* p_sprite_editor{nullptr};
    MIDGAME_SHOT_BG* p_foreground_editor{nullptr};
public:
    DebugAnimationPlayer();
    ~DebugAnimationPlayer();
    void renderMenu() override;
    void renderUI(void) override;
    void midgameChoser();
    void showEditor();
    void editMidGameShotBG(MIDGAME_SHOT_BG* bg);
    void editMidGameShotSprite(MIDGAME_SHOT_SPRITE* sprite);
};
