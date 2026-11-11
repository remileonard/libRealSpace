#include "PSEntity.h"

void PSEntity::parseKIND(uint8_t *data, size_t size) {
    if (size >= 1) {
        this->kind = data[0];
    }
}

// Classe déjà lue par RSEntity : seul le chunk KIND est relevé ici.
void PSEntity::parseClassKind(std::string class_name, uint8_t *data, size_t size) {
    this->object_class = class_name;
    IFFSaxLexer lexer;
    std::unordered_map<std::string, std::function<void(uint8_t *data, size_t size)>> handlers;
    handlers["KIND"] = std::bind(&PSEntity::parseKIND, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

// Navire (SHIP) et porte-avions (CARR) : copie de RSEntity::parseREAL_OBJT_GRND, plus KIND, SIGN et SMOK. Les chunks
// CHLD, SHPD, CARD et LSOD ne sont pas lus.
void PSEntity::parseREAL_OBJT_SHIP(std::string class_name, uint8_t *data, size_t size) {
    this->object_class = class_name;
    IFFSaxLexer lexer;
    std::unordered_map<std::string, std::function<void(uint8_t *data, size_t size)>> handlers;
    handlers["EXPL"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_EXPL, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DEBR"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_DEBR, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DEST"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_DEST, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DAMG"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_DAMG, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TRGT"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_TRGT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DYNM"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_DYNM, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SIGN"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_SIGN, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SMOK"] = std::bind(&PSEntity::parseREAL_OBJT_MISS_SMOK, this, std::placeholders::_1, std::placeholders::_2);
    handlers["KIND"] = std::bind(&PSEntity::parseKIND, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

// Torpille (TORP) et napalm (NPLM) : copie de RSEntity::parseREAL_OBJT_BOMB, plus KIND. Les chunks RTRP, SHAD, SPLS
// et NEXP ne sont pas lus.
void PSEntity::parseREAL_OBJT_TORP(std::string class_name, uint8_t *data, size_t size) {
    this->object_class = class_name;
    IFFSaxLexer lexer;
    std::unordered_map<std::string, std::function<void(uint8_t *data, size_t size)>> handlers;
    handlers["EXPL"] = std::bind(&PSEntity::parseREAL_OBJT_JETP_EXPL, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SMOK"] = std::bind(&PSEntity::parseREAL_OBJT_MISS_SMOK, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DAMG"] = std::bind(&PSEntity::parseREAL_OBJT_MISS_DAMG, this, std::placeholders::_1, std::placeholders::_2);
    handlers["WDAT"] = std::bind(&PSEntity::parseREAL_OBJT_MISS_WDAT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DATA"] = std::bind(&PSEntity::parseREAL_OBJT_BOMB_DATA, this, std::placeholders::_1, std::placeholders::_2);
    handlers["DYNM"] = std::bind(&PSEntity::parseREAL_OBJT_MISS_DYNM, this, std::placeholders::_1, std::placeholders::_2);
    handlers["KIND"] = std::bind(&PSEntity::parseKIND, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

// Type de libRealSpace des classes propres à Pacific Strike. Les classes communes gardent le type posé par RSEntity.
void PSEntity::setPacificEntityType() {
    if (this->object_class == "SHIP") {
        this->entity_type = EntityType::ground;
    } else if (this->object_class == "CARR") {
        this->entity_type = EntityType::ground;
    } else if (this->object_class == "TORP") {
        this->entity_type = EntityType::bomb;
    } else if (this->object_class == "NPLM") {
        this->entity_type = EntityType::bomb;
    } else if (this->object_class == "LSON") {
        this->entity_type = EntityType::ornt;
    } else if (this->object_class == "EJEC") {
        this->entity_type = EntityType::object_mobile;
    }
}

void PSEntity::parseREAL_OBJT(uint8_t *data, size_t size) {
    RSEntity::parseREAL_OBJT(data, size);
    IFFSaxLexer lexer;
    std::unordered_map<std::string, std::function<void(uint8_t *data, size_t size)>> handlers;
    handlers["JETP"] = std::bind(&PSEntity::parseClassKind, this, std::string("JETP"), std::placeholders::_1, std::placeholders::_2);
    handlers["GRND"] = std::bind(&PSEntity::parseClassKind, this, std::string("GRND"), std::placeholders::_1, std::placeholders::_2);
    handlers["ORNT"] = std::bind(&PSEntity::parseClassKind, this, std::string("ORNT"), std::placeholders::_1, std::placeholders::_2);
    handlers["SWPN"] = std::bind(&PSEntity::parseClassKind, this, std::string("SWPN"), std::placeholders::_1, std::placeholders::_2);
    handlers["XMIT"] = std::bind(&PSEntity::parseClassKind, this, std::string("XMIT"), std::placeholders::_1, std::placeholders::_2);
    handlers["BOMB"] = std::bind(&PSEntity::parseClassKind, this, std::string("BOMB"), std::placeholders::_1, std::placeholders::_2);
    handlers["MISS"] = std::bind(&PSEntity::parseClassKind, this, std::string("MISS"), std::placeholders::_1, std::placeholders::_2);
    handlers["TRCR"] = std::bind(&PSEntity::parseClassKind, this, std::string("TRCR"), std::placeholders::_1, std::placeholders::_2);
    handlers["EXPL"] = std::bind(&PSEntity::parseClassKind, this, std::string("EXPL"), std::placeholders::_1, std::placeholders::_2);
    handlers["DEBR"] = std::bind(&PSEntity::parseClassKind, this, std::string("DEBR"), std::placeholders::_1, std::placeholders::_2);
    handlers["OMOB"] = std::bind(&PSEntity::parseClassKind, this, std::string("OMOB"), std::placeholders::_1, std::placeholders::_2);
    handlers["SMKG"] = std::bind(&PSEntity::parseClassKind, this, std::string("SMKG"), std::placeholders::_1, std::placeholders::_2);
    handlers["LSON"] = std::bind(&PSEntity::parseClassKind, this, std::string("LSON"), std::placeholders::_1, std::placeholders::_2);
    handlers["EJEC"] = std::bind(&PSEntity::parseClassKind, this, std::string("EJEC"), std::placeholders::_1, std::placeholders::_2);
    handlers["SHIP"] = std::bind(&PSEntity::parseREAL_OBJT_SHIP, this, std::string("SHIP"), std::placeholders::_1, std::placeholders::_2);
    handlers["CARR"] = std::bind(&PSEntity::parseREAL_OBJT_SHIP, this, std::string("CARR"), std::placeholders::_1, std::placeholders::_2);
    handlers["TORP"] = std::bind(&PSEntity::parseREAL_OBJT_TORP, this, std::string("TORP"), std::placeholders::_1, std::placeholders::_2);
    handlers["NPLM"] = std::bind(&PSEntity::parseREAL_OBJT_TORP, this, std::string("NPLM"), std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
    this->setPacificEntityType();
    printf("PSEntity: %s, classe %s, KIND %d, type libRealSpace %d, vie %d\n", this->name.c_str(), this->object_class.c_str(), this->kind, (int)this->entity_type, this->health);
}
