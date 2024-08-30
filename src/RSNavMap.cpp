#include "RSNavMap.h"

void RSNavMap::parseNMAP(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;

	std::map<std::string, std::function<void(uint8_t* data, size_t size)>> handlers;
	handlers["MAPS"] = std::bind(&RSNavMap::parseNMAP_MAPS, this, std::placeholders::_1, std::placeholders::_2);
	handlers["FONT"] = std::bind(&RSNavMap::parseNMAP_FONT, this, std::placeholders::_1, std::placeholders::_2);
	handlers["TEXT"] = std::bind(&RSNavMap::parseNMAP_TEXT, this, std::placeholders::_1, std::placeholders::_2);
	
    lexer.InitFromRAM(data, size, handlers);
}
void RSNavMap::parseNMAP_MAPS(uint8_t *data, size_t size) {
    ByteStream stream(data, size);
}
void RSNavMap::parseNMAP_FONT(uint8_t *data, size_t size) {

}
void RSNavMap::parseNMAP_TEXT(uint8_t *data, size_t size) {

}

RSNavMap::RSNavMap() {

}
RSNavMap::~RSNavMap() {

}
void RSNavMap::InitFromRam(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;

	std::map<std::string, std::function<void(uint8_t* data, size_t size)>> handlers;
	handlers["NMAP"] = std::bind(&RSNavMap::parseNMAP, this, std::placeholders::_1, std::placeholders::_2);
	lexer.InitFromRAM(data, size, handlers);
}