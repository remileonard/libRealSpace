//
//  TOMainMenu.cpp
//  libRealSpace
//

#include "TOMainMenu.h"

// Copie de SCMainMenu::LoadGame avec SCState::LoadTO.
void TOMainMenu::LoadGame(std::string filename) {
    GameState.LoadTO(filename);
    SCGameFlow *gfl = SCGameFlow::create();
    gfl->InitFromGameState();
    Game->addActivity(gfl);
    if (this->frequest) {
        this->frequest->requested_file = "";
    }
}
