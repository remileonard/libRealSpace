//
//  Mt32Rhythm.h
//  libRealSpace
//
//  Rhythm setup of the Roland MT-32 for Strike Commander, as the original game programs it after
//  every MT-32 reset (taken from a capture of the MIDI output of the original game):
//    1. the 14 custom percussion timbres of STRIKE.MT (bank 0x40, patches 0 1 2 4 5 6 7 10..16) are
//       loaded into the timbre memory slots 0..13, each one assigned to its patch (05 00 00 + patch * 8),
//    2. the Rhythm Setup Table (SysEx 03 01 10 .. 03 03 0F, 4 bytes per key from key 24: timbre, output
//       level, panpot, reverb switch) is written for 29 keys. The timbre byte is the memory slot of the
//       percussion timbre, or the factory rhythm timbre 70 for the keys 30 and 32,
//    3. the timbres of the tunes follow from slot 14 on.
//  Without it the MT-32 plays its factory rhythm sounds on these keys. The XMIDI files ask for the keys
//  as (bank 0x7F, patch = key); the timbre library has no such record.
//
//  On by default, [Sound] mt32_rhythm_setup = 0 switches it off.
//
//  Reverb control: the tunes carry the reverb of the MT-32 as XMIDI controllers on MIDI channel 1 (61 = mode,
//  62 = time, 63 = level). The original driver (ROLAND.DRV) turns them into the SysEx 10 00 01, 10 00 02 and
//  10 00 03 and does not pass them on; the MT-32 itself ignores the controllers.
//  On by default, [Sound] mt32_reverb_control = 0 switches it off.
//
#pragma once
#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <vector>
#include "Config.hpp"
#include "../realspace/SCMusicSet.h"

class Mt32Rhythm {
public:
    typedef std::function<void(const std::vector<uint8_t> &)> Send;

    static const int BANK = 0x7F;            // bank of the rhythm timbre requests (patch = key), AILMidi::RHYTHM_BANK
    static const int PERCUSSION_BANK = 0x40; // bank of the custom percussion timbres in STRIKE.MT
    static const int PERCUSSION_COUNT = 14;  // loaded into the timbre memory slots 0..13
    static const int FIRST_MELODIC_SLOT = 14;

    static bool enabled() {
        static const bool on = Config::getInstance().getString("Sound", "mt32_rhythm_setup", "1") == "1";
        return on;
    }

    static bool reverbControlEnabled() {
        static const bool on = Config::getInstance().getString("Sound", "mt32_reverb_control", "1") == "1";
        return on;
    }

    // NEOSC_MT32_SYSEX=1 prints every message as hex, to compare with the MIDI output of the original game
    static void dumpSysex(const std::vector<uint8_t> &m) {
        static const bool dump = getenv("NEOSC_MT32_SYSEX") != nullptr;
        if (!dump) {
            return;
        }
        std::printf("[mt32-sysex]");
        for (uint8_t b : m) {
            std::printf(" %02X", b);
        }
        std::printf("\n");
    }

    // Control change status/d1/d2 of the tune: returns true when it was a reverb controller and the SysEx was sent
    // instead (mode 0..3, time 0..7, level 0..7).
    static bool reverbControl(int status, int d1, int d2, const Send &sendSysex) {
        if (!reverbControlEnabled() || (status & 0xF0) != 0xB0 || (status & 0x0F) != 0 || d1 < 61 || d1 > 63) {
            return false;
        }
        static const int maxValue[3] = { 3, 7, 7 };
        const uint8_t value = (uint8_t)(std::min)(d2 & 0x7F, maxValue[d1 - 61]);
        const std::vector<uint8_t> m = message(0x10, 1 + (d1 - 61), &value, 1); // 10 00 01 / 02 / 03
        dumpSysex(m);
        sendSysex(m);
        return true;
    }

