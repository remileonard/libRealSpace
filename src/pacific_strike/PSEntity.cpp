#include "PSEntity.h"
#include "../strike_commander/SCenums.h"

void PSEntity::parseREAL_OBJT(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    std::unordered_map<std::string, std::function<void(uint8_t *data, size_t size)>> handlers;
    handlers["INFO"] = std::bind(&PSEntity::parseREAL_OBJT_INFO, this, std::placeholders::_1, std::placeholders::_2);
    handlers["AFTB"] = std::bind(&PSEntity::parseREAL_OBJT_AFTB, this, std::placeholders::_1, std::placeholders::_2);
    handlers["JETP"] = std::bind(&PSEntity::parseREAL_OBJT_JETP, this, std::placeholders::_1, std::placeholders::_2);
    handlers["GRND"] = std::bind(&PSEntity::parseREAL_OBJT_GRND, this, std::placeholders::_1, std::placeholders::_2);
    handlers["ORNT"] = std::bind(&PSEntity::parseREAL_OBJT_ORNT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["EXTE"] = std::bind(&PSEntity::parseREAL_OBJT_EXTE, this, std::placeholders::_1, std::placeholders::_2);
    handlers["RNWY"] = std::bind(&PSEntity::parseREAL_OBJT_RNWY, this, std::placeholders::_1, std::placeholders::_2);
    handlers["MISS"] = std::bind(&PSEntity::parseREAL_OBJT_MISS, this, std::placeholders::_1, std::placeholders::_2);
    handlers["BOMB"] = std::bind(&PSEntity::parseREAL_OBJT_BOMB, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TRCR"] = std::bind(&PSEntity::parseREAL_OBJT_TRCR, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SWPN"] = std::bind(&PSEntity::parseREAL_OBJT_SWPN, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SMKG"] = std::bind(&PSEntity::parseREAL_OBJT_SMKG, this, std::placeholders::_1, std::placeholders::_2);
    handlers["EXPL"] = std::bind(&PSEntity::parseREAL_OBJT_EXPL, this, std::placeholders::_1, std::placeholders::_2);
    handlers["OMOB"] = std::bind(&PSEntity::parseREAL_OBJT_OMOB, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DEBR"] = std::bind(&PSEntity::parseREAL_OBJT_DEBR, this, std::placeholders::_1, std::placeholders::_2);
    handlers["PODR"] = std::bind(&PSEntity::parseREAL_OBJT_PODR, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DECY"] = std::bind(&PSEntity::parseREAL_OBJT_DECY, this, std::placeholders::_1, std::placeholders::_2);
    handlers["XMIT"] = std::bind(&PSEntity::parseREAL_OBJT_XMIT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["LSON"] = std::bind(&PSEntity::parseREAL_OBJT_LSON, this, std::placeholders::_1, std::placeholders::_2);
    handlers["EJEC"] = std::bind(&PSEntity::parseREAL_OBJT_EJEC, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SHIP"] = std::bind(&PSEntity::parseREAL_OBJT_SHIP, this, std::placeholders::_1, std::placeholders::_2);
    handlers["CARR"] = std::bind(&PSEntity::parseREAL_OBJT_CARR, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TORP"] = std::bind(&PSEntity::parseREAL_OBJT_TORP, this, std::placeholders::_1, std::placeholders::_2);
    handlers["NPLM"] = std::bind(&PSEntity::parseREAL_OBJT_NPLM, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
    printf("PSEntity: %s, KIND %d, type libRealSpace %d, vie %d\n", this->name.c_str(), this->kind, (int)this->entity_type, this->health);
}

void PSEntity::parseREAL_OBJT_KIND(uint8_t *data, size_t size) {
    if (size >= 1) {
        this->kind = (PSKind)data[0];
    }
}

void PSEntity::parseREAL_OBJT_JETP(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    this->entity_type = EntityType::jet;
    std::unordered_map<std::string, std::function<void(uint8_t *data, size_t size)>> handlers;
    handlers["EXPL"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_EXPL, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DEBR"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_DEBR, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DEST"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_DEST, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SMOK"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_SMOK, this, std::placeholders::_1, std::placeholders::_2);
    handlers["CHLD"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_CHLD, this, std::placeholders::_1, std::placeholders::_2);
    handlers["JINF"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_JINF, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DAMG"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_DAMG, this, std::placeholders::_1, std::placeholders::_2);
    handlers["EJEC"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_EJEC, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SIGN"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_SIGN, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TRGT"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_TRGT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["CTRL"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_CTRL, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TOFF"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_TOFF, this, std::placeholders::_1, std::placeholders::_2);
    handlers["LAND"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_LAND, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DYNM"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_DYNM, this, std::placeholders::_1, std::placeholders::_2);
    handlers["WEAP"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_WEAP, this, std::placeholders::_1, std::placeholders::_2);
    handlers["CKPT"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_CKPT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["KIND"] = std::bind(&PSEntity::parseREAL_OBJT_KIND, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

void PSEntity::parseREAL_OBJT_GRND(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    this->entity_type = EntityType::ground;
    std::unordered_map<std::string, std::function<void(uint8_t *data, size_t size)>> handlers;
    handlers["EXPL"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_EXPL, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DEBR"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_DEBR, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DEST"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_DEST, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DAMG"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_DAMG, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TRGT"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_TRGT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DYNM"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_DYNM, this, std::placeholders::_1, std::placeholders::_2);
    handlers["KIND"] = std::bind(&PSEntity::parseREAL_OBJT_KIND, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

void PSEntity::parseREAL_OBJT_ORNT(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    this->entity_type = EntityType::ornt;
    std::unordered_map<std::string, std::function<void(uint8_t *data, size_t size)>> handlers;
    handlers["EXPL"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_EXPL, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DEBR"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_DEBR, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DEST"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_DEST, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DAMG"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_DAMG, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TRGT"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_TRGT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SIGN"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_SIGN, this, std::placeholders::_1, std::placeholders::_2);
    handlers["KIND"] = std::bind(&PSEntity::parseREAL_OBJT_KIND, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

void PSEntity::parseREAL_OBJT_SWPN(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    this->entity_type = EntityType::swpn;
    std::unordered_map<std::string, std::function<void(uint8_t *data, size_t size)>> handlers;
    handlers["EXPL"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_EXPL, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DEBR"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_DEBR, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DEST"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_DEST, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DAMG"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_DAMG, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SIGN"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_SIGN, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TRGT"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_TRGT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DYNM"] = std::bind(&PSEntity::parseREAL_OBJT_SWPN_DYNM, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DATA"] = std::bind(&PSEntity::parseREAL_OBJT_SWPN_DATA, this, std::placeholders::_1, std::placeholders::_2);
    handlers["ALGN"] = std::bind(&PSEntity::parseREAL_OBJT_SWPN_ALGN, this, std::placeholders::_1, std::placeholders::_2);
    handlers["KIND"] = std::bind(&PSEntity::parseREAL_OBJT_KIND, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

void PSEntity::parseREAL_OBJT_XMIT(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    this->entity_type = EntityType::xmit;
    std::unordered_map<std::string, std::function<void(uint8_t *data, size_t size)>> handlers;
    handlers["EXPL"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_EXPL, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DEBR"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_DEBR, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DEST"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_DEST, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DAMG"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_DAMG, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TRGT"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_TRGT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SIGN"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_SIGN, this, std::placeholders::_1, std::placeholders::_2);
    handlers["KIND"] = std::bind(&PSEntity::parseREAL_OBJT_KIND, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

void PSEntity::parseREAL_OBJT_BOMB(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    this->entity_type = EntityType::bomb;
    std::unordered_map<std::string, std::function<void(uint8_t *data, size_t size)>> handlers;
    handlers["EXPL"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_EXPL, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SIGN"] = std::bind(&PSEntity::parseREAL_OBJT_MISS_SIGN, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TRGT"] = std::bind(&PSEntity::parseREAL_OBJT_MISS_TRGT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SMOK"] = std::bind(&PSEntity::parseREAL_OBJT_MISS_SMOK, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DAMG"] = std::bind(&PSEntity::parseREAL_OBJT_MISS_DAMG, this, std::placeholders::_1, std::placeholders::_2);
    handlers["WDAT"] = std::bind(&PSEntity::parseREAL_OBJT_MISS_WDAT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DATA"] = std::bind(&PSEntity::parseREAL_OBJT_BOMB_DATA, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DYNM"] = std::bind(&PSEntity::parseREAL_OBJT_MISS_DYNM, this, std::placeholders::_1, std::placeholders::_2);
    handlers["KIND"] = std::bind(&PSEntity::parseREAL_OBJT_KIND, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

void PSEntity::parseREAL_OBJT_MISS(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    this->entity_type = EntityType::missiles;
    std::unordered_map<std::string, std::function<void(uint8_t *data, size_t size)>> handlers;
    handlers["EXPL"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_EXPL, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SIGN"] = std::bind(&PSEntity::parseREAL_OBJT_MISS_SIGN, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TRGT"] = std::bind(&PSEntity::parseREAL_OBJT_MISS_TRGT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SMOK"] = std::bind(&PSEntity::parseREAL_OBJT_MISS_SMOK, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DAMG"] = std::bind(&PSEntity::parseREAL_OBJT_MISS_DAMG, this, std::placeholders::_1, std::placeholders::_2);
    handlers["WDAT"] = std::bind(&PSEntity::parseREAL_OBJT_MISS_WDAT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DATA"] = std::bind(&PSEntity::parseREAL_OBJT_MISS_DATA, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DYNM"] = std::bind(&PSEntity::parseREAL_OBJT_MISS_DYNM, this, std::placeholders::_1, std::placeholders::_2);
    handlers["KIND"] = std::bind(&PSEntity::parseREAL_OBJT_KIND, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

void PSEntity::parseREAL_OBJT_TRCR(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    this->entity_type = EntityType::tracer;
    std::unordered_map<std::string, std::function<void(uint8_t *data, size_t size)>> handlers;
    handlers["INFO"] = std::bind(&PSEntity::parseREAL_OBJT_TRCR_INFO, this, std::placeholders::_1, std::placeholders::_2);
    handlers["EXPL"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_EXPL, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SIGN"] = std::bind(&PSEntity::parseREAL_OBJT_MISS_SIGN, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TRGT"] = std::bind(&PSEntity::parseREAL_OBJT_MISS_TRGT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SMOK"] = std::bind(&PSEntity::parseREAL_OBJT_MISS_SMOK, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DAMG"] = std::bind(&PSEntity::parseREAL_OBJT_MISS_DAMG, this, std::placeholders::_1, std::placeholders::_2);
    handlers["WDAT"] = std::bind(&PSEntity::parseREAL_OBJT_MISS_WDAT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DATA"] = std::bind(&PSEntity::parseREAL_OBJT_MISS_DATA, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DYNM"] = std::bind(&PSEntity::parseREAL_OBJT_MISS_DYNM, this, std::placeholders::_1, std::placeholders::_2);
    handlers["KIND"] = std::bind(&PSEntity::parseREAL_OBJT_KIND, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

void PSEntity::parseREAL_OBJT_EXPL(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    this->entity_type = EntityType::explosion;
    std::unordered_map<std::string, std::function<void(uint8_t *data, size_t size)>> handlers;
    handlers["KIND"] = std::bind(&PSEntity::parseREAL_OBJT_KIND, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

void PSEntity::parseREAL_OBJT_DEBR(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    this->entity_type = EntityType::debris;
    std::unordered_map<std::string, std::function<void(uint8_t *data, size_t size)>> handlers;
    handlers["KIND"] = std::bind(&PSEntity::parseREAL_OBJT_KIND, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

void PSEntity::parseREAL_OBJT_OMOB(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    this->entity_type = EntityType::object_mobile;
    std::unordered_map<std::string, std::function<void(uint8_t *data, size_t size)>> handlers;
    handlers["KIND"] = std::bind(&PSEntity::parseREAL_OBJT_KIND, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

void PSEntity::parseREAL_OBJT_SMKG(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    this->entity_type = EntityType::destroyed_object;
    std::unordered_map<std::string, std::function<void(uint8_t *data, size_t size)>> handlers;
    handlers["INFO"] = std::bind(&PSEntity::parseREAL_OBJT_SMKG_INFO, this, std::placeholders::_1, std::placeholders::_2);
    handlers["KIND"] = std::bind(&PSEntity::parseREAL_OBJT_KIND, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

// Seul KIND est lu.
void PSEntity::parseREAL_OBJT_LSON(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    this->entity_type = EntityType::lson;
    std::unordered_map<std::string, std::function<void(uint8_t *data, size_t size)>> handlers;
    handlers["KIND"] = std::bind(&PSEntity::parseREAL_OBJT_KIND, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

// Seul KIND est lu.
void PSEntity::parseREAL_OBJT_EJEC(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    this->entity_type = EntityType::ejec;
    std::unordered_map<std::string, std::function<void(uint8_t *data, size_t size)>> handlers;
    handlers["KIND"] = std::bind(&PSEntity::parseREAL_OBJT_KIND, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

// Les chunks CHLD et SHPD ne sont pas lus.
void PSEntity::parseREAL_OBJT_SHIP(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    this->entity_type = EntityType::ship;
    std::unordered_map<std::string, std::function<void(uint8_t *data, size_t size)>> handlers;
    handlers["EXPL"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_EXPL, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DEBR"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_DEBR, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DEST"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_DEST, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DAMG"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_DAMG, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TRGT"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_TRGT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DYNM"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_DYNM, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SIGN"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_SIGN, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SMOK"] = std::bind(&PSEntity::parseREAL_OBJT_MISS_SMOK, this, std::placeholders::_1, std::placeholders::_2);
    handlers["KIND"] = std::bind(&PSEntity::parseREAL_OBJT_KIND, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

// 8 entiers en 24.8 ; un point du fichier (c0, c1, c2) se lit x = c0, y = c2, z = -c1.
void PSEntity::parseREAL_OBJT_CARR_CARD(uint8_t *data, size_t size) {
    if (size < 32) {
        return;
    }
    ByteStream bs(data, size);
    this->deck_height = bs.ReadFixedFloatLE();
    this->deck_value_2 = bs.ReadFixedFloatLE();
    float a0 = bs.ReadFixedFloatLE();
    float a1 = bs.ReadFixedFloatLE();
    float a2 = bs.ReadFixedFloatLE();
    this->deck_point_a = Vector3D(a0, a2, -a1);
    float b0 = bs.ReadFixedFloatLE();
    float b1 = bs.ReadFixedFloatLE();
    float b2 = bs.ReadFixedFloatLE();
    this->deck_point_b = Vector3D(b0, b2, -b1);
    this->has_carrier_deck = true;
    printf("PSEntity: %s CARD pont a %.1f m, valeur 2 %.1f, point A (%.1f, %.1f, %.1f), point B (%.1f, %.1f, %.1f)\n", this->name.c_str(), this->deck_height, this->deck_value_2, this->deck_point_a.x, this->deck_point_a.y, this->deck_point_a.z, this->deck_point_b.x, this->deck_point_b.y, this->deck_point_b.z);
}

// Les chunks CHLD et LSOD ne sont pas lus.
void PSEntity::parseREAL_OBJT_CARR(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    this->entity_type = EntityType::carr;
    std::unordered_map<std::string, std::function<void(uint8_t *data, size_t size)>> handlers;
    handlers["CARD"] = std::bind(&PSEntity::parseREAL_OBJT_CARR_CARD, this, std::placeholders::_1, std::placeholders::_2);
    handlers["EXPL"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_EXPL, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DEBR"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_DEBR, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DEST"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_DEST, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DAMG"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_DAMG, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TRGT"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_TRGT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DYNM"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_DYNM, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SIGN"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_SIGN, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SMOK"] = std::bind(&PSEntity::parseREAL_OBJT_MISS_SMOK, this, std::placeholders::_1, std::placeholders::_2);
    handlers["KIND"] = std::bind(&PSEntity::parseREAL_OBJT_KIND, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

// Les chunks RTRP, SHAD, SPLS et NEXP ne sont pas lus.
void PSEntity::parseREAL_OBJT_TORP(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    this->entity_type = EntityType::torp;
    std::unordered_map<std::string, std::function<void(uint8_t *data, size_t size)>> handlers;
    handlers["EXPL"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_EXPL, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SMOK"] = std::bind(&PSEntity::parseREAL_OBJT_MISS_SMOK, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DAMG"] = std::bind(&PSEntity::parseREAL_OBJT_MISS_DAMG, this, std::placeholders::_1, std::placeholders::_2);
    handlers["WDAT"] = std::bind(&PSEntity::parseREAL_OBJT_MISS_WDAT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DATA"] = std::bind(&PSEntity::parseREAL_OBJT_BOMB_DATA, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DYNM"] = std::bind(&PSEntity::parseREAL_OBJT_MISS_DYNM, this, std::placeholders::_1, std::placeholders::_2);
    handlers["KIND"] = std::bind(&PSEntity::parseREAL_OBJT_KIND, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

// Les chunks RTRP, SHAD, SPLS et NEXP ne sont pas lus.
void PSEntity::parseREAL_OBJT_NPLM(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    this->entity_type = EntityType::nplm;
    std::unordered_map<std::string, std::function<void(uint8_t *data, size_t size)>> handlers;
    handlers["EXPL"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_EXPL, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SMOK"] = std::bind(&PSEntity::parseREAL_OBJT_MISS_SMOK, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DAMG"] = std::bind(&PSEntity::parseREAL_OBJT_MISS_DAMG, this, std::placeholders::_1, std::placeholders::_2);
    handlers["WDAT"] = std::bind(&PSEntity::parseREAL_OBJT_MISS_WDAT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DATA"] = std::bind(&PSEntity::parseREAL_OBJT_BOMB_DATA, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DYNM"] = std::bind(&PSEntity::parseREAL_OBJT_MISS_DYNM, this, std::placeholders::_1, std::placeholders::_2);
    handlers["KIND"] = std::bind(&PSEntity::parseREAL_OBJT_KIND, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

// Copie de RSEntity::parseREAL_OBJT_JETP_DYNM avec les lecteurs THRS et JDYN de Pacific Strike.
void PSEntity::parseREAL_OBJT_JETP_DYNM(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    std::unordered_map<std::string, std::function<void(uint8_t *data, size_t size)>> handlers;
    handlers["DYNM"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_DYNM_DYNM, this, std::placeholders::_1, std::placeholders::_2);
    handlers["ORDY"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_DYNM_ORDY, this, std::placeholders::_1, std::placeholders::_2);
    handlers["STBL"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_DYNM_STBL, this, std::placeholders::_1, std::placeholders::_2);
    handlers["ATMO"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_DYNM_ATMO, this, std::placeholders::_1, std::placeholders::_2);
    handlers["GRAV"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_DYNM_GRAV, this, std::placeholders::_1, std::placeholders::_2);
    handlers["THRS"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_DYNM_THRS, this, std::placeholders::_1, std::placeholders::_2);
    handlers["JDYN"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_DYNM_JDYN, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

// AircraftDynamics_ReadThrustChunk_B81AC : u32 poussée maximale, u32 puissance, 3 octets comme Strike Commander
// (le 3e = plafond en centaines de mètres), puis deux u32 ; les u32 sont en 24.8.
void PSEntity::parseREAL_OBJT_JETP_DYNM_THRS(uint8_t *data, size_t size) {
    if (size < 19) {
        return;
    }
    ByteStream bs(data, size);
    this->thrust_in_newton = (int)bs.ReadFixedFloatLE();
    this->engine_power_hp = bs.ReadFixedFloatLE();
    this->thrust_mil_fraction = bs.ReadByte() / 256.0f;
    this->thrust_ref_alt_fraction = bs.ReadByte() / 256.0f;
    this->thrust_cutoff_alt_raw = bs.ReadByte();
    this->ceiling_m = this->thrust_cutoff_alt_raw * 100.0f;
    this->thrs_field_6 = bs.ReadFixedFloatLE();
    this->stall_speed_ms = bs.ReadFixedFloatLE();
    printf("PSEntity: %s THRS puissance %.0f ch, plafond %.0f m, champ 6 %.3f, decrochage %.3f m/s\n", this->name.c_str(), this->engine_power_hp, this->ceiling_m, this->thrs_field_6, this->stall_speed_ms);
}

// Même lecture que Strike Commander ; les champs 23 et 26 servent à la poussée de l'hélice. Le champ 26 est un entier
// (pas en 24.8) : relu ici depuis le fichier.
void PSEntity::parseREAL_OBJT_JETP_DYNM_JDYN(uint8_t *data, size_t size) {
    RSEntity::parseREAL_OBJT_JETP_DYNM_JDYN(data, size);
    if (size < 73) {
        return;
    }
    ByteStream bs(data + 61, size - 61);
    this->reference_speed_ms = (float)bs.ReadShort();
    ByteStream altitude(data + 67, size - 67);
    this->rated_altitude_m = (float)altitude.ReadInt32LE();
    printf("PSEntity: %s JDYN vitesse de reference %.0f m/s, altitude de retablissement %.0f m\n", this->name.c_str(), this->reference_speed_ms, this->rated_altitude_m);
}

// sub_B0360 de ovr315 : WDAT de 14 octets (Strike Commander : 22). mot dégâts, mot rayon, octet catégorie de l'objet
// tiré, octet numéro d'arme (PSWeaponId), deux u32. Le jeu lit toujours 14 octets (le WDAT de HVAR n'en a que 11).
// Les deux u32 sont rangés comme les deux derniers champs de Strike Commander ; leur rôle n'est pas relu dans PACIFIC.EXE.
void PSEntity::parseREAL_OBJT_MISS_WDAT(uint8_t *data, size_t size) {
    WDAT *wdat = new WDAT();
    ByteStream bs(data, 14);
    wdat->damage = bs.ReadShort();
    wdat->radius = bs.ReadShort();
    wdat->launch_class = bs.ReadByte();
    this->ps_weapon_id = (PSWeaponId)bs.ReadByte();
    switch (this->ps_weapon_id) {
    case PS_WEAPON_GUN:
        wdat->weapon_id = ID_20MM;
        break;
    case PS_WEAPON_BOMB_100LB:
    case PS_WEAPON_BOMB_500LB:
    case PS_WEAPON_BOMB_1000LB:
    case PS_WEAPON_TORPEDO:
    case PS_WEAPON_NAPALM:
        wdat->weapon_id = ID_MK82;
        break;
    case PS_WEAPON_ROCKET:
        wdat->weapon_id = ID_AIM9J;
        break;
    default:
        wdat->weapon_id = 0;
        break;
    }
    wdat->effective_range = bs.ReadInt32LE();
    wdat->fire_interval = (float)bs.ReadInt32LE() / 256.0f;
    this->wdat = wdat;
    printf("PSEntity: %s WDAT degats %d, rayon %d, categorie %d, arme %d (libRealSpace %d), u32 %u, intervalle %.3f\n", this->name.c_str(), wdat->damage, wdat->radius, wdat->launch_class, (int)this->ps_weapon_id, wdat->weapon_id, wdat->effective_range, wdat->fire_interval);
}

// Copie de RSEntity::parseREAL_OBJT_JETP_WEAP avec le lecteur WPNS de Pacific Strike.
void PSEntity::parseREAL_OBJT_JETP_WEAP(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    std::unordered_map<std::string, std::function<void(uint8_t *data, size_t size)>> handlers;
    handlers["INFO"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_WEAP_INFO, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DCOY"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_WEAP_DCOY, this, std::placeholders::_1, std::placeholders::_2);
    handlers["WPNS"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_WEAP_WPNS, this, std::placeholders::_1, std::placeholders::_2);
    handlers["HPTS"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_WEAP_HPTS, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DAMG"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_WEAP_DAMG, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

// Copie de RSEntity::parseREAL_OBJT_JETP_WEAP_WPNS : les armes sont chargées en PSEntity (WDAT de Pacific Strike).
void PSEntity::parseREAL_OBJT_JETP_WEAP_WPNS(uint8_t *data, size_t size) {
    if (size < 10) {
        return;
    }
    size_t nb_weap = size / 10;
    ByteStream bs(data, size);
    for (size_t i = 0; i < nb_weap; i++) {
        WEAPS *weapon = new WEAPS();
        weapon->nb_weap = bs.ReadShort();
        weapon->name = bs.ReadString(8);
        std::transform(weapon->name.begin(), weapon->name.end(), weapon->name.begin(), ::toupper);
        weapon->objct = nullptr;
        std::string tmpname = assetsManager.object_root_path + weapon->name + ".IFF";
        TreEntry *entry = assetsManager.GetEntryByName(tmpname);
        if (entry != nullptr) {
            PSEntity *objct = new PSEntity();
            objct->InitFromRAM(entry->data, entry->size, tmpname);
            weapon->objct = objct;
        }
        this->weaps.push_back(weapon);
    }
}

// Une arme d'un avion de Pacific Strike est chargée en PSEntity (parseREAL_OBJT_JETP_WEAP_WPNS).
PSWeaponId PSEntity::weaponId(RSEntity *weapon) {
    if (weapon == nullptr) {
        return PS_WEAPON_NONE;
    }
    return static_cast<PSEntity *>(weapon)->ps_weapon_id;
}

// Emport imposé par la mission (fichier nommé dans PART) : relit les chunks de l'avion avec les lecteurs de PSEntity.
void PSEntity::loadWeaponLoad(uint8_t *data, size_t size) {
    this->parseREAL_OBJT_JETP(data, size);
}
