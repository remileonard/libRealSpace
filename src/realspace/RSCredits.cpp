#include "RSCredits.h"

RSCredits::RSCredits() {
    this->palette_rgb.assign(768, 0);
}

RSCredits::~RSCredits() {
    for (auto &line : this->lines) {
        delete line.image;
    }
    for (auto font : this->fonts) {
        delete font;
    }
}

void RSCredits::InitFromRam(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    std::unordered_map<std::string, std::function<void(uint8_t *data, size_t size)>> handlers;
    handlers["CRED"] = std::bind(&RSCredits::parseCRED, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
    printf("CREDITS: %zu lignes, %u palette(s), %zu police(s), %u images/s\n", this->lines.size(), this->palette_count,
           this->fonts.size(), this->frames_per_second);
}

void RSCredits::parseCRED(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    std::unordered_map<std::string, std::function<void(uint8_t *data, size_t size)>> handlers;
    handlers["INFO"] = std::bind(&RSCredits::parseCRED_INFO, this, std::placeholders::_1, std::placeholders::_2);
    handlers["PALT"] = std::bind(&RSCredits::parseCRED_PALT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["FONT"] = std::bind(&RSCredits::parseCRED_FONT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["LINE"] = std::bind(&RSCredits::parseCRED_LINE, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

void RSCredits::parseCRED_INFO(uint8_t *data, size_t size) {
    ByteStream stream(data, size);
    this->frames_per_second = stream.ReadUShort();
    this->right_height = stream.ReadUShort();
    this->left_height = stream.ReadUShort();
    this->title_height = stream.ReadUShort();
    this->role_gap = stream.ReadUShort();
    this->name_gap = stream.ReadUShort();
    this->section_gap = stream.ReadUShort();
    this->title_gap = stream.ReadUShort();
    this->right_x = stream.ReadUShort();
    this->left_x = stream.ReadUShort();
    this->right_font = stream.ReadByte();
    this->left_font = stream.ReadByte();
    this->title_font = stream.ReadByte();
}

void RSCredits::parseCRED_PALT(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    std::unordered_map<std::string, std::function<void(uint8_t *data, size_t size)>> handlers;
    handlers["INFO"] = std::bind(&RSCredits::parseCRED_PALT_INFO, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DATA"] = std::bind(&RSCredits::parseCRED_PALT_DATA, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

void RSCredits::parseCRED_PALT_INFO(uint8_t *data, size_t size) {
    this->palette_count = data[0];
}

// Palette_LoadFromResource_2CCE5 : octet premier indice, octet ignoré, uint16 nombre de couleurs, puis RVB de 0 à 63.
void RSCredits::parseCRED_PALT_DATA(uint8_t *data, size_t size) {
    uint8_t first = data[0];
    uint16_t count = (uint16_t)(data[2] | (data[3] << 8));
    for (size_t i = 0; i < (size_t)count * 3 && first * 3 + i < this->palette_rgb.size(); i++) {
        this->palette_rgb[first * 3 + i] = data[4 + i];
    }
    for (int c = first; c < first + count && c < 256; c++) {
        this->palette.colors[c].r = convertFrom6To8(this->palette_rgb[c * 3]);
        this->palette.colors[c].g = convertFrom6To8(this->palette_rgb[c * 3 + 1]);
        this->palette.colors[c].b = convertFrom6To8(this->palette_rgb[c * 3 + 2]);
        this->palette.colors[c].a = 255;
    }
}

void RSCredits::parseCRED_FONT(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    std::unordered_map<std::string, std::function<void(uint8_t *data, size_t size)>> handlers;
    handlers["INFO"] = std::bind(&RSCredits::parseCRED_FONT_INFO, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DATA"] = std::bind(&RSCredits::parseCRED_FONT_DATA, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

void RSCredits::parseCRED_FONT_INFO(uint8_t *data, size_t size) {
    this->font_count = data[0];
}

void RSCredits::parseCRED_FONT_DATA(uint8_t *data, size_t size) {
    PakArchive font_archive;
    uint8_t *archive_data = (uint8_t *)malloc(size);
    memcpy(archive_data, data, size);
    font_archive.InitFromRAM("credits_font", archive_data, size);
    RSFont *font = new RSFont();
    font->InitFromPAK(&font_archive);
    this->fonts.push_back(font);
}

// Credits_Play_BDE10 : types 0 et 1, uint16 écart puis uint16 hauteur ; type 0, uint16 x puis une forme ; type 1,
// uint8 justification, uint16 x, uint8 police ; type 8, uint16 écart ; le texte occupe la fin du chunk.
void RSCredits::parseCRED_LINE(uint8_t *data, size_t size) {
    ByteStream stream(data, size);
    RSCreditsLine line;
    line.type = stream.ReadByte();
    if (line.type < 2) {
        line.gap = stream.ReadUShort();
        line.height = stream.ReadUShort();
    }
    if (line.type == 0) {
        line.x = stream.ReadUShort();
        size_t shape_size = size - 7;
        uint8_t *shape_data = (uint8_t *)malloc(shape_size);
        memcpy(shape_data, data + 7, shape_size);
        // Forme RealSpace : uint32 taille, puis décalage de la première image (24 bits bas).
        uint32_t first_image = (uint32_t)(shape_data[4] | (shape_data[5] << 8) | (shape_data[6] << 16));
        line.image = new RLEShape();
        line.image->init(shape_data + first_image, shape_size - first_image);
        this->lines.push_back(line);
        return;
    }
    if (line.type == 8) {
        line.gap = stream.ReadUShort();
        this->lines.push_back(line);
        return;
    }
    if (line.type == 1) {
        line.justify = stream.ReadByte();
        line.x = stream.ReadUShort();
        line.font = stream.ReadByte();
    }
    size_t start = line.type == 1 ? 9 : 1;
    line.text = std::string((char *)data + start, strnlen((char *)data + start, size - start));
    this->lines.push_back(line);
}
