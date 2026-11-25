//
//  R4Mixer.cpp
//  libRealSpace
//
#include "R4Mixer.h"

#include "engine/Config.hpp"
#include "engine/SCRenderer.h"

#include <algorithm>
#include <cstdio>

namespace {

constexpr int kRate = 44100;
// the game's rule (SoundFX_Play3D_59902): the volume goes down to 0 at 1000 units
constexpr float kReach = 1000.0f;
// a move of the camera longer than this in a frame is a change of view, not a speed
constexpr float kJump = 300.0f;
// the channels of playSoundVoc without a channel, as the channels of SDL_mixer
constexpr int kChannels = 16;
// the effect number of playOrUpdateSoundVoc in the sources of the emitters
constexpr int kVocEffect = -1000;

r4::Vec3 vec(const Vector3D &v) {
    return {v.x, v.y, v.z};
}

std::shared_ptr<r4::audio::Sound> kNoSound;

// the notes and the reverb of a device end: it can render the next effect
void drain(MusicDevice &device) {
    std::vector<int16_t> stereo(1024 * 2);
    for (int i = 0; i < 3 * kRate / 1024; i++) {
        std::fill(stereo.begin(), stereo.end(), 0);
        device.render(stereo.data(), 1024);
        bool quiet = true;
        for (int16_t s : stereo) {
            quiet = quiet && std::abs((int)s) <= 2;
        }
        if (quiet) {
            return;
        }
    }
}

} // namespace

R4Mixer::R4Mixer() = default;

R4Mixer::~R4Mixer() {
    shuttingDown = true;
    music_.reset();
    oneShots_.clear();
    emitters_.clear();
    channels_.clear();
    for (auto &c : fxChannels_) {
        c.reset();
    }
    effects_.clear();
    effectPcm_.clear();
    vocs_.clear();
    context_.reset();
}

R4Mixer *R4Mixer::current() {
    R4Mixer *m = dynamic_cast<R4Mixer *>(s_instance.get());
    return m && m->context_ ? m : nullptr;
}

void R4Mixer::init() {
    if (has_been_initialized) {
        return;
    }
    Config &cfg = Config::getInstance();
    r4::audio::Settings settings;
    settings.maxVoices = std::max(1, cfg.getInt("Sound", "max_voices", 64));
    settings.hrtf = cfg.getBool("Sound", "hrtf", false);
    std::string error;
    context_ = r4::audio::Context::create(settings, &error);
    if (!context_) {
        std::printf("R4Mixer: no OpenAL (%s), SDL_mixer instead\n", error.c_str());
        RSMixer::init();
        return;
    }
    has_been_initialized = true;
    std::printf("R4Mixer: OpenAL on %s, %d voices%s\n", context_->deviceName().c_str(), context_->voices(), context_->hrtf() ? ", HRTF" : "");
    context_->setDistanceModel(r4::audio::DistanceModel::LINEAR_CLAMPED);
    context_->setSpeedOfSound((float)cfg.getInt("Sound", "speed_of_sound", 1125));
    context_->setDoppler((float)cfg.getInt("Sound", "doppler", 1));
    initEngine(kRate);
    renderEffects();
    // the music: the synthesis of RSMixer, as Mix_HookMusic did
    music_ = context_->createStream(2, kRate, [this](int16_t *out, int frames) {
        std::lock_guard<std::recursive_mutex> lock(engineMutex);
        if (shuttingDown) {
            std::fill(out, out + frames * 2, (int16_t)0);
            return;
        }
        render(out, frames);
        for (int i = 0; i < frames * 2; i++) {
            out[i] = (int16_t)(out[i] * musicVolume / MIX_MAX_VOLUME);
        }
    });
    music_->source().play();
    lastUpdate_ = SDL_GetPerformanceCounter();
}

