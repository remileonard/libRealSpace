#pragma once
#include "../strike_commander/precomp.h"
#include "PSMission.h"

class PSStrike : public SCStrike {
protected:

public:
    PSStrike();
    ~PSStrike();
    void setMission(char const *missionName);
};