#include "OptionsPanel.h"
#include <string>

namespace enhanced {

OptionsPanel::Action OptionsPanel::draw(const std::vector<OptionEntry> &entries, float mouseX, float mouseY, float logicalWidth,
                                        float logicalHeight, bool down, bool pressed, int &toggled) {
    if (!this->ui.init()) {
        return Action::None;
    }
    const int rowHeight = 18;
    const int rowGap = 4;
    const int rows = (int)entries.size() + 1;
    const int panelH = 52 + rows * (rowHeight + rowGap) + 12;
    const int panelW = 250;
    const int panelX = (GameUI::UI_W - panelW) / 2;
    const int panelY = (GameUI::UI_H - panelH) / 2;
    this->ui.begin(mouseX, mouseY, logicalWidth, logicalHeight, down, pressed);
    this->ui.panel(panelX, panelY, panelW, panelH);
    this->ui.label("OPTIONS", GameUI::UI_W / 2, panelY + 12, 2);
    Action action = Action::None;
    int y = panelY + 46;
    for (size_t i = 0; i < entries.size(); i++) {
        const std::string text = std::string(entries[i].label) + (*entries[i].value ? "   ON" : "   OFF");
        if (this->ui.button(panelX + 20, y, panelW - 40, rowHeight, text.c_str())) {
            toggled = (int)i;
            action = Action::Toggled;
        }
        y += rowHeight + rowGap;
    }
    y += 4;
    if (this->ui.button(panelX + 20, y, panelW - 40, rowHeight, "CLOSE")) {
        action = Action::Close;
    }
    this->ui.end();
    return action;
}

}  // namespace enhanced
