#pragma once
#include <vector>
#include "../engine/Texture.h"

class RSFont;
class FrameBuffer;

namespace enhanced {

/**
 * Small immediate-mode UI in the look of the game's main menu: a panel of any size cut from the silver board, buttons with
 * bevel and free text, and a mouse cursor. Everything is drawn into a 320x200 palette picture, uploaded as one texture and
 * drawn over the finished frame.
 */
class GameUI {
public:
    static constexpr int UI_W = 320;
    static constexpr int UI_H = 200;

    /** Loads board, palette and font from the game data once; false if they are not available. */
    bool init();
    /**
     * Starts a frame. The mouse is given in logical screen coordinates with the size of that logical screen.
     * @param down    left button is held
     * @param pressed left button went down in this frame
     */
    void begin(float mouseX, float mouseY, float logicalWidth, float logicalHeight, bool down, bool pressed);
    void panel(int x, int y, int w, int h);
    /** @param scale whole-number enlargement of the text (1 = font size) */
    void label(const char *text, int centerX, int y, int scale = 1);
    /** @return true if the button was clicked in this frame */
    bool button(int x, int y, int w, int h, const char *text);
    /** Debug aid: draws all images of a game .SHP file (fonts, buttons, dialogs) as a sheet, e.g. "CHKEXIT". */
    void sheet(const char *name);
    /** Uploads the picture and draws it with the cursor over the current frame. */
    void end();

private:
    void fillRect(int x, int y, int w, int h, uint8_t color);
    void drawCursor(float scale) const;

    bool tried{false};
    bool ready{false};
    RSFont *font{nullptr};
    VGAPalette *palette{nullptr};
    FrameBuffer *picture{nullptr};
    std::vector<uint8_t> plate;
    int plateW{0};
    int plateH{0};
    uint8_t light{0};
    uint8_t dark{0};
    uint8_t body{0};
    int capHeight{7};
    unsigned int texture{0};

    float viewW{1.0f};
    float viewH{1.0f};
    float scale{1.0f};
    float originX{0.0f};
    float originY{0.0f};
    float cursorX{0.0f};
    float cursorY{0.0f};
    int mouseX{-1};
    int mouseY{-1};
    bool mouseDown{false};
    bool mousePressed{false};
};

}  // namespace enhanced
