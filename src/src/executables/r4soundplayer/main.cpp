//
//  main.cpp
//  libRealSpace
//
//  r4soundplayer: the sounds of Strike Commander as neosc_r4 plays them with R4Mixer. The effects of SOUNDFX (XMIDI rendered
//  into PCM with the synthesizer of [Sound] midi_music), the digitized effects of DSOUNDFX.PAK (VOC) and the music. A sound
//  plays at a direction and a distance from the listener, or passes in front of it (Doppler effect). Export writes a WAV.
//  Usage: r4soundplayer [-data folder of the TRE] [-hrtf] [-frames N]
//  The data: ./SC.DAT and ./assets, as neosc_r4; or -data, a folder with BIGTRE.TRE and LILTRE.TRE.
//
#include "../../engine/Config.hpp"
#include "../../r4game/R4Mixer.h"
#include "../../realspace/AssetManager.h"
#include "../../realspace/RSSound.h"
#include "r4/gpu/device.h"
#include "r4/ui/ui.h"

#include <SDL.h>

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace {

const char *kEffectNames[] = {"MK82_DROP", "MK20_DROP", "EXPLOSION_1", "EXPLOSION_2", "EXPLOSION_3", "EXPLOSION_4", "AIM9_SHOOT", "AIM120_SHOOT", "POD_SHOOT", "MAVERICK_SHOOT", "GUN_IMPACT_1", "GUN_IMPACT_2", "GUN_IMPACT_3", "GUN_IMPACT_4", "GUN_IMPACT_5", "GUN_IMPACT_6", "PLANE_PASS_BY_1", "PLANE_PASS_BY_2", "PLANE_PASS_BY_3", "GEARS_UP", "GEARS_DOWN", "TOUCH_DOWN", "", "ENGINE_MIL", "ENGINE_AFB", "ENGINE_START_MIL", "ENGINE_START_AFB", "ENGINE_SHUT_DOWN", "ENGINE_MIL_SHUT_DOWN", "ENGINE_AFB_SHUT_DOWN", "ENGINE_AFB_TO_MIL", "GUN_HIT", "PLAYER_HIT", "WATER_SPLASH", "GUN_FIRE", "TURRET_FIRE"};
constexpr int kEffectIds = (int)(sizeof kEffectNames / sizeof kEffectNames[0]);

struct Item {
    std::string name;
    const R4SoundPcm *pcm = nullptr;
    std::shared_ptr<r4::audio::Sound> sound;
};

bool exists(const std::string &path) {
    if (FILE *f = std::fopen(path.c_str(), "rb")) {
        std::fclose(f);
        return true;
    }
    return false;
}

// the introduction and two passes of the loop, as the game plays it
bool writeWav(const std::string &path, const R4SoundPcm &pcm) {
    std::vector<int16_t> heard(pcm.samples);
    if (pcm.loops) {
        for (int k = 0; k < 2; k++) {
            heard.insert(heard.end(), pcm.samples.begin() + (long)pcm.loopStart, pcm.samples.begin() + (long)pcm.loopEnd);
        }
    }
    FILE *f = std::fopen(path.c_str(), "wb");
    if (!f) {
        return false;
    }
    const uint32_t data = (uint32_t)heard.size() * 2;
    const uint32_t rate = (uint32_t)pcm.rate;
    const uint32_t header[] = {0x46464952u, 36 + data, 0x45564157u, 0x20746d66u, 16, 0x00010001u, rate, rate * 2, 0x00100002u, 0x61746164u, data};
    std::fwrite(header, sizeof header, 1, f);
    std::fwrite(heard.data(), 2, heard.size(), f);
    std::fclose(f);
    return true;
}

// the peaks of the sound, 0 to 1, in count columns
std::vector<float> envelope(const R4SoundPcm &pcm, int count) {
    std::vector<float> peaks((size_t)count, 0.0f);
    if (pcm.samples.empty()) {
        return peaks;
    }
    for (size_t i = 0; i < pcm.samples.size(); i++) {
        float &p = peaks[i * (size_t)count / pcm.samples.size()];
        p = std::max(p, std::abs((float)pcm.samples[i]) / 32768.0f);
    }
    return peaks;
}

} // namespace

