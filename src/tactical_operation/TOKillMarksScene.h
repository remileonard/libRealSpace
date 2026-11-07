#pragma once
//
//  TOKillMarksScene.h
//  libRealSpace
//
//  Second écran des marques de victoires de Tactical Operations (commande 45 SHOW_KILL_MARKS_ALT,
//  Gameflow_ShowKillMarksAlt_56D57) : celui d'EndMissionScene, avec le fond 13 d'OPTSHPS2.PAK (au lieu de 143) et la
//  palette 1 d'OPTPALS2.PAK (au lieu de 20).
//

#include "../strike_commander/precomp.h"

class TOKillMarksScene : public EndMissionScene {
public:
    TOKillMarksScene(PakArchive *optShps2, PakArchive *optPals2);
    void init() override;

private:
    PakArchive *optShps2{ nullptr };
    PakArchive *optPals2{ nullptr };
};