// Each effect of SOUNDFX.ADL into PCM, with the synthesizer of the effects of the game: an OPL, or MT-32 emulated (munt). The
// MT-32 of a MIDI port cannot be recorded: munt when its ROMs are there, else the OPL with the AdLib effects.
void R4Mixer::renderEffects() {
    if (!music) {
        return;
    }
    const Uint64 start = SDL_GetPerformanceCounter();
    Config &cfg = Config::getInstance();
    const std::string romDir = cfg.getString("Sound", "mt32_rom_dir", "./assets/mt32");
    RSMusic *bank = music;
    std::unique_ptr<RSMusic> adlib;
    std::unique_ptr<Mt32EmuDevice> munt;
    const bool mt32 = dynamic_cast<Mt32EmuDevice *>(device.get()) || dynamic_cast<Mt32Device *>(device.get());
    if (mt32) {
        munt = std::make_unique<Mt32EmuDevice>(false);
        if (!munt->open(romDir, kRate)) {
            munt.reset();
            std::printf("R4Mixer: no MT-32 ROMs in %s, the effects on the OPL\n", romDir.c_str());
            adlib = std::make_unique<RSMusic>("ADL", "AMUSIC.PAK", "STRIKE.AD", "ADL", "STRIKE.AD");
            adlib->init();
            bank = adlib.get();
        }
    }
    auto set = bank->soundfx_musics.find(0);
    if (set == bank->soundfx_musics.end()) {
        return;
    }
    int loops = 0;
    for (MemMusic *fx : set->second) {
        R4SoundPcm pcm;
        bool ok = false;
        if (munt) {
            drain(*munt);
            ok = fx && renderSoundFx(*munt, &bank->sfx_timbres, fx->data, fx->size, kRate, pcm);
        } else {
            OplDevice opl;
            opl.init(kRate);
            ok = fx && renderSoundFx(opl, &bank->sfx_timbres, fx->data, fx->size, kRate, pcm);
        }
        effects_.push_back(ok ? soundOf(pcm) : nullptr);
        loops += ok && pcm.loops ? 1 : 0;
        effectPcm_.push_back(std::move(pcm));
    }
    effectSynth_ = munt ? "MT-32" : "OPL";
    const double ms = (double)(SDL_GetPerformanceCounter() - start) * 1000.0 / (double)SDL_GetPerformanceFrequency();
    std::printf("R4Mixer: %zu effects in PCM (%s), %d loops, %.0f ms\n", effects_.size(), effectSynth_.c_str(), loops, ms);
}

std::shared_ptr<r4::audio::Sound> R4Mixer::soundOf(const R4SoundPcm &pcm) {
    if (pcm.samples.empty()) {
        return nullptr;
    }
    return r4::audio::Sound::fromPCM(pcm.samples.data(), pcm.samples.size(), 1, pcm.rate, pcm.loops ? pcm.loopStart : 0, pcm.loops ? pcm.loopEnd : 0);
}

std::shared_ptr<r4::audio::Sound> R4Mixer::vocSound(uint8_t *data, size_t size) {
    if (data == nullptr || !context_) {
        return nullptr;
    }
    auto it = vocs_.find(data);
    if (it != vocs_.end()) {
        return it->second;
    }
    R4SoundPcm pcm;
    std::shared_ptr<r4::audio::Sound> sound = decodeVoc(data, size, pcm) ? soundOf(pcm) : nullptr;
    if (!sound) {
        std::printf("R4Mixer: a VOC of %zu bytes cannot be read\n", size);
    }
    vocs_[data] = sound;
    return sound;
}

const std::shared_ptr<r4::audio::Sound> &R4Mixer::effect(int index) const {
    if (index < 0 || index >= (int)effects_.size()) {
        return kNoSound;
    }
    return effects_[(size_t)index];
}

const R4SoundPcm &R4Mixer::effectPcm(int index) const {
    static const R4SoundPcm none;
    if (index < 0 || index >= (int)effectPcm_.size()) {
        return none;
    }
    return effectPcm_[(size_t)index];
}

int R4Mixer::effectIndex(MemMusic *fx) const {
    if (!music || fx == nullptr) {
        return -1;
    }
    auto set = music->soundfx_musics.find(0);
    if (set == music->soundfx_musics.end()) {
        return -1;
    }
    auto it = std::find(set->second.begin(), set->second.end(), fx);
    return it == set->second.end() ? -1 : (int)(it - set->second.begin());
}

