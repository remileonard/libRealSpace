#include "EndMissionPrompt.h"
#include <cstdlib>

namespace enhanced {

EndMissionPrompt::Result EndMissionPrompt::draw(float mouseX, float mouseY, float logicalWidth, float logicalHeight, bool down,
                                                bool pressed) {
    if (!this->ui.init()) {
        return Result::None;
    }
    this->ui.begin(mouseX, mouseY, logicalWidth, logicalHeight, down, pressed);
    const char *sheet = getenv("NEOSC_UI_SHEET");
    if (sheet != nullptr) {
        this->ui.sheet(sheet);
        this->ui.end();
        return Result::None;
    }
    const int panelW = 220;
    const int panelH = 100;
    const int panelX = (GameUI::UI_W - panelW) / 2;
    const int panelY = (GameUI::UI_H - panelH) / 2;
    this->ui.panel(panelX, panelY, panelW, panelH);
    this->ui.label("END MISSION", GameUI::UI_W / 2, panelY + 12, 2);
    Result result = Result::None;
    if (this->ui.button(panelX + 20, panelY + 44, panelW - 40, 18, "END MISSION")) {
        result = Result::End;
    }
    if (this->ui.button(panelX + 20, panelY + 68, panelW - 40, 18, "STAY IN COCKPIT")) {
        result = Result::Stay;
    }
    this->ui.end();
    return result;
}

}  // namespace enhanced
