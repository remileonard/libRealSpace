#pragma once
#include "../../strike_commander/precomp.h"

class DebugAnimationPlayer: public SCAnimationPlayer {
private:
    PakArchive *current_mid{nullptr};
public:
    DebugAnimationPlayer();
    ~DebugAnimationPlayer();
    void renderMenu() override;
    void renderUI(void) override;
    void midgameChoser();
};
