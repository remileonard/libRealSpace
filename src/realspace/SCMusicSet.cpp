//
//  SCMusicSet.cpp
//  libRealSpace
//
#include "SCMusicSet.h"

bool SCMusicSet::parseDat(const uint8_t *r, size_t n) {
    size_t p = 0;
    if (n < 1) {
        return false;
    }
    trackCount = r[p++];
    if (p + 2u * trackCount > n) {
        return false;
    }
    phraseLen.resize(trackCount);
    phraseLast.resize(trackCount);
    for (int i = 0; i < trackCount; i++) {
        phraseLen[i] = r[p++];
        phraseLast[i] = r[p++];
    }
    size_t m = (size_t)trackCount * trackCount;
    if (p + m + 1 > n) {
        return false;
    }
    matrix.assign(r + p, r + p + m);
    p += m;
    int e = r[p++];
    linkEntries.clear();
    for (int k = 0; k < e; k++) {
        if (p >= n) {
            return false;
        }
        size_t len = (size_t)r[p++] + 1;
        if (p + len > n) {
            return false;
        }
        linkEntries.emplace_back(r + p, r + p + len);
        p += len;
    }
    return true;
}

const uint8_t *SCTimbreLibrary::find(int bank, int patch) const {
    if (!lib) {
        return nullptr;
    }
    for (size_t o = 0; o + 6 <= libSize; o += 6) {
        if (lib[o + 1] == 0xFF) {
            break;
        }
        if (lib[o] == patch && lib[o + 1] == bank) {
            uint32_t off = (uint32_t)lib[o + 2] | ((uint32_t)lib[o + 3] << 8) | ((uint32_t)lib[o + 4] << 16) | ((uint32_t)lib[o + 5] << 24);
            if (off + 2 > libSize) {
                return nullptr;
            }
            uint16_t len = (uint16_t)(lib[off] | (lib[off + 1] << 8));
            if (off + len > libSize) {
                return nullptr;
            }
            return lib + off;
        }
    }
    return nullptr;
}
