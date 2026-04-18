#include "precomp.h"
#include "FrameBuffer.h"

FrameBuffer::FrameBuffer(int w, int h) {
    this->framebuffer = new uint8_t[w * h]();
    this->width = w;
    this->height = h;
}
FrameBuffer::~FrameBuffer() {
    if (this->framebuffer == nullptr)
        return;
    delete (this->framebuffer);
    this->framebuffer = nullptr;
    this->width = 0;
    this->height = 0;
}
void FrameBuffer::clear() { memset(this->framebuffer, 255, this->width * this->height); }
void FrameBuffer::fillWithColor(uint8_t color) { memset(this->framebuffer, color, this->width * this->height); }
bool FrameBuffer::drawShape(RLEShape *shape) {
    if (shape != nullptr) {
        size_t byteRead = 0;
        shape->buffer_size.x = this->width;
        shape->buffer_size.y = this->height;
        return shape->Expand(this->framebuffer, &byteRead);
    }
    return (false);
}
bool FrameBuffer::drawShapeWithBox(RLEShape *shape, int bx1, int bx2, int by1, int by2) {
    if (shape != nullptr) {
        size_t byteRead = 0;
        shape->buffer_size.x = this->width;
        shape->buffer_size.y = this->height;
        return shape->ExpandWithBox(this->framebuffer, &byteRead, bx1, bx2, by1, by2);
    }
    return (false);
}
void FrameBuffer::fillLineColor(size_t lineIndex, uint8_t color) {
    memset(this->framebuffer + lineIndex * this->width, color, this->width);
}
void FrameBuffer::plot_pixel(int x, int y, uint8_t color) {
    if (x < 0 || x >= this->width || y < 0 || y >= this->height)
        return;
    if ((y * this->width) + x < 0 || (y * this->width) + x >= this->width * this->height)
        return;
    this->framebuffer[(y * this->width) + x] = color;
}

/**************************************************************************
 *  line                                                                  *
 *    draws a line using Bresenham's line-drawing algorithm, which uses   *
 *    no multiplication or division.                                      *
 **************************************************************************/

void FrameBuffer::line(int x1, int y1, int x2, int y2, uint8_t color) {
    this->lineWithBoxWithSkip(x1, y1, x2, y2, color, 0, this->width, 0, this->height, 1);
}

void FrameBuffer::lineWithSkip(int x1, int y1, int x2, int y2, uint8_t color, int skip) {
    this->lineWithBoxWithSkip(x1, y1, x2, y2, color, 0, this->width, 0, this->height, skip);
}
void FrameBuffer::lineWithBox(int x1, int y1, int x2, int y2, uint8_t color, int bx1, int bx2, int by1, int by2) {
    this->lineWithBoxWithSkip(x1, y1, x2, y2, color, bx1, bx2, by1, by2, 1);
}

void FrameBuffer::lineWithBoxWithSkip(int x1, int y1, int x2, int y2, uint8_t color, int bx1, int bx2, int by1, int by2,
                                      int skip) {
    if (x1 > this->width || x2 > this->width || y1 > this->height || y2 > this->height)
        return;
    int i, dx, dy, sdx, sdy, dxabs, dyabs, x, y, px, py;

    dx = x2 - x1; /* the horizontal distance of the line */
    dy = y2 - y1; /* the vertical distance of the line */
    dxabs = abs(dx);
    dyabs = abs(dy);
    sdx = sgn(dx);
    sdy = sgn(dy);
    x = dyabs >> 1;
    y = dxabs >> 1;
    px = x1;
    py = y1;

    if (dxabs >= dyabs) /* the line is more horizontal than vertical */
    {
        for (i = 0; i < dxabs; i++) {
            y += dyabs;
            if (y >= dxabs) {
                y -= dxabs;
                py += sdy;
            }
            px += sdx;
            if (i % skip == 0) {
                if (px >= bx1 && px <= bx2 && py >= by1 && py <= by2) {
                    plot_pixel(px, py, color);
                }
            }
        }
    } else /* the line is more vertical than horizontal */
    {
        for (i = 0; i < dyabs; i++) {
            x += dxabs;
            if (x >= dyabs) {
                x -= dyabs;
                px += sdx;
            }
            py += sdy;
            if (i % skip == 0) {
                if (px >= bx1 && px <= bx2 && py >= by1 && py <= by2) {
                    plot_pixel(px, py, color);
                }
            }
        }
    }
}

/**************************************************************************
 *  rect_slow                                                             *
 *    Draws a rectangle by calling the line function four times.          *
 **************************************************************************/

