#pragma once
#include "../strike_commander/precomp.h"
#include "PSMission.h"
#include "PSCockpit.h"

class PSStrike : public virtual SCStrike {
protected:
public:
    PSStrike();
    ~PSStrike();
    void setMission(char const *missionName) override;
    void start(void) override;
    SCCockpit *createCockpit() override;
    SCVirtualCockpit *createVirtualCockpit(SCCockpit *cockpit) override;
    void firePrimary() override;
    void selectNextWeapon() override;
    void selectFirstWeapon();
    void placePlayerOnGround();
    SCMission *createMission(char const *missionName) override;
};