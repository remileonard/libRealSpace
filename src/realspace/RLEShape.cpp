//
//  rle.cpp
//  pak
//
//  Created by Fabien Sanglard on 12/23/2013.
//  Copyright (c) 2013 Fabien Sanglard. All rights reserved.
//

#include "RLEShape.h"

RLEShape::RLEShape() : colorOffset(0) {
    position.x = 0;
    position.y = 0;
    this->data = nullptr;
    this->stream = ByteStream();
}
RLEShape::RLEShape(const RLEShape& other)
    : colorOffset(other.colorOffset),
      size(other.size),
      uncompressed(other.uncompressed),
      uncompressed_size(other.uncompressed_size),
      position(other.position),
      buffer_size(other.buffer_size),
      data(other.data),
      leftDist(other.leftDist),
      rightDist(other.rightDist),
      topDist(other.topDist),
      botDist(other.botDist),
      stream(other.stream),
      expand_buffer(nullptr)
{
    if (other.expand_buffer) {
        size_t buf_size = (size_t)(other.leftDist + other.rightDist) *
                          (size_t)(other.topDist  + other.botDist);
        this->expand_buffer = new uint8_t[buf_size];
        memcpy(this->expand_buffer, other.expand_buffer, buf_size);
    }
}

RLEShape& RLEShape::operator=(const RLEShape& other) {
    if (this == &other) return *this;

    delete[] this->expand_buffer;
    this->expand_buffer = nullptr;

    colorOffset      = other.colorOffset;
    size             = other.size;
    uncompressed     = other.uncompressed;
    uncompressed_size = other.uncompressed_size;
    position         = other.position;
    buffer_size      = other.buffer_size;
    data             = other.data;
    leftDist         = other.leftDist;
    rightDist        = other.rightDist;
    topDist          = other.topDist;
    botDist          = other.botDist;
    stream           = other.stream;

    if (other.expand_buffer) {
        size_t buf_size = (size_t)(other.leftDist + other.rightDist) *
                          (size_t)(other.topDist  + other.botDist);
        this->expand_buffer = new uint8_t[buf_size];
        memcpy(this->expand_buffer, other.expand_buffer, buf_size);
    }
    return *this;
}

RLEShape::~RLEShape() {
    if (this->uncompressed && this->expand_buffer) {
        delete[] this->expand_buffer;
    }
}

void RLEShape::ReadFragment(RLEFragment *frag) {

    uint16_t code = stream.ReadUShort();

    if (code == 0) {
        frag->type = FRAG_END;
        return;
    }

    frag->dx = stream.ReadShort();
    frag->dy = stream.ReadShort();

    frag->isCompressed = (code & 0x01);
    frag->numTexels = code >> 1;

    if (frag->isCompressed) {
        frag->type = FRAG_COMPOSITE;
    } else {
        frag->type = FRAG_RAW;
    }
}
void RLEShape::InitFromPakEntry(PakEntry *entry) {
    PakArchive arc;
    arc.InitFromRAM("id", entry->data, entry->size);
    this->init(arc.GetEntry(0)->data, arc.GetEntry(0)->size);
}
bool RLEShape::ExpandFragment(RLEFragment *frag, uint8_t *dst) {

    bool error;

    switch (frag->type) {
        case FRAG_RAW: {
            for (int i = 0; i < frag->numTexels; i++) {
                uint8_t color = stream.ReadByte();
                error = WriteColor(dst, frag->dx + i, frag->dy, color);
                if (error)
                    return true;
            }
        } break;

        case FRAG_COMPOSITE: {
            int numOfTexelsWritten = 0;

            while (numOfTexelsWritten < frag->numTexels) {
                uint8_t subCode = stream.ReadByte();
                uint8_t subCodeType = subCode % 2;

                uint16_t fragNumTexels = subCode >> 1;

                if (subCodeType == SUB_FRAG_RAW) {
                    for (int i = 0; i < fragNumTexels; i++) {
                        uint8_t color = stream.ReadByte();
                        error = WriteColor(dst, frag->dx + numOfTexelsWritten, frag->dy, color);
                        if (error)
                            return true;
                        numOfTexelsWritten++;
                    }

                } else {
                    uint8_t color = stream.ReadByte();
                    for (int i = 0; i < fragNumTexels; i++) {
                        error = WriteColor(dst, frag->dx + numOfTexelsWritten, frag->dy, color);
                        if (error)
                            return true;

                        numOfTexelsWritten++;
                    }
                }
            }
        } break;

        case FRAG_END: {
            return false;
        } break;
    }
    return false;
}