void FrameBuffer::rect_slow(int left, int top, int right, int bottom, uint8_t color) {
    line(left, top, right, top, color);
    line(left, top, left, bottom, color);
    line(right, top, right, bottom, color);
    line(left, bottom, right, bottom, color);
}

/**************************************************************************
 *  circle_slow                                                           *
 *    Draws a circle by using floating point numbers and math fuctions.   *
 **************************************************************************/

void FrameBuffer::circle_slow(int x, int y, int radius, uint8_t color) {
    float n = 0, invradius = 1 / (float)radius;
    int dx = 0, dy = radius - 1;
    uint16_t dxoffset, dyoffset, offset = y * this->width + x;

    while (dx <= dy) {
        dxoffset = dx * this->width;
        dyoffset = dy * this->width;
        if (offset + dy - dxoffset<0) {
            dx++;
            n += invradius;
            dy = (int)((float) radius * sin(acos(n)));
            continue;
        }
        if (offset + dx - dyoffset < 0) {
            dx++;
            n += invradius;
            dy = (int)((float) radius * sin(acos(n)));
            continue;
        }
        if (offset + dy - dxoffset < this->width * this->height &&
            offset + dx - dyoffset < this->width * this->height &&
            offset - dx - dyoffset < this->width * this->height &&
            offset - dy - dxoffset < this->width * this->height &&
            offset + dy + dxoffset < this->width * this->height &&
            offset + dx + dyoffset < this->width * this->height &&
            offset - dx + dyoffset < this->width * this->height &&
            offset - dy + dxoffset < this->width * this->height) {
                this->framebuffer[offset + dy - dxoffset] = color;
                this->framebuffer[offset + dx - dyoffset] = color;
                this->framebuffer[offset - dx - dyoffset] = color;
                this->framebuffer[offset - dy - dxoffset] = color;
                this->framebuffer[offset - dy + dxoffset] = color;
                this->framebuffer[offset - dx + dyoffset] = color;
                this->framebuffer[offset + dx + dyoffset] = color;
                this->framebuffer[offset + dy + dxoffset] = color;
        }
        dx++;
        n += invradius;
        dy = (int)((float) radius * sin(acos(n)));
    }
}
void FrameBuffer::printText(RSFont *font, Point2D *coo, char *text, uint8_t color, size_t start, uint32_t size,
                            size_t interLetterSpace, size_t spaceSize) {
    this->printText_SM(font, coo, text, color, start, size, interLetterSpace, spaceSize, true, false);
}
void FrameBuffer::printTextFixedWidth(RSFont *font, Point2D coo, std::string text, uint8_t color) {
    int32_t width;
    RLEShape *shape = font->GetShapeForChar('A');
    if (shape == nullptr) {
        shape = font->GetShapeForChar('0');
    }
    width = shape->GetWidth();
    this->printText_SM(font, &coo, (char *)text.c_str(), color, 0, (uint32_t)text.size(), 2, width, true, true);
}

void FrameBuffer::printText(RSFont *font, Point2D coo, std::string text, uint8_t color) {
    int32_t width;
    RLEShape *shape = font->GetShapeForChar('A');
    if (shape == nullptr) {
        shape = font->GetShapeForChar('0');
    }
    width = shape->GetWidth();
    this->printText(font, &coo, (char *)text.c_str(), color, 0, (uint32_t)text.size(), 2, width);
}

