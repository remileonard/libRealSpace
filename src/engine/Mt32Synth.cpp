//
//  Mt32Synth.cpp
//  libRealSpace
//
#include "Mt32Synth.h"
#include <algorithm>
#include <cstdio>
#include <boost/filesystem.hpp>

#define MT32EMU_API_TYPE 1
#include <mt32emu.h>

namespace {
// Same setup as a MT-32 on a MIDI port (MidiOut::open): parts 1-8 on channels 2-9, rhythm on 10,
// partial reserve and reverb.
const uint8_t kMt32Channels[] = { 0xF0,0x41,0x10,0x16,0x12,0x10,0x00,0x0D,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x36,0xF7 };
const uint8_t kMt32Partials[] = { 0xF0,0x41,0x10,0x16,0x12,0x10,0x00,0x04,0x03,0x04,0x03,0x04,0x03,0x04,0x03,0x04,0x04,0x4C,0xF7 };
const uint8_t kMt32Reverb[]   = { 0xF0,0x41,0x10,0x16,0x12,0x10,0x00,0x01,0x00,0x03,0x02,0x6A,0xF7 };
}

Mt32Synth::~Mt32Synth() {
    close();
}

bool Mt32Synth::open(const std::string &romDir, int sampleRate) {
    close();
    std::lock_guard<std::mutex> lk(mtx);

    boost::system::error_code ec;
    if (romDir.empty() || !boost::filesystem::is_directory(romDir, ec)) {
        std::printf("Mt32Synth: ROM directory '%s' not found\n", romDir.c_str());
        return false;
    }
    std::vector<boost::filesystem::path> files;
    for (boost::filesystem::directory_iterator it(romDir, ec), end; !ec && it != end; it.increment(ec)) {
        if (boost::filesystem::is_regular_file(it->path(), ec)) {
            files.push_back(it->path());
        }
    }
    // Deterministic choice when several machines are present: the last ROM of a type wins.
    std::sort(files.begin(), files.end());

    mt32emu_report_handler_i handler;
    handler.v0 = nullptr;
    mt32emu_context ctx = mt32emu_create_context(handler, nullptr);
    if (ctx == nullptr) {
        std::printf("Mt32Synth: cannot create the emulation context\n");
        return false;
    }
    bool control = false;
    bool pcm = false;
    for (const auto &file : files) {
        mt32emu_return_code rc = mt32emu_add_rom_file(ctx, file.string().c_str());
        control = control || rc == MT32EMU_RC_ADDED_CONTROL_ROM;
        pcm = pcm || rc == MT32EMU_RC_ADDED_PCM_ROM;
    }
    if (!control || !pcm) {
        std::printf("Mt32Synth: %s missing in %s\n", !control ? "control ROM" : "PCM ROM", romDir.c_str());
        mt32emu_free_context(ctx);
        return false;
    }
    mt32emu_set_stereo_output_samplerate(ctx, (double)sampleRate);
    mt32emu_set_samplerate_conversion_quality(ctx, MT32EMU_SRCQ_GOOD);
    if (mt32emu_open_synth(ctx) != MT32EMU_RC_OK) {
        std::printf("Mt32Synth: cannot open the synth\n");
        mt32emu_free_context(ctx);
        return false;
    }
    mt32emu_rom_info info;
    mt32emu_get_rom_info(ctx, &info);
    std::printf("Mt32Synth: libmt32emu %s, %s, %u Hz\n", mt32emu_get_library_version_string(),
                info.control_rom_description ? info.control_rom_description : "?",
                mt32emu_get_actual_stereo_output_samplerate(ctx));

    mt32emu_play_sysex_now(ctx, kMt32Channels, sizeof kMt32Channels);
    mt32emu_play_sysex_now(ctx, kMt32Partials, sizeof kMt32Partials);
    mt32emu_play_sysex_now(ctx, kMt32Reverb, sizeof kMt32Reverb);
    this->context = ctx;
    return true;
}

void Mt32Synth::close() {
    std::lock_guard<std::mutex> lk(mtx);
    if (this->context != nullptr) {
        mt32emu_close_synth(this->context);
        mt32emu_free_context(this->context);
        this->context = nullptr;
    }
}

void Mt32Synth::send(int status, int d1, int d2) {
    std::lock_guard<std::mutex> lk(mtx);
    if (this->context == nullptr) {
        return;
    }
    const mt32emu_bit32u msg = (mt32emu_bit32u)(status & 0xFF) | ((mt32emu_bit32u)(d1 & 0x7F) << 8) |
                               ((mt32emu_bit32u)(d2 & 0x7F) << 16);
    if (mt32emu_play_msg(this->context, msg) != MT32EMU_RC_OK) {
        mt32emu_play_msg_now(this->context, msg);
    }
}

void Mt32Synth::sysex(const uint8_t *data, size_t size) {
    std::lock_guard<std::mutex> lk(mtx);
    if (this->context == nullptr || data == nullptr || size == 0) {
        return;
    }
    if (mt32emu_play_sysex(this->context, data, (mt32emu_bit32u)size) != MT32EMU_RC_OK) {
        mt32emu_play_sysex_now(this->context, data, (mt32emu_bit32u)size);
    }
}

void Mt32Synth::render(int16_t *out, int frames) {
    std::lock_guard<std::mutex> lk(mtx);
    if (this->context == nullptr || frames <= 0) {
        return;
    }
    this->buffer.resize((size_t)frames * 2);
    mt32emu_render_bit16s(this->context, this->buffer.data(), (mt32emu_bit32u)frames);
    for (size_t i = 0; i < this->buffer.size(); i++) {
        int v = out[i] + this->buffer[i];
        out[i] = (int16_t)std::max(-32768, std::min(32767, v));
    }
}