std::shared_ptr<r4::audio::Source> R4Mixer::spatial(const std::shared_ptr<r4::audio::Sound> &sound, const Vector3D &position) {
    auto src = context_->createSource(sound);
    src->positional = true;
    src->position = vec(position);
    src->referenceDistance = 0;
    src->maxDistance = kReach;
    src->rolloff = 1;
    return src;
}

std::shared_ptr<r4::audio::Source> R4Mixer::flat(const std::shared_ptr<r4::audio::Sound> &sound, int volume) {
    auto src = context_->createSource(sound);
    src->gain = (float)std::max(0, std::min(100, volume)) / 100.0f;
    return src;
}

void R4Mixer::update() {
    if (!context_) {
        return;
    }
    const Uint64 now = SDL_GetPerformanceCounter();
    float dt = (float)(now - lastUpdate_) / (float)SDL_GetPerformanceFrequency();
    lastUpdate_ = now;
    dt = std::min(std::max(dt, 0.0f), 0.25f);
    r4::audio::Listener &listener = context_->listener();
    const r4::Vec3 before = listener.position();
    if (Camera *camera = SCRenderer::getInstance().getCamera()) {
        listener.setPose(vec(camera->getPosition()), vec(camera->getForward()), vec(camera->getUp()));
    }
    // a change of view moves the camera far in one frame: no Doppler effect from it
    const bool jump = r4::length(listener.position() - before) > kJump;
    if (jump) {
        context_->setDoppler(0);
    }
    context_->update(dt);
    if (jump) {
        context_->setDoppler((float)Config::getInstance().getInt("Sound", "doppler", 1));
    }
    oneShots_.erase(std::remove_if(oneShots_.begin(), oneShots_.end(),
                                   [](const std::shared_ptr<r4::audio::Source> &s) {
                                       return !s->isPlaying();
                                   }),
                    oneShots_.end());
    for (auto it = emitters_.begin(); it != emitters_.end();) {
        if (!it->second->isPlaying()) {
            it = emitters_.erase(it);
        } else {
            ++it;
        }
    }
    for (auto &kv : channels_) {
        Channel &c = kv.second;
        if (c.source && !c.source->isPlaying() && c.loopsLeft > 0) {
            c.loopsLeft--;
            c.source->play();
        }
    }
}

// ---- the effects of the game ----

void R4Mixer::playEffectAt(int effect, uint8_t *voc, size_t vocSize, const Vector3D &position, float distance) {
    if (!context_) {
        RSMixer::playEffectAt(effect, voc, vocSize, position, distance);
        return;
    }
    std::shared_ptr<r4::audio::Sound> sound = voc ? vocSound(voc, vocSize) : this->effect(effect);
    if (!sound || distance >= kReach) {
        return;
    }
    auto src = spatial(sound, position);
    src->play();
    oneShots_.push_back(src);
}

void R4Mixer::playOrUpdateEffectAt(int effect, uint8_t *voc, size_t vocSize, const void *emitter, const Vector3D &position, float distance) {
    if (!context_) {
        RSMixer::playOrUpdateEffectAt(effect, voc, vocSize, emitter, position, distance);
        return;
    }
    auto key = std::make_pair(emitter, effect);
    auto it = emitters_.find(key);
    if (it != emitters_.end()) {
        it->second->position = vec(position);
        if (!it->second->isPlaying()) {
            it->second->play();
        }
        return;
    }
    std::shared_ptr<r4::audio::Sound> sound = voc ? vocSound(voc, vocSize) : this->effect(effect);
    if (!sound || distance >= kReach) {
        return;
    }
    auto src = spatial(sound, position);
    src->play();
    emitters_[key] = src;
}

void R4Mixer::stopSoundFx(int effect, const void *emitter) {
    if (!context_) {
        RSMixer::stopSoundFx(effect, emitter);
        return;
    }
    auto it = emitters_.find(std::make_pair(emitter, effect));
    if (it != emitters_.end()) {
        it->second->stop();
        emitters_.erase(it);
    }
}