void FrameBuffer::printText_SM(RSFont *font, Point2D *coo, char *text, uint8_t color, size_t start, uint32_t size,
                               size_t interLetterSpace, size_t spaceSize, bool isSmall, bool fixedWidth) {

    if (text == NULL)
        return;

    if (size <= 0)
        return;
    int startx = coo->x;
    if (coo->y < 0)
        return;
    if (coo->y > this->height)
        return;
    if (coo->x < 0)
        return;
    if (coo->x > this->width)
        return;
    for (size_t i = 0; i < size; i++) {

        char chartoDraw = text[start + i];
        if (chartoDraw == ' ') {
            coo->x += static_cast<int32_t>(spaceSize);
            continue;
        }
        RLEShape *shape = font->GetShapeForChar(chartoDraw);
        if (shape == nullptr) {
            continue;
        }
        shape->SetColorOffset(color);
        // Adjust height
        int32_t lineHeight = coo->y;
        coo->y -= shape->GetHeight();

        if (isSmall &&
            (chartoDraw == 'p' || chartoDraw == 'y' || chartoDraw == 'g' || chartoDraw == 'q' || chartoDraw == 'j'))
            coo->y += 1;
        if (chartoDraw == '\n') {
            RLEShape *sp = font->GetShapeForChar('A');
            coo->y += sp->GetHeight() + 1;
            lineHeight = coo->y;
            coo->x = startx;
        }
        if (coo->y < 0)
            continue;
        if (coo->y > this->height)
            continue;
        if (coo->x < 0)
            continue;
        if (coo->x + static_cast<int32_t>(spaceSize) > this->width)
            continue;
        shape->SetPosition(coo);
        drawShape(shape);
        coo->y = lineHeight;

        if (chartoDraw == ' ') {
            coo->x += static_cast<int32_t>(spaceSize);
        } else {
            if (!fixedWidth) {
                coo->x = static_cast<int32_t>(coo->x + shape->GetWidth() + interLetterSpace-1);
            } else {
                coo->x = static_cast<int32_t>(coo->x + shape->GetHeight());
            }
        }
    }
}
void FrameBuffer::blit(uint8_t *srcBuffer, int x, int y, int w, int h) {
    int startRow = (y < 0) ? -y : 0;
    int startCol = (x < 0) ? -x : 0;
    int endCol   = (std::min)(w, this->width - x);

    if (endCol <= startCol) return;
    int len = endCol - startCol;

    for (int row = startRow; row < h; ++row) {
        int destRow = y + row;
        if (destRow >= this->height) break;
        memcpy(framebuffer + destRow * this->width + x + startCol,
               srcBuffer   + row * w              +     startCol,
               len);
    }
}
void FrameBuffer::blitWithMask(uint8_t *srcBuffer, int x, int y, int width, int height, uint8_t maxk) {
    int startRow = (y < 0) ? -y : 0;
    int startCol = (x < 0) ? -x : 0;
    int endCol   = (std::min)(width, this->width - x);

    if (endCol <= startCol) return;

    for (int row = startRow; row < height; ++row) {
        int destRow = y + row;
        if (destRow >= this->height) break;

        const uint8_t* src_row = srcBuffer   + row * width  + startCol;
        uint8_t*       dst_row = framebuffer  + destRow * this->width + x + startCol;
        int len = endCol - startCol;

        int i = 0;
        while (i < len) {
            if (src_row[i] == maxk) { i++; continue; }
            int span_start = i;
            while (i < len && src_row[i] != maxk) i++;
            memcpy(dst_row + span_start, src_row + span_start, i - span_start);
        }
    }
}
void FrameBuffer::blitWithMaskAndOffset(uint8_t *srcBuffer, int x, int y, int width, int height, uint8_t maxk, uint8_t offset) {
    int startRow = (y < 0) ? -y : 0;
    int startCol = (x < 0) ? -x : 0;
    int endCol   = (std::min)(width, this->width - x);

    if (endCol <= startCol) return;

    for (int row = startRow; row < height; ++row) {
        int destRow = y + row;
        if (destRow >= this->height) break;

        const uint8_t* src_row = srcBuffer  + row * width       + startCol;
        uint8_t*       dst_row = framebuffer + destRow * this->width + x + startCol;
        int len = endCol - startCol;

        for (int i = 0; i < len; ++i) {
            if (src_row[i] != maxk)
                dst_row[i] = src_row[i] + offset;
        }
    }
}
void FrameBuffer::blitLargeBuffer(uint8_t *srcBuffer, int srcWidth, int srcHeight, int srcX, int srcY, int destX,
                                  int destY, int width, int height) {
    // Clipping combiné src + dst sur les colonnes
    int startCol = (std::max)({0, -srcX, -destX});
    int endCol   = (std::min)({width, srcWidth - srcX, this->width - destX});
    if (endCol <= startCol) return;

    // Clipping combiné src + dst sur les lignes
    int startRow = (std::max)({0, -srcY, -destY});
    int endRow   = (std::min)({height, srcHeight - srcY, this->height - destY});

    for (int row = startRow; row < endRow; ++row) {
        const uint8_t* src_row = srcBuffer      + (srcY + row) * srcWidth  + srcX + startCol;
        uint8_t*       dst_row = this->framebuffer + (destY + row) * this->width + destX + startCol;
        int len = endCol - startCol;

        int i = 0;
        while (i < len) {
            if (src_row[i] == 255) { i++; continue; }
            int span_start = i;
            while (i < len && src_row[i] != 255) i++;
            memcpy(dst_row + span_start, src_row + span_start, i - span_start);
        }
    }
}
Texel *FrameBuffer::getTexture(VGAPalette *palette) { 
    Texel *texture = new Texel[this->width * this->height];
    for (int i = 0; i < this->width * this->height; i++) {
        Texel entry = palette->colors[this->framebuffer[i]];
        texture[i] = entry;
    }
    return texture;    
}