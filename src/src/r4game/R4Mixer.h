//
//  R4Mixer.h
//  libRealSpace
//
//  The sound of neosc_r4 with r4nkine_audio (OpenAL Soft), instead of SDL_mixer. RSMixer keeps the music: its synthesis goes in a
//  stereo stream. The sound effects of the game are PCM sounds in space (R4SoundFx): the XMIDI effects are rendered at init with
//  the synthesizer of [Sound] midi_music, the VOC are decoded. The VOC of the channels (voices, radio, engine of the player) play
//  at the listener. All the effects are heard: [Sound] max_voices (64) is the limit, then the farther are virtual.
//  The listener is the camera of the game: update() once a frame (R4Screen::refresh).
//  [Sound] keys: max_voices, hrtf, speed_of_sound (in units of the world a second, 1125: feet), doppler (0 stops it),
//  music_volume and sfx_volume (0 to 100), sfx_synth (auto, opl, mt32: the synthesizer that renders the effects).
//
#pragma once

#include "R4SoundFx.h"
#include "engine/RSMixer.h"
#include "r4/audio/audio.h"

#include <map>
#include <string>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

class R4Mixer : public RSMixer {
public:
    R4Mixer();
    ~R4Mixer() override;

    // the mixer of the program when it is a R4Mixer with its context, else null
    static R4Mixer *current();
    // the listener on the camera of the game, the sources, the voices and the end of the sounds
    void update();

    void init() override;

    // the volume groups of the context: the user sets their volumes, [Sound] music_volume and sfx_volume (0 to 100)
    enum Group {
        GROUP_EFFECTS = 0,
        GROUP_MUSIC = 1
    };
    // the volumes of config.ini, again: after a change in the configuration
    void loadVolumes();
    // the synthesizer of the effects: AUTO is the one of the music ([Sound] sfx_synth: auto, opl, mt32)
    enum class EffectSynth {
        AUTO,
        OPL,
        MT32
    };
    // the effects into PCM again with this synthesizer; false (the effects stay) for MT-32 without its ROMs
    bool renderEffects(EffectSynth synth);

    int playSoundFx(MemMusic *fx, int volume = 100) override;
    void setSoundFxVolume(int fxChannel, int volume) override;
    void stopSoundFx(int fxChannel) override;
    void stopSoundFx(int effect, const void *emitter) override;
    bool isSoundFxPlaying(int fxChannel) override;
    void stopAllSoundFx() override;
    void playOrUpdateSoundFx(int effect, const void *emitter, float distance) override;
    void playSoundFxAtDistance(int effect, float distance) override;
    void playEffectAt(int effect, uint8_t *voc, size_t vocSize, const Vector3D &position, float distance) override;
    void playOrUpdateEffectAt(int effect, uint8_t *voc, size_t vocSize, const void *emitter, const Vector3D &position, float distance) override;

    void stopSound() override;
    void stopSound(int chanl) override;
    void playSoundVoc(uint8_t *data, size_t vocSize) override;
    void playSoundVoc(uint8_t *data, size_t vocSize, int channel, int loop = 0) override;
    void playOrUpdateSoundVoc(uint8_t *data, size_t vocSize, const void *emitter, float distance) override;
    void playSoundVocAtDistance(uint8_t *data, size_t vocSize, float distance) override;
    void setVolume(int volume, int channel = -1) override;
    bool isSoundPlaying() override;
    bool isSoundPlaying(int chanl) override;

    // the effects rendered at init: their sounds, their PCM and the synthesizer that made them (a player, the tests)
    size_t effectCount() const {
        return effects_.size();
    }
    const std::shared_ptr<r4::audio::Sound> &effect(int index) const;
    const R4SoundPcm &effectPcm(int index) const;
    const std::string &effectSynth() const {
        return effectSynth_;
    }
    // the context of OpenAL, null without OpenAL (then RSMixer plays with SDL_mixer)
    r4::audio::Context *context() const {
        return context_.get();
    }

private:
    // a VOC played on a channel, as Mix_PlayChannel: loops plays left after this one (-1: without end)
    struct Channel {
        std::shared_ptr<r4::audio::Source> source;
        int loopsLeft = 0;
        int volume = 128;
    };
    std::shared_ptr<r4::audio::Sound> soundOf(const R4SoundPcm &pcm);
    std::shared_ptr<r4::audio::Sound> vocSound(uint8_t *data, size_t size);
    int effectIndex(MemMusic *fx) const;
    // a source of an effect at the position (in the axes of the game), the game's rule of distance
    std::shared_ptr<r4::audio::Source> spatial(const std::shared_ptr<r4::audio::Sound> &sound, const Vector3D &position);
    // a source at the listener, its gain from the volume of the game (0..100)
    std::shared_ptr<r4::audio::Source> flat(const std::shared_ptr<r4::audio::Sound> &sound, int volume);
    void playChannel(int channel, const std::shared_ptr<r4::audio::Sound> &sound, int loops);
    int freeChannel() const;

    std::unique_ptr<r4::audio::Context> context_;
    std::shared_ptr<r4::audio::Stream> music_;
    std::vector<std::shared_ptr<r4::audio::Sound>> effects_;
    std::vector<R4SoundPcm> effectPcm_;
    std::string effectSynth_;
    std::unordered_map<const uint8_t *, std::shared_ptr<r4::audio::Sound>> vocs_;
    std::map<int, Channel> channels_;
    int lastChannel_ = -1;
    // the effects that play once, released at their end
    std::vector<std::shared_ptr<r4::audio::Source>> oneShots_;
    // an effect for each emitter: (emitter, effect) -> its source
    std::map<std::pair<const void *, int>, std::shared_ptr<r4::audio::Source>> emitters_;
    // the channels of playSoundFx (5)
    std::shared_ptr<r4::audio::Source> fxChannels_[5];
    Uint64 lastUpdate_ = 0;
};
