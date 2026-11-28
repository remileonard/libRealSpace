//
//  Mt32Synth.h
//  libRealSpace
//
//  Roland MT-32 software synthesizer (libmt32emu / Munt), the emulated
//  counterpart of a MIDI port with a MT-32 behind it. Needs the MT-32 (or
//  CM-32L) control and PCM ROM images.
//
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

struct mt32emu_data;

// The files of a ROM folder in memory, read once: several synths can start from them without reading the disk again
struct Mt32Roms {
    std::string dir;
    std::vector<std::vector<uint8_t>> files;
};

class Mt32Synth {
public:
    ~Mt32Synth();

    // Loads every ROM image found in romDir and starts the synth at sampleRate.
    bool open(const std::string &romDir, int sampleRate);
    // The same from ROMs already read (readRoms); the synth keeps them while it is open
    bool open(std::shared_ptr<const Mt32Roms> roms, int sampleRate);
    // the files of romDir, null when the folder is not there
    static std::shared_ptr<const Mt32Roms> readRoms(const std::string &romDir);
    void close();
    bool active() const { return context != nullptr; }

    void send(int status, int d1, int d2);
    void sysex(const uint8_t *data, size_t size);
    void sysex(const std::vector<uint8_t> &message) { sysex(message.data(), message.size()); }
    // Adds the synth output to an interleaved stereo int16 buffer
    void render(int16_t *out, int frames);

private:
    std::mutex mtx;
    mt32emu_data *context{nullptr};
    std::shared_ptr<const Mt32Roms> roms;
    std::vector<int16_t> buffer;
    // NEOSC_MT32_WAV=file: writes the synth output alone (without sound effects) as 16 bit stereo WAV, a debug aid
    FILE *wav{nullptr};
    uint32_t wavBytes{0};
    int wavRate{0};
    void closeWav();
};
