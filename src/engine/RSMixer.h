//
//  RSMixer.h
//  libRealSpace
//
//  Created by Rémi LEONARD on 02/09/2024.
//  Copyright (c) 2013 Fabien Sanglard. All rights reserved.
//

#pragma once
#include "../realspace/AssetManager.h"
#include "../realspace/RSMusic.h"
#include <SDL2/SDL_mixer_ext.h>

class RSMixer {
    int initted;
    bool isplaying;
    uint32_t current_music{UINT32_MAX};
    int channel{0};
    uint8_t *voc_data{nullptr};

public:
    RSMusic *music;
    bool shuttingDown = false;
    std::unordered_map<int, Mix_Chunk*> channelChunks;
    Mix_Music* currentMusicPtr = nullptr;
    static RSMixer &getInstance() {
        static RSMixer instance;
        return instance;
    }
    RSMixer();
    ~RSMixer();
    void init();
    void playMusic(uint32_t index, int loop=1);
    void switchBank(uint8_t bank);
    void stopMusic();
    uint32_t getMusicID() { return this->current_music; };
    void stopSound();
    void stopSound(int chanl);
    void playSoundVoc(uint8_t *data, size_t vocSize);
    void playSoundVoc(uint8_t *data, size_t vocSize, int channel, int loop=0);
    void setVolume(int volume, int channel = -1);
    bool isSoundPlaying();
    bool isSoundPlaying(int chanl);
};