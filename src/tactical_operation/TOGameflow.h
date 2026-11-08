#pragma once
#include "../strike_commander/precomp.h"

class TOGameFlow : public virtual SCGameFlow {
protected:
    // Images et palettes du second écran d'armement (sub_870B avec 1 : OPTSHPS2.PAK, OPTPALS2.PAK).
    PakArchive optShps2;
    PakArchive optPals2;

public:
    TOGameFlow();
    ~TOGameFlow();
    void initInstructions() override;
    void playMidGame(uint8_t midGameId) override;
    void playConv(uint8_t convId) override;
    void opWeaponLoadout(EFCT *instruction) override;
    // Fichier de partie de Tactical Operations : 598 octets (Gameflow_SaveGameFile_5357F).
    void loadGame(std::string filename) override;
    void saveGame(std::string filename) override;
    // WEAPON_LOADOUT_ALT (43) : écran d'armement du F-22, test comme WEAPON_LOADOUT.
    virtual void opWeaponLoadoutAlt(EFCT *instruction);
    // IF_MISSION_SUCCESS (32) : de plus, vol terminé avec le code 2 et drapeau 88 posé : 7e valeur de STAT à 1.
    void opIfMissSuccess(EFCT *instruction) override;
    // IF_RESULT_7 (42, IF_LOADED_FLAG du CD de Strike Commander) : test, 7e valeur de STAT non nulle.
    virtual void opIfResult7(EFCT *instruction);
    // RESTORE_PILOT (44) : remet le pilote n au tableau des victoires, inverse de REMOVE_PILOT.
    virtual void opRestorePilot(EFCT *instruction);
    // SHOW_KILL_MARKS_ALT (45) : second écran des marques de victoires, même condition que SHOW_KILL_MARKS.
    virtual void opShowKillMarksAlt(EFCT *instruction);
};
