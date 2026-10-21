#pragma once
#include "GameUI.h"

namespace enhanced {

/** "End mission?" question with two buttons, drawn with the game-style UI over the finished frame. */
class EndMissionPrompt {
public:
    enum class Result { None, End, Stay };

    /** False if the game menu assets are missing; the question is then skipped. */
    bool available() { return this->ui.init(); }
    Result draw(float mouseX, float mouseY, float logicalWidth, float logicalHeight, bool down, bool pressed);

private:
    GameUI ui;
};

}  // namespace enhanced
