#include "PSGameFlowParser.h"
#include <unordered_map>

void PSGameFlowParser::parseOpCode(uint8_t *data, size_t size, std::vector<EFCT *>* efct_list) {
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
        {GameFlowOpCode::EFECT_OPT_U8                   ,1},
        {GameFlowOpCode::EFECT_OPT_U10                  ,1},
        {GameFlowOpCode::EFECT_OPT_U11                  ,1},
        {GameFlowOpCode::EFECT_OPT_U12                  ,1},
        {GameFlowOpCode::EFECT_OPT_U14                  ,2},
        {GameFlowOpCode::EFECT_OPT_U15                  ,1},
        {GameFlowOpCode::EFECT_OPT_U16                  ,1},
        {GameFlowOpCode::EFFCT_OPT_U17                  ,2},
        {GameFlowOpCode::EFFCT_OPT_U18                  ,1},
        {GameFlowOpCode::EFFCT_OPT_U19                  ,1},
        {GameFlowOpCode::EFFCT_OPT_U20                  ,1}
    };
    if (size < 2) {
        return;
    }
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
                efct->value = data[i + 1] | data[i + 2] << 8;
                i = i + 2;
            }
        }
        efct_list->push_back(efct);
    }
}

PSGameFlowParser::PSGameFlowParser() {}

PSGameFlowParser::~PSGameFlowParser() {}
