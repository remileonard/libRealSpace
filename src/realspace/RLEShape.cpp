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

RLEShape::~RLEShape() {}

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
    this->size = isize;
    uint8_t *tmpdata = idata;
    if (idata[0] == 'L' && idata[1] == 'Z') {
        LZBuffer lzbuffer;
        lzbuffer.init(idata+2, isize-2);
        size_t csize = 0;
        tmpdata = lzbuffer.DecodeLZW(idata+2, isize-2, csize);
        this->size = csize;
    }
    stream.Set(tmpdata);
    

    this->rightDist = stream.ReadShort();
    this->leftDist = stream.ReadShort();
    this->topDist = stream.ReadShort();
    this->botDist = stream.ReadShort();
    this->data = stream.GetPosition();
}

void RLEShape::InitWithPosition(uint8_t *idata, size_t isize, Point2D *position) {
    init(idata, isize);
    SetPosition(position);
}

bool RLEShape::Expand(uint8_t *dst, size_t *byteRead) {

    this->stream.Set(data);
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

    this->stream.Set(data);
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

    static RLEShape shape;

    shape.init(data, 9);

    return &shape;
}
