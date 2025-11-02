#include "RSGameflow.h"
#include "RSGameflow.h"
#include "../commons/IFFSaxLexer.h"
#include "../strike_commander/SCenums.h"

#include <unordered_map>
#include <string>
#include <functional>

RSGameFlow::RSGameFlow() {
}

RSGameFlow::~RSGameFlow() {
}

void RSGameFlow::InitFromRam(uint8_t* data, size_t size) {
	IFFSaxLexer lexer;

	std::unordered_map<std::string, std::function<void(uint8_t* data, size_t size)>> handlers;
	handlers["GAME"] = std::bind(&RSGameFlow::parseGAME, this, std::placeholders::_1, std::placeholders::_2);
	handlers["WRLD"] = std::bind(&RSGameFlow::parseWRLD, this, std::placeholders::_1, std::placeholders::_2);
	handlers["LOAD"] = std::bind(&RSGameFlow::parseLOAD, this, std::placeholders::_1, std::placeholders::_2);
	handlers["MLST"] = std::bind(&RSGameFlow::parseMLST, this, std::placeholders::_1, std::placeholders::_2);
	handlers["WNGS"] = std::bind(&RSGameFlow::parseWNGS, this, std::placeholders::_1, std::placeholders::_2);
	handlers["STAT"] = std::bind(&RSGameFlow::parseSTAT, this, std::placeholders::_1, std::placeholders::_2);

	lexer.InitFromRAM(data, size, handlers);
}

void RSGameFlow::parseGAME(uint8_t* data, size_t size) {
	IFFSaxLexer lexer;

	std::unordered_map<std::string, std::function<void(uint8_t* data, size_t size)>> handlers;
	handlers["MISS"] = std::bind(&RSGameFlow::parseMISS, this, std::placeholders::_1, std::placeholders::_2);

	lexer.InitFromRAM(data, size, handlers);
}

void RSGameFlow::parseMISS(uint8_t* data, size_t size) {
	IFFSaxLexer lexer;
	this->tmpmiss = new MISS();
	std::unordered_map<std::string, std::function<void(uint8_t* data, size_t size)>> handlers;
	handlers["INFO"] = std::bind(&RSGameFlow::parseMISS_INFO, this, std::placeholders::_1, std::placeholders::_2);
	handlers["EFCT"] = std::bind(&RSGameFlow::parseMISS_EFCT, this, std::placeholders::_1, std::placeholders::_2);
	handlers["SCEN"] = std::bind(&RSGameFlow::parseMISS_SCEN, this, std::placeholders::_1, std::placeholders::_2);

	lexer.InitFromRAM(data, size, handlers);

	this->game.game[this->tmpmiss->info.ID] = this->tmpmiss;
}

void RSGameFlow::parseMISS_INFO(uint8_t* data, size_t size) {
	if (size < 1)
		return;
	this->tmpmiss->info.ID = data[0];
	if (size > 1) {
		this->tmpmiss->info.UNKOWN = data[1];
	}
}

void RSGameFlow::parseMISS_EFCT(uint8_t* data, size_t size) {
	if (size < 2) {
		return;
	}
	ByteStream stream(data, size);
	this->tmpmiss->efct = new std::vector<EFCT *>();
	this->parseOpCode(data, size, this->tmpmiss->efct);
}

void RSGameFlow::parseMISS_SCEN(uint8_t* data, size_t size) {
	IFFSaxLexer lexer;
	this->tmpgfsc = new GAMEFLOW_SCEN();

	std::unordered_map<std::string, std::function<void(uint8_t* data, size_t size)>> handlers;
	handlers["INFO"] = std::bind(&RSGameFlow::parseMISS_SCEN_INFO, this, std::placeholders::_1, std::placeholders::_2);
	handlers["SPRT"] = std::bind(&RSGameFlow::parseMISS_SCEN_SPRT, this, std::placeholders::_1, std::placeholders::_2);
	
	lexer.InitFromRAM(data, size, handlers);
	this->tmpmiss->scen.push_back(this->tmpgfsc);
}

void RSGameFlow::parseMISS_SCEN_INFO(uint8_t* data, size_t size) {
	if (size < 1)
		return;
	this->tmpgfsc->info.ID = data[0];
	if (size > 1)
		this->tmpgfsc->info.UNKOWN = data[1];
}

