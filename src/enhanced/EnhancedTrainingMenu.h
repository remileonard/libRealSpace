#pragma once
#include "../strike_commander/precomp.h"
#include "../strike_commander/SCTrainingMenu.h"

class EnhancedTrainingMenu : public SCTrainingMenu {
public:
    EnhancedTrainingMenu() = default;

protected:
    void OnSearchAndDestroy() override;
};