bool RLEShape::ExpandFragmentWithBox(RLEFragment *frag, uint8_t *dst, int bx1, int bx2, int by1, int by2) {

    bool error;

    switch (frag->type) {
        case FRAG_RAW: {
            for (int i = 0; i < frag->numTexels; i++) {
                uint8_t color = stream.ReadByte();
                error = WriteColorWithBox(dst, frag->dx + i, frag->dy, color, bx1, bx2, by1, by2);
                if (error)
                    return true;
            }
        } break;

        case FRAG_COMPOSITE: {
            int numOfTexelsWritten = 0;

            while (numOfTexelsWritten < frag->numTexels) {
                uint8_t subCode = stream.ReadByte();
                uint8_t subCodeType = subCode % 2;

                uint16_t fragNumTexels = subCode >> 1;

                if (subCodeType == SUB_FRAG_RAW) {
                    for (int i = 0; i < fragNumTexels; i++) {
                        uint8_t color = stream.ReadByte();
                        error = WriteColorWithBox(dst, frag->dx + numOfTexelsWritten, frag->dy, color, bx1, bx2, by1, by2);
                        if (error)
                            return true;
                        numOfTexelsWritten++;
                    }

                } else {
                    uint8_t color = stream.ReadByte();
                    for (int i = 0; i < fragNumTexels; i++) {
                        error = WriteColorWithBox(dst, frag->dx + numOfTexelsWritten, frag->dy, color, bx1, bx2, by1, by2);
                        if (error)
                            return true;

                        numOfTexelsWritten++;
                    }
                }
            }
        } break;

        case FRAG_END: {
            return false;
        } break;
    }
    return false;
}

void RLEShape::init(uint8_t *idata, size_t isize) {
    if (isize == 0) {
        this->size = 0;
        this->data = nullptr;
        return;
    }
    this->size = isize;
    uint8_t *tmpdata = idata;
    if (idata[0] == 'L' && idata[1] == 'Z') {
        LZBuffer lzbuffer;
        lzbuffer.init(idata+2, isize-2);
        size_t csize = 0;
        tmpdata = lzbuffer.DecodeLZW(idata+2, isize-2, csize);
        this->size = csize;
    }
    stream.Set(tmpdata, this->size);
    

    this->rightDist = stream.ReadShort()+1;
    this->leftDist = stream.ReadShort();
    this->topDist = stream.ReadShort();
    this->botDist = stream.ReadShort()+1;
    this->data = stream.GetPosition();
    
    this->buffer_size.x = 320;
    this->buffer_size.y = 200;
    if (this->rightDist + this->leftDist == 0 && this->topDist + this->botDist > 0) {
        this->leftDist = 1;
    }
    if (this->rightDist > 320 || 
        this->leftDist > 320 || 
        this->topDist > 200 || 
        this->botDist > 200 || 
        this->rightDist < -320 || 
        this->topDist < -200 || 
        this->leftDist < -320 || 
        this->botDist < -200) {
        this->uncompressed = false;
        return;
    }
    if (this->rightDist + this->leftDist > 0 && this->topDist + this->botDist > 0) {
        this->buffer_size.x = this->leftDist + this->rightDist;  // sprite width
        this->buffer_size.y = this->topDist + this->botDist;       // sprite height
        if (this->buffer_size.x > 320 || this->buffer_size.y > 200) {
            printf("Warning: RLE shape has dimensions larger than screen, clipping will occur\n");
            this->buffer_size.x = 320;
            this->buffer_size.y = 200;
        }
        this->expand_buffer = new uint8_t[this->buffer_size.x * this->buffer_size.y];
        memset(this->expand_buffer, 255, this->buffer_size.x * this->buffer_size.y);
        bool error = this->Expand(this->expand_buffer, &this->uncompressed_size);
        this->buffer_size.x = 320;  // restaurer les dimensions écran
        this->buffer_size.y = 200;
        this->uncompressed = true;
    }
}

void RLEShape::InitWithPosition(uint8_t *idata, size_t isize, Point2D *position) {
    init(idata, isize);
    SetPosition(position);
}

