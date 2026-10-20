#pragma once
#include "../strike_commander/precomp.h"
#include "../strike_commander/SCMainMenu.h"

class EnhancedMainMenu : public SCMainMenu {
public:
    EnhancedMainMenu() = default;

protected:
    void OnStartNewGame() override;
    void LoadGame(std::string filename) override;
    void OnTrainingMission() override;
};