int main(int argc, char *argv[]) {
    std::string data;
    bool hrtf = false;
    int frames = -1;
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a == "-data" && i + 1 < argc) {
            data = argv[++i];
        } else if (a == "-hrtf") {
            hrtf = true;
        } else if (a == "-frames" && i + 1 < argc) {
            frames = std::atoi(argv[++i]);
        }
    }
    Config &config = Config::getInstance();
    config.load("./assets/config.ini");
    if (hrtf) {
        config.setString("Sound", "hrtf", "true");
    }
    AssetManager &assets = AssetManager::getInstance();
    if (data.empty() && exists("./SC.DAT")) {
        assets.SetBase("./assets");
        assets.ReadISOImage("./SC.DAT");
        assets.init({"BIGTRE.TRE", "LILTRE.TRE", "VOCLIST.TRE"});
    } else {
        assets.SetBase(data.empty() ? "./assets" : data.c_str());
        assets.init({"BIGTRE.TRE", "LILTRE.TRE"});
    }
    assets.sound_root_path = "..\\..\\DATA\\SOUND\\";

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "r4soundplayer: %s\n", SDL_GetError());
        return 1;
    }
#ifdef __APPLE__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_Window *window = SDL_CreateWindow("r4soundplayer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 960, 640, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_GLContext gl = window ? SDL_GL_CreateContext(window) : nullptr;
    if (!gl) {
        std::fprintf(stderr, "r4soundplayer: %s\n", SDL_GetError());
        return 1;
    }
    SDL_GL_SetSwapInterval(1);
    std::string error;
    std::unique_ptr<r4::gpu::Device> device = r4::gpu::createGLDevice(
        [](const char *n) {
            return SDL_GL_GetProcAddress(n);
        },
        &error);
    if (!device) {
        std::fprintf(stderr, "r4soundplayer: %s\n", error.c_str());
        return 1;
    }
    auto gui = std::make_unique<r4::ui::Context>(*device);

    RSMixer::setInstance(std::make_unique<R4Mixer>());
    RSMixer::getInstance().init();
    R4Mixer *mixer = R4Mixer::current();
    if (!mixer) {
        std::fprintf(stderr, "r4soundplayer: no OpenAL\n");
        return 1;
    }
    r4::audio::Context &audio = *mixer->context();
    audio.listener().setPose({0, 0, 0}, {0, 1, 0}, {0, 0, 1});

    // the lists: the XMIDI effects (named by the ids of the game that use them), the VOC, the music of the banks
    RSSound &sounds = RSSound::getInstance();
    sounds.init();
    std::vector<Item> xmidi;
    // the effects of the mixer: again after they are rendered with another synthesizer
    auto listEffects = [&]() {
        xmidi.clear();
        for (size_t i = 0; i < mixer->effectCount(); i++) {
            Item it;
            it.name = "SOUNDFX " + std::to_string(i);
            for (int id = 0; id < kEffectIds; id++) {
                if (kEffectNames[id][0] != 0 && sounds.fxEffect((SoundEffectIds)id) == (int)i) {
                    it.name += std::string(" ") + kEffectNames[id];
                }
            }
            it.pcm = &mixer->effectPcm((int)i);
            it.sound = mixer->effect((int)i);
            xmidi.push_back(it);
        }
    };
    listEffects();
    std::vector<R4SoundPcm> vocPcm(sounds.sounds.size());
    std::vector<Item> vocs;
    for (size_t i = 0; i < sounds.sounds.size(); i++) {
        if (!decodeVoc(sounds.sounds[i]->data, sounds.sounds[i]->size, vocPcm[i])) {
            continue;
        }
        Item it;
        it.name = "DSOUNDFX " + std::to_string(i) + (i < (size_t)kEffectIds ? std::string(" ") + kEffectNames[i] : std::string());
        it.pcm = &vocPcm[i];
        it.sound = r4::audio::Sound::fromPCM(vocPcm[i].samples.data(), vocPcm[i].samples.size(), 1, vocPcm[i].rate);
        vocs.push_back(it);
    }
    std::printf("r4soundplayer: %zu XMIDI effects (%s), %zu VOC\n", xmidi.size(), mixer->effectSynth().c_str(), vocs.size());

    int tab = 0;
    int selected = -1;
    std::shared_ptr<r4::audio::Source> source;
    bool loop = false;
    float gain = 1;
    float azimuth = 0;
    float distance = 0;
    bool passing = false;
    float passX = 0;
    float speed = 250;
    int bank = 0;
    int track = -1;
    float musicVolume = 100;
    float userMusic = 100.0f * audio.groupGain(R4Mixer::GROUP_MUSIC);
    float userEffects = 100.0f * audio.groupGain(R4Mixer::GROUP_EFFECTS);
    int synth = mixer->effectSynth() == "MT-32" ? 1 : 0;
    std::string status;
    r4::ui::Input input;
    Uint64 last = SDL_GetPerformanceCounter();
    bool quit = false;
    while (!quit) {
        input.pressed = false;
        input.released = false;
        input.wheel = 0;
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) {
                quit = true;
            } else if (e.type == SDL_MOUSEMOTION) {
                input.mouseX = (float)e.motion.x;
                input.mouseY = (float)e.motion.y;
            } else if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) {
                bool down = e.type == SDL_MOUSEBUTTONDOWN;
                (e.button.button == SDL_BUTTON_LEFT ? input.left : input.right) = down;
                (down ? input.pressed : input.released) = true;
            } else if (e.type == SDL_MOUSEWHEEL) {
                input.wheel += (float)e.wheel.y;
            }
        }
        if (frames >= 0 && frames-- == 0) {
            quit = true;
        }
        const Uint64 now = SDL_GetPerformanceCounter();
        const float dt = std::min(0.1f, (float)(now - last) / (float)SDL_GetPerformanceFrequency());
        last = now;
        int w = 0;
        int h = 0;
        int pw = 0;
        int ph = 0;
        SDL_GetWindowSize(window, &w, &h);
        SDL_GL_GetDrawableSize(window, &pw, &ph);

        std::vector<Item> &items = tab == 0 ? xmidi : vocs;
        gui->beginFrame(w, h, input, w > 0 ? (float)pw / (float)w : 1);
        const float listWidth = std::min(420.0f, (float)w * 0.45f);
        gui->beginPanel("sounds", {0, 0, listWidth, (float)h}, "Sounds");
        int previousTab = tab;
        gui->tabs("kind", tab, {"XMIDI", "VOC", "Music"});
        if (tab != previousTab) {
            selected = -1;
        }
        gui->beginBox("list");
        if (tab < 2) {
            for (size_t i = 0; i < items.size(); i++) {
                const R4SoundPcm &p = *items[i].pcm;
                char label[160];
                std::snprintf(label, sizeof label, "%s%s  %.2f s%s", (int)i == selected ? "> " : "", items[i].name.c_str(), p.rate ? (double)p.samples.size() / p.rate : 0.0, p.loops ? "  loop" : "");
                if (gui->button(label, items[i].sound != nullptr)) {
                    selected = (int)i;
                    status.clear();
                    if (source) {
                        source->stop();
                    }
                    source = audio.createSource(items[i].sound);
                    source->referenceDistance = 0;
                    source->maxDistance = 1000;
                    loop = p.loops;
                    passX = -800;
                    source->play();
                }
            }
        } else {
            const std::vector<MemMusic *> &tracks = mixer->music->musics[(uint8_t)bank];
            for (size_t i = 0; i < tracks.size(); i++) {
                std::string label = ((int)i == track ? "> track " : "track ") + std::to_string(i);
                if (gui->button(label.c_str())) {
                    track = (int)i;
                    mixer->playMusic((uint32_t)i, -1);
                }
            }
        }
        gui->endBox();
        gui->endPanel();

        gui->beginPanel("player", {listWidth, 0, (float)w - listWidth, (float)h}, "Player");
        if (tab < 2 && selected >= 0 && selected < (int)items.size()) {
            const Item &it = items[(size_t)selected];
            const R4SoundPcm &p = *it.pcm;
            gui->label(it.name.c_str(), r4::ui::Font::BOLD);
            char info[200];
            if (p.loops) {
                std::snprintf(info, sizeof info, "%d Hz, %.2f s: introduction %.2f s, loop %.2f s", p.rate, (double)p.samples.size() / p.rate, (double)p.loopStart / p.rate, (double)(p.loopEnd - p.loopStart) / p.rate);
            } else {
                std::snprintf(info, sizeof info, "%d Hz, %.2f s, no loop", p.rate, (double)p.samples.size() / p.rate);
            }
            gui->message(info);
            std::vector<float> peaks = envelope(p, 300);
            float peak = 1.0f / 32768.0f;
            for (float v : peaks) {
                peak = std::max(peak, v);
            }
            char scale[64];
            std::snprintf(scale, sizeof scale, "%s, peak %.0f %%", tab == 0 ? mixer->effectSynth().c_str() : "VOC", peak * 100.0f);
            gui->plot(peaks.data(), (int)peaks.size(), peak, r4::ui::Color::rgb(0.4f, 0.8f, 0.5f), scale, 80);
            gui->columns(3);
            if (gui->button("Play") && it.sound) {
                if (source) {
                    source->stop();
                }
                source = audio.createSource(it.sound);
                source->referenceDistance = 0;
                source->maxDistance = 1000;
                passX = -800;
                source->play();
            }
            if (gui->button("Stop") && source) {
                source->stop();
            }
            if (gui->button("Export WAV")) {
                std::string path = "r4sound_" + std::string(tab == 0 ? "xmidi_" : "voc_") + std::to_string(selected) + ".wav";
                status = writeWav(path, p) ? "written: " + path : "cannot write " + path;
            }
            gui->columns(1);
            gui->toggle("Loop", loop);
            gui->slider("Gain", gain, 0, 2, "%.2f");
            gui->space();
            gui->label("In space (the listener looks ahead)");
            gui->toggle("Pass in front (Doppler)", passing);
            if (passing) {
                gui->slider("Speed (units/s)", speed, 50, 1000, "%.0f");
                gui->message("the source goes from left to right, 100 units ahead");
            } else {
                gui->slider("Direction (degrees)", azimuth, -180, 180, "%.0f");
                gui->slider("Distance (0: at the listener)", distance, 0, 1000, "%.0f");
            }
            if (!status.empty()) {
                gui->message(status.c_str());
            }
        } else if (tab == 2) {
            gui->radio("Bank", bank, {"0: midgames", "1: gameflow", "2: combat"});
            if (gui->button("Use this bank")) {
                mixer->stopMusic();
                mixer->switchBank((uint8_t)bank);
                track = -1;
            }
            if (gui->button("Stop the music")) {
                mixer->stopMusic();
                track = -1;
            }
            if (gui->slider("Volume", musicVolume, 0, 100, "%.0f")) {
                mixer->setVolume((int)(musicVolume * MIX_MAX_VOLUME / 100));
            }
            gui->message("Select a bank, then a track in the list.");
        } else {
            gui->message("Select a sound in the list.");
        }
        gui->space();
        char voices[96];
        std::snprintf(voices, sizeof voices, "OpenAL: %s, %d voices in use%s", audio.deviceName().c_str(), audio.voicesInUse(), audio.hrtf() ? ", HRTF" : "");
        gui->message(voices);
        gui->label("Volumes of the user ([Sound] music_volume, sfx_volume)");
        if (gui->slider("Music", userMusic, 0, 100, "%.0f")) {
            audio.setGroupGain(R4Mixer::GROUP_MUSIC, userMusic / 100.0f);
        }
        if (gui->slider("Effects", userEffects, 0, 100, "%.0f")) {
            audio.setGroupGain(R4Mixer::GROUP_EFFECTS, userEffects / 100.0f);
        }
        if (tab == 0) {
            gui->radio("Synthesizer of the effects", synth, {"AdLib (OPL)", "MT-32 (munt)"});
            if (gui->button("Render the effects again")) {
                if (source) {
                    source->stop();
                    source.reset();
                }
                selected = -1;
                const bool done = mixer->renderEffects(synth == 1 ? R4Mixer::EffectSynth::MT32 : R4Mixer::EffectSynth::OPL);
                listEffects();
                status = done ? "rendered with " + mixer->effectSynth() : "no MT-32 ROMs in [Sound] mt32_rom_dir";
                synth = mixer->effectSynth() == "MT-32" ? 1 : 0;
            }
            if (selected < 0 && !status.empty()) {
                gui->message(status.c_str());
            }
        }
        gui->endPanel();
        gui->endFrame();

        if (source) {
            source->loop = loop;
            source->gain = gain;
            if (passing) {
                passX += speed * dt;
                if (passX > 800) {
                    passX = -800;
                }
                source->positional = true;
                source->position = {passX, 100, 0};
                source->velocity = {speed, 0, 0};
            } else {
                const float a = azimuth * 3.14159265f / 180.0f;
                source->positional = distance > 0;
                source->position = {std::sin(a) * distance, std::cos(a) * distance, 0};
                source->velocity = {0, 0, 0};
            }
        }
        audio.update(dt);

        r4::gpu::ClearValues clear;
        clear.r = 0.08f;
        clear.g = 0.09f;
        clear.b = 0.12f;
        device->beginPass(0, 0, pw, ph, clear);
        device->endPass();
        gui->render();
        SDL_GL_SwapWindow(window);
    }
    source.reset();
    xmidi.clear();
    vocs.clear();
    RSMixer::setInstance(nullptr);
    gui.reset();
    device.reset();
    SDL_GL_DeleteContext(gl);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
