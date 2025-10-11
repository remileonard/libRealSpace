#include "PKWareDecompressor.h"
#include <cstring>
#include <stdexcept>

PKWareDecompressor::PKWareDecompressor() {
}

PKWareDecompressor::~PKWareDecompressor() {
}

unsigned int PKWareDecompressor::ReadCallback(char* buf, unsigned int* size, void* param) {
    DecompressionContext* ctx = static_cast<DecompressionContext*>(param);
    
    unsigned int bytesToRead = *size;
    unsigned int bytesAvailable = static_cast<unsigned int>(ctx->inputSize - ctx->inputPos);
    
    if (bytesToRead > bytesAvailable) {
        bytesToRead = bytesAvailable;
    }
    
    if (bytesToRead > 0) {
        std::memcpy(buf, ctx->inputData + ctx->inputPos, bytesToRead);
        ctx->inputPos += bytesToRead;
    }
    
    *size = bytesToRead;
    return bytesToRead;
}

void PKWareDecompressor::WriteCallback(char* buf, unsigned int* size, void* param) {
    DecompressionContext* ctx = static_cast<DecompressionContext*>(param);
    
    if (*size > 0) {
        size_t oldSize = ctx->outputData.size();
        ctx->outputData.resize(oldSize + *size);
        std::memcpy(ctx->outputData.data() + oldSize, buf, *size);
    }
}

uint8_t* PKWareDecompressor::DecompressPKWare(const uint8_t* compData, size_t compSize, size_t& uncompSize) {
    if (!compData || compSize == 0) {
        uncompSize = 0;
        return nullptr;
    }
    
    // Préparer le contexte
    DecompressionContext ctx;
    ctx.inputData = compData;
    ctx.inputSize = compSize;
    ctx.inputPos = 0;
    ctx.outputData.reserve(compSize * 3); // Estimation initiale
    
    // Allouer le buffer de travail pour pklib
    std::vector<char> workBuf(EXP_BUFFER_SIZE);
    
    // Appeler explode
    unsigned int result = explode(ReadCallback, WriteCallback, workBuf.data(), &ctx);
    
    if (result != CMP_NO_ERROR) {
        uncompSize = 0;
        return nullptr;
    }
    
    // Copier les résultats
    uncompSize = ctx.outputData.size();
    if (uncompSize == 0) {
        return nullptr;
    }
    
    uint8_t* output = new uint8_t[uncompSize];
    std::memcpy(output, ctx.outputData.data(), uncompSize);
    
    return output;
}