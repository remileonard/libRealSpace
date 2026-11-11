#include "PSEntity.h"

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

// Les chunks CHLD, CARD et LSOD ne sont pas lus.
void PSEntity::parseREAL_OBJT_CARR(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    this->entity_type = EntityType::carr;
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
