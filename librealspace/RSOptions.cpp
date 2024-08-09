#include "RSOption.h"
#include "IFFSaxLexer.h"
#include <map>
#include <string>
#include <functional>

RSOption::RSOption() {
	this->mark.DATA = nullptr;
	this->tmpback = nullptr;
	this->tmpfore = nullptr;
	this->tmpscen = nullptr;
	this->tmpsprt = nullptr;
}

RSOption::~RSOption() {
	for (auto& it : this->opts) {
		delete it.second;
	}
	for (auto& it : this->estb) {
		delete it.second;
	}
	if (this->mark.DATA != nullptr) {
		delete[] this->mark.DATA;
	}
}

void RSOption::InitFromRam(uint8_t* data, size_t size) {
	IFFSaxLexer lexer;

	std::map<std::string, std::function<void(uint8_t* data, size_t size)>> handlers;
	handlers["OPTS"] = std::bind(&RSOption::parseOPTS, this, std::placeholders::_1, std::placeholders::_2);
	handlers["ESTB"] = std::bind(&RSOption::parseETSB, this, std::placeholders::_1, std::placeholders::_2);
	handlers["MARK"] = std::bind(&RSOption::parseMARK, this, std::placeholders::_1, std::placeholders::_2);
	lexer.InitFromRAM(data, size, handlers);

}

void RSOption::parseOPTS(uint8_t* data, size_t size) {
	IFFSaxLexer lexer;
	std::map<std::string, std::function<void(uint8_t* data, size_t size)>> handlers;

	handlers["SCEN"] = std::bind(&RSOption::parseSCEN, this, std::placeholders::_1, std::placeholders::_2);

	lexer.InitFromRAM(data, size, handlers);
}

void RSOption::parseSCEN(uint8_t* data, size_t size) {
	IFFSaxLexer lexer;
	std::map<std::string, std::function<void(uint8_t* data, size_t size)>> handlers;

	printf("Parsing SCEN %llu\n", size);
	this->tmpscen = new SCEN();
	
	handlers["INFO"] = std::bind(&RSOption::parseSCEN_INFO, this, std::placeholders::_1, std::placeholders::_2);
	handlers["COLR"] = std::bind(&RSOption::parseCOLR, this, std::placeholders::_1, std::placeholders::_2);
	handlers["TUNE"] = std::bind(&RSOption::parseTUNE, this, std::placeholders::_1, std::placeholders::_2);
	handlers["BACK"] = std::bind(&RSOption::parseBACK, this, std::placeholders::_1, std::placeholders::_2);
	handlers["FORE"] = std::bind(&RSOption::parseFORE, this, std::placeholders::_1, std::placeholders::_2);

	lexer.InitFromRAM(data, size, handlers);
	printf("SCEN INFO ID %d\n", this->tmpscen->infos.ID);
	this->opts[this->tmpscen->infos.ID] = this->tmpscen;
}

void RSOption::parseSCEN_INFO(uint8_t* data, size_t size) {
	printf("Parsing INFO %llu\n", size);
	this->tmpscen->infos.ID = *data++;
	this->tmpscen->infos.UNKOWN = *data;
}

void RSOption::parseINFO(uint8_t* data, size_t size) {
	printf("Parsing INFO %llu\n", size);
}

void RSOption::parseCOLR(uint8_t* data, size_t size) {
	printf("Parsing COLR %llu\n", size);
	this->tmpscen->colr.UNKOWN_1 = *data++;
	this->tmpscen->colr.UNKOWN_2 = *data;
}

void RSOption::parseTUNE(uint8_t* data, size_t size) {
	printf("Parsing TUNE %llu\n", size);
	this->tmpscen->tune.ID = *data++;
	this->tmpscen->tune.UNKOWN = *data;
}

void RSOption::parseSPRT_TUNE(uint8_t* data, size_t size) {
	printf("Parsing TUNE %llu\n", size);
	this->tmpscen->tune.ID = *data++;
	this->tmpscen->tune.UNKOWN = *data;
}

void RSOption::parseBACK(uint8_t* data, size_t size) {
	IFFSaxLexer lexer;
	std::map<std::string, std::function<void(uint8_t* data, size_t size)>> handlers;

	printf("Parsing BACK %llu\n", size);
	this->tmpback = new BACK();
	handlers["PALT"] = std::bind(&RSOption::parseBACK_PALT, this, std::placeholders::_1, std::placeholders::_2);
	handlers["SHAP"] = std::bind(&RSOption::parseBACK_SHAP, this, std::placeholders::_1, std::placeholders::_2);
	lexer.InitFromRAM(data, size, handlers);
	this->tmpscen->background = this->tmpback;
}

void RSOption::parseBACK_PALT(uint8_t* data, size_t size) {
	PALT *palt = new PALT();

	printf("Parsing PALT %llu\n", size);
	palt->ID = *data++;
	palt->UNKOWN = *data;
	this->tmpback->palette = palt;
}