void R4Mixer::stopAllSoundFx() {
    if (!context_) {
        RSMixer::stopAllSoundFx();
        return;
    }
    for (auto &kv : emitters_) {
        kv.second->stop();
    }
    emitters_.clear();
    for (auto &s : oneShots_) {
        s->stop();
    }
    oneShots_.clear();
    for (auto &c : fxChannels_) {
        if (c) {
            c->stop();
        }
        c.reset();
    }
}

// the effects of the volume of the game (RSMixer's API without a position): at the listener

void R4Mixer::playOrUpdateSoundFx(int effect, const void *emitter, float distance) {
    if (!context_) {
        RSMixer::playOrUpdateSoundFx(effect, emitter, distance);
        return;
    }
    const int volume = distanceVolume(distance);
    auto key = std::make_pair(emitter, effect);
    auto it = emitters_.find(key);
    if (it != emitters_.end()) {
        it->second->gain = (float)volume / 100.0f;
        if (volume == 0) {
            it->second->stop();
            emitters_.erase(it);
        }
        return;
    }
    if (volume == 0 || !this->effect(effect)) {
        return;
    }
    auto src = flat(this->effect(effect), volume);
    src->play();
    emitters_[key] = src;
}

void R4Mixer::playSoundFxAtDistance(int effect, float distance) {
    if (!context_) {
        RSMixer::playSoundFxAtDistance(effect, distance);
        return;
    }
    const int volume = distanceVolume(distance);
    if (volume == 0 || !this->effect(effect)) {
        return;
    }
    auto src = flat(this->effect(effect), volume);
    src->play();
    oneShots_.push_back(src);
}

int R4Mixer::playSoundFx(MemMusic *fx, int volume) {
    if (!context_) {
        return RSMixer::playSoundFx(fx, volume);
    }
    const std::shared_ptr<r4::audio::Sound> &sound = this->effect(effectIndex(fx));
    if (!sound) {
        return -1;
    }
    for (int c = 0; c < 5; c++) {
        if (!fxChannels_[c] || !fxChannels_[c]->isPlaying()) {
            fxChannels_[c] = flat(sound, volume);
            fxChannels_[c]->play();
            return c;
        }
    }
    return -1;
}

void R4Mixer::setSoundFxVolume(int fxChannel, int volume) {
    if (!context_) {
        RSMixer::setSoundFxVolume(fxChannel, volume);
        return;
    }
    if (fxChannel >= 0 && fxChannel < 5 && fxChannels_[fxChannel]) {
        fxChannels_[fxChannel]->gain = (float)volume / 100.0f;
    }
}

void R4Mixer::stopSoundFx(int fxChannel) {
    if (!context_) {
        RSMixer::stopSoundFx(fxChannel);
        return;
    }
    if (fxChannel >= 0 && fxChannel < 5 && fxChannels_[fxChannel]) {
        fxChannels_[fxChannel]->stop();
        fxChannels_[fxChannel].reset();
    }
}

bool R4Mixer::isSoundFxPlaying(int fxChannel) {
    if (!context_) {
        return RSMixer::isSoundFxPlaying(fxChannel);
    }
    return fxChannel >= 0 && fxChannel < 5 && fxChannels_[fxChannel] && fxChannels_[fxChannel]->isPlaying();
}

// ---- the VOC of the channels: voices, radio, the engine of the player, at the listener ----

int R4Mixer::freeChannel() const {
    for (int c = 0; c < kChannels; c++) {
        auto it = channels_.find(c);
        if (it == channels_.end() || !it->second.source || (!it->second.source->isPlaying() && it->second.loopsLeft <= 0)) {
            return c;
        }
    }
    return -1;
}

