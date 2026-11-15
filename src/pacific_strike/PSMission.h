#include "../strike_commander/precomp.h"

class PSMissionActorsTurret;

class PSMission: public SCMission {
public:
    static const size_t FLAG_COUNT = 65536;
    PSMission();
    PSMission(std::string mission_name, std::unordered_map<std::string, RSEntity *> *objCache);
    ~PSMission();
    void loadMission() override;
    void update() override;
    void createAiPlane(SCMissionActors *actor) override;
    void finishScenes() override;
    RSEntity *createEntity() override;
    static bool isRunwayName(const std::string &name);
    RSEntity *createRunwayEntity(MISN_PART *part);
    SCProg *createProg(SCMissionActors *actor, std::vector<PROG> prog, uint8_t prog_id) override;
    std::unordered_map<SCMissionActors *, SCMissionActors *> random_targets;
    std::unordered_map<SCMissionActors *, size_t> landing_spots;
    std::vector<PSMissionActorsTurret *> turrets;

protected:
    void createTurrets(SCMissionActors *actor);
    void onMusicUpdate() override;
    void onActorDestroyed(SCMissionActors *actor) override;
    uint8_t musicScanEnemies(SCMissionActors *excluded) override;
    bool musicFallback() override;
    bool musicIsPunctuation(int tune) override;

private:
    bool musicTuneLocked();
    bool musicTargetWithinRange();
};