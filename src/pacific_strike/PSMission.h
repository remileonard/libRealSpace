#include "../strike_commander/precomp.h"

class PSMission: public SCMission {
public:
    PSMission();
    PSMission(std::string mission_name, std::unordered_map<std::string, RSEntity *> *objCache);
    ~PSMission();
    void loadMission() override;
    void update() override;
    void createAiPlane(SCMissionActors *actor) override;
    RSEntity *createEntity() override;
    static bool isRunwayName(const std::string &name);
    RSEntity *createRunwayEntity(MISN_PART *part);
    SCProg *createProg(SCMissionActors *actor, std::vector<PROG> prog, uint8_t prog_id) override;
    // Cible tirée au hasard par OP_ATTACK_RANDOM_ENEMY_OF_TYPE et OP_DIVE_ON_RANDOM_ENEMY_OF_TYPE, gardée par objet.
    std::unordered_map<SCMissionActors *, SCMissionActors *> random_targets;
    // OP_ORDER_OBJ_AND_SPOT : rang, dans la liste des SPOT, du point d'approche ajouté pour l'objet ; le point de
    // toucher est au rang suivant.
    std::unordered_map<SCMissionActors *, size_t> landing_spots;
};