void RSGameFlow::parseMISS_SCEN_SPRT(uint8_t* data, size_t size) {
	IFFSaxLexer lexer;
	this->tmpscsp = new GAMEFLOW_SPRT();
	this->tmpscsp->efct = nullptr;
	this->tmpscsp->requ = nullptr;
	
	std::unordered_map<std::string, std::function<void(uint8_t* data, size_t size)>> handlers;
	handlers["INFO"] = std::bind(&RSGameFlow::parseMISS_SCEN_SPRT_INFO, this, std::placeholders::_1, std::placeholders::_2);
	handlers["EFCT"] = std::bind(&RSGameFlow::parseMISS_SCEN_SPRT_EFCT, this, std::placeholders::_1, std::placeholders::_2);
	handlers["REQU"] = std::bind(&RSGameFlow::parseMISS_SCEN_SPRT_REQU, this, std::placeholders::_1, std::placeholders::_2);

	lexer.InitFromRAM(data, size, handlers);
	this->tmpgfsc->sprt.push_back(this->tmpscsp);
}

void RSGameFlow::parseMISS_SCEN_SPRT_INFO(uint8_t* data, size_t size) {
	if (size < 1)
		return;
	this->tmpscsp->info.ID = data[0];
	if (size > 1)
		this->tmpscsp->info.UNKOWN = data[1];
}
void RSGameFlow::parseOpCode(uint8_t *data, size_t size, std::vector<EFCT *>* efct_list) {
	for (int i = 0; i < size; i=i+2) {
		EFCT* efct = new EFCT();
		efct->opcode = data[i];
		if (i + 1 < size) {
			efct->value = data[i + 1];
			efct_list->push_back(efct);
		}
	}
}
void RSGameFlow::parseMISS_SCEN_SPRT_EFCT(uint8_t* data, size_t size) {
	if (size < 2) {
		return;
	}
	this->tmpscsp->efct = new std::vector<EFCT *>();
	this->parseOpCode(data, size, this->tmpscsp->efct);
}
void RSGameFlow::parseRequBytes(uint8_t* data, size_t size, std::vector<REQU *>* requ_list) {
	for (int i = 0; i < size; i = i + 2) {
		REQU* requ = new REQU();
		requ->op = data[i];
		if (i + 1 < size) {
			requ->value = data[i + 1];
			requ_list->push_back(requ);
		}
	}
}
void RSGameFlow::parseMISS_SCEN_SPRT_REQU(uint8_t* data, size_t size) {
	if (size < 2) {
		return;
	}
	this->tmpscsp->requ = new std::vector<REQU *>();
	this->parseRequBytes(data, size, this->tmpscsp->requ);
}

void RSGameFlow::parseWRLD(uint8_t* data, size_t size) {
	IFFSaxLexer lexer;

	std::unordered_map<std::string, std::function<void(uint8_t* data, size_t size)>> handlers;
	handlers["MAPS"] = std::bind(&RSGameFlow::parseWRLD_MAPS, this, std::placeholders::_1, std::placeholders::_2);
	
	lexer.InitFromRAM(data, size, handlers);
}

void RSGameFlow::parseWRLD_MAPS(uint8_t* data, size_t size) {
	IFFSaxLexer lexer;
	this->tmpmap = new MAP();
	std::unordered_map<std::string, std::function<void(uint8_t* data, size_t size)>> handlers;
	handlers["INFO"] = std::bind(&RSGameFlow::parseWRLD_MAPS_INFO, this, std::placeholders::_1, std::placeholders::_2);
	handlers["SPED"] = std::bind(&RSGameFlow::parseWRLD_MAPS_SPED, this, std::placeholders::_1, std::placeholders::_2);
	handlers["DATA"] = std::bind(&RSGameFlow::parseWRLD_MAPS_DATA, this, std::placeholders::_1, std::placeholders::_2);

	lexer.InitFromRAM(data, size, handlers);
	this->game.wrld[this->tmpmap->info.ID] = this->tmpmap;
}

void RSGameFlow::parseWRLD_MAPS_INFO(uint8_t* data, size_t size) {
	if (size < 1)
		return;
	this->tmpmap->info.ID = data[0];
	if (size > 1)
		this->tmpmap->info.UNKOWN = data[1];
}

void RSGameFlow::parseWRLD_MAPS_SPED(uint8_t* data, size_t size) {
	if (size < 1)
		return;
	SPED* tmpsped = new SPED();
	for (int i = 0; i < size; i++) {
		tmpsped->unkown.push_back(data[i]);
	}
	this->tmpmap->sped = tmpsped;
}

