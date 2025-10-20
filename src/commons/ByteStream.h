//
//  Stream.h
//  pak
//
//  Created by Fabien Sanglard on 12/23/2013.
//  Copyright (c) 2013 Fabien Sanglard. All rights reserved.
//

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

/*

 This is the class used to parse all byte streams.
 It is just a class which encapsulate convenient operations
 and can read byte,short,int as Little or Big Endian.

*/
class ByteStream {
private:
    uint8_t *cursor;
    size_t size;
    size_t position{0};
public:
    ByteStream(uint8_t *cursor, size_t size);
    ByteStream(ByteStream &stream);
    ByteStream();
    ~ByteStream();

    void dump(size_t lenght, int hexonly);

    void Set(uint8_t *cursor, size_t size);
    std::string ReadString(size_t lenght);
    std::string ReadStringNoSize(size_t maxLenght);
    void MoveForward(size_t bytes);
    uint8_t ReadByte(void);
    uint8_t PeekByte(void);
    uint8_t CurrentByte(void);
    uint16_t ReadUShort(void);
    int16_t ReadShort(void);
    uint8_t *GetPosition(void);
    uint32_t ReadUInt32LE(void);
    int32_t ReadInt32LE(void);
    int32_t ReadInt24LE(void);
    int32_t ReadInt24LEByte3(void);
    uint32_t ReadUInt32BE(void);
    uint16_t ReadUShortBE(void);
    std::vector<uint8_t> ReadBytes(size_t count);
    void ReadBytes(uint8_t *buffer, size_t count);
    int GetCurrentPosition() { return this->position; }
};
