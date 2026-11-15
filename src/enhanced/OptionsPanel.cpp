#include "OptionsPanel.h"

namespace enhanced {

OptionsPanel::Action OptionsPanel::draw(const std::vector<OptionEntry> &entries, float mouseX, float mouseY, float logicalWidth,
                                        float logicalHeight, bool down, bool pressed, int &toggled) {
    if (!this->ui.init()) {
        return Action::None;
    }
    const int rowHeight = 17;
    const int rowGap = 2;
    const int panelH = 36 + (int)entries.size() * (rowHeight + rowGap) + 4 + rowHeight + 12;
    const int panelW = 340;
    const int panelX = (GameUI::UI_W - panelW) / 2;
    const int panelY = (GameUI::UI_H - panelH) / 2;
    this->ui.begin(mouseX, mouseY, logicalWidth, logicalHeight, down, pressed);
    this->ui.panel(panelX, panelY, panelW, panelH);
    this->ui.label("OPTIONS", GameUI::UI_W / 2, panelY + 8, 2);
    Action action = Action::None;
    int y = panelY + 36;
    for (size_t i = 0; i < entries.size(); i++) {
        if (this->ui.toggle(panelX + 24, y, panelW - 48, rowHeight, entries[i].label, *entries[i].value)) {
            toggled = (int)i;
            action = Action::Toggled;
        }
        y += rowHeight + rowGap;
    }
    y += 4;
    if (this->ui.button(panelX + 24, y, panelW - 48, rowHeight, "CLOSE")) {
        action = Action::Close;
    }
    this->ui.end();
    return action;
}

}  // namespace enhanced