void RSGameFlow::parseWRLD_MAPS_DATA(uint8_t* data, size_t size) {
	if (size < 1)
		return;
	MAP_DATA* tmpmapdata = new MAP_DATA();
	for (int i = 0; i < size; i++) {
		tmpmapdata->unkown.push_back(data[i]);
	}
	int read = 0;
	ByteStream bs(data, size);
	int nbpoint = bs.ReadByte();
	
	while (read < nbpoint) {
		MAP_POINT* tmp_point = new MAP_POINT();
		tmp_point->x = bs.ReadUShort();
		tmp_point->y = bs.ReadUShort();
		tmp_point->label = bs.ReadStringNoSize(128);
		tmpmapdata->points.push_back(tmp_point);
		read++;
	}
	this->tmpmap->data = tmpmapdata;
}

void RSGameFlow::parseLOAD(uint8_t* data, size_t size) {
	IFFSaxLexer lexer;

	std::unordered_map<std::string, std::function<void(uint8_t* data, size_t size)>> handlers;
	handlers["LOAD"] = std::bind(&RSGameFlow::parseLOAD_LOAD, this, std::placeholders::_1, std::placeholders::_2);

	lexer.InitFromRAM(data, size, handlers);
}

void RSGameFlow::parseLOAD_LOAD(uint8_t* data, size_t size) {
	if (size < 1)
		return;
	LOAD* tmpload = new LOAD();
	for (int i = 0; i < size; i++) {
		tmpload->load.push_back(data[i]);
	}
	this->game.load.push_back(tmpload);
}

void RSGameFlow::parseMLST(uint8_t* data, size_t size) {
	IFFSaxLexer lexer;
	this->tmpmisslt = new MLST();
	std::unordered_map<std::string, std::function<void(uint8_t* data, size_t size)>> handlers;
	handlers["DATA"] = std::bind(&RSGameFlow::parseMLST_DATA, this, std::placeholders::_1, std::placeholders::_2);
	handlers["PRTL"] = std::bind(&RSGameFlow::parseMLST_PRTL, this, std::placeholders::_1, std::placeholders::_2);

	lexer.InitFromRAM(data, size, handlers);
	this->game.mlst = tmpmisslt;
}

void RSGameFlow::parseMLST_DATA(uint8_t* data, size_t size) {
	if (size < 8)
		return;
	size_t read = 0;
	while (read < size) {
		char* tmpstring =  (char *)malloc(sizeof(char)*9);
#pragma warning( push )
#pragma warning( disable : 6387)
		memcpy(tmpstring, data, 9);
		tmpstring[8] = '\0';
		std::string* misname = new std::string(tmpstring);
#pragma warning( pop )
		data += 8;
		read += 8;
		this->tmpmisslt->data.push_back(misname);
	}
}

void RSGameFlow::parseMLST_PRTL(uint8_t* data, size_t size) {
	if (size < 9)
		return;
	size_t read = 0;
	while (read < size) {
		char* tmpstring = (char*)malloc(sizeof(char) * 9);
#pragma warning( push )
#pragma warning( disable : 6387)
		memcpy(tmpstring, data, 9);
		std::string* prtlst = new std::string(tmpstring);
#pragma warning( pop )
		data += 9;
		read += 9;
		this->tmpmisslt->prtl.push_back(prtlst);
	}
}

void RSGameFlow::parseWNGS(uint8_t* data, size_t size) {
	IFFSaxLexer lexer;

	std::unordered_map<std::string, std::function<void(uint8_t* data, size_t size)>> handlers;
	handlers["WING"] = std::bind(&RSGameFlow::parseWNGS_WING, this, std::placeholders::_1, std::placeholders::_2);
	
	lexer.InitFromRAM(data, size, handlers);
}

void RSGameFlow::parseWNGS_WING(uint8_t* data, size_t size) {
	IFFSaxLexer lexer;
	this->tmpwings = new WING();
	std::unordered_map<std::string, std::function<void(uint8_t* data, size_t size)>> handlers;
	handlers["INFO"] = std::bind(&RSGameFlow::parseWNGS_WING_INFO, this, std::placeholders::_1, std::placeholders::_2);
	handlers["PILT"] = std::bind(&RSGameFlow::parseWNGS_WING_PILT, this, std::placeholders::_1, std::placeholders::_2);

	lexer.InitFromRAM(data, size, handlers);
	this->game.wngs.push_back(tmpwings);
}

void RSGameFlow::parseWNGS_WING_INFO(uint8_t* data, size_t size) {
	if (size < 1)
		return;
	this->tmpwings->info.ID = data[0];
	if (size > 1)
		this->tmpwings->info.UNKOWN = data[1];
}

