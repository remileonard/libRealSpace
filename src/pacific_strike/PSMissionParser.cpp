#include "PSMissionParser.h"
#include "../strike_commander/SCenums.h"

// Instructions de drapeau (strike_commander_re/analysis/pacific/README.md § 6.7, règle 5) : 4 octets, numéro de
// variable sur 2 octets (bas puis haut) suivi de la marque 5.
bool PSMissionParser::isFlagInstruction(uint8_t opcode) {
    switch (opcode) {
    case 17:
    case 18:
    case 33:
    case 34:
    case 35:
    case 37:
    case 38:
    case 39:
    case 41:
    case 43:
    case 45:
    case 47:
    case 49:
    case 66:
    case 67:
    case 69:
    case 80:
    case 81:
    case 82:
    case 83:
    case 85:
    case 86:
        return true;
    default:
        return false;
    }
}

// Expr_VM_ReadNextToken_51D33 de PACIFIC.EXE : une instruction a 2 octets, plus 2 octets par argument
// supplémentaire (marque 9 puis valeur, 3 au plus), ou 4 octets pour une instruction de drapeau. Un mot nul ferme
// le programme. Un argument supplémentaire est rangé comme une instruction OP_SPOT_DATA à la suite, là où SCProg
// le cherche déjà.
void PSMissionParser::paseMissionScript(uint8_t *data, size_t size) {
    std::vector<PROG> *prog = new std::vector<PROG>();
    size_t i = 0;
    while (i + 1 < size) {
        PROG tmp;
        tmp.opcode = data[i];
        tmp.arg = data[i + 1];
        i += 2;
        if (tmp.opcode == OP_NOOP && tmp.arg == 0) {
            printf("PSMissionParser: programme %zu, %zu instructions, fin a l'octet %zu\n", this->mission_data.prog.size(), prog->size(), i);
            this->mission_data.prog.push_back(prog);
            prog = new std::vector<PROG>();
            continue;
        }
        std::vector<PROG> extras;
        while (extras.size() < 3 && i + 1 < size && data[i] == OP_SPOT_DATA) {
            PROG extra;
            extra.opcode = OP_SPOT_DATA;
            extra.arg = data[i + 1];
            extras.push_back(extra);
            i += 2;
        }
        if (isFlagInstruction(tmp.opcode) && i + 1 < size) {
            tmp.arg = (uint16_t)(tmp.arg | (data[i] << 8));
            if (data[i + 1] != 5) {
                printf("PSMissionParser: instruction %d a l'octet %zu, marque 5 attendue, %d lu\n", tmp.opcode, i - 2, data[i + 1]);
            }
            i += 2;
        }
        prog->push_back(tmp);
        for (auto &extra : extras) {
            prog->push_back(extra);
        }
    }
    if (prog->size() > 0) {
        printf("PSMissionParser: %zu instructions apres le dernier mot nul\n", prog->size());
    }
    delete prog;
    printf("PSMissionParser: PROG de %zu octets, %zu programmes\n", size, this->mission_data.prog.size());
}

PSMissionParser::PSMissionParser() {
}

PSMissionParser::~PSMissionParser() {
}
