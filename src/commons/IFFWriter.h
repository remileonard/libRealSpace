//
//  IFFWriter.h
//  libRealSpace
//
//  Created on 2/10/2025.
//

#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <fstream>

class IFFWriter {
public:
    IFFWriter();
    ~IFFWriter();

    // Commence un nouveau fichier IFF avec le type donné
    void StartIFF(const char* formType);
    
    // Commence un nouveau chunk avec l'ID donné
    void StartChunk(const char* chunkId);
    
    // Termine le chunk actuel
    void EndChunk();
    
    // Écrit des données dans le chunk actuel
    void WriteData(const void* data, uint32_t size);
    void WriteUint8(uint8_t value);
    void WriteUint16(uint16_t value);
    void WriteUint32(uint32_t value);
    void WriteInt8(int8_t value);
    void WriteInt16(int16_t value);
    void WriteInt32(int32_t value);
    void WriteString(const char* str, uint32_t length);

    // Sauvegarde le fichier IFF
    bool SaveToFile(const char* filename);
    bool SaveToMemory(uint8_t** data, size_t* size);

private:
    struct Chunk {
        char id[4];
        uint32_t startPos;
        uint32_t dataSize;
    };

    std::vector<uint8_t> buffer;
    std::vector<Chunk> chunkStack;
    
    // Écrit la taille du chunk actuel et met à jour la position
    void WriteChunkSize(Chunk& chunk);
};