#pragma once
#include "../../strike_commander/precomp.h"

class DebugAnimationPlayer: public SCAnimationPlayer {
private:
    PakArchive *current_mid{nullptr};
    PakEntry *current_entry{nullptr};
    PakArchive *current_palette_mid{nullptr};
    int current_palette_entry_index{-1};
    int current_entry_index{-1};
    MIDGAME_SHOT_BG* p_bg_editor{nullptr};
    MIDGAME_SHOT_SPRITE* p_sprite_editor{nullptr};
    MIDGAME_SHOT_BG* p_foreground_editor{nullptr};
    MIDGAME_SHOT_CHARACTER* p_character_editor{nullptr};
    MIDGAME_SHOT* p_shot_editor{nullptr};
public:
    DebugAnimationPlayer();
    ~DebugAnimationPlayer();
    void renderMenu() override;
    void renderUI(void) override;
    void midgameChoser();
    void midgamePaletteChoser();
    void showEditor();
    void editMidGameShotBG(MIDGAME_SHOT_BG* bg);
    void editMidGameShotSprite(MIDGAME_SHOT_SPRITE* sprite);
    void editMidGameShotCharacter(MIDGAME_SHOT_CHARACTER* character);
    void editMidGameShot(MIDGAME_SHOT *shot);
};
