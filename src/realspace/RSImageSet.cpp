//
//  RSImageSet.cpp
//  libRealSpace
//
//  Created by fabien sanglard on 2/3/2014.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//

#include "RSImageSet.h"

RSImageSet::RSImageSet() {}

RSImageSet::~RSImageSet() {}

void RSImageSet::InitFromPakEntry(PakEntry *entry) {

    ByteStream index(entry->data, entry->size);

    uint32_t palettesOffset = index.ReadUInt32LE();
    uint32_t firstImageOffset = index.ReadUInt32LE();
    std::vector<uint32_t> imageOffsets;
    imageOffsets.push_back(firstImageOffset);
    while (index.GetCurrentPosition() < firstImageOffset) {
        uint32_t imageOffset = index.ReadUInt32LE();
        imageOffsets.push_back(imageOffset);
    }
    RLEShape *shape = RLEShape::GetEmptyShape();
    this->shapes.push_back(shape);
    for (size_t i = 0; i < imageOffsets.size(); i++) {
        uint32_t imageOffset = imageOffsets[i];
        size_t size = entry->size - imageOffset;
        if (i < imageOffsets.size() - 1) {
            size = imageOffsets[i+1] - imageOffset;
        }
        uint8_t *currImage = entry->data + imageOffset;
        RLEShape *shape = new RLEShape();
        shape->init(currImage, size);
        if (shape->uncompressed) {
            this->shapes.push_back(shape);
            this->sequence.push_back((uint8_t)i);
        }
    }
    if (palettesOffset < entry->size) {
        uint8_t *palettesData = entry->data + palettesOffset;
        RSPalette *palette = new RSPalette();
        uint32_t pal_size = 0;
        pal_size |= *(palettesData+4) << 24;
        pal_size |= *(palettesData+5) << 16;
        pal_size |= *(palettesData+6) << 8;
        pal_size |= *(palettesData+7) << 0;
        if (*(palettesData+8)=='P') {
            palette->initFromFileRam(palettesData, pal_size+8);
            this->palettes.push_back(palette);
            RLEShape *shape = RLEShape::GetEmptyShape();
            this->shapes.push_back(shape);
        }
    }
}

void RSImageSet::InitFromSubPakEntry(PakArchive *entry) {
    for (int i = 0; i < entry->GetNumEntries(); i++) {
        RLEShape *shape = new RLEShape();
        shape->init(entry->GetEntry(i)->data, entry->GetEntry(i)->size);
        Point2D pos = {0, 0};
        shape->SetPosition(&pos);
        this->shapes.push_back(shape);
        this->sequence.push_back(i);
    }
}

void RSImageSet::InitFromPakArchive(PakArchive *entry) {
    this->InitFromPakArchive(entry, 8);
}

void RSImageSet::InitFromPakArchive(PakArchive *entry, uint8_t data_offset) {
    for (int i = 0; i < entry->GetNumEntries(); i++) {
        RLEShape *shape = new RLEShape();
        shape->init(entry->GetEntry(i)->data + data_offset, entry->GetEntry(i)->size - data_offset);
        if (shape->uncompressed) {
            Point2D pos = {0, 0};
            shape->SetPosition(&pos);
            this->shapes.push_back(shape);
            this->sequence.push_back(i);
        } else {
            delete shape;
        }
        
    }
}


RLEShape *RSImageSet::GetShape(size_t index) {
    if (this->shapes.size() == 0) {
        return RLEShape::GetEmptyShape();
    }
    if (index > this->shapes.size()) {
        return this->shapes[0];
    }
    return this->shapes[index];
}

void RSImageSet::Add(RLEShape *shape) { this->shapes.push_back(shape); }

size_t RSImageSet::GetNumImages(void) { return this->shapes.size(); }

void RSImageSet::removeFirstEmptyShape(void) {
    if (this->shapes.size() <=1 ) {
        return;
    }
    RLEShape *firstShape = this->shapes[0];
    if (firstShape->GetWidth() <= 1 || firstShape->GetHeight() <= 1) {
        this->shapes.erase(this->shapes.begin());
        delete firstShape;
    }
}