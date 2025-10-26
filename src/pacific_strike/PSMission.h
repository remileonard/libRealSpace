#include "../strike_commander/precomp.h"

class PSMission: public SCMission {
public:
    PSMission();
    PSMission(std::string mission_name, std::unordered_map<std::string, RSEntity *> *objCache);
    ~PSMission();
    void loadMission() override;
    void update() override;
};