void R4Mixer::playChannel(int channel, const std::shared_ptr<r4::audio::Sound> &sound, int loops) {
    Channel &c = channels_[channel];
    if (c.source) {
        c.source->stop();
    }
    c.source = context_->createSource(sound);
    c.source->loop = loops < 0;
    c.source->gain = (float)c.volume / (float)MIX_MAX_VOLUME;
    c.loopsLeft = std::max(0, loops);
    c.source->play();
    lastChannel_ = channel;
}

void R4Mixer::playSoundVoc(uint8_t *data, size_t vocSize) {
    if (!context_) {
        RSMixer::playSoundVoc(data, vocSize);
        return;
    }
    std::shared_ptr<r4::audio::Sound> sound = vocSound(data, vocSize);
    const int c = freeChannel();
    if (!sound || c < 0) {
        return;
    }
    channels_[c].volume = MIX_MAX_VOLUME;
    playChannel(c, sound, 0);
}

void R4Mixer::playSoundVoc(uint8_t *data, size_t vocSize, int channel, int loop) {
    if (!context_) {
        RSMixer::playSoundVoc(data, vocSize, channel, loop);
        return;
    }
    std::shared_ptr<r4::audio::Sound> sound = vocSound(data, vocSize);
    if (!sound) {
        return;
    }
    playChannel(channel < 0 ? std::max(0, freeChannel()) : channel, sound, loop);
}

void R4Mixer::playOrUpdateSoundVoc(uint8_t *data, size_t vocSize, const void *emitter, float distance) {
    if (!context_) {
        RSMixer::playOrUpdateSoundVoc(data, vocSize, emitter, distance);
        return;
    }
    const int volume = distanceVolume(distance);
    // no effect number: one digitized sound for each emitter
    auto key = std::make_pair(emitter, kVocEffect);
    auto it = emitters_.find(key);
    if (it != emitters_.end()) {
        it->second->gain = (float)volume / 100.0f;
        if (volume == 0) {
            it->second->stop();
            emitters_.erase(it);
        }
        return;
    }
    std::shared_ptr<r4::audio::Sound> sound = vocSound(data, vocSize);
    if (volume == 0 || !sound) {
        return;
    }
    auto src = flat(sound, volume);
    src->play();
    emitters_[key] = src;
}

void R4Mixer::playSoundVocAtDistance(uint8_t *data, size_t vocSize, float distance) {
    if (!context_) {
        RSMixer::playSoundVocAtDistance(data, vocSize, distance);
        return;
    }
    const int volume = distanceVolume(distance);
    std::shared_ptr<r4::audio::Sound> sound = vocSound(data, vocSize);
    if (volume == 0 || !sound) {
        return;
    }
    auto src = flat(sound, volume);
    src->play();
    oneShots_.push_back(src);
}

void R4Mixer::stopSound() {
    if (!context_) {
        RSMixer::stopSound();
        return;
    }
    for (auto &kv : channels_) {
        if (kv.second.source) {
            kv.second.source->stop();
        }
        kv.second.loopsLeft = 0;
    }
}

void R4Mixer::stopSound(int chanl) {
    if (!context_) {
        RSMixer::stopSound(chanl);
        return;
    }
    auto it = channels_.find(chanl);
    if (it != channels_.end() && it->second.source) {
        it->second.source->stop();
        it->second.loopsLeft = 0;
    }
}

bool R4Mixer::isSoundPlaying() {
    if (!context_) {
        return RSMixer::isSoundPlaying();
    }
    return isSoundPlaying(lastChannel_);
}

bool R4Mixer::isSoundPlaying(int chanl) {
    if (!context_) {
        return RSMixer::isSoundPlaying(chanl);
    }
    auto it = channels_.find(chanl);
    return it != channels_.end() && it->second.source && (it->second.source->isPlaying() || it->second.loopsLeft > 0);
}

void R4Mixer::setVolume(int volume, int channel) {
    if (!context_ || channel == -1) {
        RSMixer::setVolume(volume, channel);
        return;
    }
    Channel &c = channels_[channel];
    c.volume = std::max(0, std::min(MIX_MAX_VOLUME, volume));
    if (c.source) {
        c.source->gain = (float)c.volume / (float)MIX_MAX_VOLUME;
    }
}
