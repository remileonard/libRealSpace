#include "../strike_commander/precomp.h"
#include "EnhancedMainMenu.h"
#include "EnhancedGameFlow.h"
#include "EnhancedTrainingMenu.h"

void EnhancedMainMenu::OnStartNewGame() {
    EnhancedGameFlow *gfl = new EnhancedGameFlow();
    GameState.Reset();
    gfl->init();
    SCRegister *registerActivity = new SCRegister();
    registerActivity->init();
    Game->addActivity(gfl);
    Game->addActivity(registerActivity);
}

void EnhancedMainMenu::LoadGame(std::string filename) {
    GameState.Load(filename);
    EnhancedGameFlow *gfl = new EnhancedGameFlow();
    gfl->InitFromGameState();
    Game->addActivity(gfl);
    if (this->frequest) {
        this->frequest->requested_file = "";
    }
}

void EnhancedMainMenu::OnTrainingMission() {
    EnhancedTrainingMenu *trainingActivity = new EnhancedTrainingMenu();
    trainingActivity->init();
    Game->addActivity(trainingActivity);
}
