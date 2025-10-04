#include "DebugEmptyActivity.h"

DebugEmptyActivity::DebugEmptyActivity() {}

DebugEmptyActivity::~DebugEmptyActivity() {}

void DebugEmptyActivity::init(void) {}

void DebugEmptyActivity::runFrame(void) {
    VGA.activate();
    VGA.getFrameBuffer()->fillWithColor(0);
    RSFont* font = FontManager.GetFont("..\\..\\DATA\\FONTS\\FORMFONT.SHP");
    if (font != nullptr) {
        VGA.getFrameBuffer()->printText(font, {10, 10}, "Neo Strike Commander DebugTool", 18);
        VGA.getFrameBuffer()->printText(font, {10, 20}, "Use the file menu to choose your activity", 18);
    }
    VGA.vSync();
}

void DebugEmptyActivity::renderMenu(void) {}

void DebugEmptyActivity::renderUI(void) {}
