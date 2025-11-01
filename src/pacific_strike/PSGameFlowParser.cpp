#include "PSGameFlowParser.h"
#include <unordered_map>

void PSGameFlowParser::parseMISS_EFCT(uint8_t *data, size_t size) {
    const static std::unordered_map<GameFlowOpCode, uint8_t> opcodeValueTypes = {
        {GameFlowOpCode::EFECT_OPT_CONV                 ,2},
        {GameFlowOpCode::EFECT_OPT_SCEN                 ,1},
        {GameFlowOpCode::EFECT_OPT_PLAY_MIDGAME         ,1},
        {GameFlowOpCode::EFECT_OPT_FLYM2                ,1},
        {GameFlowOpCode::EFECT_OPT_SETFLAG_TRUE         ,2},
        {GameFlowOpCode::EFECT_OPT_SETFLAG_FALSE        ,2},
        {GameFlowOpCode::EFECT_OPT_SHOT                 ,1},
        {GameFlowOpCode::EFECT_OPT_IF_NOT_FLAG          ,2},
        {GameFlowOpCode::EFECT_OPT_IF_FLAG              ,2},
        {GameFlowOpCode::EFECT_OPT_FLYM                 ,1},
        {GameFlowOpCode::EFECT_OPT_MIS2                 ,1},
        {GameFlowOpCode::EFECT_OPT_GO                   ,1},
        {GameFlowOpCode::EFECT_OPT_SAVE_GAME            ,1},
        {GameFlowOpCode::EFECT_OPT_LOAD_GAME            ,1},
        {GameFlowOpCode::EFFCT_OPT_SHOW_MAP             ,1},
        {GameFlowOpCode::EFECT_OPT_LOOK_AT_KILLBOARD    ,1},
        {GameFlowOpCode::EFECT_OPT_MISS_ACCEPTED        ,1},
        {GameFlowOpCode::EFECT_OPT_MISS_REJECTED        ,1},
        {GameFlowOpCode::EFECT_OPT_END_MISS             ,1},
        {GameFlowOpCode::EFECT_OPT_VIEW_CATALOG         ,1},
        {GameFlowOpCode::EFECT_OPT_LOOK_AT_LEDGER       ,1},
        {GameFlowOpCode::EFECT_OPT_MISS_ELSE            ,1},
        {GameFlowOpCode::EFECT_OPT_MISS_ENDIF           ,1},
        {GameFlowOpCode::EFFCT_OPT_IF_MISS_SUCCESS      ,1},
        {GameFlowOpCode::EFECT_OPT_APPLY_CHANGE         ,1},
        {GameFlowOpCode::EFECT_OPT_TUNE_MODIFIER        ,1},
        {GameFlowOpCode::EFECT_OPT_U2                   ,1},
        {GameFlowOpCode::EFECT_OPT_U8                   ,2},
        {GameFlowOpCode::EFECT_OPT_U10                  ,1},
        {GameFlowOpCode::EFECT_OPT_U11                  ,1},
        {GameFlowOpCode::EFECT_OPT_U12                  ,1},
        {GameFlowOpCode::EFECT_OPT_U14                  ,2},
        {GameFlowOpCode::EFECT_OPT_U15                  ,1},
        {GameFlowOpCode::EFECT_OPT_U16                  ,1},
        {GameFlowOpCode::EFFCT_OPT_U17                  ,2}
    };
    if (size < 4) {
        return;
    }
    this->tmpmiss->efct = new std::vector<EFCT *>();
    for (int i = 0; i < size; i++) {
        EFCT* efct = new EFCT();
        efct->opcode = data[i];
        GameFlowOpCode opcode = static_cast<GameFlowOpCode>(efct->opcode);
        auto it = opcodeValueTypes.find(opcode);
        if (it == opcodeValueTypes.end()) {
            continue;
        } else {
            uint8_t valueType = it->second;
            if (valueType == 1) {
                efct->value = data[i + 1];
                i = i + 1;
            } else if (valueType == 2) {
                efct->value = (data[i + 1] << 8) | data[i + 2];
                i = i + 2;
            }
        }
        this->tmpmiss->efct->push_back(efct);
    }
}

void PSGameFlowParser::InitFromRam(uint8_t* data, size_t size) {
	IFFSaxLexer lexer;

	std::unordered_map<std::string, std::function<void(uint8_t* data, size_t size)>> handlers;
	handlers["GAME"] = std::bind(&PSGameFlowParser::parseGAME, this, std::placeholders::_1, std::placeholders::_2);
	handlers["WRLD"] = std::bind(&PSGameFlowParser::parseWRLD, this, std::placeholders::_1, std::placeholders::_2);
	handlers["LOAD"] = std::bind(&PSGameFlowParser::parseLOAD, this, std::placeholders::_1, std::placeholders::_2);
	handlers["MLST"] = std::bind(&PSGameFlowParser::parseMLST, this, std::placeholders::_1, std::placeholders::_2);
	handlers["WNGS"] = std::bind(&PSGameFlowParser::parseWNGS, this, std::placeholders::_1, std::placeholders::_2);
	handlers["STAT"] = std::bind(&PSGameFlowParser::parseSTAT, this, std::placeholders::_1, std::placeholders::_2);

	lexer.InitFromRAM(data, size, handlers);
}

void PSGameFlowParser::parseGAME(uint8_t* data, size_t size) {
	IFFSaxLexer lexer;

	std::unordered_map<std::string, std::function<void(uint8_t* data, size_t size)>> handlers;
	handlers["MISS"] = std::bind(&PSGameFlowParser::parseMISS, this, std::placeholders::_1, std::placeholders::_2);

	lexer.InitFromRAM(data, size, handlers);
}

void PSGameFlowParser::parseMISS(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
	this->tmpmiss = new MISS();
	std::unordered_map<std::string, std::function<void(uint8_t* data, size_t size)>> handlers;
    handlers["INFO"] = std::bind(&PSGameFlowParser::parseMISS_INFO, this, std::placeholders::_1, std::placeholders::_2);
	handlers["EFCT"] = std::bind(&PSGameFlowParser::parseMISS_EFCT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SCEN"] = std::bind(&PSGameFlowParser::parseMISS_SCEN, this, std::placeholders::_1, std::placeholders::_2);

	lexer.InitFromRAM(data, size, handlers);

	this->game.game[this->tmpmiss->info.ID] = this->tmpmiss;
}

PSGameFlowParser::PSGameFlowParser() {}

PSGameFlowParser::~PSGameFlowParser() {}