void RSGameFlow::parseWNGS_WING_PILT(uint8_t* data, size_t size) {
	if (size < 1)
		return;
	for (int i = 0; i < size; i++) {
		this->tmpwings->pilt.push_back(data[i]);
	}
}

void RSGameFlow::parseSTAT(uint8_t* data, size_t size) {
	IFFSaxLexer lexer;

	std::unordered_map<std::string, std::function<void(uint8_t* data, size_t size)>> handlers;
	handlers["CHNG"] = std::bind(&RSGameFlow::parseSTAT_CHNG, this, std::placeholders::_1, std::placeholders::_2);
	
	lexer.InitFromRAM(data, size, handlers);
}

void RSGameFlow::parseSTAT_CHNG(uint8_t* data, size_t size) {
	if (size < 1)
		return;
	IFFSaxLexer lexer;
	this->tmpstat = new CHNG();
	this->tmpstat->cash = nullptr;
	this->tmpstat->pilt = nullptr;
	this->tmpstat->weap = nullptr;
	this->tmpstat->over = nullptr;
	std::unordered_map<std::string, std::function<void(uint8_t* data, size_t size)>> handlers;
	handlers["INFO"] = std::bind(&RSGameFlow::parseSTAT_CHNG_INFO, this, std::placeholders::_1, std::placeholders::_2);
	handlers["PILT"] = std::bind(&RSGameFlow::parseSTAT_CHNG_PILT, this, std::placeholders::_1, std::placeholders::_2);
	handlers["CASH"] = std::bind(&RSGameFlow::parseSTAT_CHNG_CASH, this, std::placeholders::_1, std::placeholders::_2);
	handlers["OVER"] = std::bind(&RSGameFlow::parseSTAT_CHNG_OVER, this, std::placeholders::_1, std::placeholders::_2);
	handlers["WEAP"] = std::bind(&RSGameFlow::parseSTAT_CHNG_WEAP, this, std::placeholders::_1, std::placeholders::_2);
	
	lexer.InitFromRAM(data, size, handlers);
	this->game.stat[this->tmpstat->info.ID] = tmpstat;
}

void RSGameFlow::parseSTAT_CHNG_INFO(uint8_t* data, size_t size) {
	if (size < 1)
		return;
	this->tmpstat->info.ID = data[0];
	if (size > 1)
		this->tmpstat->info.UNKOWN = data[1];
}

void RSGameFlow::parseSTAT_CHNG_PILT(uint8_t* data, size_t size) {
	if (size < 5) {
		return;
	}
	std::vector<PILT_CHANGE *>* tpilt = new std::vector<PILT_CHANGE*>();
	ByteStream bs;
	bs.Set(data, size);
	size_t to_read = size / 5;
	for (size_t i = 0; i < to_read; i++) {
		PILT_CHANGE *pil = new PILT_CHANGE();
		pil->op = bs.ReadByte();
		pil->pilot_id = bs.ReadByte();
		pil->air = bs.ReadByte();
		pil->value = bs.ReadByte();
		bs.ReadByte();
		tpilt->push_back(pil);
	}
	this->tmpstat->pilt = tpilt;
}

void RSGameFlow::parseSTAT_CHNG_CASH(uint8_t* data, size_t size) {
	if (size < 4)
		return;
	CHNG_CASH *cash_op;
	ByteStream bs;
	bs.Set(data, size);
	cash_op = new CHNG_CASH();
	cash_op->op = bs.ReadByte();
	cash_op->value = bs.ReadInt24LEByte3();
	this->tmpstat->cash = cash_op;
}

void RSGameFlow::parseSTAT_CHNG_OVER(uint8_t* data, size_t size) {
	if (size < 4)
		return;
	CHNG_CASH *cash_op;
	ByteStream bs;
	bs.Set(data, size);
	cash_op = new CHNG_CASH();
	cash_op->op = bs.ReadByte();
	cash_op->value = bs.ReadInt24LEByte3();
	this->tmpstat->over = cash_op;
}

void RSGameFlow::parseSTAT_CHNG_WEAP(uint8_t* data, size_t size) {
	if (size < 4) {
		return;
	}
	std::vector<WEAP_CHANGE*>* tweap = new std::vector<WEAP_CHANGE*>();

	size_t to_read = size / 4;
	ByteStream bs;
	bs.Set(data, size);
	for (size_t i = 0; i < to_read; i++) {
		WEAP_CHANGE *weap = new WEAP_CHANGE();
		weap->op = bs.ReadByte();
		weap->weap_id = bs.ReadByte();
		weap->value = bs.ReadShort();
		tweap->push_back(weap);
	}
	this->tmpstat->weap = tweap;
}