//
//  IFFWriter.cpp
//  libRealSpace
//
//  Created on 2/10/2025.
//

#include "IFFWriter.h"
#include <cstring>
#include <algorithm>

IFFWriter::IFFWriter() {
}

IFFWriter::~IFFWriter() {
}

void IFFWriter::StartIFF(const char* formType) {
    // FORM + size(4) + formType(4)
    buffer.clear();
    chunkStack.clear();
    
    // Ajouter "FORM"
    buffer.push_back('F');
    buffer.push_back('O');
    buffer.push_back('R');
    buffer.push_back('M');
    
    // Réserver 4 octets pour la taille
    buffer.push_back(0);
    buffer.push_back(0);
    buffer.push_back(0);
    buffer.push_back(0);
    
    // Ajouter le type de formulaire (toujours 4 caractères)
    buffer.push_back(formType[0]);
    buffer.push_back(formType[1]);
    buffer.push_back(formType[2]);
    buffer.push_back(formType[3]);
    
    // Ajouter la racine à la pile
    Chunk root;
    memcpy(root.id, "FORM", 4);
    root.startPos = 0;
    root.dataSize = 4; // Inclut déjà le type du formulaire
    chunkStack.push_back(root);
}

void IFFWriter::StartChunk(const char* chunkId) {
    // Ajouter l'ID du chunk (4 caractères)
    buffer.push_back(chunkId[0]);
    buffer.push_back(chunkId[1]);
    buffer.push_back(chunkId[2]);
    buffer.push_back(chunkId[3]);
    
    // Réserver 4 octets pour la taille
    buffer.push_back(0);
    buffer.push_back(0);
    buffer.push_back(0);
    buffer.push_back(0);
    
    // Ajouter le chunk à la pile
    Chunk chunk;
    memcpy(chunk.id, chunkId, 4);
    chunk.startPos = buffer.size() - 8;
    chunk.dataSize = 0;
    chunkStack.push_back(chunk);
}

void IFFWriter::EndChunk() {
    if (chunkStack.size() > 1) { // On garde toujours FORM à la base de la pile
        Chunk& chunk = chunkStack.back();
        
        // Mettre à jour la taille du chunk
        uint32_t size = chunk.dataSize;
        uint32_t pos = chunk.startPos + 4;
        buffer[pos] = (size >> 24) & 0xFF;
        buffer[pos+1] = (size >> 16) & 0xFF;
        buffer[pos+2] = (size >> 8) & 0xFF;
        buffer[pos+3] = size & 0xFF;
        
        // Ajouter la taille du chunk au chunk parent
        if (chunkStack.size() > 1) {
            chunkStack[chunkStack.size() - 2].dataSize += chunk.dataSize + 8; // données + header
        }
        
        // Padding si nécessaire (tailles impaires)
        if (chunk.dataSize % 2 == 1) {
            buffer.push_back(0);
            if (chunkStack.size() > 1) {
                chunkStack[chunkStack.size() - 2].dataSize++;
            }
        }
        
        chunkStack.pop_back();
    } else if (chunkStack.size() == 1) {
        // C'est le chunk FORM racine
        Chunk& chunk = chunkStack.back();
        uint32_t size = chunk.dataSize;
        uint32_t pos = chunk.startPos + 4;
        buffer[pos] = (size >> 24) & 0xFF;
        buffer[pos+1] = (size >> 16) & 0xFF;
        buffer[pos+2] = (size >> 8) & 0xFF;
        buffer[pos+3] = size & 0xFF;
        
        chunkStack.pop_back();
    }
}

void IFFWriter::WriteData(const void* data, uint32_t size) {
    if (!chunkStack.empty()) {
        uint8_t* byteData = (uint8_t*)data;
        buffer.insert(buffer.end(), byteData, byteData + size);
        chunkStack.back().dataSize += size;
    }
}

void IFFWriter::WriteUint8(uint8_t value) {
    WriteData(&value, 1);
}

void IFFWriter::WriteUint16(uint16_t value) {
    uint8_t bytes[2] = {
        (uint8_t)(value & 0xFF),
        (uint8_t)((value >> 8) & 0xFF)
    };
    WriteData(bytes, 2);
}

void IFFWriter::WriteUint32(uint32_t value) {
    uint8_t bytes[4] = {
        (uint8_t)(value & 0xFF),
        (uint8_t)((value >> 8) & 0xFF),
        (uint8_t)((value >> 16) & 0xFF),
        (uint8_t)((value >> 24) & 0xFF)
    };
    WriteData(bytes, 4);
}

void IFFWriter::WriteInt8(int8_t value) {
    WriteData(&value, 1);
}

void IFFWriter::WriteInt16(int16_t value) {
    uint8_t bytes[2] = {
        (uint8_t)(value & 0xFF),
        (uint8_t)((value >> 8) & 0xFF)
    };
    WriteData(bytes, 2);
}

void IFFWriter::WriteInt32(int32_t value) {
    uint8_t bytes[4] = {
        (uint8_t)(value & 0xFF),
        (uint8_t)((value >> 8) & 0xFF),
        (uint8_t)((value >> 16) & 0xFF),
        (uint8_t)((value >> 24) & 0xFF)
    };
    WriteData(bytes, 4);
}

void IFFWriter::WriteString(const char* str, uint32_t length) {
    WriteData(str, length);
}

bool IFFWriter::SaveToFile(const char* filename) {
    // Terminer tous les chunks ouverts
    while (!chunkStack.empty()) {
        EndChunk();
    }
    
    // Écrire le fichier
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    return file.good();
}

bool IFFWriter::SaveToMemory(uint8_t** data, size_t* size) {
    // Terminer tous les chunks ouverts
    while (!chunkStack.empty()) {
        EndChunk();
    }
    
    *size = buffer.size();
    *data = new uint8_t[*size];
    std::copy(buffer.begin(), buffer.end(), *data);
    return true;
}