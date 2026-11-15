#pragma once
#include <cstddef>
#include <cstdint>

// « Speech Compression » (chaîne de seg012) : décompresseur des entrées de type 5 des PAK de Pacific Strike (sons de DSOUNDFX.PAK) : u32 taille décompressée, puis
// des blocs (en-tête de 10 octets ou plus, contrôle XOR 0xACED) décodés comme PACIFIC.EXE (sub_1C574 de seg044,
// sub_7CF8, sub_7C3F et sub_797C de seg012).
class SpeechDecompressor {
public:
    uint8_t *Decompress(const uint8_t *compData, size_t compSize, size_t &uncompSize);

private:
    bool decodeBlock(const uint8_t *block, uint8_t *output);
};
