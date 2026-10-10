//
//  SCTimbreTest.h
//  libRealSpace
//
//  Test des timbres de la bibliotheque (STRIKE.AD), en particulier TVFX.
//  Joue un timbre directement par le pilote, comme le ferait une sequence XMIDI :
//  controleur 114 (banque), changement de programme (patch), Note On, puis Note Off.
//  Le timbre est installe dans le cache du pilote comme Music_InstallTimbre_5A62A.
//
#pragma once
#include <vector>
#include "AILAdlibDriver.h"
#include "../realspace/SCMusicSet.h"

struct SCTimbreInfo {
    enum Kind {
        OPL = 0,
        TVFX_NOTE = 1,
        TVFX_ABS = 2,
        OTHER = 3
    };
    int bank = 0;
    int patch = 0;
    int length = 0;
    int kind = OTHER; // longueur 0x0E = OPL simple ; sinon octet +3 = type TVFX
    int duration = 0; // TVFX : octet +4 (ticks a 60 Hz) ; 0xFFFF = jusqu'au Note Off
    const uint8_t *data = nullptr;
    bool isTvfx() const {
        return kind == TVFX_NOTE || kind == TVFX_ABS;
    }
};

namespace SCTimbreTest {
std::vector<SCTimbreInfo> list(const SCTimbreLibrary &lib);
void play(AILAdlibDriver &adl, const SCTimbreInfo &t, int chan, int note, int vel);
void stop(AILAdlibDriver &adl, int chan, int note);
const char *kindLabel(int kind);
} // namespace SCTimbreTest
