//
//  TOMainMenu.cpp
//  libRealSpace
//

#include "TOMainMenu.h"
#include "TOGameflow.h"

// Copie de SCMainMenu::LoadGame : SCState::LoadTO et TOGameFlow.
void TOMainMenu::LoadGame(std::string filename) {
    GameState.LoadTO(filename);
    TOGameFlow *gfl = new TOGameFlow();
    gfl->InitFromGameState();
    Game->addActivity(gfl);
    if (this->frequest) {
        this->frequest->requested_file = "";
    }
}

// Copie de SCMainMenu::OnStartNewGame : TOGameFlow.
void TOMainMenu::OnStartNewGame() {
    TOGameFlow *gfl = new TOGameFlow();
    GameState.Reset();
    gfl->init();
    SCRegister *registerActivity = new SCRegister();
    registerActivity->init();
    Game->addActivity(gfl);
    Game->addActivity(registerActivity);
}
