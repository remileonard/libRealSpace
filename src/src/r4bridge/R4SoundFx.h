//
//  R4SoundFx.h
//  libRealSpace
//
//  The sound effects of the game in PCM, for a sound in space: an XMIDI effect of SOUNDFX.ADL rendered off line by a synthesizer
//  (OPL or MT-32), and a digitized sound (VOC) decoded.
//  An effect with a loop without end (FOR_LOOP with a count of 0) gives its introduction and its body: the render goes through the
//  introduction and two passes of the loop, and keeps the second pass, when the state of the synthesizer is stable. A held
//  sound (a note without end, without a loop in the sequence) loops from 1 s to the point of the sixth second where the wave
//  is the most like the wave at 1 s.
//
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

class MusicDevice;
class SCTimbreLibrary;

struct R4SoundPcm {
    // mono, 16 bits
    std::vector<int16_t> samples;
    int rate = 0;
    // a loop without end: the source plays [0, loopStart) one time, then loops on [loopStart, loopEnd)
    bool loops = false;
    size_t loopStart = 0;
    size_t loopEnd = 0;
};

// The effect data (XMIDI) on device, with the timbres of timbres, at rate. device is used alone and is not reset: a new device for
// each effect, or one whose notes have ended. false when the sequence or one of its timbres cannot be used.
bool renderSoundFx(MusicDevice &device, const SCTimbreLibrary *timbres, const uint8_t *data, size_t size, int rate, R4SoundPcm &out, float maxSeconds = 10.0f);

// A Creative Voice File (VOC): 8 or 16 bits PCM, mono or stereo (blocks 1, 2, 3, 8 and 9). false when there is no sound in it.
bool decodeVoc(const uint8_t *data, size_t size, R4SoundPcm &out);
