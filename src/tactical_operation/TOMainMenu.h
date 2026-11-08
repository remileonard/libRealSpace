#pragma once
//
//  TOMainMenu.h
//  libRealSpace
//
//  Menu principal de Tactical Operations : celui de Strike Commander (SCMainMenu, MAINMENU.PAK de TOBIGTRE.TRE),
//  dont la nouvelle partie et le chargement passent par TOGameFlow et le fichier de partie de 598 octets.
//

#include "../strike_commander/precomp.h"

class TOMainMenu : public SCMainMenu {
protected:
    void LoadGame(std::string filename) override;
    void OnStartNewGame() override;
};
