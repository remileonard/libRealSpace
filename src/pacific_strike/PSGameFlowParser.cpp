#include "PSGameFlowParser.h"
#include <unordered_map>

// Gameflow_RunScript_604D4 : chaque commande occupe 2 octets (commande, argument), 3 pour les commandes 0, 6, 7, 9,
// 10, 35 et 36 (argument sur un mot, bas puis haut), y compris quand elle n'existe pas dans la table.
void PSGameFlowParser::parseOpCode(uint8_t *data, size_t size, std::vector<EFCT *> *efct_list) {
    size_t i = 0;
    while (i + 1 < size) {
        EFCT *efct = new EFCT();
        efct->opcode = data[i];
        bool word = efct->opcode == 0 || efct->opcode == 6 || efct->opcode == 7 || efct->opcode == 9 || efct->opcode == 10 || efct->opcode == 35 || efct->opcode == 36;
        if (word && i + 2 < size) {
            efct->value = data[i + 1] | (data[i + 2] << 8);
            i += 3;
        } else {
            efct->value = data[i + 1];
            i += 2;
        }
        efct_list->push_back(efct);
    }
}
void PSGameFlowParser::parseRequBytes(uint8_t* data, size_t size, std::vector<REQU *>* requ_list) {
    for (int i = 0; i < size; i++) {
		REQU* requ = new REQU();
		requ->op = data[i];
		if (i + 2 < size) {
			requ->value = data[i + 1]  | data[i + 2] << 8;
			requ_list->push_back(requ);
            i = i + 2;
		}
	}
}
PSGameFlowParser::PSGameFlowParser() {}

PSGameFlowParser::~PSGameFlowParser() {}
