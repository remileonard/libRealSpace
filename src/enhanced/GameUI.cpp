#include "GameUI.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include "../strike_commander/precomp.h"

namespace enhanced {

namespace {

const uint8_t TRANSPARENT_INDEX = 255;
const uint8_t TEXT_COLOR = 0;
const int CORNER = 8;
const float SCREEN_FRACTION = 0.85f;

// source coordinate in the board for a destination coordinate: fixed corners, repeated middle
int sliceAxis(int d, int total, int srcTotal) {
    if (d < CORNER) {
        return d;
    }
    if (d >= total - CORNER) {
        return srcTotal - (total - d);
    }
    return CORNER + (d - CORNER) % std::max(1, srcTotal - 2 * CORNER);
}

}  // namespace

bool GameUI::init() {
    if (this->tried) {
        return this->ready;
    }
    this->tried = true;
    AssetManager &assets = AssetManager::getInstance();
    TreEntry *menu = assets.GetEntryByName(MAINMENU_PAK_PATH);
    TreEntry *palettes = assets.GetEntryByName(OPTPALS_PAK_PATH);
    this->font = RSFontManager::getInstance().GetFont("..\\..\\DATA\\FONTS\\OPTFONT.SHP");
    if (menu == nullptr || palettes == nullptr || this->font == nullptr) {
        printf("[enhanced] game UI: menu assets not found\n");
        return false;
    }
    RLEShape *capital = this->font->GetShapeForChar('A');
    if (capital != nullptr) {
        this->capHeight = capital->GetHeight();
    }
    // same recipe as the main menu: game palette plus the mountain, sky and silver board patches
    PakArchive menuPak;
    menuPak.InitFromRAM("MAINMENU.PAK", menu->data, menu->size);
    PakArchive pals;
    pals.InitFromRAM("OPTPALS.PAK", palettes->data, palettes->size);
    this->palette = new VGAPalette(*RSVGA::getInstance().getPalette());
    const int patches[2] = {OPTPALS_PAK_MOUTAIN_PALETTE_PATCH_ID, OPTPALS_PAK_SKY_PALETTE_PATCH_ID};
    for (int id : patches) {
        PakEntry *patch = pals.GetEntry(id);
        if (patch != nullptr) {
            ByteStream reader;
            reader.Set(patch->data, patch->size);
            this->palette->ReadPatch(&reader);
        }
    }
    PakEntry *boardPatch = menuPak.GetEntry(MAINMENU_PAK_BOARD_PALETTE);
    if (boardPatch != nullptr) {
        ByteStream reader;
        reader.Set(boardPatch->data, boardPatch->size);
        this->palette->ReadPatch(&reader);
    }
    PakEntry *boardEntry = menuPak.GetEntry(MAINMENU_PAK_BOARD_INDICE);
    if (boardEntry == nullptr) {
        return false;
    }
    PakArchive boardPak;
    boardPak.InitFromRAM("subPak board", boardEntry->data, boardEntry->size);
    PakEntry *boardShape = boardPak.GetEntry(0);
    if (boardShape == nullptr) {
        return false;
    }
    RLEShape board;
    Point2D origin = {0, 0};
    board.InitWithPosition(boardShape->data, boardShape->size, &origin);
    FrameBuffer source(UI_W, UI_H);
    source.fillWithColor(TRANSPARENT_INDEX);
    source.drawShape(&board);
    this->plateW = std::min(board.GetWidth(), UI_W);
    this->plateH = std::min(board.GetHeight(), UI_H);
    if (this->plateW < CORNER * 3 || this->plateH < CORNER * 3) {
        return false;
    }
    this->plate.resize((size_t)this->plateW * this->plateH);
    for (int y = 0; y < this->plateH; y++) {
        for (int x = 0; x < this->plateW; x++) {
            this->plate[(size_t)y * this->plateW + x] = source.framebuffer[y * UI_W + x];
        }
    }
    // shades of the board: bright top edge, dark bottom edge, brushed body
    this->light = this->plate[(size_t)1 * this->plateW + this->plateW / 2];
    this->dark = this->plate[(size_t)(this->plateH - 2) * this->plateW + this->plateW / 2];
    this->body = this->plate[(size_t)(this->plateH / 2) * this->plateW + this->plateW / 2];
    this->onBody = this->nearestColor(96, 168, 96);
    this->onHover = this->nearestColor(128, 200, 128);
    this->picture = new FrameBuffer(UI_W, UI_H);
    this->ready = true;
    printf("[enhanced] game UI: board %dx%d, shades light %d body %d dark %d\n", this->plateW, this->plateH, this->light,
           this->body, this->dark);
    return true;
}

void GameUI::begin(float mouseX, float mouseY, float logicalWidth, float logicalHeight, bool down, bool pressed) {
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    this->viewW = std::max(1.0f, (float)viewport[2]);
    this->viewH = std::max(1.0f, (float)viewport[3]);
    this->scale = std::max(1.0f, this->viewH * SCREEN_FRACTION / (float)UI_H);
    this->originX = std::floor((this->viewW - (float)UI_W * this->scale) * 0.5f);
    this->originY = std::floor((this->viewH - (float)UI_H * this->scale) * 0.5f);
    this->cursorX = mouseX * this->viewW / std::max(1.0f, logicalWidth);
    this->cursorY = mouseY * this->viewH / std::max(1.0f, logicalHeight);
    this->mouseX = (int)std::floor((this->cursorX - this->originX) / this->scale);
    this->mouseY = (int)std::floor((this->cursorY - this->originY) / this->scale);
    this->mouseDown = down;
    this->mousePressed = pressed;
    this->picture->fillWithColor(TRANSPARENT_INDEX);
}

void GameUI::fillRect(int x, int y, int w, int h, uint8_t color) {
    for (int yy = y; yy < y + h; yy++) {
        for (int xx = x; xx < x + w; xx++) {
            if (xx >= 0 && yy >= 0 && xx < UI_W && yy < UI_H) {
                this->picture->framebuffer[yy * UI_W + xx] = color;
            }
        }
    }
}

// The board carries the recessed button slots (and faint texts) in its middle, so only its clean margins are used:
// corners and edges come from the margins, the inside repeats the brushed metal of the left margin column.
void GameUI::panel(int x, int y, int w, int h) {
    const int innerW = std::max(1, this->plateW - 2 * CORNER);
    const int innerH = std::max(1, this->plateH - 2 * CORNER);
    for (int dy = 0; dy < h; dy++) {
        for (int dx = 0; dx < w; dx++) {
            const bool left = dx < CORNER;
            const bool right = dx >= w - CORNER;
            const bool top = dy < CORNER;
            const bool bottom = dy >= h - CORNER;
            int sx = 0;
            int sy = 0;
            if ((left || right) && (top || bottom)) {
                sx = left ? dx : this->plateW - (w - dx);
                sy = top ? dy : this->plateH - (h - dy);
            } else if (top || bottom) {
                sx = CORNER + (dx - CORNER) % innerW;
                sy = top ? dy : this->plateH - (h - dy);
            } else if (left || right) {
                sx = left ? dx : this->plateW - (w - dx);
                sy = CORNER + (dy - CORNER) % innerH;
            } else {
                sx = CORNER + 1;
                sy = CORNER + (dy - CORNER) % innerH;
            }
            const int px = x + dx;
            const int py = y + dy;
            if (px >= 0 && py >= 0 && px < UI_W && py < UI_H) {
                this->picture->framebuffer[py * UI_W + px] = this->plate[(size_t)sy * this->plateW + sx];
            }
        }
    }
}

void GameUI::label(const char *text, int centerX, int y, int scale) {
    std::string s = text;
    // printText advances by the glyph advance plus 2, so measure the same way
    const int width = this->picture->getJustifiedTextWidth(this->font, s, 2);
    if (scale <= 1) {
        this->picture->printText(this->font, {centerX - width / 2, y}, s, TEXT_COLOR);
        return;
    }
    // draw once at font size on a scratch picture, then copy every pixel as a scale x scale block
    FrameBuffer scratch(UI_W, UI_H);
    scratch.fillWithColor(TRANSPARENT_INDEX);
    const int margin = 2;
    scratch.printText(this->font, {margin, margin}, s, TEXT_COLOR);
    const int srcW = std::min(UI_W, width + margin * 2 + 2);
    const int srcH = std::min(UI_H, this->capHeight + margin * 2 + 4);
    const int destX = centerX - (width * scale) / 2 - margin * scale;
    const int destY = y - margin * scale;
    for (int sy = 0; sy < srcH; sy++) {
        for (int sx = 0; sx < srcW; sx++) {
            const uint8_t color = scratch.framebuffer[sy * UI_W + sx];
            if (color != TRANSPARENT_INDEX) {
                this->fillRect(destX + sx * scale, destY + sy * scale, scale, scale, color);
            }
        }
    }
}

uint8_t GameUI::nearestColor(int r, int g, int b) const {
    int best = 0;
    int bestDistance = 1 << 30;
    for (int i = 0; i < 255; i++) {
        const Texel &c = this->palette->colors[i];
        const int dr = (int)c.r - r;
        const int dg = (int)c.g - g;
        const int db = (int)c.b - b;
        const int distance = dr * dr + dg * dg + db * db;
        if (distance < bestDistance) {
            bestDistance = distance;
            best = i;
        }
    }
    return (uint8_t)best;
}

// raised bevel; pressed = sunken
bool GameUI::drawButton(int x, int y, int w, int h, const char *text, bool pressed, uint8_t fill) {
    this->fillRect(x, y, w, h, fill);
    const uint8_t topLeft = pressed ? this->dark : this->light;
    const uint8_t bottomRight = pressed ? this->light : this->dark;
    this->fillRect(x, y, w, 1, topLeft);
    this->fillRect(x, y, 1, h, topLeft);
    if (pressed) {
        this->fillRect(x + 1, y + 1, w - 2, 1, topLeft);
        this->fillRect(x + 1, y + 1, 1, h - 2, topLeft);
    }
    this->fillRect(x, y + h - 1, w, 1, bottomRight);
    this->fillRect(x + w - 1, y, 1, h, bottomRight);
    if (!pressed) {
        this->fillRect(x + 1, y + h - 2, w - 2, 1, bottomRight);
        this->fillRect(x + w - 2, y + 1, 1, h - 2, bottomRight);
    }
    std::string s = text;
    // printText advances by the glyph advance plus 2, so measure the same way
    const int width = this->picture->getJustifiedTextWidth(this->font, s, 2);
    const int shift = pressed ? 1 : 0;
    this->picture->printText(this->font, {x + (w - width) / 2 + shift, y + 2 + shift}, s,
                             TEXT_COLOR);
    return this->mouseX >= x && this->mouseX < x + w && this->mouseY >= y && this->mouseY < y + h && this->mousePressed;
}

bool GameUI::button(int x, int y, int w, int h, const char *text) {
    const bool hover = this->mouseX >= x && this->mouseX < x + w && this->mouseY >= y && this->mouseY < y + h;
    const bool held = hover && this->mouseDown;
    return this->drawButton(x, y, w, h, text, held, hover && !held ? this->light : this->body);
}

bool GameUI::toggle(int x, int y, int w, int h, const char *text, bool on) {
    const bool hover = this->mouseX >= x && this->mouseX < x + w && this->mouseY >= y && this->mouseY < y + h;
    const bool held = hover && this->mouseDown;
    const uint8_t fill = on ? (hover ? this->onHover : this->onBody) : (hover && !held ? this->light : this->body);
    return this->drawButton(x, y, w, h, text, on || held, fill);
}

void GameUI::sheet(const char *name) {
    std::string path = std::string("..\\..\\DATA\\FONTS\\") + name + ".SHP";
    TreEntry *entry = AssetManager::getInstance().GetEntryByName(path.c_str());
    if (entry == nullptr) {
        printf("[enhanced] game UI sheet: %s not found\n", path.c_str());
        return;
    }
    PakArchive pak;
    pak.InitFromRAM("sheet", entry->data, entry->size);
    RSImageSet images;
    images.InitFromPakArchive(&pak, 0);
    printf("[enhanced] game UI sheet %s: %d images\n", name, (int)images.GetNumImages());
    int x = 2;
    int y = 2;
    int rowHeight = 0;
    for (size_t i = 0; i < images.GetNumImages(); i++) {
        RLEShape *shape = images.GetShape(i);
        if (shape == nullptr) {
            continue;
        }
        const int w = shape->GetWidth();
        const int h = shape->GetHeight();
        if (x + w > UI_W - 2) {
            x = 2;
            y += rowHeight + 2;
            rowHeight = 0;
        }
        Point2D position = {x, y};
        shape->SetPosition(&position);
        this->picture->drawShape(shape);
        x += w + 2;
        rowHeight = std::max(rowHeight, h);
    }
}

void GameUI::drawCursor(float scale) const {
    const float s = std::max(1.0f, scale * 0.9f);
    const float x = this->cursorX;
    const float y = this->cursorY;
    // arrow: head triangle and a short tail, black outline first
    for (int pass = 0; pass < 2; pass++) {
        const float g = pass == 0 ? 1.0f : 0.0f;
        if (pass == 0) {
            glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
        } else {
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        }
        glBegin(GL_TRIANGLES);
        glVertex2f(x - g * s, y - g * s);
        glVertex2f(x - g * s, y + (15.0f + g * 1.5f) * s);
        glVertex2f(x + (10.5f + g * 1.5f) * s, y + (10.5f + g * 0.5f) * s);
        glEnd();
        glBegin(GL_QUADS);
        glVertex2f(x + (3.5f - g) * s, y + 10.5f * s);
        glVertex2f(x + (7.0f + g) * s, y + 10.5f * s);
        glVertex2f(x + (9.5f + g) * s, y + (18.0f + g) * s);
        glVertex2f(x + (6.0f - g) * s, y + (18.0f + g) * s);
        glEnd();
    }
}

void GameUI::end() {
    std::vector<unsigned char> rgba((size_t)UI_W * UI_H * 4);
    Texel *texels = this->picture->getTexture(this->palette);
    const Texel edge = this->palette->colors[this->dark];
    for (int i = 0; i < UI_W * UI_H; i++) {
        const bool clear = this->picture->framebuffer[i] == TRANSPARENT_INDEX;
        rgba[(size_t)i * 4] = clear ? edge.r : texels[i].r;
        rgba[(size_t)i * 4 + 1] = clear ? edge.g : texels[i].g;
        rgba[(size_t)i * 4 + 2] = clear ? edge.b : texels[i].b;
        rgba[(size_t)i * 4 + 3] = clear ? 0 : 255;
    }
    delete[] texels;
    if (this->texture == 0) {
        this->texture = SCRenderer::getInstance().createTexture(UI_W, UI_H, rgba.data(), false);
    }

    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT | GL_TEXTURE_BIT);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, this->viewW, this->viewH, 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDisable(GL_TEXTURE_2D);
    glColor4f(0.0f, 0.0f, 0.0f, 0.55f);
    glBegin(GL_QUADS);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(this->viewW, 0.0f);
    glVertex2f(this->viewW, this->viewH);
    glVertex2f(0.0f, this->viewH);
    glEnd();

    if (this->texture != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, this->texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, UI_W, UI_H, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        const float x1 = this->originX + (float)UI_W * this->scale;
        const float y1 = this->originY + (float)UI_H * this->scale;
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f);
        glVertex2f(this->originX, this->originY);
        glTexCoord2f(1.0f, 0.0f);
        glVertex2f(x1, this->originY);
        glTexCoord2f(1.0f, 1.0f);
        glVertex2f(x1, y1);
        glTexCoord2f(0.0f, 1.0f);
        glVertex2f(this->originX, y1);
        glEnd();
        glDisable(GL_TEXTURE_2D);
    }
    this->drawCursor(this->scale);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopAttrib();
}

}  // namespace enhanced
