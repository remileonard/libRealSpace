//
//  R4SoundFx.cpp
//  libRealSpace
//
#include "R4SoundFx.h"

#include "engine/AILXmidiDriver.h"
#include "engine/MusicOutput.h"
#include "engine/SCMusicSequencer.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace {

// a sample below it is silence: the end of the release (the TVFX voices can keep a noise of a few units for minutes)
constexpr int kSilence = 16;
// a sound still heard at this time without the end of its sequence nor a loop is held: a note without end
constexpr float kHeldSeconds = 6.0f;

int16_t clamp16(int v) {
    return (int16_t)std::max(-32768, std::min(32767, v));
}

// true when the samples of the end, on a length of window, are silence
bool silentEnd(const std::vector<int16_t> &pcm, size_t window) {
    if (pcm.size() < window) {
        return false;
    }
    for (size_t i = pcm.size() - window; i < pcm.size(); i++) {
        if (std::abs((int)pcm[i]) > kSilence) {
            return false;
        }
    }
    return true;
}

// A held sound (a note without end, no loop in the sequence): the loop starts at 1 s, after the attack; its end, in the last
// second, is where the wave is the most like the wave at its start, so that the join cannot be heard.
void findHeldLoop(const std::vector<int16_t> &pcm, int rate, size_t &start, size_t &end) {
    const size_t window = 256;
    start = (size_t)rate;
    size_t first = pcm.size() > (size_t)rate + window ? pcm.size() - (size_t)rate : start + window;
    end = pcm.size() - window;
    double best = -1;
    for (size_t e = first; e + window <= pcm.size(); e++) {
        double cost = 0;
        for (size_t k = 0; k < window && (best < 0 || cost < best); k++) {
            const double d = (double)pcm[start + k] - (double)pcm[e + k];
            cost += d * d;
        }
        if (best < 0 || cost < best) {
            best = cost;
            end = e;
        }
    }
}

void trimSilence(std::vector<int16_t> &pcm) {
    size_t end = pcm.size();
    while (end > 0 && std::abs((int)pcm[end - 1]) <= kSilence) {
        end--;
    }
    pcm.resize(end);
}

} // namespace

bool renderSoundFx(MusicDevice &device, const SCTimbreLibrary *timbres, const uint8_t *data, size_t size, int rate, R4SoundPcm &out, float maxSeconds) {
    out = R4SoundPcm();
    out.rate = rate;
    if (data == nullptr || size == 0 || rate <= 0) {
        return false;
    }
    AILXmidiDriver xmi;
    xmi.init(&device);
    int error = 0;
    int h = SCMusicSequencer::registerAndStart(&xmi, timbres, data, size, &error);
    if (h < 0) {
        return false;
    }
    const size_t maxFrames = (size_t)(maxSeconds * (float)rate);
    const size_t heldFrames = (size_t)(kHeldSeconds * (float)rate);
    const size_t silenceWindow = (size_t)(rate / 20);
    const double tick = (double)rate / 120.0;
    std::vector<int16_t> stereo;
    size_t loopMark = 0;
    bool marked = false;
    size_t jumps[2] = {0, 0};
    int jumpCount = 0;
    bool done = false;
    double nextTick = 0;
    while (out.samples.size() < maxFrames) {
        // the step of the driver at its time: its events act on the frames after it
        if ((double)out.samples.size() >= nextTick) {
            const int starts = xmi.endlessLoopStarts;
            const int loopJumps = xmi.endlessLoopJumps;
            xmi.serve();
            if (xmi.endlessLoopStarts > starts && !marked) {
                loopMark = out.samples.size();
                marked = true;
            }
            if (xmi.endlessLoopJumps > loopJumps) {
                jumps[jumpCount] = out.samples.size();
                jumpCount++;
                if (jumpCount == 2) {
                    break;
                }
            }
            if (xmi.status(h) == SEQ_DONE) {
                done = true;
            }
            nextTick += tick;
        }
        size_t frames = (size_t)std::max(1.0, nextTick - (double)out.samples.size());
        frames = std::min(frames, maxFrames - out.samples.size());
        stereo.assign(frames * 2, 0);
        device.render(stereo.data(), (int)frames);
        for (size_t i = 0; i < frames; i++) {
            out.samples.push_back(clamp16(((int)stereo[i * 2] + (int)stereo[i * 2 + 1]) / 2));
        }
        if (done && silentEnd(out.samples, silenceWindow)) {
            break;
        }
        if (!done && jumpCount == 0 && out.samples.size() >= heldFrames) {
            break;
        }
    }
    xmi.stop(h);
    xmi.release(h);
    if (jumpCount == 2) {
        // the second pass: the voices of the first one have their release in it, as in all the next passes
        out.loops = true;
        out.loopStart = jumps[0];
        out.loopEnd = jumps[1];
        out.samples.resize(jumps[1]);
    } else if (jumpCount == 1 && marked && jumps[0] > loopMark) {
        // too long for two passes: the first one
        out.loops = true;
        out.loopStart = loopMark;
        out.loopEnd = jumps[0];
        out.samples.resize(jumps[0]);
    } else if (!done && out.samples.size() >= heldFrames && !silentEnd(out.samples, silenceWindow)) {
        out.loops = true;
        findHeldLoop(out.samples, rate, out.loopStart, out.loopEnd);
        out.samples.resize(out.loopEnd);
    } else {
        trimSilence(out.samples);
    }
    return !out.samples.empty();
}