void RSOption::parseBACK_SHAP(uint8_t* data, size_t size) {
	BACK_SHAP *shps = new BACK_SHAP();

	printf("Parsing BACK SHAP %llu\n", size);
	shps->ID = *data++;
	shps->UNKOWN_1 = *data;
	this->tmpback->images.push_back(shps);
}

void RSOption::parsePALT(uint8_t* data, size_t size) {
	PALT *palette = new PALT();
	printf("Parsing PALT %llu\n", size);
	palette->ID = *data++;
	palette->UNKOWN = *data;
	this->tmpfore->palette = palette;
}

void RSOption::parseSHAP(uint8_t* data, size_t size) {
	printf("Parsing SHAP %llu\n", size);
	this->tmpsprt->sprite.GID = *data++;
	this->tmpsprt->sprite.SHP_ID = *data++;
	this->tmpsprt->sprite.UNKOWN_1 = *data++;
	this->tmpsprt->sprite.UNKOWN_2 = *data;
}

void RSOption::parseFORE(uint8_t* data, size_t size) {
	IFFSaxLexer lexer;
	std::map<std::string, std::function<void(uint8_t* data, size_t size)>> handlers;
	this->tmpfore = new FORE();
	printf("Parsing FORE %llu\n", size);

	handlers["PALT"] = std::bind(&RSOption::parsePALT, this, std::placeholders::_1, std::placeholders::_2);
	handlers["SPRT"] = std::bind(&RSOption::parseSPRT, this, std::placeholders::_1, std::placeholders::_2);
	lexer.InitFromRAM(data, size, handlers);
	this->tmpscen->foreground = this->tmpfore;
}

void RSOption::parseSPRT(uint8_t* data, size_t size) {
	IFFSaxLexer lexer;
	std::map<std::string, std::function<void(uint8_t* data, size_t size)>> handlers;

	printf("Parsing SPRT %llu\n", size);

	handlers["SHAP"] = std::bind(&RSOption::parseSHAP, this, std::placeholders::_1, std::placeholders::_2);
	handlers["LABL"] = std::bind(&RSOption::parseLABL, this, std::placeholders::_1, std::placeholders::_2);
	handlers["RECT"] = std::bind(&RSOption::parseRECT, this, std::placeholders::_1, std::placeholders::_2);
	handlers["TUNE"] = std::bind(&RSOption::parseSPRT_TUNE, this, std::placeholders::_1, std::placeholders::_2);
	handlers["SEQU"] = std::bind(&RSOption::parseSPRT_SEQU, this, std::placeholders::_1, std::placeholders::_2);
	handlers["INFO"] = std::bind(&RSOption::parseSPRT_INFO, this, std::placeholders::_1, std::placeholders::_2);
	handlers["CLCK"] = std::bind(&RSOption::parseCLCK, this, std::placeholders::_1, std::placeholders::_2);
	handlers["QUAD"] = std::bind(&RSOption::parseQUAD, this, std::placeholders::_1, std::placeholders::_2);
	this->tmpsprt = new SPRT();
	this->tmpsprt->CLCK = 0;
	lexer.InitFromRAM(data, size, handlers);
	this->tmpfore->sprites.push_back(this->tmpsprt);
}

void RSOption::parseCLCK(uint8_t* data, size_t size) {
	this->tmpsprt->CLCK = 1;
}

void RSOption::parseQUAD(uint8_t* data, size_t size) {
	QUAD* quad = new QUAD();
	printf("Parsing QUAD %llu\n", size);
	quad->xa1 = *data++;
	quad->ya1 = *data++;
	quad->xa2 = *data++;
	quad->ya2 = *data++;
	quad->xb1 = *data++;
	quad->yb1 = *data++;
	quad->xb2 = *data++;
	quad->yb2 = *data;
	this->tmpsprt->quad = quad;
}

void RSOption::parseSPRT_INFO(uint8_t* data, size_t size) {
	printf("Parsing INFO %llu\n", size);
	SPRT_INFO *info = new SPRT_INFO();
	info->ID = *data++;
	info->UNKOWN_1 = *data++;
	info->UNKOWN_2 = *data++;
	info->UNKOWN_3 = *data;
}

void RSOption::parseSPRT_SEQU(uint8_t* data, size_t size) {
	printf("Parsing SEQU %llu\n", size);
	if (size > 0) {
		this->tmpsprt->SEQU = (uint8_t*)malloc(size);
#pragma warning( push )
#pragma warning( disable : 6387)
		memcpy(this->tmpsprt->SEQU, data, size);
#pragma warning( pop )
	}
}

