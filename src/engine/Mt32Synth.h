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
#include <mutex>
#include <string>
#include <vector>

struct mt32emu_data;

class Mt32Synth {
public:
    ~Mt32Synth();

    // Loads every ROM image found in romDir and starts the synth at sampleRate.
    bool open(const std::string &romDir, int sampleRate);
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
    std::vector<int16_t> buffer;
};
