#pragma once
#include <vector>
#include "GameUI.h"

namespace enhanced {

/** One switch of the options screen. The value lives in the owner, the key is where it is stored in config.ini. */
struct OptionEntry {
    const char *label;
    bool *value;
};

/** ALT+O options screen: a title, one button per switch (label and ON/OFF) and a close button. */
class OptionsPanel {
public:
    enum class Action { None, Toggled, Close };

    /** False if the game menu assets are missing. */
    bool available() { return this->ui.init(); }
    /**
     * Draws the panel and handles the mouse.
     * @param toggled receives the index of the switch that was clicked when Toggled is returned
     */
    Action draw(const std::vector<OptionEntry> &entries, float mouseX, float mouseY, float logicalWidth, float logicalHeight,
                bool down, bool pressed, int &toggled);

private:
    GameUI ui;
};

}  // namespace enhanced
