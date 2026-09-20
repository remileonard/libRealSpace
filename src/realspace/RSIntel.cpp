#include "RSIntel.h"

RSIntel::RSIntel() {

}
RSIntel::~RSIntel() {

}
void RSIntel::InitFromRAM(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;

    std::unordered_map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["INTL"] = std::bind(&RSIntel::parseINTL, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}
void RSIntel::parseINTL(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;

    std::unordered_map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["VERS"] = std::bind(&RSIntel::parseINTL_VERS, this, std::placeholders::_1, std::placeholders::_2);
    handlers["NUMS"] = std::bind(&RSIntel::parseINTL_NUMS, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}
void RSIntel::parseINTL_VERS(uint8_t *data, size_t size) {
    this->version = *(uint16_t*)data;
}
void RSIntel::parseINTL_NUMS(uint8_t *data, size_t size) {
    ByteStream stream;
    if (data == nullptr) {
        return;
    }
    stream.Set(data, size);
    this->unknown_offset = stream.ReadFixedFloatLE();
    this->unknown_a = stream.ReadShort();
    this->unknown_b = stream.ReadShort();
    float x = stream.ReadFixedFloatLE();
    float y = stream.ReadFixedFloatLE();
    float z = stream.ReadFixedFloatLE();
    this->formation_offset = {x, z, y};
    x = stream.ReadFixedFloatLE();
    y = stream.ReadFixedFloatLE();
    z = stream.ReadFixedFloatLE();
    this->unknown_vector = {x, z, y};
    this->range_gun = stream.ReadFixedFloatLE();
    this->range_medium = stream.ReadInt32LE();
    this->range_far = stream.ReadInt32LE();
    this->range_close = stream.ReadInt32LE();
    this->range_long = stream.ReadInt32LE();
    this->range_ground = stream.ReadInt32LE();
    this->unknown_range = stream.ReadInt32LE();
    this->unknown_value = stream.ReadFixedFloatLE();
    this->status_flag = (stream.ReadByte() & 1) != 0;
}
