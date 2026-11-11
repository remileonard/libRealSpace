#pragma once
#include "../strike_commander/precomp.h"

// Instruments du FORM INST, dans l'ordre de lecture de sub_96000 (ovr254 de PACIFIC.EXE).
enum PSInstrumentType : uint8_t {
    PS_INST_GUNS,
    PS_INST_ALTI,
    PS_INST_AIRS,
    PS_INST_COMP,
    PS_INST_TACH,
    PS_INST_VASI,
    PS_INST_FUEL,
    PS_INST_BANK,
    PS_INST_HORI,
    PS_INST_GEAR,
    PS_INST_FLAP,
    PS_INST_STAL,
    PS_INST_AMMO,
    PS_INST_AGNS,
    PS_INST_BCOM,
    PS_INST_RUDR
};

// Un instrument : chunk INFO (sub_97050 de ovr255, sub_1BB54 de seg041 pour GUNS), images SHAP, chunk 3SPC.
struct PSCockpitInstrument {
    PSInstrumentType type{ PS_INST_GUNS };
    // INFO, mots dans l'ordre du fichier : un cadre (4 mots, recopiés comme bornes de l'instrument), un point, puis
    // un second cadre et un second point ; le second cadre manque si son premier mot vaut 65 535.
    int16_t left{ 0 };
    int16_t top{ 0 };
    int16_t right{ 0 };
    int16_t bottom{ 0 };
    int16_t point_x{ 0 };
    int16_t point_y{ 0 };
    bool has_second_box{ false };
    int16_t second_left{ 0 };
    int16_t second_top{ 0 };
    int16_t second_right{ 0 };
    int16_t second_bottom{ 0 };
    int16_t second_point_x{ 0 };
    int16_t second_point_y{ 0 };
    RSImageSet SHAP;
    // 3SPC : 5 entiers (absent pour GUNS).
    bool has_space{ false };
    int32_t space[5];
    // GUNS : chunk GUNS de 5 mots.
    uint16_t guns_count{ 0 };
    int16_t guns_left{ 0 };
    int16_t guns_top{ 0 };
    int16_t guns_right{ 0 };
    int16_t guns_bottom{ 0 };
};

// Une entrée du chunk DATA de FORM DAMG (13 octets, sub_1BF5C de seg043) : 6 mots, un octet.
struct PSCockpitDamage {
    int16_t values[6];
    uint8_t last{ 0 };
};

// Fichier de cockpit de Pacific Strike (OBJECTS\<nom du chunk CKPT de l'avion>.IFF), lu par sub_95311 (ovr253).
// ARTP, VTMP, GUNF et REAL sont lus comme Strike Commander (RSCockpit) ; INFO, INST, DAMG et FADE ont leur format.
class PSCockpitDef : public RSCockpit {
protected:
    void parseCKPT(uint8_t *data, size_t size) override;
    void parsePSINFO(uint8_t *data, size_t size);
    void parsePSINST(uint8_t *data, size_t size);
    void parseInstrument(PSInstrumentType type, uint8_t *data, size_t size);
    void parseInstrumentINFO(PSCockpitInstrument *instrument, uint8_t *data, size_t size);
    void parseInstrumentSHAP(PSCockpitInstrument *instrument, uint8_t *data, size_t size);
    void parseInstrument3SPC(PSCockpitInstrument *instrument, uint8_t *data, size_t size);
    void parseInstrumentGUNS(PSCockpitInstrument *instrument, uint8_t *data, size_t size);
    void parseDAMG(uint8_t *data, size_t size);
    void parseDAMG_DATA(uint8_t *data, size_t size);
    void parseDAMG_SHAP(uint8_t *data, size_t size);
    void parsePSFADE(uint8_t *data, size_t size);

public:
    // INFO : un octet = nombre d'entrées, un octet, puis les entrées de 29 octets (contenu non lu).
    std::vector<std::vector<uint8_t>> info_entries;
    std::vector<PSCockpitInstrument *> instruments;
    std::vector<PSCockpitDamage> damage;
    RSImageSet damage_shapes;
    // FADE : 8 entiers, 3 octets, 2 entiers, 3 octets (46 octets), rôles non lus.
    int32_t fade_values[8];
    uint8_t fade_bytes[3];
    int32_t fade_values_2[2];
    uint8_t fade_bytes_2[3];

    PSCockpitInstrument *instrument(PSInstrumentType type);
};
