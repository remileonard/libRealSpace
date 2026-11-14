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

// Rôle de l'avion, octet 5 du chunk JINF (RSEntity::combat_class), lu par Targeting_MayAcquireTarget_FF05.
enum PSCombatClass : uint8_t {
    // HELLDIVE, JUDY, SBD, VAL
    PS_COMBAT_CLASS_DIVE_BOMBER = 0,
    // JILL, KATETORP, TBD, TBF
    PS_COMBAT_CLASS_TORPEDO_BOMBER = 1,
    // BETTY, EMILY, KATE
    PS_COMBAT_CLASS_BOMBER = 2,
    // BEARCAT, CLAUDE, CORSAIR, F4F3, F4F4, FRANK, GEORGE, HELLCAT, HELLCAT2, ZERO, ZERO5, ZEROF
    PS_COMBAT_CLASS_FIGHTER = 3,
    // BAKA, ZEROK
    PS_COMBAT_CLASS_KAMIKAZE = 4
};

// Numéro d'arme de l'octet 5 du chunk WDAT (PACIFIC.EXE, sub_B0360 de ovr315), nommé d'après les objets qui le portent.
enum PSWeaponId : uint8_t {
    PS_WEAPON_NONE = 0,
    // B100LB
    PS_WEAPON_BOMB_100LB = 1,
    // B500LB
    PS_WEAPON_BOMB_500LB = 2,
    // HVAR
    PS_WEAPON_ROCKET = 3,
    // B1000LB
    PS_WEAPON_BOMB_1000LB = 4,
    // MK-13
    PS_WEAPON_TORPEDO = 5,
    // NAPALM
    PS_WEAPON_NAPALM = 6,
    // 12-7MM, 20MM, 20MM99-1, 30CAL, 50CAL, 7-7MM97 (classe TRCR)
    PS_WEAPON_GUN = 7
};

class PSEntity : public RSEntity {
public:
    // Chunk KIND : type cherché par les scripts de mission ; 22 quand le chunk est absent (PACIFIC.EXE).
    PSKind kind{ PS_KIND_NONE };
    // Numéro d'arme du WDAT de Pacific Strike ; wdat->weapon_id reçoit l'arme de libRealSpace qui se tire de la même
    // façon (canon, bombe, missile).
    PSWeaponId ps_weapon_id{ PS_WEAPON_NONE };
    // Chunk CARD d'un porte-avions (CarrierDeck_LoadCARD_B0E70) : hauteur de l'avion posé sur le pont, une valeur de
    // rôle non établi, et deux points dans le repère du navire.
    bool has_carrier_deck{ false };
    float deck_height{ 0.0f };
    float deck_value_2{ 0.0f };
    Vector3D deck_point_a;
    Vector3D deck_point_b;
    static PSWeaponId weaponId(RSEntity *weapon);
    void loadWeaponLoad(uint8_t *data, size_t size);
    // Chunk THRS de Pacific Strike (19 octets, AircraftDynamics_ReadThrustChunk_B81AC) : puissance du moteur (ch),
    // plafond (m), un u32 non employé par la poussée, vitesse de décrochage (m/s, sa moitié = vitesse plancher de la poussée).
    float engine_power_hp{ 0.0f };
    float ceiling_m{ 0.0f };
    float thrs_field_6{ 0.0f };
    float stall_speed_ms{ 0.0f };
    // JDYN champ 23 (mot, octet 61) : vitesse de référence de l'hélice (m/s) ; champ 26 (u32 entier, octet 67) :
    // altitude à partir de laquelle le moteur perd de la puissance (m).
    float reference_speed_ms{ 0.0f };
    float rated_altitude_m{ 0.0f };

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
    void parseREAL_OBJT_CARR_CARD(uint8_t *data, size_t size);
    void parseREAL_OBJT_JETP_DYNM(uint8_t *data, size_t size);
    void parseREAL_OBJT_JETP_DYNM_THRS(uint8_t *data, size_t size);
    void parseREAL_OBJT_JETP_DYNM_JDYN(uint8_t *data, size_t size);
    void parseREAL_OBJT_MISS_WDAT(uint8_t *data, size_t size);
    void parseREAL_OBJT_JETP_WEAP(uint8_t *data, size_t size);
    void parseREAL_OBJT_JETP_WEAP_WPNS(uint8_t *data, size_t size);
};
