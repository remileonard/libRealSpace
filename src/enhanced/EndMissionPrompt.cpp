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
    this->ui.panel(50, 50, 220, 100);
    this->ui.label("END MISSION", 160, 62, 2);
    Result result = Result::None;
    if (this->ui.button(70, 94, 180, 18, "END MISSION")) {
        result = Result::End;
    }
    if (this->ui.button(70, 118, 180, 18, "STAY IN COCKPIT")) {
        result = Result::Stay;
    }
    this->ui.end();
    return result;
}

}  // namespace enhanced
