//
//  RSMixer.h
//  libRealSpace
//
//  Created by Rémi LEONARD on 02/09/2024.
//  Copyright (c) 2013 Fabien Sanglard. All rights reserved.
//
//  Musique et effets XMIDI : chaine du jeu original, sans ADLMIDI ni banque WOPL.
//    SCMusicSequencer (20 Hz) -> AILXmidiDriver (120 Hz) -> MusicDevice (OPL3, MT-32, General MIDI),
//  rendue dans le flux de SDL_mixer par Mix_HookMusic. Les VOC restent sur les canaux SDL_mixer.
//  Une autre sortie (R4Mixer de r4game, OpenAL) hérite de RSMixer : setInstance avant le premier getInstance.
//
#pragma once
#include "../realspace/AssetManager.h"
#include "../realspace/RSMusic.h"
#include <SDL2/SDL_mixer_ext.h>
#include <deque>
#include <mutex>
#include <vector>
#include "../commons/Nuked-OPL3/opl3.h"
#include "AILAdlibDriver.h"
#include "AILXmidiDriver.h"
#include "SCMusicSequencer.h"
#include "MusicOutputs.h"
#include "Mt32EmuOutput.h"
#include "../commons/Matrix.h"
#include <memory>

class RSMixer {
protected:
    inline static std::unique_ptr<RSMixer> s_instance{};
    int initted;
    bool has_been_initialized{ false };
    bool isplaying;
    uint32_t current_music{ UINT32_MAX };
    int channel{ 0 };
    uint8_t *voc_data{ nullptr };

    // --- moteur musical (protege par engineMutex : le rendu tourne dans le fil audio) ---
    std::recursive_mutex engineMutex;
    std::unique_ptr<MusicDevice> device; // musique, et effets sauf en General MIDI
    std::unique_ptr<OplDevice> sfxOpl;   // General MIDI : les effets (SOUNDFX.ADL) restent en FM
    AILXmidiDriver xmi;
    AILXmidiDriver sfxOwnXmi;
    AILXmidiDriver *sfxXmi{ &xmi };
    SCMusicSequencer sequencer;
    int sampleRate{ 44100 };
    uint16_t audioFormat{ AUDIO_S16SYS };
    int audioChannels{ 2 };
    double accDriver{ 0 };
    double accGame{ 0 };
    int musicVolume{ MIX_MAX_VOLUME };
    int musicHandle{ -1 };             // piste isolee (playMusic) : handle XMIDI
    int loopsLeft{ 0 };                // -1 = sans fin
    static const int SFX_CHANNELS = 5; // objet effets du jeu : 5 canaux (MUSIC_SYSTEM.md 7.1)
    int sfxHandle[SFX_CHANNELS]{
        -1,
        -1,
        -1,
        -1,
        -1
    };
    int sfxChannels{ SFX_CHANNELS };
    int traceTicks{ 0 };
    void traceDriverState();
    MemMusic *sfxEffect[SFX_CHANNELS]{};
    const void *sfxEmitter[SFX_CHANNELS]{};
    std::vector<int16_t> mixBuffer;

    static void musicHook(void *udata, Uint8 *stream, int len);
    void render(int16_t *out, int frames); // appele sous engineMutex
    bool sequenced{ false };               // la musique courante passe par le sequenceur (banque avec .dat)
    int isolatedTrack{ -1 };               // piste isolee : index dans la banque (-1 si inconnu), pour les evenements
    std::deque<SCMusicEvent> events;
    void pushEvent(SCMusicEvent::Type type, int track, int fromTrack, int link, int measure);
    void drainSequencer(); // recopie les evenements du sequenceur dans 'events'
    void stopMusicLocked();
    void playSequenced(const SCMusicSet *set, uint32_t index);
    void playIsolated(MemMusic *mus, int loop, int index);
    void serveDriver();
    // les appareils de la musique et des effets selon [Sound] midi_music, a la frequence rate
    void initEngine(int rate);
    bool outputOpened{ false };

public:
    RSMusic *music;
    bool shuttingDown = false;
    std::unordered_map<int, Mix_Chunk *> channelChunks;
    std::unordered_map<int, const void *> vocEmitters;
    std::unordered_map<int, uint8_t *> vocData;
    MemMusic *currentMusicMemPtr = nullptr;
    static RSMixer &getInstance() {
        if (!s_instance) {
            s_instance = std::make_unique<RSMixer>();
        }
        return *s_instance;
    }
    static void setInstance(std::unique_ptr<RSMixer> inst) {
        s_instance = std::move(inst);
    }
    RSMixer();
    virtual ~RSMixer();
    virtual void init();
    // Joue la piste index de la banque courante.
    // - Banque avec un .dat (COMBAT.ADL + COMBAT.DAT) : le changement de piste attend la barre
    //   de mesure suivante et passe par la piste de liaison que donne le .dat, s'il y en a une
    //   (Music_RequestTune_5A984). loop est ignore : les pistes bouclent d'elles-memes.
    // - Autre banque : piste jouee seule. loop : -1 = sans fin, n >= 1 = n fois (0 = une fois).
    void playMusic(uint32_t index, int loop = 1);
    // Meme chose ; si mus appartient a une banque avec un .dat, il passe par les transitions.
    void playMusic(MemMusic *mus, int loop = 1);
    void switchBank(uint8_t bank);
    // fade : fondu d'une seconde (Music_StopWithFade_AB1EF), sinon arret immediat
    void stopMusic(bool fade = false);
    // Piste en train de jouer (apres une transition), UINT32_MAX si aucune
    uint32_t getMusicID();
    // Piste demandee (celle vers laquelle une transition est en cours), UINT32_MAX si arret
    uint32_t getRequestedMusicID();
    // Une piste de liaison du .dat est en train de jouer
    bool isInTransition();
    // Mesure courante de la piste principale (compteur du pilote XMIDI)
    int getMeasure();

