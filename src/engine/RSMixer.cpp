#include "RSMixer.h"
#include <unordered_map>
#include <mutex>
#include <atomic>

static RSMixer* g_rsmixer_instance = nullptr;

namespace {
    std::mutex g_mixMutex;
}


static void ChannelFinishedCallback(int ch) {
    // Capture locale de l'instance pour éviter les accès à une instance détruite
    RSMixer* inst = g_rsmixer_instance;
    if (!inst || inst->shuttingDown) return;
    
    // Limiter la durée du verrouillage
    Mix_Chunk* chunkToFree = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_mixMutex);
        auto it = inst->channelChunks.find(ch);
        if (it != inst->channelChunks.end()) {
            chunkToFree = it->second;
            inst->channelChunks.erase(it);
        }
    }
    
    // Libérer la ressource hors du mutex
    if (chunkToFree) {
        Mix_FreeChunk(chunkToFree);
    }
}

RSMixer::RSMixer() {
    this->shuttingDown = false;
    g_rsmixer_instance = this;
}

void RSMixer::init() {
    this->initted = Mix_Init(MIX_INIT_MID);
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) < 0) {
        printf("Erreur d'initialisation audio: %s\n", Mix_GetError());
        return;
    }
    this->music = new RSMusic();
    this->music->init();
    this->isplaying = false;
    Mix_ChannelFinished(ChannelFinishedCallback);
}

RSMixer::~RSMixer() {
    // Signaler arrêt avec un atomic bool serait préférable
    shuttingDown = true;
    
    // Bloquer les opérations pendant la destruction
    std::lock_guard<std::mutex> lock(g_mixMutex);
    
    // Désenregistrer immédiatement le callback
    Mix_ChannelFinished(nullptr);
    
    // Stopper tous les canaux pour éviter callbacks tardifs
    Mix_HaltChannel(-1);
    Mix_HaltMusic();
    
    // Nettoyer les ressources audio
    for (auto &kv : channelChunks) {
        if (kv.second) {
            Mix_FreeChunk(kv.second);
        }
    }
    channelChunks.clear();
    
    if (currentMusicPtr) {
        Mix_FreeMusic(currentMusicPtr);
        currentMusicPtr = nullptr;
    }
    
    if (this->music) {
        delete this->music;
        this->music = nullptr;
    }
    
    // Plus personne ne doit utiliser l'instance
    if (g_rsmixer_instance == this) {
        g_rsmixer_instance = nullptr;
    }
    
    Mix_CloseAudio();
    Mix_Quit();
}


void RSMixer::playMusic(uint32_t index, int loop) {
    if (this->isplaying && this->current_music == index) return;
    if (this->isplaying) {
        this->stopMusic();
    }
    if (shuttingDown) return;
    Mix_ADLMIDI_setCustomBankFile("./assets/STRIKE.wopl");
    MemMusic *mus = this->music->GetMusic(index);
    SDL_RWops *rw = SDL_RWFromConstMem(mus->data, (int) mus->size);
    Mix_Music *music = Mix_LoadMUSType_RW(rw, MUS_ADLMIDI, 1);
    if (!music) {
        printf("Error loading music: %s\n", Mix_GetError());
        return;
    }
    if (currentMusicPtr) {
        Mix_HaltMusic();
        Mix_FreeMusic(currentMusicPtr);
    }
    currentMusicPtr = music;
    this->isplaying = true;
    if (Mix_PlayMusic(music, loop) == -1) {
        printf("Error playing music: %s\n", Mix_GetError());
    } else {
        this->current_music = index;
    }
}

void RSMixer::playSoundVoc(uint8_t *data, size_t vocSize) {
    if (shuttingDown || !data) return;
    
    // Ne pas stocker le pointeur data, faire une copie si nécessaire
    SDL_RWops* rw = SDL_RWFromConstMem(data, static_cast<int>(vocSize));
    if (!rw) {
        printf("Erreur création RWops: %s\n", SDL_GetError());
        return;
    }
    
    Mix_Chunk* chunk = Mix_LoadWAV_RW(rw, 1); // 1 = SDL_RWops sera automatiquement libéré
    if (!chunk) {
        printf("Erreur chargement VOC: %s\n", Mix_GetError());
        return;
    }
    
    int ch = Mix_PlayChannel(-1, chunk, 0);
    if (ch == -1) {
        printf("Erreur lecture son: %s\n", Mix_GetError());
        Mix_FreeChunk(chunk);
        return;
    }
    
    // Ajouter le chunk à la map avec protection mutex
    {
        std::lock_guard<std::mutex> lock(g_mixMutex);
        channelChunks[ch] = chunk;
    }
    
    channel = ch;
}

void RSMixer::playSoundVoc(uint8_t *data, size_t vocSize, int channel, int loop) {
    if (shuttingDown) return;
    // Ne pas stocker le pointeur data, faire une copie si nécessaire
    SDL_RWops* rw = SDL_RWFromConstMem(data, static_cast<int>(vocSize));
    if (!rw) {
        printf("Erreur création RWops: %s\n", SDL_GetError());
        return;
    }
    
    Mix_Chunk* chunk = Mix_LoadWAV_RW(rw, 1); // 1 = SDL_RWops sera automatiquement libéré
    if (!chunk) {
        printf("Erreur chargement VOC: %s\n", Mix_GetError());
        return;
    }
    int result = Mix_PlayChannel(channel, chunk, loop);
    if (result == -1) {
        printf("Error playing VOC sound on channel %d: %s\n", channel, Mix_GetError());
        Mix_FreeChunk(chunk);
        return;
    }
    // Ajouter le chunk à la map avec protection mutex
    {
        std::lock_guard<std::mutex> lock(g_mixMutex);
        channelChunks[channel] = chunk;
    }
}

void RSMixer::stopMusic() {
    if (shuttingDown) return;
    Mix_HaltMusic();
    this->isplaying = false;
}

void RSMixer::stopSound() {
    if (shuttingDown) return;
    Mix_HaltChannel(-1);
    std::lock_guard<std::mutex> lock(g_mixMutex);
    for (auto &kv : channelChunks) {
        if (kv.second) {
            Mix_FreeChunk(kv.second);
        }
    }
    channelChunks.clear();
}

void RSMixer::stopSound(int chanl) {
    if (shuttingDown) return;
    Mix_HaltChannel(chanl);
    std::lock_guard<std::mutex> lock(g_mixMutex);
    auto it = channelChunks.find(chanl);
    if (it != channelChunks.end()) {
        if (it->second) Mix_FreeChunk(it->second);
        channelChunks.erase(it);
    }
}

bool RSMixer::isSoundPlaying() {
    if (shuttingDown) return false;
    return Mix_Playing(channel) != 0;
}

bool RSMixer::isSoundPlaying(int chanl) {
    if (shuttingDown) return false;
    return Mix_Playing(chanl) != 0;
}
void RSMixer::setVolume(int volume, int channel) {
    if (channel == -1) {
        Mix_VolumeMusic(volume);
    } else {
        Mix_Volume(channel, volume);
    }
}
void RSMixer::switchBank(uint8_t bank) { 
    this->music->SwitchBank(bank);
}
