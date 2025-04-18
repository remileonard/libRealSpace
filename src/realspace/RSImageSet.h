//
//  RSImageSet.h
//  libRealSpace
//
//  Created by fabien sanglard on 2/3/2014.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//

#pragma once

#include "PakArchive.h"
#include "RLEShape.h"
#include <stdint.h>
#include <vector>
#include "RSPalette.h"
#include "../commons/LZBuffer.h"
class RSImageSet {

public:
    RSImageSet();
    ~RSImageSet();

    void InitFromPakEntry(PakEntry *entry);
    void InitFromTreEntry(TreEntry *entry);
    void InitFromSubPakEntry(PakArchive *entry);
    void InitFromPakArchive(PakArchive *entry);
    void InitFromPakArchive(PakArchive *entry, uint8_t data_offset);
    void InitFromRam(uint8_t *data, size_t size);
    RLEShape *GetShape(size_t index);
    size_t GetNumImages(void);
    std::vector<uint8_t> sequence;
    void Add(RLEShape *shape);
    std::vector<RSPalette *> palettes;
    std::vector<RLEShape *> shapes;
    
};
