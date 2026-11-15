#include "PSConvCommanderPointer.h"

void PSConvCommanderPointer::InitFromRam(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;

    std::unordered_map<std::string, std::function<void(uint8_t *data, size_t size)>> handlers;
    handlers["CPTR"] = std::bind(&PSConvCommanderPointer::parseCPTR, this, std::placeholders::_1, std::placeholders::_2);

    lexer.InitFromRAM(data, size, handlers);
}

void PSConvCommanderPointer::parseCPTR(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;

    std::unordered_map<std::string, std::function<void(uint8_t *data, size_t size)>> handlers;
    handlers["DATA"] = std::bind(&PSConvCommanderPointer::parseCPTR_DATA, this, std::placeholders::_1, std::placeholders::_2);

    lexer.InitFromRAM(data, size, handlers);
}

void PSConvCommanderPointer::parseCPTR_DATA(uint8_t *data, size_t size) {
    ByteStream stream(data, size);
    for (size_t k = 0; k + 4 <= size; k += 4) {
        Point2D point;
        point.x = stream.ReadShort();
        point.y = stream.ReadShort();
        this->hand_points.push_back(point);
    }
}
