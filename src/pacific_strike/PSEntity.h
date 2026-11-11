#pragma once
#include "../realspace/RSEntity.h"

// Objet de Pacific Strike : lecture de RSEntity, plus le chunk KIND et les classes d'objets que Strike Commander
// n'a pas (SHIP, CARR, TORP, NPLM, LSON, EJEC), associées à un type de libRealSpace.
class PSEntity : public RSEntity {
public:
    // Chunk KIND : type cherché par les scripts de mission ; 22 quand le chunk est absent (PACIFIC.EXE).
    uint8_t kind{ 22 };

protected:
    void parseREAL_OBJT(uint8_t *data, size_t size) override;
    void parseREAL_OBJT_JETP(uint8_t *data, size_t size);
    void parseREAL_OBJT_GRND(uint8_t *data, size_t size);
    void parseREAL_OBJT_ORNT(uint8_t *data, size_t size);
    void parseREAL_OBJT_SWPN(uint8_t *data, size_t size);
    void parseREAL_OBJT_XMIT(uint8_t *data, size_t size);
    void parseREAL_OBJT_BOMB(uint8_t *data, size_t size);
    void parseREAL_OBJT_MISS(uint8_t *data, size_t size);
    void parseREAL_OBJT_TRCR(uint8_t *data, size_t size);
    void parseREAL_OBJT_EXPL(uint8_t *data, size_t size);
    void parseREAL_OBJT_DEBR(uint8_t *data, size_t size);
    void parseREAL_OBJT_OMOB(uint8_t *data, size_t size);
    void parseREAL_OBJT_SMKG(uint8_t *data, size_t size);
    void parseREAL_OBJT_LSON(uint8_t *data, size_t size);
    void parseREAL_OBJT_EJEC(uint8_t *data, size_t size);
    void parseREAL_OBJT_SHIP(uint8_t *data, size_t size);
    void parseREAL_OBJT_CARR(uint8_t *data, size_t size);
    void parseREAL_OBJT_TORP(uint8_t *data, size_t size);
    void parseREAL_OBJT_NPLM(uint8_t *data, size_t size);
    void parseREAL_OBJT_KIND(uint8_t *data, size_t size);
};