void RSOption::parseRECT(uint8_t* data, size_t size) {
	printf("Parsing RECT %llu\n", size);
	this->tmpsprt->zone.X1 = *data++;
	this->tmpsprt->zone.Y1 = *data++;
	this->tmpsprt->zone.X2 = *data++;
	this->tmpsprt->zone.Y2 = *data;
}

void RSOption::parseLABL(uint8_t* data, size_t size) {
	printf("Parsing LABL %llu\n", size);
	char* label = new char[size + 1];
	memcpy(label, data, size);
	label[size] = '\0';
	this->tmpsprt->label = label;
	printf("Label %s\n", label);
}

void RSOption::parseETSB(uint8_t* data, size_t size) {
	IFFSaxLexer lexer;
	std::map<std::string, std::function<void(uint8_t* data, size_t size)>> handlers;

	handlers["SHOT"] = std::bind(&RSOption::parseSHOT, this, std::placeholders::_1, std::placeholders::_2);
	printf("Parsing ETSB %llu\n", size);
	lexer.InitFromRAM(data, size, handlers);
}

void RSOption::parseSHOT_INFO(uint8_t* data, size_t size) {
	printf("Parsing INFO %llu\n", size);
	this->tmpshot->infos.ID = *data++;
	this->tmpshot->infos.UNKOWN = *data;
}

void RSOption::parseSHOT_SHPS(uint8_t* data, size_t size) {
	printf("Parsing SHPS %llu\n", size);
	IFFSaxLexer lexer;
	std::map<std::string, std::function<void(uint8_t* data, size_t size)>> handlers;
	handlers["SHAP"] = std::bind(&RSOption::parseSHOT_SHAP, this, std::placeholders::_1, std::placeholders::_2);
	handlers["MOBL"] = std::bind(&RSOption::parseSHOT_MOBL, this, std::placeholders::_1, std::placeholders::_2);
	lexer.InitFromRAM(data, size, handlers);
	
}

void RSOption::parseSHOT_SHAP(uint8_t* data, size_t size) {
	printf("Parsing SHAPE %llu\n", size);
	SHPS* shps = new SHPS();
	shps->type = 1;
	shps->OptshapeID = *data++;
	shps->UNKOWN = *data++;
	shps->x1 = *data++;
	shps->y1 = *data++;
	shps->x2 = *data++;
	shps->y2 = *data;
	this->tmpshot->images.push_back(shps);
}

void RSOption::parseSHOT_MOBL(uint8_t* data, size_t size) {
	printf("Parsing MOBL %llu\n", size);
	SHPS* shps = new SHPS();
	shps->type = 2;
	shps->OptshapeID = *data++;
	shps->UNKOWN = *data++;
	shps->x1 = *data++;
	shps->y1 = *data++;
	shps->x2 = *data++;
	shps->y2 = *data;
	this->tmpshot->images.push_back(shps);
}

void RSOption::parseSHOT_PALT(uint8_t* data, size_t size) {
	printf("Parsing PALT %llu\n", size);
	PALT* pal = new PALT();
	pal->ID = *data++;
	pal->UNKOWN = *data;
	this->tmpshot->palettes.push_back(pal);
}

void RSOption::parseSHOT_PALS(uint8_t* data, size_t size) {
	printf("Parsing PALS %llu\n", size);
	IFFSaxLexer lexer;
	std::map<std::string, std::function<void(uint8_t* data, size_t size)>> handlers;
	handlers["PALT"] = std::bind(&RSOption::parseSHOT_PALT, this, std::placeholders::_1, std::placeholders::_2);
	lexer.InitFromRAM(data, size, handlers);
}

void RSOption::parseSHOT(uint8_t* data, size_t size) {
	printf("Parsing SHOT %llu\n", size);
	IFFSaxLexer lexer;
	std::map<std::string, std::function<void(uint8_t* data, size_t size)>> handlers;
	handlers["INFO"] = std::bind(&RSOption::parseSHOT_INFO, this, std::placeholders::_1, std::placeholders::_2);
	handlers["SHPS"] = std::bind(&RSOption::parseSHOT_SHPS, this, std::placeholders::_1, std::placeholders::_2);
	handlers["PALS"] = std::bind(&RSOption::parseSHOT_PALS, this, std::placeholders::_1, std::placeholders::_2);
	this->tmpshot = new SHOT();
	lexer.InitFromRAM(data, size, handlers);
	this->estb[tmpshot->infos.ID] = tmpshot;
}

void RSOption::parseMARK(uint8_t* data, size_t size) {
	IFFSaxLexer lexer;
	std::map<std::string, std::function<void(uint8_t* data, size_t size)>> handlers;

	//handlers["SHOT"] = std::bind(&RSOption::parseSHOT, this, std::placeholders::_1, std::placeholders::_2);
	printf("Parsing MARK %llu\n", size);
	lexer.InitFromRAM(data, size, handlers);
}