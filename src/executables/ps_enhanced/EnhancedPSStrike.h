//
//  EnhancedPSStrike.h
//  libRealSpace
//
//  Pacific Strike flight activity with the enhanced extensions that do not depend on Strike Commander only:
//  mouse orbit for the external cameras (F2 chase, F7 target, F8 missile camera), shared with EnhancedStrike through the
//  EnhancedOrbit mixin, and the engine sound volume.
//
#pragma once
#include "../../pacific_strike/PSStrike.h"
#include "../../pacific_strike/PSGameFlow.h"
#include "../../enhanced/EnhancedOrbit.h"

class EnhancedPSStrike : public EnhancedOrbit<PSStrike> {
public:
    void init() override;

protected:
    void afterInput() override;
    void afterCameraPlaced() override;

private:
    int engineVolume{100}; // [Sound] engine_volume, -1 = as the core
};

// Pacific Strike campaign flow that flies the missions with an EnhancedPSStrike (copy of PSGameFlow::flyMission)
class EnhancedPSGameFlow : public PSGameFlow {
public:
    void flyMission(uint8_t missionid) override;
};
