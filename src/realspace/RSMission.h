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


// Zone du chunk AREA (ExprSlot_ConstructMain_A974A). position : centre pour une sphère, centre du plancher pour
// un cylindre ou une boîte.
struct AREA {
    int id;
    unsigned char AreaType;
    char AreaName[33];
    Vector3D position;
    // Valeurs historiques, gardées pour l'affichage : sphère rayon / rayon, cylindre hauteur / rayon, boîte
    // étendue est / étendue sud.
    unsigned int AreaWidth;
    unsigned int AreaHeight;
    std::vector<uint8_t> unknown_bytes;
    float radius{0.0f}; // 'S', 'C'
    float height{0.0f}; // 'C', 'B' : vers le haut depuis le plancher
    float east{0.0f};   // 'B' : vers +X
    float west{0.0f};   // 'B' : vers -X
    float north{0.0f};  // 'B' : vers -Z
    float south{0.0f};  // 'B' : vers +Z
    // Test d'appartenance du jeu, bornes incluses (HitShape_Sphere_ContainsPoint_52E91,
    // HitShape_Radial2D_ContainsPoint_52F99, HitShape_Box_ContainsPoint_52F0F).
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

    // Champs lus par PartEntry_LoadAndResolveNames_A9E3C.
    uint8_t placement{0};       // octet 28 : 0 = décalage depuis la zone, 1 = repère de l'avion du joueur
    int16_t speed{0};           // octets 43-44 : vitesse initiale le long du nez, 0 = à l'arrêt
    uint8_t team{0};            // octet 45 : camp, 1 = allié, 255 = ennemi
    uint8_t follow_ally{255};   // octet 46 : allié par défaut (index PART), 255 = aucun ; OP_SET_OBJ_FOLLOW_ALLY 255 le désigne
    int16_t formation_side{0};  // octets 48-49 : poste en formation, m
    int16_t formation_ahead{0}; // octets 50-51
    int16_t formation_up{0};    // octets 52-53 ; l'octet 47 n'est pas recopié par le chargeur, rôle non établi
    // Décalage et cap tels que lus, jamais modifiés : le placement dans le repère du joueur repart d'eux à chaque activation.
    Vector3D relative_position;
    uint16_t relative_azymuth{0};
    uint8_t unknown_byte_47{0};
    RSEntity *entity{nullptr};
    bool alive{true};
};

// Chunk SPOT : enregistrements de 14 octets (Mission_LoadSpotList_AA6C9).
struct SPOT {
    int id;
    short area_id;     // octets 0-1 : index de la zone AREA ; -1 (65535) = aucune, le décalage est alors absolu
    Vector3D position; // octets 2-13 : décalage depuis le point de référence de la zone (3 int32 24.8), m
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
    int16_t on_is_activated {-1};   // octets 3-4 : quand la scène devient la scène courante
    int16_t on_mission_update {-1}; // octets 5-6 : à chaque image (scène globale, scène courante)
    int16_t on_leaving {-1};        // octets 7-8 : quand la scène cesse d'être courante, et en fin de mission
    bool is_coord_on_area {false};
    std::vector<uint8_t> unknown_bytes;
    std::vector<uint8_t> cast;
};
// Chunk LOAD : enregistrements de 9 octets, lus jusqu'à la fin du chunk (MissionScenario_LoadFieldGroup_A8225).
// Chaque nom est chargé comme modèle (IFF_LoadModelMain : OBJECTS\<nom>.IFF) au chargement de la mission.
struct MISN_LOAD {
    std::string name; // octets 0-7 : nom du modèle
    uint8_t flag{0};  // octet 8 : rangé dans le modèle chargé (+2) ; rôle non établi (0 partout, sauf un F-16 à 1 dans 6 fichiers hors campagne)
};
// Chunk CACH : enregistrements de 10 octets. Format lu dans les fichiers ; aucun lecteur trouvé dans le jeu.
struct MISN_CACH {
    std::string name;   // octets 0-7 : nom d'un modèle d'OBJECTS
    uint16_t count{0};  // octets 8-9 : 1 (1 801 enregistrements) ou 0 (15) dans les missions ; rôle non établi
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