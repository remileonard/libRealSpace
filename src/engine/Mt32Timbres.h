#pragma once
#include "MidiOut.h"
#include "Mt32Rhythm.h"
#include <cstdint>
#include <map>
#include <set>
#include <vector>
#include <utility>

// Laedt Timbres aus STRIKE.MT (Record: u16 LE 0x00F8 + 246 Byte) per SysEx in den MT-32.
// Speicherplatz n liegt bei Adresse 08 00 00 + n*256, Patch p wird bei 05 00 00 + p*8 zugewiesen.
class Mt32Uploader {
public:
    MidiOut *out = nullptr;

    void setLibrary(const SCTimbreLibrary *l) {
        lib = l;
        if (out && Mt32Rhythm::enabled()) {
            rhythm.programAll(lib, [this](const std::vector<uint8_t> &m) { out->sysex(m); });
        }
    }

    bool has(int bank, int patch) const {
        if (bank == Mt32Rhythm::BANK) {
            return rhythmDone.count(patch) > 0;
        }
        auto it = slotOf.find(patch);
        return it != slotOf.end() && it->second.first == bank;
    }

    void install(int bank, int patch, const uint8_t *rec) {
        if (bank == Mt32Rhythm::BANK) {
            // Rhythmus-Taste (Patch = Taste): das ganze Rhythm Setup schreibt setLibrary(), siehe Mt32Rhythm.h
            rhythmDone.insert(patch); // als erledigt markieren, sonst fragt der Sequenzer endlos nach
            return;
        }
        if (!out || !rec || (rec[0] | (rec[1] << 8)) != 248) {
            slotOf[patch] = { bank, -1 }; // als erledigt markieren, sonst fragt der Sequenzer endlos nach
            return;
        }
        int slot;
        auto it = slotOf.find(patch);
        if (it != slotOf.end() && it->second.second >= 0) {
            slot = it->second.second; // Patch hat schon einen Platz: ueberschreiben
        } else {
            // MT-32 hat 64 Plaetze, die ersten halten die Schlagzeug-Timbres des Rhythm Setup: von vorn anfangen
            const int first = Mt32Rhythm::enabled() ? Mt32Rhythm::FIRST_MELODIC_SLOT : 0;
            if (next < first) {
                next = first;
            }
            if (next >= 64) {
                slotOf.clear();
                next = first;
            }
            slot = next++;
        }
        const uint8_t *t = rec + 2;
        static const int pos[5] = { 0, 14, 72, 130, 188 };
        static const int len[5] = { 14, 58, 58, 58, 58 };
        for (int k = 0; k < 5; k++) {
            out->sysex(sx(0x08, slot * 256 + pos[k], t + pos[k], len[k]));
        }
        const uint8_t pd[2] = { 0x02, (uint8_t)slot };
        out->sysex(sx(0x05, patch * 8, pd, 2));
        slotOf[patch] = { bank, slot };
    }

private:
    std::map<int, std::pair<int, int>> slotOf; // Patch -> (Bank, Slot)
    std::set<int> rhythmDone;                  // Rhythmus-Tasten (Bank 0x7F), schon erledigt
    int next = 0;
    const SCTimbreLibrary *lib = nullptr;
    Mt32Rhythm rhythm;
    static std::vector<uint8_t> sx(int base, int offset, const uint8_t *p, size_t n) {
        std::vector<uint8_t> m = { 0xF0, 0x41, 0x10, 0x16, 0x12,
            (uint8_t)(base + ((offset >> 14) & 0x7F)),
            (uint8_t)((offset >> 7) & 0x7F), (uint8_t)(offset & 0x7F) };
            for (size_t i = 0; i < n; i++) {
                m.push_back(p[i]);
            }
            int sum = 0;
            for (size_t i = 5; i < m.size(); i++) {
                sum += m[i];
            }
            m.push_back((uint8_t)((128 - (sum & 0x7F)) & 0x7F));
            m.push_back(0xF7);
            return m;
    }
};
