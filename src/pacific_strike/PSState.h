#pragma once
#include <cstdint>
#include <unordered_map>

// État de la partie de Pacific Strike : les drapeaux, communs au déroulement (GAMEFLOW.IFF), aux conversations
// (CONV.PAK) et aux scripts de mission (PROG). Fonctions de PACIFIC.EXE (seg126, pas encore nommées) : sub_56414,
// sub_5646D, sub_56520 et sub_564BB. Un numéro sous 256 porte une valeur (mot) ; à partir de 256, 0 ou 1.
class PSState {
public:
    static PSState &getInstance() {
        static PSState instance;
        return instance;
    }
    std::unordered_map<uint16_t, int16_t> flags;

    int16_t getFlag(uint16_t number);
    void setFlag(uint16_t number, int16_t value);
    void Reset();

private:
    PSState() = default;
};
