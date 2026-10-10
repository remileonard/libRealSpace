//
//  SCTimbreTest.cpp
//  libRealSpace
//
#include "SCTimbreTest.h"
#include "AILMidi.h"

using namespace AILMidi;

std::vector<SCTimbreInfo> SCTimbreTest::list(const SCTimbreLibrary &lib) {
    std::vector<SCTimbreInfo> v;
    const uint8_t *p = lib.data();
    if (!p) {
        return v;
    }
    for (size_t o = 0; o + 6 <= lib.size(); o += 6) {
        if (p[o + 1] == 0xFF) {
            break; // fin de l'index : banque 0xFF
        }
        const uint8_t *t = lib.find(p[o + 1], p[o]);
        if (!t) {
            continue;
        }
        SCTimbreInfo i;
        i.patch = p[o];
        i.bank = p[o + 1];
        i.length = t[0] | (t[1] << 8);
        i.data = t;
        if (i.length == 0x0E) {
            i.kind = SCTimbreInfo::OPL;
        } else if (i.length == 0x19) {
            i.kind = SCTimbreInfo::OTHER;
        } else {
            i.kind = (t[3] == 1) ? SCTimbreInfo::TVFX_NOTE : (t[3] == 2 ? SCTimbreInfo::TVFX_ABS : SCTimbreInfo::OTHER);
            i.duration = t[4] | (t[5] << 8);
        }
        v.push_back(i);
    }
    return v;
}

void SCTimbreTest::play(AILAdlibDriver &adl, const SCTimbreInfo &t, int chan, int note, int vel) {
    adl.send(CONTROL_CHANGE | chan, PATCH_BANK_SEL, t.bank);
    adl.installTimbre(t.bank, t.patch, t.data);
    adl.send(PROGRAM_CHANGE | chan, t.patch, 0);
    adl.send(NOTE_ON | chan, note, vel);
}

void SCTimbreTest::stop(AILAdlibDriver &adl, int chan, int note) {
    adl.send(NOTE_OFF | chan, note, 0);
}

const char *SCTimbreTest::kindLabel(int kind) {
    switch (kind) {
    case SCTimbreInfo::OPL:
        return "OPL simple";
    case SCTimbreInfo::TVFX_NOTE:
        return "TVFX 1 (hauteur = note)";
    case SCTimbreInfo::TVFX_ABS:
        return "TVFX 2 (freq. absolue)";
    default:
        return "autre";
    }
}
