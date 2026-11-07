#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
}
#endif

#include "../commons/IFFSaxLexer.h"
#include "TreArchive.h"
#include "RSEntity.h"
#include "RSProf.h"
#include <stdint.h>
#include <string>
#include <algorithm>
#include <array>


struct AREA {
    int id;
    unsigned char AreaType;
    char AreaName[33];
    Vector3D position;
    unsigned int AreaWidth;
    unsigned int AreaHeight;
    std::vector<uint8_t> unknown_bytes;
    float radius{0.0f};
    float height{0.0f};
    float east{0.0f};
    float west{0.0f};
    float north{0.0f};
    float south{0.0f};
    bool contains(const Vector3D &point) const;
};

struct MISN_PART {
    uint8_t id{0};
    std::string member_name;
	std::string member_name_destroyed;
	std::string weapon_load;
	uint8_t area_id{0};      
    Vector3D position{0, 0, 0};
    uint16_t azymuth{0};
    uint16_t roll{0};
    uint16_t pitch{0};
    std::vector<uint8_t> progs_id;
    
    uint8_t on_is_activated{255};
    uint8_t on_mission_update{255};
    uint8_t on_is_destroyed{255};
    uint8_t on_missions_init{255};

    uint8_t placement{0};
    int16_t speed{0};
    uint8_t team{0};
    uint8_t follow_ally{255};
    int16_t formation_side{0};
    int16_t formation_ahead{0};
    int16_t formation_up{0};
    Vector3D relative_position;
    uint16_t relative_azymuth{0};
    uint8_t unknown_byte_47{0};
    RSEntity *entity{nullptr};
    bool alive{true};
};

struct SPOT {
    int id;
    short area_id;
    Vector3D position;
    Vector3D origin;
};
struct MSGS {
    char message[255];
    int id;
};
struct CAST {
    std::string actor;
    RSProf *profile;
};
struct PROG {
    uint8_t opcode;
    uint16_t arg;
};
struct MISN_SCEN {
    uint8_t is_active {0};
    uint8_t has_been_activated {0};
    int16_t area_id {0};
    std::vector<int16_t> progs_id;
    int16_t on_is_activated {-1};
    int16_t on_mission_update {-1};
    int16_t on_leaving {-1};
    bool has_arrival_point {false};
    int16_t arrival_heading {-1};
    Vector3D arrival_position;
    std::vector<uint16_t> cast;
};

struct MISN_LOAD {
    std::string name;
    uint8_t flag{0};
};

struct MISN_CACH {
    std::string name;
    uint16_t count{0};
};
struct MISN {
    uint16_t version;
    std::string info;
    uint8_t tune;
    std::string name;
    std::string world_filename;
    std::vector<AREA *> areas;
    std::vector<SPOT *> spots;
    std::vector<std::string *> messages;
    std::vector<int16_t> flags;
    std::vector<CAST *> casting;
    std::vector<std::vector<PROG> *> prog;
    std::vector<uint8_t> nums;
    std::vector<MISN_PART *> parts;
    std::vector<uint16_t> team;
    std::vector<MISN_SCEN *> scenes;
    std::vector<MISN_LOAD *> load;
    std::vector<MISN_CACH *> cache;
};
// Mission d'entraînement (TRAINING MISSION d'OPTEST.EXE) : un avion de l'écran de choix, table du segment de données
// d'OPTEST lue par Training_AircraftScreen_56B04 et Training_BuildInstantMission_58EEA (15 octets par avion : nom affiché,
// fichier objet, emport, deux dwords non lus par ce code, drapeau). strike_commander_re/analysis/TRAINING_MISSION.md § 5.
struct MISN_TRAINING_PLANE {
    const char *name;
    const char *object;
    const char *loadout;
    // Drapeau : pilote CAST au niveau choisi, programme de mise à jour 1, vitesse 200 ; sinon « TARGET », programme 2,
    // vitesse 100.
    bool armed;
};

// Réglages des écrans d'entraînement (Training_InitSettings_598FC, 42 octets dans OPTEST).
struct MISN_TRAINING {
    // +40 : 1 = DOGFIGHT, 2 = SEARCH AND DESTROY.
    uint8_t type{0};
    // +12, +14, +16 : nombre d'avions des groupes 0, 1, 2.
    std::array<uint8_t, 3> group_count{};
    // +6, +8, +10 : type d'avion des groupes (rang dans RSMission::training_planes).
    std::array<uint8_t, 3> group_plane{};
    // +0 : altitude choisie, en pieds (5000 par défaut).
    int32_t altitude_feet{5000};
    // +18 : 0 = rookie, 1 = veteran, 2 = ace.
    uint8_t pilot_level{0};
};

class RSMission {
public:
    std::unordered_map<std::string, RSEntity *> *objCache;
    TreArchive *tre;
    
    MISN mission_data;
    RSMission();
    ~RSMission();

    MISN_PART *getPlayerCoord();
    MISN_PART *getObject(const char *name);
    void InitFromRAM(uint8_t *data, size_t size);
    // Les 16 avions de l'écran de choix, dans l'ordre des boutons.
    static const MISN_TRAINING_PLANE training_planes[16];
    // Ajoute à mission_data, lue depuis MISSIONS\TEMPLATE.IFF, ce que Training_BuildInstantMission_58EEA ajoute au
    // modèle pour écrire instant.iff : le jeu n'a pas besoin du fichier pour voler la mission.
    void addTraining(const MISN_TRAINING &training);

protected:
    MISN_PART *player{nullptr};
    virtual void paseMissionScript(uint8_t *data, size_t size);
    void parseMISN(uint8_t *data, size_t size);
    void parseMISN_VERS(uint8_t *data, size_t size);
    void parseMISN_INFO(uint8_t *data, size_t size);
    void parseMISN_TUNE(uint8_t *data, size_t size);
    void parseMISN_NAME(uint8_t *data, size_t size);
    void parseMISN_WRLD(uint8_t *data, size_t size);
    void parseMISN_AREA(uint8_t *data, size_t size);
    void parseMISN_SPOT(uint8_t *data, size_t size);
    void parseMISN_NUMS(uint8_t *data, size_t size);
    void parseMISN_MSGS(uint8_t *data, size_t size);
    void parseMISN_FLAG(uint8_t *data, size_t size);
    void parseMISN_CAST(uint8_t *data, size_t size);
    void parseMISN_PROG(uint8_t *data, size_t size);
    void parseMISN_PART(uint8_t *data, size_t size);
    void parseMISN_TEAM(uint8_t *data, size_t size);
    void parseMISN_PLAY(uint8_t *data, size_t size);
    void parseMISN_LOAD(uint8_t *data, size_t size);
    void parseMISN_CACH(uint8_t *data, size_t size);
    void parseMISN_PLAY_SCEN(uint8_t *data, size_t size);
    void parseMISN_WRLD_FILE(uint8_t *data, size_t size);
};