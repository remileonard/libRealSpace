//
//  Mt32EmuOutput.h
//  libRealSpace
//
//  Emulated Roland MT-32 music device (libmt32emu), see MusicOutput.h. Same tunes and
//  timbres as Mt32Device (.ROL + STRIKE.MT) but rendered by Mt32Synth instead of a MIDI port.
//
#pragma once
#include <cstdio>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "MusicOutput.h"
#include "Mt32Synth.h"
#include "Mt32Rhythm.h"

// Loads the STRIKE.MT timbres (record: u16 LE 0x00F8 + 246 bytes) into the emulated MT-32 with SysEx.
// Memory slot n is at address 08 00 00 + n*256, patch p is assigned at 05 00 00 + p*8.
class Mt32SynthTimbres {
public:
    Mt32Synth *synth{nullptr};

    void setLibrary(const SCTimbreLibrary *l) {
        lib = l;
        if (synth != nullptr && Mt32Rhythm::enabled()) {
            rhythm.programAll(lib, [this](const std::vector<uint8_t> &m) { synth->sysex(m); });
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
            // rhythm key (patch = key): the whole rhythm setup is written by setLibrary(), see Mt32Rhythm.h
            rhythmDone.insert(patch); // marked as done, the sequencer would ask again otherwise
            return;
        }
        if (synth == nullptr || rec == nullptr || (rec[0] | (rec[1] << 8)) != 248) {
            slotOf[patch] = { bank, -1 }; // marked as done, the sequencer would ask again otherwise
            return;
        }
        int slot;
        auto it = slotOf.find(patch);
        if (it != slotOf.end() && it->second.second >= 0) {
            slot = it->second.second;
        } else {
            // 64 timbre memory slots, the first ones hold the percussion timbres of the rhythm setup: start over
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
            synth->sysex(message(0x08, slot * 256 + pos[k], t + pos[k], len[k]));
        }
        const uint8_t pd[2] = { 0x02, (uint8_t)slot };
        synth->sysex(message(0x05, patch * 8, pd, 2));
        slotOf[patch] = { bank, slot };
    }

private:
    std::map<int, std::pair<int, int>> slotOf; // patch -> (bank, slot)
    std::set<int> rhythmDone;                  // rhythm keys (bank 0x7F) already handled
    int next{0};
    const SCTimbreLibrary *lib{nullptr};
    Mt32Rhythm rhythm;

    static std::vector<uint8_t> message(int base, int offset, const uint8_t *p, size_t n) {
        std::vector<uint8_t> m = { 0xF0, 0x41, 0x10, 0x16, 0x12,
            (uint8_t)(base + ((offset >> 14) & 0x7F)),
            (uint8_t)((offset >> 7) & 0x7F), (uint8_t)(offset & 0x7F) };
        m.insert(m.end(), p, p + n);
        int sum = 0;
        for (size_t i = 5; i < m.size(); i++) {
            sum += m[i];
        }
        m.push_back((uint8_t)((128 - (sum & 0x7F)) & 0x7F));
        m.push_back(0xF7);
        return m;
    }
};

// Emulated Roland MT-32: .ROL tunes, timbres of STRIKE.MT sent as SysEx to Mt32Synth
class Mt32EmuDevice : public MusicDevice {
public:
    explicit Mt32EmuDevice(bool debug) : debugLog(debug) { timbres.synth = &synth; }
    bool open(const std::string &romDir, int sampleRate) { return synth.open(romDir, sampleRate); }
    void send(int status, int d1, int d2) override {
        if (debugLog && (status & 0xF0) == 0xC0) {
            std::printf("MT-32 emu ch%d program %d\n", (status & 0x0F) + 1, d1);
        }
        if (Mt32Rhythm::reverbControl(status, d1, d2, [this](const std::vector<uint8_t> &m) { synth.sysex(m); })) {
            return;
        }
        synth.send(status, d1, d2);
    }
    void sysex(const uint8_t *data, size_t size) override { synth.sysex(data, size); }
    bool hasTimbre(int bank, int patch) override { return timbres.has(bank, patch); }
    void installTimbre(int bank, int patch, const uint8_t *rec) override { timbres.install(bank, patch, rec); }
    void setTimbreLibrary(const SCTimbreLibrary *l) override { timbres.setLibrary(l); }
    void render(int16_t *out, int frames) override { synth.render(out, frames); }

private:
    bool debugLog;
    Mt32Synth synth;
    Mt32SynthTimbres timbres;
};
