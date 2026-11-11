#include "PSCockpitDef.h"

void PSCockpitDef::parseCKPT(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    std::unordered_map<std::string, std::function<void(uint8_t *data, size_t size)>> handlers;
    handlers["INFO"] = std::bind(&PSCockpitDef::parsePSINFO, this, std::placeholders::_1, std::placeholders::_2);
    handlers["ARTP"] = std::bind(&PSCockpitDef::parseARTP, this, std::placeholders::_1, std::placeholders::_2);
    handlers["VTMP"] = std::bind(&PSCockpitDef::parseVTMP, this, std::placeholders::_1, std::placeholders::_2);
    handlers["GUNF"] = std::bind(&PSCockpitDef::parseGUNF, this, std::placeholders::_1, std::placeholders::_2);
    handlers["REAL"] = std::bind(&PSCockpitDef::parseREAL, this, std::placeholders::_1, std::placeholders::_2);
    handlers["INST"] = std::bind(&PSCockpitDef::parsePSINST, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DAMG"] = std::bind(&PSCockpitDef::parseDAMG, this, std::placeholders::_1, std::placeholders::_2);
    handlers["FADE"] = std::bind(&PSCockpitDef::parsePSFADE, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
    printf("PSCockpitDef: %zu entrees INFO, %d images ARTP, %zu instruments, %zu entrees DAMG\n", this->info_entries.size(), (int)this->ARTP.GetNumImages(), this->instruments.size(), this->damage.size());
}

void PSCockpitDef::parsePSINFO(uint8_t *data, size_t size) {
    if (size < 2) {
        return;
    }
    uint8_t count = data[0];
    for (size_t i = 0; i < count && 2 + (i + 1) * 29 <= size; i++) {
        this->info_entries.push_back(std::vector<uint8_t>(data + 2 + i * 29, data + 2 + (i + 1) * 29));
    }
}

void PSCockpitDef::parsePSINST(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    std::unordered_map<std::string, std::function<void(uint8_t *data, size_t size)>> handlers;
    handlers["GUNS"] = std::bind(&PSCockpitDef::parseInstrument, this, PS_INST_GUNS, std::placeholders::_1, std::placeholders::_2);
    handlers["ALTI"] = std::bind(&PSCockpitDef::parseInstrument, this, PS_INST_ALTI, std::placeholders::_1, std::placeholders::_2);
    handlers["AIRS"] = std::bind(&PSCockpitDef::parseInstrument, this, PS_INST_AIRS, std::placeholders::_1, std::placeholders::_2);
    handlers["COMP"] = std::bind(&PSCockpitDef::parseInstrument, this, PS_INST_COMP, std::placeholders::_1, std::placeholders::_2);
    handlers["TACH"] = std::bind(&PSCockpitDef::parseInstrument, this, PS_INST_TACH, std::placeholders::_1, std::placeholders::_2);
    handlers["VASI"] = std::bind(&PSCockpitDef::parseInstrument, this, PS_INST_VASI, std::placeholders::_1, std::placeholders::_2);
    handlers["FUEL"] = std::bind(&PSCockpitDef::parseInstrument, this, PS_INST_FUEL, std::placeholders::_1, std::placeholders::_2);
    handlers["BANK"] = std::bind(&PSCockpitDef::parseInstrument, this, PS_INST_BANK, std::placeholders::_1, std::placeholders::_2);
    handlers["HORI"] = std::bind(&PSCockpitDef::parseInstrument, this, PS_INST_HORI, std::placeholders::_1, std::placeholders::_2);
    handlers["GEAR"] = std::bind(&PSCockpitDef::parseInstrument, this, PS_INST_GEAR, std::placeholders::_1, std::placeholders::_2);
    handlers["FLAP"] = std::bind(&PSCockpitDef::parseInstrument, this, PS_INST_FLAP, std::placeholders::_1, std::placeholders::_2);
    handlers["STAL"] = std::bind(&PSCockpitDef::parseInstrument, this, PS_INST_STAL, std::placeholders::_1, std::placeholders::_2);
    handlers["AMMO"] = std::bind(&PSCockpitDef::parseInstrument, this, PS_INST_AMMO, std::placeholders::_1, std::placeholders::_2);
    handlers["AGNS"] = std::bind(&PSCockpitDef::parseInstrument, this, PS_INST_AGNS, std::placeholders::_1, std::placeholders::_2);
    handlers["BCOM"] = std::bind(&PSCockpitDef::parseInstrument, this, PS_INST_BCOM, std::placeholders::_1, std::placeholders::_2);
    handlers["RUDR"] = std::bind(&PSCockpitDef::parseInstrument, this, PS_INST_RUDR, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

void PSCockpitDef::parseInstrument(PSInstrumentType type, uint8_t *data, size_t size) {
    PSCockpitInstrument *instrument = new PSCockpitInstrument();
    instrument->type = type;
    IFFSaxLexer lexer;
    std::unordered_map<std::string, std::function<void(uint8_t *data, size_t size)>> handlers;
    handlers["INFO"] = std::bind(&PSCockpitDef::parseInstrumentINFO, this, instrument, std::placeholders::_1, std::placeholders::_2);
    handlers["SHAP"] = std::bind(&PSCockpitDef::parseInstrumentSHAP, this, instrument, std::placeholders::_1, std::placeholders::_2);
    handlers["3SPC"] = std::bind(&PSCockpitDef::parseInstrument3SPC, this, instrument, std::placeholders::_1, std::placeholders::_2);
    handlers["GUNS"] = std::bind(&PSCockpitDef::parseInstrumentGUNS, this, instrument, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
    this->instruments.push_back(instrument);
    printf("PSCockpitDef: instrument %d, bornes (%d, %d)-(%d, %d), point (%d, %d), second cadre %s (%d, %d)-(%d, %d), point 2 (%d, %d), %d images, 3SPC %s\n", (int)type, instrument->left, instrument->top, instrument->right, instrument->bottom, instrument->point_x, instrument->point_y, instrument->has_second_box ? "oui" : "non", instrument->second_left, instrument->second_top, instrument->second_right, instrument->second_bottom, instrument->second_point_x, instrument->second_point_y, (int)instrument->SHAP.GetNumImages(), instrument->has_space ? "oui" : "non");
}

void PSCockpitDef::parseInstrumentINFO(PSCockpitInstrument *instrument, uint8_t *data, size_t size) {
    ByteStream bs(data, size);
    instrument->left = bs.ReadShort();
    instrument->top = bs.ReadShort();
    instrument->right = bs.ReadShort();
    instrument->bottom = bs.ReadShort();
    instrument->point_x = bs.ReadShort();
    instrument->point_y = bs.ReadShort();
    uint16_t second_box = bs.ReadUShort();
    if (second_box == 65535) {
        return;
    }
    instrument->has_second_box = true;
    instrument->second_left = (int16_t)second_box;
    instrument->second_top = bs.ReadShort();
    instrument->second_right = bs.ReadShort();
    instrument->second_bottom = bs.ReadShort();
    instrument->second_point_x = bs.ReadShort();
    instrument->second_point_y = bs.ReadShort();
}

// Comme RSCockpit::parseMONI_INST_ALTI_SHAP : le chunk est un PAK d'images.
void PSCockpitDef::parseInstrumentSHAP(PSCockpitInstrument *instrument, uint8_t *data, size_t size) {
    uint8_t *copy = (uint8_t *)malloc(size);
    memcpy(copy, data, size);
    PakArchive *pak = new PakArchive();
    pak->InitFromRAM("SHAP", copy, size);
    instrument->SHAP.InitFromSubPakEntry(pak);
}

void PSCockpitDef::parseInstrument3SPC(PSCockpitInstrument *instrument, uint8_t *data, size_t size) {
    ByteStream bs(data, size);
    for (int k = 0; k < 5; k++) {
        instrument->space[k] = bs.ReadInt32LE();
    }
    instrument->has_space = true;
}

void PSCockpitDef::parseInstrumentGUNS(PSCockpitInstrument *instrument, uint8_t *data, size_t size) {
    ByteStream bs(data, size);
    instrument->guns_count = bs.ReadUShort();
    instrument->guns_left = bs.ReadShort();
    instrument->guns_top = bs.ReadShort();
    instrument->guns_right = bs.ReadShort();
    instrument->guns_bottom = bs.ReadShort();
}

void PSCockpitDef::parseDAMG(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    std::unordered_map<std::string, std::function<void(uint8_t *data, size_t size)>> handlers;
    handlers["DATA"] = std::bind(&PSCockpitDef::parseDAMG_DATA, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SHAP"] = std::bind(&PSCockpitDef::parseDAMG_SHAP, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

void PSCockpitDef::parseDAMG_DATA(uint8_t *data, size_t size) {
    ByteStream bs(data, size);
    size_t count = size / 13;
    for (size_t i = 0; i < count; i++) {
        PSCockpitDamage entry;
        for (int k = 0; k < 6; k++) {
            entry.values[k] = bs.ReadShort();
        }
        entry.last = bs.ReadByte();
        this->damage.push_back(entry);
    }
}

void PSCockpitDef::parseDAMG_SHAP(uint8_t *data, size_t size) {
    uint8_t *copy = (uint8_t *)malloc(size);
    memcpy(copy, data, size);
    PakArchive *pak = new PakArchive();
    pak->InitFromRAM("DAMG", copy, size);
    this->damage_shapes.InitFromSubPakEntry(pak);
}

void PSCockpitDef::parsePSFADE(uint8_t *data, size_t size) {
    if (size < 46) {
        return;
    }
    ByteStream bs(data, size);
    for (int k = 0; k < 8; k++) {
        this->fade_values[k] = bs.ReadInt32LE();
    }
    for (int k = 0; k < 3; k++) {
        this->fade_bytes[k] = bs.ReadByte();
    }
    for (int k = 0; k < 2; k++) {
        this->fade_values_2[k] = bs.ReadInt32LE();
    }
    for (int k = 0; k < 3; k++) {
        this->fade_bytes_2[k] = bs.ReadByte();
    }
}

PSCockpitInstrument *PSCockpitDef::instrument(PSInstrumentType type) {
    for (auto candidate : this->instruments) {
        if (candidate->type == type) {
            return candidate;
        }
    }
    return nullptr;
}
