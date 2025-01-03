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
public:
    ByteStream(uint8_t *cursor);
    ByteStream(ByteStream &stream);
    ByteStream();
    ~ByteStream();

    void dump(size_t lenght, int hexonly);

    inline void Set(uint8_t *cursor) { this->cursor = cursor; }
    inline std::string ReadString(size_t lenght) {
        std::string str;
        uint8_t c = ReadByte();
        for (size_t i = 1; i < lenght; i++) {
            if (c != 0) {
                str += c;
            }
            c = ReadByte();
        }
        if (c != 0) {
            str += c;
        }
        return str;
    }
    inline std::string ReadStringNoSize(size_t maxLenght) {
        std::string str;
        uint8_t c = ReadByte();
        for (size_t i = 1; i < maxLenght && c != 0; i++) {
            str += c;
            c = ReadByte();
        }
        return str;
    }
    inline void MoveForward(size_t bytes) { this->cursor += bytes; }

    inline uint8_t ReadByte(void) { return *this->cursor++; }

    inline uint8_t PeekByte(void) { return *(this->cursor + 1); }

    inline uint16_t ReadUShort(void) {
        uint16_t *ushortP = (uint16_t *)this->cursor;
        this->cursor += 2;
        return *ushortP;
    }


    inline int16_t ReadShort(void) {
        int16_t *shortP = (int16_t *)this->cursor;
        this->cursor += 2;
        return *shortP;
    }

    inline uint8_t *GetPosition(void) { return this->cursor; }

    inline uint32_t ReadUInt32LE(void) {
        uint32_t *i = (uint32_t *)cursor;
        cursor += 4;
        return *i;
    }

    inline int32_t ReadInt32LE(void) {
        int32_t *i = (int32_t *)cursor;
        cursor += 4;
        return *i;
    }

    inline int32_t ReadInt24LE(void) {
        int32_t i = 0;
        uint8_t buffer[4];
        buffer[0] = *(cursor++);
        buffer[1] = *(cursor++);
        buffer[2] = *(cursor++);
        buffer[3] = *(cursor++);
        i = (buffer[2] << 16) | (buffer[1] << 8) | (buffer[0] << 0);
        if (buffer[2] & 0x80) {
            i = (0xff << 24) | i;
        }
        return i;
    }

    inline int32_t ReadInt24LEByte3(void) {
        int32_t i = 0;
        uint8_t buffer[3];
        buffer[0] = *(cursor++);
        buffer[1] = *(cursor++);
        buffer[2] = *(cursor++);
        i = (buffer[2] << 16) | (buffer[1] << 8) | (buffer[0] << 0);
        if (buffer[2] & 0x80) {
            i = (0xff << 24) | i;
        }
        return i;
    }

    inline uint32_t ReadUInt32BE(void) {

        uint32_t toLittleEndian = 0;
        toLittleEndian |= *(cursor++) << 24;
        toLittleEndian |= *(cursor++) << 16;
        toLittleEndian |= *(cursor++) << 8;
        toLittleEndian |= *(cursor++) << 0;

        return toLittleEndian;
    }

    inline uint16_t ReadUShortBE(void) {

        uint16_t toLittleEndian = 0;
        toLittleEndian |= *(cursor++) << 8;
        toLittleEndian |= *(cursor++) << 0;

        return toLittleEndian;
    }

    inline std::vector<uint8_t> ReadBytes(size_t count) {
        std::vector<uint8_t> bytes;
        bytes.reserve(count);
        for (size_t i = 0; i < count; i++) {
            bytes.push_back(ReadByte());
        }
        return bytes;
    }

private:
    uint8_t *cursor;
};