    // Evenements musicaux, a lire depuis la boucle de jeu (une fois par frame, jusqu'a false).
    // RSMixer ne sait pas ce que represente une piste : c'est au consommateur de reconnaitre
    // une ponctuation (0x10 a 0x12 dans COMBAT) et de demander la piste suivante.
    //   TRANSITION_STARTED : track = piste demandee, fromTrack = piste quittee, link = liaison
    //   TRACK_STARTED      : track = piste qui demarre, fromTrack = precedente (-1 au depart)
    //   TRACK_FINISHED     : track = piste qui ne boucle pas et vient de se terminer
    //   MUSIC_STOPPED      : arret effectif (stopMusic, fondu termine, ou autre musique lancee)
    // File limitee a SCMusicSequencer::MAX_EVENTS : au-dela, les plus anciens sont perdus.
    bool pollMusicEvent(SCMusicEvent &e);

    // Effets XMIDI (SOUNDFX.ADL) : 5 canaux comme le jeu. volume en pourcentage (0..100,
    // SoundFX_Play3D_59902 : 100 - distance/10). Renvoie le canal, -1 si aucun libre.
    virtual int playSoundFx(MemMusic *fx, int volume = 100);
    virtual void setSoundFxVolume(int fxChannel, int volume);
    virtual void stopSoundFx(int fxChannel); // SoundFX_StopEffect_59A8A : Note Off seulement
    virtual void stopSoundFx(int effect, const void *emitter);
    virtual bool isSoundFxPlaying(int fxChannel);
    virtual void stopAllSoundFx(); // SoundFX_StopAll_5AA95 : arrete les effets et rend leurs canaux verrouilles
    // Nombre d'effets XMIDI joués en même temps (5 dans Strike Commander, 1 dans Pacific Strike).
    void setSoundFxChannels(int count);
    // SoundFX_PlayOrUpdate_5A906 : effet de SOUNDFX.ADL pour cet emetteur, volume selon la distance
    // au point de vue (SoundFX_Play3D_59902) ; demarre s'il ne joue pas, sinon met a jour son volume.
    virtual void playOrUpdateSoundFx(int effect, const void *emitter, float distance);
    // Effet de SOUNDFX.ADL joué une fois, volume selon la distance.
    virtual void playSoundFxAtDistance(int effect, float distance);
    // SoundFX_Play3D_59902 : 100 - d / 10 sous 1 000 m, 5 jusqu'à 5 000 m, 0 au-delà.
    static int distanceVolume(float distance);
    // Effet du jeu à une position (SCMission) : le son numérisé voc s'il y en a un, sinon l'effet de SOUNDFX.ADL. Ici, volume
    // selon la distance ; une sortie dans l'espace se sert de la position.
    virtual void playEffectAt(int effect, uint8_t *voc, size_t vocSize, const Vector3D &position, float distance);
    // Même chose, relancé ou mis à jour par émetteur.
    virtual void playOrUpdateEffectAt(int effect, uint8_t *voc, size_t vocSize, const void *emitter, const Vector3D &position, float distance, bool loop = false, float gain = 1.0f);
    // L'effet tenu de cet émetteur joue encore (clé effect, ou le VOC voc avec SDL_mixer).
    virtual bool isEffectPlaying(int effect, uint8_t *voc, const void *emitter);
    // Arrête le VOC tenu voc de cet émetteur (SDL_mixer ; R4Mixer arrête par la clé de stopSoundFx).
    virtual void stopSoundVoc(uint8_t *voc, const void *emitter);
    // Vrai si le mixer place les sons dans l'espace (R4Mixer avec OpenAL) ; sinon le volume ne dépend que de la distance.
    virtual bool isSpatial();

    virtual void stopSound();
    virtual void stopSound(int chanl);
    virtual void playSoundVoc(uint8_t *data, size_t vocSize);
    virtual void playSoundVoc(uint8_t *data, size_t vocSize, int channel, int loop = 0);
    // Même règle que playOrUpdateSoundFx pour un son numérisé : relancé ou mis à jour par émetteur.
    virtual void playOrUpdateSoundVoc(uint8_t *data, size_t vocSize, const void *emitter, float distance);
    // Son numérisé sur un canal libre, joué loops fois de plus (-1 : sans fin) ; rend le canal, -1 en cas d'échec.
    int playSoundVocOnFreeChannel(uint8_t *data, size_t vocSize, int loops);
    // Son numérisé joué une fois, volume selon la distance.
    virtual void playSoundVocAtDistance(uint8_t *data, size_t vocSize, float distance);
    virtual void setVolume(int volume, int channel = -1);
    virtual bool isSoundPlaying();
    virtual bool isSoundPlaying(int chanl);
};
