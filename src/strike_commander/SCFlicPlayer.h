//
//  SCFlicPlayer.h
//  libRealSpace
//
//  Animation d'introduction de la version CD-ROM (DATA\FLIC\THEFLIC.FLX), jouée comme MENU.EXE :
//  cadence, musique et sons numérisés de DATA\FLIC\FLIC.PAK.
//  strike_commander_re/analysis/menu_cd/FLX_FORMAT.md, analysis/IMPL_FLIC_INTRO.md
//
#pragma once
#include "../realspace/RSFlic.h"

class SCFlicPlayer : public IActivity {
private:
    RSFlic flic;
    PakArchive flicPak;
    MemMusic music{};
    bool started{ false };
    bool finished{ false };
    double nextFrameMs{ 0.0 };

    void PlaySoundForFrame(int frame);
    void Finish(void);

public:
    // Le fichier n'existe que dans les données de la version CD-ROM.
    static bool IsAvailable(void);

    SCFlicPlayer();
    ~SCFlicPlayer();
    virtual void init(void);
    virtual void runFrame(void);
};
