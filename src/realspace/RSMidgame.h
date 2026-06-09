#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
}
#endif

#include "../commons/IFFSaxLexer.h"
#include "TreArchive.h"
#include "RSEntity.h"
#include "RSProf.h"
#include <stdint.h>
#include <string>
#include <algorithm>

typedef struct MIDGAME_TEXT {
    std::vector<std::string> DATA;
    std::vector<std::string> SPOK;
} MIDGAME_TEXT;
class RSMidgame {
public:
    std::unordered_map<std::string, MIDGAME_TEXT> midgame_texts;
    RSMidgame();
    ~RSMidgame();
    void InitFromRAM(uint8_t *data, size_t size);
private:
    std::string tmp_key;
    void parseMG_GEN(uint8_t *data, size_t size);
    
    void parseMIDG(uint8_t *data, size_t size);
    void parseMG01(uint8_t *data, size_t size);
    void parseMG02(uint8_t *data, size_t size);
    void parseMG05(uint8_t *data, size_t size);
    void parseMG12(uint8_t *data, size_t size);
    void parseMG15(uint8_t *data, size_t size);
    void parseMG16(uint8_t *data, size_t size);
    void parseMG17(uint8_t *data, size_t size);
    void parseMG20(uint8_t *data, size_t size);
    void parseMG33(uint8_t *data, size_t size);
    void parseMG34(uint8_t *data, size_t size);
    void parseMG36(uint8_t *data, size_t size);
    void parseMG_DATA(uint8_t *data, size_t size);
    void parseMG_SPOK(uint8_t *data, size_t size);
    void parseMG_DATA_TEXT(uint8_t *data, size_t size);
    void parseMG_SPOK_TEXT(uint8_t *data, size_t size);
};