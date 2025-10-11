#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include "pklib/pklib.h"

// Wrapper pour la bibliothèque PKWARE Data Compression Library
class PKWareDecompressor {
private:
    // Structure pour passer les données aux callbacks
    struct DecompressionContext {
        const uint8_t* inputData;
        size_t inputSize;
        size_t inputPos;
        
        std::vector<uint8_t> outputData;
        size_t outputPos;
    };
    
    // Callbacks pour pklib
    static unsigned int ReadCallback(char* buf, unsigned int* size, void* param);
    static void WriteCallback(char* buf, unsigned int* size, void* param);
    
public:
    PKWareDecompressor();
    ~PKWareDecompressor();
    
    // Décompresse les données en utilisant pklib
    uint8_t* DecompressPKWare(const uint8_t* compData, size_t compSize, size_t& uncompSize);
};