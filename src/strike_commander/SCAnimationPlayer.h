//
//  SCAnimationPlayer.h
//  libRealSpace
//
//  Created by Fabien Sanglard on 1/31/2014.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//

#pragma once


class SCAnimationPlayer: public IActivity {
protected:
    AssetManager &Assets = AssetManager::getInstance();
    RSMixer &Mixer = RSMixer::getInstance();
    GameEngine *Game = &GameEngine::instance();
    ConvAssetManager &ConvAssets = ConvAssetManager::getInstance();
    RSVGA &VGA = RSVGA::getInstance();
    PakArchive midgames;
    PakArchive optShps;
    PakArchive optPals;
    PakArchive convShps;
    PakArchive convPals;
    std::unordered_map<uint8_t, MIDGAME_DATA> midgames_data;
    std::vector<PakArchive*> mid;
    std::vector<PakArchive*> midvoc;
    
    int shot_counter{0};
    int fps_counter{0};
    int fps{1};
    int fps_timer{0};
    int current_music{255};
    
    bool auto_stop{true};
    void initMid1();
    VGAPalette original_palette;
public:
    SCAnimationPlayer();
    ~SCAnimationPlayer();
    virtual void init();
    virtual void runFrame(void);
    virtual void renderMenu() {};
    virtual void renderUI(void) {};
    bool pause{false};
    std::unordered_map<uint8_t, std::vector<MIDGAME_SHOT *>> midgames_shots;
};