    // Loads the percussion timbres and writes the rhythm setup table, once. Returns false while the library
    // is not there or does not have all percussion timbres.
    bool programAll(const SCTimbreLibrary *lib, const Send &sendRaw) {
        if (done || lib == nullptr || !lib->loaded()) {
            return false;
        }
        const Send send = [&](const std::vector<uint8_t> &m) {
            dumpSysex(m);
            sendRaw(m);
        };
        const uint8_t *rec[PERCUSSION_COUNT];
        for (int i = 0; i < PERCUSSION_COUNT; i++) {
            rec[i] = lib->find(PERCUSSION_BANK, PERCUSSION_PATCH[i]);
            if (rec[i] == nullptr || (rec[i][0] | (rec[i][1] << 8)) != 248) {
                return false;
            }
        }
        static const int pos[5] = { 0, 14, 72, 130, 188 };
        static const int len[5] = { 14, 58, 58, 58, 58 };
        for (int i = 0; i < PERCUSSION_COUNT; i++) {
            const uint8_t *t = rec[i] + 2;
            for (int k = 0; k < 5; k++) {
                send(message(0x08, i * 256 + pos[k], t + pos[k], (size_t)len[k]));
            }
            const uint8_t patch[2] = { 0x02, (uint8_t)i }; // timbre group memory, number = slot
            send(message(0x05, PERCUSSION_PATCH[i] * 8, patch, 2));
        }
        for (const Entry &e : TABLE) {
            const uint8_t data[4] = { e.timbre, e.level, e.pan, e.reverb };
            send(message(0x03, keyOffset(e.key), data, 4));
        }
        // key 33: only the timbre byte (slot 13, Pushy), level, pan and reverb stay as the MT-32 has them
        const uint8_t key33 = 13;
        send(message(0x03, keyOffset(33), &key33, 1));
        done = true;
        std::printf("[mt32] rhythm setup: %d percussion timbres in memory slots 0..%d, %d rhythm keys\n", PERCUSSION_COUNT,
                    PERCUSSION_COUNT - 1, (int)(sizeof TABLE / sizeof TABLE[0]) + 1);
        return true;
    }

private:
    struct Entry {
        uint8_t key;
        uint8_t timbre; // memory slot of the percussion timbre, 70 = factory rhythm timbre
        uint8_t level;  // 0..100
        uint8_t pan;    // 0..14, 7 = centre
        uint8_t reverb; // reverb switch
    };

    static const int PERCUSSION_PATCH[PERCUSSION_COUNT];
    static const Entry TABLE[28];

    // 03 01 10 is the table entry of key 24; the SysEx address bytes are 7 bit: 0x90 = 01 10
    static int keyOffset(int key) { return 0x90 + (key - 24) * 4; }

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

    bool done{false};
};

// Toybell Sigmunds Scratch Gong Afrika Taikocym Bussy Elsnare Milsnare Revophat Windbells Pizzaf Lowcrash Pushy
inline const int Mt32Rhythm::PERCUSSION_PATCH[Mt32Rhythm::PERCUSSION_COUNT] = { 0, 1, 2, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16 };

// key, timbre byte, level, pan, reverb
inline const Mt32Rhythm::Entry Mt32Rhythm::TABLE[28] = {
    { 28, 3, 90, 7, 0 },    // Gong
    { 29, 6, 100, 7, 0 },   // Bussy
    { 30, 70, 100, 11, 1 }, // factory rhythm timbre
    { 31, 7, 95, 7, 1 },    // Elsnare
    { 32, 70, 100, 3, 1 },  // factory rhythm timbre
    { 34, 12, 95, 11, 1 },  // Lowcrash
    { 35, 11, 90, 7, 0 },   // Pizzaf
    { 52, 1, 75, 14, 1 },   // Sigmunds
    { 53, 1, 75, 9, 1 },    // Sigmunds
    { 55, 1, 80, 7, 1 },    // Sigmunds
    { 57, 1, 80, 3, 1 },    // Sigmunds
    { 58, 2, 85, 7, 0 },    // Scratch
    { 59, 1, 80, 0, 1 },    // Sigmunds
    { 71, 8, 90, 7, 1 },    // Milsnare
    { 72, 8, 90, 7, 1 },    // Milsnare
    { 74, 4, 90, 5, 0 },    // Afrika
    { 76, 4, 90, 6, 0 },    // Afrika
    { 77, 4, 90, 7, 0 },    // Afrika
    { 78, 5, 100, 7, 1 },   // Taikocym
    { 79, 4, 90, 8, 0 },    // Afrika
    { 80, 0, 90, 7, 1 },    // Toybell
    { 81, 4, 90, 9, 0 },    // Afrika
    { 82, 9, 95, 7, 1 },    // Revophat
    { 83, 10, 100, 4, 1 },  // Windbells
    { 84, 10, 100, 4, 1 },  // Windbells
    { 85, 10, 100, 6, 1 },  // Windbells
    { 86, 10, 100, 7, 1 },  // Windbells
    { 87, 10, 100, 8, 1 },  // Windbells
};
