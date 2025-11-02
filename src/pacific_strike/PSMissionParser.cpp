#include "PSMissionParser.h"
#include "../strike_commander/SCenums.h"

void PSMissionParser::paseMissionScript(uint8_t *data, size_t size) {
    std::unordered_map<prog_op, uint8_t> opcode_parmeter_size {
        {prog_op::OP_MOVE_FLAG_TO_WORK_REGISTER         ,3},
        {prog_op::OP_MOVE_WORK_REGISTER_TO_FLAG         ,3},
        {prog_op::OP_SAVE_VALUE_TO_GAMFLOW_REGISTER     ,3},
        {prog_op::OP_ADD_WORK_REGISTER_TO_FLAG          ,3},
        {prog_op::OP_TEST_FLAG                          ,3},
        {prog_op::OP_SET_FLAG_TO_TRUE                   ,3},
        {prog_op::OP_SET_FLAG_TO_FALSE                  ,3},
        {prog_op::OP_ADD_1_TO_FLAG                      ,3},
        {prog_op::OP_REMOVE_1_TO_FLAG                   ,3},
    };
    std::vector<PROG> *prog;
    prog = new std::vector<PROG>();
    for (int i = 0; i < size; i++) {
        PROG tmp;
        uint8_t parameter_size = 1;
        tmp.opcode = data[i];
        if (opcode_parmeter_size.find(prog_op(tmp.opcode)) != opcode_parmeter_size.end()) {
            parameter_size = opcode_parmeter_size[prog_op(tmp.opcode)];
        }
        if (parameter_size == 1) {
            tmp.arg = data[i+1];
            
        } else if (parameter_size == 3) {
            tmp.arg = (data[i+2]) | data[i+1] << 8;

        }
        i += parameter_size;
        if (tmp.opcode == 0x00 && tmp.arg == 0x00) {
            //if (prog->size() > 0) {
                this->mission_data.prog.push_back(prog);
                prog = new std::vector<PROG>();
            //}
        } else {
            prog->push_back(tmp);
        }
    }
}

PSMissionParser::PSMissionParser() {}

PSMissionParser::~PSMissionParser() {}