bool RLEShape::Expand(uint8_t *dst, size_t *byteRead) {
    if (this->uncompressed) {
        int sprite_w = this->leftDist + this->rightDist;
        int sprite_h = this->topDist + this->botDist;
        int screen_w = this->buffer_size.x;  // 320
        int screen_h = this->buffer_size.y;  // 200
        int px = position.x;
        int py = position.y;

        int x_src_start = (std::max)(0, -px);
        int x_src_end   = (std::min)(sprite_w, screen_w - px);

        for (int y = 0; y < sprite_h; y++) {
            int dst_y = y + py;
            if (dst_y < 0 || dst_y >= screen_h) continue;
            if (x_src_end <= x_src_start) continue;

            const uint8_t* src_row = this->expand_buffer + y * sprite_w + x_src_start;
            uint8_t*       dst_row = dst + dst_y * screen_w + x_src_start + px;
            int len = x_src_end - x_src_start;

            int i = 0;
            while (i < len) {
                if (src_row[i] == 255) { i++; continue; }
                int span_start = i;
                while (i < len && src_row[i] != 255) i++;
                int span_len = i - span_start;
                if (colorOffset == 0) {
                    memcpy(dst_row + span_start, src_row + span_start, span_len);
                } else {
                    for (int j = 0; j < span_len; j++) {
                        dst_row[span_start + j] = src_row[span_start + j] + colorOffset;
                    }
                }
            }
        }
        *byteRead = this->uncompressed_size;
        return false;
    }
    if (this->size == 0) {
        *byteRead = 0;
        return false;
    }
    this->stream.Set(data, size);
    RLEFragment frag;

    ReadFragment(&frag);

    while (frag.type != FRAG_END) {
        bool error = ExpandFragment(&frag, dst);
        if (error) {
            return true;
        }
        ReadFragment(&frag);
    }

    *byteRead = stream.GetPosition() - data;

    return false;
}

bool RLEShape::ExpandWithBox(uint8_t *dst, size_t *byteRead, int bx1, int bx2, int by1, int by2) {

    if (this->uncompressed) {
        int sprite_w = this->leftDist + this->rightDist;
        int sprite_h = this->topDist + this->botDist;
        int screen_w = this->buffer_size.x;
        int screen_h = this->buffer_size.y;
        int px = position.x;
        int py = position.y;

        // Clipping combiné : écran + box, en coordonnées buffer-sprite
        int x_src_start = (std::max)({0, -px, bx1 - px + leftDist});
        int x_src_end   = (std::min)({sprite_w, screen_w - px, bx2 - px + leftDist + 1});
        int y_start     = (std::max)({0, -py, by1 - py + topDist});
        int y_end       = (std::min)({sprite_h, screen_h - py, by2 - py + topDist + 1});

        for (int y = y_start; y < y_end; y++) {
            int dst_y = y + py;
            if (x_src_end <= x_src_start) continue;

            const uint8_t* src_row = this->expand_buffer + y * sprite_w + x_src_start;
            uint8_t*       dst_row = dst + dst_y * screen_w + x_src_start + px;
            int len = x_src_end - x_src_start;

            int i = 0;
            while (i < len) {
                if (src_row[i] == 255) { i++; continue; }
                int span_start = i;
                while (i < len && src_row[i] != 255) i++;
                memcpy(dst_row + span_start, src_row + span_start, i - span_start);
            }
        }
        *byteRead = this->uncompressed_size;
        return false;
    }
    this->stream.Set(data, size);
    RLEFragment frag;

    ReadFragment(&frag);

    while (frag.type != FRAG_END) {
        bool error = ExpandFragmentWithBox(&frag, dst, bx1, bx2, by1, by2);
        if (error) {
            return true;
        }
        ReadFragment(&frag);
    }

    *byteRead = stream.GetPosition() - data;

    return false;
}

bool RLEShape::WriteColor(uint8_t *dst, int16_t dx, int16_t dy, uint8_t color) {

    uint8_t *finalDest = dst + leftDist + topDist * this->buffer_size.x + dx + dy * this->buffer_size.x + position.x + position.y * this->buffer_size.x;

    uint16_t finalDestX = leftDist + dx + position.x;
    uint16_t finalDestY = topDist + dy + position.y;
    if (finalDest < dst || finalDest >= dst + (this->buffer_size.x * this->buffer_size.y))
        return true;

    color += colorOffset;

    *finalDest = color;

    return false;
}
bool RLEShape::WriteColorWithBox(uint8_t *dst, int16_t dx, int16_t dy, uint8_t color, int bx1, int bx2, int by1,
                                 int by2) {

    if ((position.x + dx) < bx1 || (position.x + dx) > bx2 || (position.y + dy) < by1 || (position.y + dy) > by2)
        return false;
    uint8_t *finalDest = dst + leftDist + topDist * this->buffer_size.x + dx + dy * this->buffer_size.x + position.x + position.y * this->buffer_size.x;

    if (finalDest < dst || finalDest >= dst + (this->buffer_size.x * this->buffer_size.y))
        return true;

    color += colorOffset;

    *finalDest = color;

    return false;
}
RLEShape *RLEShape::GetEmptyShape(void) {

    static uint8_t data[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

    RLEShape * shape;
    shape = new RLEShape();
    shape->init(data, 9);

    return shape;
}
