#pragma once
#include "../realspace/RSEntity.h"

// Objet de Pacific Strike : lecture de RSEntity, plus le chunk KIND et les classes d'objets que Strike Commander
// n'a pas (SHIP, CARR, TORP, NPLM, LSON, EJEC), associées à un type de libRealSpace.
// Valeurs du chunk KIND, nommées d'après les objets de PACIFIC.DAT qui les portent (liste en commentaire). Les
// scripts de mission cherchent un objet par cette valeur (OP_ATTACK_RANDOM_ENEMY_OF_TYPE...). 6 est demandé par
// deux missions mais aucun objet ne le porte.
enum PSKind : uint8_t {
    // CLAUDE, GEORGE, ZERO, ZERO5, ZEROF, ZEROK
    PS_KIND_ZERO_FIGHTERS = 0,
    // BEARCAT, CORSAIR, F4F3, F4F4, FRANK, HELLCAT, HELLCAT2, SBD
    PS_KIND_FIGHTERS = 1,
    // JILL, KATETORP, TBD, TBF
    PS_KIND_TORPEDO_BOMBERS = 2,
    // BETTY, EMILY, HELLDIVE, JUDY, KATE, VAL
    PS_KIND_BOMBERS = 3,
    // BAKA
    PS_KIND_BAKA = 5,
    // CARRIER, JEEPCV, SHOHO, SHOKAKU
    PS_KIND_CARRIERS = 8,
    // BB, YAMATO
    PS_KIND_BATTLESHIPS = 9,
    // DD, FUBUKI, MLAYER, PTBOAT
    PS_KIND_LIGHT_SHIPS = 10,
    // JTRANS
    PS_KIND_TRANSPORTS = 11,
    // CRUISER
    PS_KIND_CRUISERS = 12,
    // ALC, BARGE, BARGE2
    PS_KIND_LANDING_CRAFTS = 13,
    // CTOWER, HQ, LGHQ, REVMT, UTILITY
    PS_KIND_COMMAND_BUILDINGS = 16,
    // GENERX2, HANGAR, HANGAR2, HANGARX, HOSPITAL
    PS_KIND_HANGARS = 17,
    // 20MMGUN, 20MMQUAD, 20MMTG, 20MMTRT, 30CALDBL... (classe SWPN)
    PS_KIND_GUNS = 18,
    // ARTILL, SHORBAT
    PS_KIND_ARTILLERY = 19,
    // BAKASHEL, GBETTY, GCLAUDE, GGEORGE, GJUDY, GSBD
    PS_KIND_PARKED_AIRCRAFTS = 20,
    // JTANK
    PS_KIND_TANKS = 21,
    // valeur quand le chunk est absent
    PS_KIND_NONE = 22,
    // SANDBNK, SUN, USFLAG, RADIOTW
    PS_KIND_MISC = 23,
    // MONTOON
    PS_KIND_MONTOON = 24,
    // MK-13
    PS_KIND_TORPEDOES = 26,
    // ATRENCH, BBUNKER, JFLAG, PILLBOX, RAFT, TENT, TRENCH
    PS_KIND_FORTIFICATIONS = 27,
    // CRATES, GENER, GREYTANK, GRNTANK, WTOWER
    PS_KIND_DEPOTS = 28
};

class PSEntity : public RSEntity {
public:
    // Chunk KIND : type cherché par les scripts de mission ; 22 quand le chunk est absent (PACIFIC.EXE).
    PSKind kind{ PS_KIND_NONE };

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
