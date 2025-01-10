#pragma once
//
//  SCGameFlow.h
//  libRealSpace
//
//  Created by Rémi LEONARD on 19/08/2024.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//
#ifndef __libRealSpace__SCGameFow__
#define __libRealSpace__SCGameFow__

#include "precomp.h"
#include <queue>


/**
 * background is a structure used to store information about a background.
 *
 * - img: a pointer to a RSImageSet, which is a collection of images that can be used for animation.
 * - frameCounter: a variable that keeps track of the current frame of the animation.
 */
struct background {
    RSImageSet *img{nullptr};
    uint8_t frameCounter{0};
};

/**
 * SCGameFlow is a class that manages the game flow of the game.
 *
 * It contains a RSGameFlow object which parses the game flow data from a PAK file.
 * It also contains a RSOption object which parses the options from a PAK file.
 * It contains a PakArchive object which stores the shapes and palettes of the game.
 * It contains a vector of background objects which stores the backgrounds of the game.
 * It contains a pointer to a uint8_t array which stores the raw palette of the game.
 * It contains a pointer to a uint8_t array which stores the palette of the game.
 * It contains a map of uint8_t to animatedSprites objects which stores the sprites of the game.
 * It contains a uint8_t variable which stores the ID of the current mission.
 * It contains a uint8_t variable which stores the ID of the current scenario.
 * It contains a uint8_t variable which stores the ID of the next mission.
 * It contains a int variable which stores the current frame rate of the game.
 * It contains a uint8_t variable which stores the ID of the current sprite.
 * It contains a uint8_t variable which stores the ID of the current option.
 * It contains a uint8_t variable which stores an unknown value.
 * It contains a pointer to a char array which stores the name of the mission to fly.
 * It contains a pointer to a vector of EFCT objects which stores the effects of the game.
 * It contains a pointer to a vector of SCZone objects which stores the zones of the game.
 *
 * It contains a queue of SCConvPlayer objects which stores the conversations of the game.
 * It contains a queue of SCShot objects which stores the cutscenes of the game.
 * It contains a queue of SCStrike objects which stores the strikes of the game.
 *
 * It has a constructor which initializes the object.
 * It has a destructor which deletes the object.
 * It has a Init method which initializes the object.
 * It has a createMiss method which creates a new mission.
 * It has a RunFrame method which runs the game frame.
 * It has a RenderMenu method which renders the menu of the game.
 * It has a clicked method which handles the clicking of a sprite.
 * It has a runEffect method which runs the effects of the game.
 * It has a CheckKeyboard method which checks the keyboard for input.
 * It has a CheckZones method which checks the zones of the game.
 * It has a createScen method which creates a new scenario.
 * It has a getShape method which gets a shape from the PakArchive.
 */
class SCGameFlow : public IActivity {

public:
    SCGameFlow();
    ~SCGameFlow();

    void Init();
    void createMiss();
    void RunFrame(void);

private:
    RSGameFlow gameFlowParser;
    RSOption optionParser;
    PakArchive optShps;
    PakArchive optPals;
    std::vector<background *> layers;
    uint8_t *rawPalette;
    uint8_t *forPalette;
    uint8_t current_miss;
    uint8_t current_scen;
    uint8_t next_miss;
    int fps;
    uint8_t currentSpriteId{0};
    uint8_t currentOptCode;
    uint8_t requ;
    char *missionToFly;
    std::vector<EFCT *> *efect;
    std::vector<SCZone *> zones;

    std::queue<SCConvPlayer *> convs;
    std::queue<SCShot *> cutsenes;
    std::queue<SCStrike *> fly_mission;
    
    void RenderMenu();
    void clicked(std::vector<EFCT *> *script, uint8_t id);
    void runEffect();
    void CheckKeyboard(void);
    SCZone *CheckZones(void);
    void createScen();
    SCEN* loadScene(uint8_t scene_id);
    RSImageSet *getShape(uint8_t shpid);
};

#endif