namespace {

uint32_t le16(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8);
}

uint32_t le24(const uint8_t *p) {
    return le16(p) | ((uint32_t)p[2] << 16);
}

uint32_t le32(const uint8_t *p) {
    return le24(p) | ((uint32_t)p[3] << 24);
}

// PCM of bits (8 unsigned or 16 signed) and channels, to mono at the end of out
void appendPcm(const uint8_t *p, size_t bytes, int bits, int channels, std::vector<int16_t> &out) {
    const size_t sampleBytes = bits == 16 ? 2 : 1;
    const size_t frameBytes = sampleBytes * (size_t)channels;
    for (size_t at = 0; at + frameBytes <= bytes; at += frameBytes) {
        int sum = 0;
        for (int c = 0; c < channels; c++) {
            const uint8_t *s = p + at + (size_t)c * sampleBytes;
            sum += bits == 16 ? (int)(int16_t)le16(s) : ((int)s[0] - 128) * 256;
        }
        out.push_back(clamp16(sum / channels));
    }
}

} // namespace

bool decodeVoc(const uint8_t *data, size_t size, R4SoundPcm &out) {
    out = R4SoundPcm();
    static const char kMagic[] = "Creative Voice File\x1A";
    if (data == nullptr || size < 26 || std::memcmp(data, kMagic, 20) != 0) {
        return false;
    }
    size_t at = le16(data + 20);
    int rate = 0;
    int bits = 8;
    int channels = 1;
    // a block 8 gives the format of the next block 1
    bool extended = false;
    while (at + 4 <= size && data[at] != 0) {
        const int type = data[at];
        const size_t length = le24(data + at + 1);
        const uint8_t *p = data + at + 4;
        const size_t avail = std::min(length, size - (at + 4));
        if (type == 1 && avail >= 2) {
            if (!extended) {
                rate = 1000000 / (256 - p[0]);
                bits = 8;
                channels = 1;
            }
            extended = false;
            if (p[1] == 0) {
                appendPcm(p + 2, avail - 2, bits, channels, out.samples);
            }
        } else if (type == 2) {
            appendPcm(p, avail, bits, channels, out.samples);
        } else if (type == 3 && avail >= 3) {
            const size_t frames = le16(p) + 1;
            if (rate == 0) {
                rate = 1000000 / (256 - p[2]);
            }
            out.samples.insert(out.samples.end(), frames, 0);
        } else if (type == 8 && avail >= 4) {
            channels = p[3] == 1 ? 2 : 1;
            rate = (int)(256000000u / ((uint32_t)channels * (65536u - le16(p))));
            bits = 8;
            extended = true;
        } else if (type == 9 && avail >= 12) {
            rate = (int)le32(p);
            bits = p[4];
            channels = std::max(1, (int)p[5]);
            const uint32_t codec = le16(p + 6);
            if ((codec == 0 && bits == 8) || (codec == 4 && bits == 16)) {
                appendPcm(p + 12, avail - 12, bits, channels, out.samples);
            }
        }
        at += 4 + length;
    }
    out.rate = rate;
    return rate > 0 && !out.samples.empty();
}
