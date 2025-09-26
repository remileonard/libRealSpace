//
//  SCAnimationPlayer.cpp
//  libRealSpace
//
//  Created by Fabien Sanglard on 1/31/2014.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//

#include "precomp.h"
#include "SCAnimationPlayer.h"
#include "SCMidGamesEnum.h"
#include <imgui.h>
#include <imgui_impl_opengl2.h>
#include <imgui_impl_sdl2.h>

void SCAnimationPlayer::initMid1() {
    MIDGAME_DATA mid1Data = {
        {
            {
                {
                    { &this->optShps, OptionShapeID::SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,0}, {0,0}, {0,0} ,0},
                    { &this->midgames, MIDGAMES_IDS::INTRO_CLOUDS_1, 0, 0, {0,0}, {0,0}, {0,0} }
                },
                {
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                }, 
                {
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                90,
                0
            },
            {
                {
                    { &this->optShps, OptionShapeID::SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,0}, {0,0}, {0,0} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_CLOUDS_1, 0, 0, {0,0}, {0,0}, {0,0} ,0 }
                },
                {
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                }, 
                {
                    { this->mid[0], MID1_IDS::EAGLE_PASS, 0, 0, {0,32}, {0,0}, {0,0} ,0 }
                },
                36,
                255,
                this->midvoc[0]->GetEntry(7)
            },
            {
                {
                    { &this->optShps, OptionShapeID::SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,0}, {0,0}, {0,0} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_CLOUDS_1, 0, 0, {0,0}, {0,0}, {0,0} ,0 }
                },
                {
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                }, 
                {
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                100,
            },
            {
                {
                    { &this->optShps, OptionShapeID::SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,0}, {0,-32}, {0,-1} ,0},
                    { &this->midgames, MIDGAMES_IDS::INTRO_CLOUDS_1, 0, 0, {0,0}, {0,-32}, {0,-1} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_BACKGROUNDS, 1, 0, {0,151}, {0,119}, {0,-1} ,0 },
                    { &this->optShps, OptionShapeID::MOUTAINS_BG, 0, 0, {0,16}, {0,-16}, {0,-1} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_BACKGROUNDS, 2, 0, {0,183}, {0,119}, {0,-2} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_BACKGROUNDS, 3, 0, {0,186}, {0,90}, {0,-3} ,0 }
                },
                {
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                {
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                32
            },
            {
                {
                    { &this->optShps, SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,-32}, {0,0}, {0,0} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_CLOUDS_1, 0, 0, {0,-32}, {0,0}, {0,0} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_BACKGROUNDS, 1, 0, {0,119}, {0,0}, {0,0} ,0 },
                    { &this->optShps, MOUTAINS_BG, 0, 0, {0,-16}, {0,0}, {0,0} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_BACKGROUNDS, 2, 0, {0,119}, {0,0}, {0,0} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_BACKGROUNDS, 3, 0, {0,90}, {0,0}, {0,0} ,0 }
                },
                { 
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                {
                    { this->mid[0], MID1_IDS::MIG29_F16_FLYBY_1_ANIM, 0, 0, {0,-10}, {0,0}, {0,0} ,0 }
                },
                109,
                255,
                this->midvoc[0]->GetEntry(8),
                60,
            },
            {
                {
                    { &this->optShps, SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,0}, {0,0}, {0,0} ,0 },
                },
                { 
                    { this->mid[0], MID1_IDS::F16_PICS, 1, 0, {0,0}, {0,0}, {0,0} ,0 },
                },
                {
                    { this->mid[0], MID1_IDS::MIG29_CHASE_ANIM, 0, 0, {0,-10}, {0,0}, {0,0} ,0 }
                },
                39,
                255,
                this->midvoc[0]->GetEntry(9)
            },
            {
                {
                    { &this->optShps, SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,0}, {0,0}, {0,0} ,0 },
                },
                { 
                    { this->mid[0], MID1_IDS::F16_PICS, 1, 0, {0,0}, {0,0}, {0,0} ,0 },
                },
                {
                    { this->mid[0], MID1_IDS::MIG29_CHASE_ANIM, 0, 0, {0,-10}, {0,0}, {0,0}  ,0}
                },
                39,
                255,
                this->midvoc[0]->GetEntry(9)
            },
            {
                {
                    { &this->optShps, SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,0}, {0,0}, {0,0} ,0 },
                    { this->mid[0], MID1_IDS::F16_PICS, 3, 0, {0,-15}, {0,0}, {0,0} ,0 },
                    { this->mid[0], MID1_IDS::AIR_HEAD, 1, 0, {0,0}, {0,0}, {0,0} ,0 },
                    { this->mid[0], MID1_IDS::AIR_HEAD, 4, 0, {0,0}, {0,0}, {0,0} ,0 },
                },
                { 
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                {
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                48
            },
            {
                {
                    { &this->optShps, SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,-32}, {0,0}, {0,0} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_CLOUDS_1, 0, 0, {0,-32}, {0,0}, {0,0} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_BACKGROUNDS, 1, 0, {0,119}, {0,0}, {0,0} ,0 },
                    { &this->optShps, MOUTAINS_BG, 0, 0, {0,-16}, {0,0}, {0,0} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_BACKGROUNDS, 2, 0, {0,119}, {0,0}, {0,0} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_BACKGROUNDS, 3, 0, {0,90}, {0,0}, {0,0} ,0 }
                },
                { 
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                {
                    { this->mid[0], MID1_IDS::MIG29_F16_FLYBY_ANIM_2, 0, 0, {0,-10}, {0,0}, {0,0} ,0 }
                },
                50,
                255,
                this->midvoc[0]->GetEntry(8)
            },
            {
                {
                    { &this->optShps, SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,0}, {0,0}, {0,0} ,0 }
                },
                { 
                    { this->mid[0], MID1_IDS::F16_PICS, 1, 0, {0,0}, {0,0}, {0,0} ,0 }
                },
                {
                    { this->mid[0], MID1_IDS::F16_MIG29_CHASE_ANIM, 0, 0, {0,-10}, {0,0}, {0,0} ,0 }
                },
                15,
            },
            {
                {
                    { &this->optShps, SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,0}, {0,0}, {0,0} ,0 },
                },
                { 
                    { this->mid[0], MID1_IDS::F16_PICS, 1, 0, {0,0}, {0,0}, {0,0} ,0 }
                },
                {
                    { this->mid[0], MID1_IDS::F16_MIG29_CHASE_ANIM, 0, 0, {0,-10}, {0,0}, {0,0} ,0 }
                },
                39,
                255,
                this->midvoc[0]->GetEntry(9)
            },
            {
                {
                    { &this->optShps, SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,0}, {0,0}, {0,0} ,0 },
                    { this->mid[0], MID1_IDS::F16_PICS, 4, 0, {0,-25}, {0,0}, {0,0} ,0 },
                    { this->mid[0], MID1_IDS::BILLY_HEAD, 1, 0, {0,0}, {0,0}, {0,0} ,0 },
                },
                { 
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                {
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                32,
                255,
                this->midvoc[0]->GetEntry(0)
            },
            {
                {
                    { &this->optShps, SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,0}, {0,0}, {0,0} ,0 },
                    { this->mid[0], MID1_IDS::F16_PICS, 2, 0, {0,-25}, {0,0}, {0,0} ,0 },
                },
                { 
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                {
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                25,
                255,
                this->midvoc[0]->GetEntry(4)
            },
            {
                {
                    { &this->optShps, SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,0}, {0,0}, {0,0} ,0 }
                },
                { 
                    { this->mid[0], MID1_IDS::F16_PICS, 1, 0, {0,0}, {0,0}, {0,0} ,0 }
                },
                {
                    { this->mid[0], MID1_IDS::MIG29_CHASE_ANIM, 0, 0, {0,-10}, {0,0}, {0,0} ,0 }
                },
                39,
                255,
                this->midvoc[0]->GetEntry(10)
            },
            {
                {
                    { &this->optShps, SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,0}, {0,0}, {0,0} ,0 },
                },
                { 
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                {
                    { this->mid[0], MID1_IDS::F16_FORMATION_ANIM, 0, 0, {0,-10}, {0,0}, {0,0} ,0 }
                },
                19,
            },
            {
                {
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                { 
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                {
                    { this->mid[0], MID1_IDS::PILOT_STICK_SHOOTING, 0, 0, {0,36}, {0,0}, {0,0} ,1 }
                },
                5
            },
            {
                {
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0},
                    { this->mid[0], MID1_IDS::PILOT_STICK_SHOOTING, 1, 0, {0,36}, {0,0}, {0,0} ,0 },
                    { this->mid[0], MID1_IDS::PILOT_STICK_SHOOTING, 5, 0, {0,36}, {0,0}, {0,0} ,0 }
                },
                { 
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                {
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                10
            },
            {
                {
                    { &this->optShps, SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,0}, {0,0}, {0,0} ,0 },
                },
                { 
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                {
                    { this->mid[0], MID1_IDS::F16_FORMATION_SHOOT_ANIM, 0, 0, {0,-10}, {0,0}, {0,0} ,0 }
                },
                19,
                255,
                this->midvoc[0]->GetEntry(11)
            },
            {
                {
                    { &this->optShps, SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,0}, {0,0}, {0,0} ,0 },
                },
                { 
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                {
                    { this->mid[0], MID1_IDS::MISSILE_CHASE_ANIM, 0, 0, {0,32}, {0,0}, {0,0} , 0 }
                },
                23
            },
            {
                {
                    { &this->optShps, SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,0}, {0,0}, {0,0} ,0 },
                },
                { 
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                {
                    { this->mid[0], MID1_IDS::MIG29_HIT_ANIM, 0, 0, {0,-10}, {0,0}, {0,0} ,0 }
                },
                13,
                255,
                this->midvoc[0]->GetEntry(12)
            },
            {
                {
                    { &this->optShps, SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,0}, {0,0}, {0,0} ,0 },
                },
                { 
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                {
                    { this->mid[0], MID1_IDS::MIG29_DESTROYED_ANIM, 0, 0, {0,-10}, {0,0}, {0,0} ,0 }
                },
                12
            },
            {
                {
                    { &this->optShps, SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,0}, {0,0}, {0,0} ,0 },
                },
                { 
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                {
                    { this->mid[0], MID1_IDS::MIG29_GOING_DOWN_ANIM, 0, 0, {0,-10}, {0,0}, {0,0} ,0 }
                },
                13
            },
            {
                {
                    { &this->optShps, SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,-32}, {0,0}, {0,0} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_CLOUDS_1, 0, 0, {0,-32}, {0,0}, {0,0} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_BACKGROUNDS, 1, 0, {0,119}, {0,0}, {0,0} ,0 },
                    { &this->optShps, MOUTAINS_BG, 0, 0, {0,-16}, {0,0}, {0,0} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_BACKGROUNDS, 2, 0, {0,119}, {0,0}, {0,0} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_BACKGROUNDS, 3, 0, {0,90}, {0,0}, {0,0} ,0 }
                },
                { 
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                {
                    { this->mid[0], MID1_IDS::SMOKE_TRAIL_WITH_CHUTE_1, 0, 0, {0,-10}, {0,0}, {0,0} ,0 },
                    { this->mid[0], MID1_IDS::MIG29_DOWN_FLYBY_ANIM, 0, 0, {0,-10}, {0,0}, {0,0} ,0 }
                },
                49
            },
            {
                {
                    { &this->optShps, SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,-32}, {0,0}, {0,0} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_CLOUDS_1, 0, 0, {0,-32}, {0,0}, {0,0} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_BACKGROUNDS, 1, 0, {0,119}, {0,0}, {0,0} ,0 },
                    { &this->optShps, MOUTAINS_BG, 0, 0, {0,-16}, {0,0}, {0,0} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_BACKGROUNDS, 2, 0, {0,119}, {0,0}, {0,0} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_BACKGROUNDS, 3, 0, {0,90}, {0,0}, {0,0} ,0 }
                },
                { 
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                {
                    { this->mid[0], MID1_IDS::SMOKE_TRAIL_WITH_CHUTE_2, 0, 0, {0,-10}, {0,0}, {0,0} ,0 }
                },
                14
            },
            {
                {
                    { &this->optShps, SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,-32}, {0,0}, {0,0} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_CLOUDS_1, 0, 0, {0,-32}, {0,0}, {0,0} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_BACKGROUNDS, 1, 0, {0,119}, {0,0}, {0,0} ,0 },
                    { &this->optShps, MOUTAINS_BG, 0, 0, {0,-16}, {0,0}, {0,0} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_BACKGROUNDS, 2, 0, {0,119}, {0,0}, {0,0} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_BACKGROUNDS, 3, 0, {0,90}, {0,0}, {0,0} ,0 }
                },
                { 
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                {
                    { this->mid[0], MID1_IDS::SMOKE_TRAIL_WITH_CHUTE_3, 0, 0, {0,-10}, {0,0}, {0,0} ,0 }
                },
                15
            },
            {
                {
                    { &this->optShps, SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,0}, {0,0}, {0,0} ,0 },
                    { this->mid[0], MID1_IDS::F16_PICS, 4, 0, {0,-25}, {0,0}, {0,0} ,0 },
                    
                },
                { 
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                {
                    { this->mid[0], MID1_IDS::BILLY_HEAD, 0, 0, {0,0}, {0,0}, {0,0} ,1 },
                },
                13,
            },
            {
                {
                    { &this->optShps, SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,0}, {0,0}, {0,0} ,0 },
                    { this->mid[0], MID1_IDS::F16_PICS, 4, 0, {0,-25}, {0,0}, {0,0} ,0 },
                    { this->mid[0], MID1_IDS::BILLY_HEAD, 1, 0, {0,0}, {0,0}, {0,0} , 0 },
                    { this->mid[0], MID1_IDS::BILLY_HEAD, 14, 0, {0,0}, {0,0}, {0,0} , 0 },
                    
                },
                { 
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                {
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                25,
                255,
                this->midvoc[0]->GetEntry(1)
            },
            {
                {
                    { &this->optShps, SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,0}, {0,0}, {0,0} ,0 },
                    { this->mid[0], MID1_IDS::F16_PICS, 3, 0, {0,-15}, {0,0}, {0,0} ,0 },
                    { this->mid[0], MID1_IDS::AIR_HEAD, 1, 0, {0,0}, {0,0}, {0,0} ,0 },
                    { this->mid[0], MID1_IDS::AIR_HEAD, 4, 0, {0,0}, {0,0}, {0,0} ,0 },
                },
                { 
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                {
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                35
            },
            {
                {
                    { &this->optShps, SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,0}, {0,0}, {0,0} ,0 },
                    { this->mid[0], MID1_IDS::F16_PICS, 2, 0, {0,-25}, {0,0}, {0,0} ,0 },
                },
                { 
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                {
                    {&this->midgames, 0,0,0,{0,0},{0,0},{0,0} ,1}
                },
                9
            },
            {
                {
                    { &this->optShps, SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,-32}, {0,0}, {0,0} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_CLOUDS_1, 0, 0, {0,-32}, {0,0}, {0,0} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_BACKGROUNDS, 1, 0, {0,119}, {0,0}, {0,0} ,0 },
                    { &this->optShps, MOUTAINS_BG, 0, 0, {0,-16}, {0,0}, {0,0} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_BACKGROUNDS, 2, 0, {0,119}, {0,0}, {0,0} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_BACKGROUNDS, 3, 0, {0,90}, {0,0}, {0,0} ,0 }
                },
                { 
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                {
                    { this->mid[0], MID1_IDS::PILOT_GROUND_FACE, 0, 0, {0,32}, {0,0}, {0,0} ,1 }
                },
                19
            },
            {
                {
                    { &this->optShps, SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,0}, {0,0}, {0,0} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_CLOUDS_1, 0, 0, {0,-32}, {0,0}, {0,0} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_BACKGROUNDS, 2, 0, {0,110}, {0,0}, {0,0} ,0 },
                    { this->mid[0], MID1_IDS::PILOT_GROUND_SHUT, 1, 0, {0,22}, {0,0}, {0,0} ,0 },
                    { this->mid[0], MID1_IDS::PILOT_GROUND_SHUT, 2, 0, {0,22}, {0,0}, {0,0} ,0 }
                },
                { 
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                {
                    { this->mid[0], MID1_IDS::F16_FLYBY_2_ANIM, 0, 0, {0,-10}, {0,0}, {0,0} ,1 }
                },
                56
            },
            {
                {
                    { &this->optShps, SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,0}, {0,0}, {0,0} ,0 },
                    { this->mid[0], MID1_IDS::F16_PICS, 4, 0, {0,-25}, {0,0}, {0,0} ,0 },
                    { this->mid[0], MID1_IDS::BILLY_HEAD, 1, 0, {0,0}, {0,0}, {0,0} , 0 },
                    { this->mid[0], MID1_IDS::BILLY_HEAD, 14, 0, {0,0}, {0,0}, {0,0} , 0 },
                    
                },
                { 
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                {
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                35,
                255,
                this->midvoc[0]->GetEntry(2)
            },
            {
                {
                    { &this->optShps, SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,0}, {0,0}, {0,0} ,0 },
                    { this->mid[0], MID1_IDS::F16_PICS, 3, 0, {0,-15}, {0,0}, {0,0} ,0 },
                    { this->mid[0], MID1_IDS::AIR_HEAD, 1, 0, {0,0}, {0,0}, {0,0} ,0 },
                },
                { 
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                {
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                25,
                255,
                this->midvoc[0]->GetEntry(5)
            },
            {
                {
                    { &this->optShps, SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,0}, {0,0}, {0,0} ,0 },
                    { this->mid[0], MID1_IDS::F16_PICS, 4, 0, {0,-25}, {0,0}, {0,0} ,0 },
                    { this->mid[0], MID1_IDS::BILLY_HEAD, 1, 0, {0,0}, {0,0}, {0,0} , 0 },
                    { this->mid[0], MID1_IDS::BILLY_HEAD, 14, 0, {0,0}, {0,0}, {0,0} , 0 },
                    
                },
                { 
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                {
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                30,
                255,
                this->midvoc[0]->GetEntry(3)
            },
            {
                {
                    { &this->optShps, SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,0}, {0,0}, {0,0} ,0 },
                    { this->mid[0], MID1_IDS::F16_PICS, 3, 0, {0,-15}, {0,0}, {0,0} ,0 },
                    { this->mid[0], MID1_IDS::AIR_HEAD, 1, 0, {0,0}, {0,0}, {0,0} ,0 },
                },
                { 
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                {
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                65,
                255,
                this->midvoc[0]->GetEntry(6)
            },
            {
                {
                    { &this->optShps, SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,-32}, {0,0}, {0,0} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_CLOUDS_1, 0, 0, {0,-32}, {0,0}, {0,0} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_BACKGROUNDS, 1, 0, {0,119}, {0,0}, {0,0} ,0 },
                    { &this->optShps, MOUTAINS_BG, 0, 0, {0,-16}, {0,0}, {0,0} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_BACKGROUNDS, 2, 0, {0,119}, {0,0}, {0,0} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_BACKGROUNDS, 3, 0, {0,90}, {0,0}, {0,0} ,0 }
                },
                { 
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                {
                    { this->mid[0], MID1_IDS::PILOT_LOOKING_AROUND, 0, 0, {0,36}, {0,0}, {0,0} ,0 }
                },
                6
            },
            {
                {
                    { &this->optShps, SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,-29}, {0,0}, {0,1} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_CLOUDS_1, 0, 0, {0,-19}, {0,10}, {0,1} ,0 },
                    { &this->optShps, MOUTAINS_BG, 0, 0, {0,-16}, {0,0}, {0,1} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_BACKGROUNDS, 2, 0, {0,119}, {0,160}, {0,3} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_BACKGROUNDS, 3, 0, {0,90}, {0,160}, {0,3} ,0 },
                    { this->mid[0], MID1_IDS::PILOT_LOOKING_AROUND, 7, 0, {0,36}, {0,80}, {0,3} ,0 }
                },
                { 
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                {
                    { this->mid[0], MID1_IDS::F16_FLYING_AWAY_ANIM_1, 0, 0, {0,32}, {0,0}, {0,0} ,0 }
                },
                29
            },
            {
                {
                    { &this->optShps, SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,0}, {0,0}, {0,0} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_CLOUDS_1, 0, 0, {0,10}, {0,10}, {0,0} ,0 },
                    { &this->optShps, MOUTAINS_BG, 0, 0, {0,0}, {0,0}, {0,0} ,0 },
                },
                { 
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                {
                    { this->mid[0], MID1_IDS::F16_FLYING_AWAY_ANIM_2, 0, 0, {0,32}, {0,0}, {0,0} ,0 }
                },
                200
            },
            {
                {
                    { &this->optShps, SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,0}, {0,0}, {0,0} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_CLOUDS_1, 0, 0, {0,10}, {0,10}, {0,0} ,0 },
                    { &this->optShps, MOUTAINS_BG, 0, 0, {0,0}, {0,0}, {0,0} ,0 },
                },
                { 
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                {
                    { this->mid[0], MID1_IDS::TITLE_SCREEN_ANIM_1, 0, 0, {0,16}, {0,0}, {0,0} ,0 }
                },
                19,
                23,
                this->midvoc[0]->GetEntry(12)
            },
            {
                {
                    { &this->optShps, SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,0}, {0,0}, {0,0} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_CLOUDS_1, 0, 0, {0,10}, {0,10}, {0,0} ,0 },
                    { &this->optShps, MOUTAINS_BG, 0, 0, {0,0}, {0,0}, {0,0} ,0 },
                },
                { 
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                {
                    { this->mid[0], MID1_IDS::TITLE_SCREEN_ANIM_2, 0, 0, {0,16}, {0,0}, {0,0} ,0 }
                },
                10
            },
            {
                {
                    { &this->optShps, SKY, 0, OPTPALS_PAK_SKY_PALETTE_PATCH_ID, {0,0}, {0,0}, {0,0} ,0 },
                    { &this->midgames, MIDGAMES_IDS::INTRO_CLOUDS_1, 0, 0, {0,0}, {0,10}, {0,0} ,0 },
                    { &this->optShps, MOUTAINS_BG, 0, 0, {0,0}, {0,0}, {0,0} ,0 },
                    { this->mid[0], MID1_IDS::TITLE_SCREEN_ANIM_2, 11, 0, {0,16}, {0,0}, {0,0} ,0 }
                },
                { 
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                {
                    {nullptr, 0,0,0,{0,0},{0,0},{0,0} ,0}
                },
                64
            },
        }
    };
    this->midgames_data[1] = mid1Data;
}

SCAnimationPlayer::SCAnimationPlayer() { this->fps_timer = SDL_GetTicks() / 10; }

SCAnimationPlayer::~SCAnimationPlayer(){
}

void SCAnimationPlayer::init(){
    TreEntry *midgames_entry = Assets.GetEntryByName(
        "..\\..\\DATA\\MIDGAMES\\MIDGAMES.PAK"
    );
    PakArchive *midgames_arch = new PakArchive();
    this->midgames.InitFromRAM("MIDGAMES.PAK", midgames_entry->data, midgames_entry->size);
    std::vector<std::string> midgames_files = {
        "MID1",
        "MID2",
        "MID3",
        "MID5",
        "MID12",
        "MID14",
        "MID15",
        "MID16",
        "MID17",
        "MID20",
        "MID36"
    };
    if (this->mid.size() == 0) {
        for (int i = 0; i < midgames_files.size(); i++) {
            std::string file_path = "..\\..\\DATA\\MIDGAMES\\" + midgames_files[i] + ".PAK";
            TreEntry *entry = Assets.GetEntryByName(file_path.c_str());
            if (entry == nullptr) {
                continue;
            }
            PakArchive *arch = new PakArchive();
            arch->InitFromRAM(midgames_files[i].c_str(), entry->data, entry->size);
            this->mid.push_back(arch);
        }
    }
    if (this->midvoc.size() == 0) {

        for (int i = 0; i < midgames_files.size(); i++) {
            std::string file_path = "..\\..\\DATA\\MIDGAMES\\" + midgames_files[i] + "VOC.PAK";
            TreEntry *entry = Assets.GetEntryByName(file_path.c_str());
            if (entry == nullptr) {
                continue;
            }
            PakArchive *arch = new PakArchive();
            arch->InitFromRAM(midgames_files[i].c_str(), entry->data, entry->size);
            this->midvoc.push_back(arch);
        }
    }
    TreEntry *optShapEntry = Assets.GetEntryByName(
        Assets.optshps_filename
    );
    this->optShps.InitFromRAM("OPTSHPS.PAK", optShapEntry->data, optShapEntry->size);
    TreEntry *optPalettesEntry = Assets.GetEntryByName(
        Assets.optpals_filename
    );
    this->optPals.InitFromRAM("OPTPALS.PAK", optPalettesEntry->data, optPalettesEntry->size);

    this->initMid1();
    
    for (auto mid_data: this->midgames_data) {
        for (auto sht: mid_data.second.shots) {
            MIDGAME_SHOT *shot = new MIDGAME_SHOT();
        
            for (auto shot_bg: sht.background) {
                if (shot_bg.pak == nullptr) {
                    continue;
                }
                MIDGAME_SHOT_BG *bg = new MIDGAME_SHOT_BG();
                RSImageSet *tmp_img = new RSImageSet();
                PakArchive *pk = new PakArchive();
                PakEntry *pe = shot_bg.pak->GetEntry(shot_bg.shape_id);
                pk->InitFromRAM("toto", pe->data, pe->size);
                tmp_img->InitFromPakArchive(pk,0);
                if (tmp_img->GetNumImages() == 0) {
                    delete tmp_img;
                    tmp_img = new RSImageSet();
                    tmp_img->InitFromPakEntry(pe);
                }
                bg->palette = shot_bg.pal_id;
                bg->image = tmp_img;
                if (tmp_img->palettes.size()>0) {
                    bg->pal = tmp_img->palettes[0];
                }
                bg->shapeid = shot_bg.sub_shape_id;
                bg->position_start = shot_bg.start;
                bg->position_end = shot_bg.end;
                bg->velocity = shot_bg.velocity;
                shot->background.push_back(bg);
            }
            for (auto sprt: sht.sprites) {
                if (sprt.pak == nullptr) {
                    continue;
                }
                MIDGAME_SHOT_SPRITE *sprite = new MIDGAME_SHOT_SPRITE();
                RSImageSet *tmp_img = new RSImageSet();
                tmp_img->InitFromPakEntry(sprt.pak->GetEntry(sprt.shape_id));
                sprite->image = tmp_img;
                if (tmp_img->palettes.size()>0) {
                    sprite->pal = tmp_img->palettes[0];
                }
                sprite->position_start = sprt.start;
                sprite->position_end = sprt.end;
                sprite->velocity = sprt.velocity;
                sprite->keep_first_frame = sprt.keep_first_frame;
                shot->sprites.push_back(sprite);
            }
            for (auto shot_bg: sht.forground) {
                if (shot_bg.pak == nullptr) {
                    continue;
                }
                MIDGAME_SHOT_BG *bg = new MIDGAME_SHOT_BG();
                RSImageSet *tmp_img = new RSImageSet();
                PakArchive *pk = new PakArchive();
                PakEntry *pe = shot_bg.pak->GetEntry(shot_bg.shape_id);
                pk->InitFromRAM("toto", pe->data, pe->size);
                tmp_img->InitFromPakArchive(pk,0);
                if (tmp_img->GetNumImages() == 0) {
                    delete tmp_img;
                    tmp_img = new RSImageSet();
                    tmp_img->InitFromPakEntry(pe);
                }
                bg->palette = shot_bg.pal_id;
                bg->image = tmp_img;
                if (tmp_img->palettes.size()>0) {
                    bg->pal = tmp_img->palettes[0];
                }
                bg->shapeid = shot_bg.sub_shape_id;
                bg->position_start = shot_bg.start;
                bg->position_end = shot_bg.end;
                bg->velocity = shot_bg.velocity;
                shot->foreground.push_back(bg);
            }
            shot->nbframe = sht.nbframe;
            shot->music = sht.music;
            if (sht.sound != nullptr) {
                MIDGAME_SOUND *sound = new MIDGAME_SOUND();
                sound->data = sht.sound->data;
                sound->size = sht.sound->size;
                shot->sound = sound;
                shot->sound_time_code = sht.sound_time_code;
            }
            this->midgames_shots[1].push_back(shot);
        }
    }

    this->original_palette.CopyFromOtherPalette(VGA.getPalette(), false);
    this->palette.CopyFromOtherPalette(VGA.getPalette(), false);
    this->shot_counter = 0;
    this->fps_counter = 0;
    this->fps = 1;
}

void SCAnimationPlayer::runFrame(void){

    
    checkKeyboard();
    VGA.activate();
    VGA.getFrameBuffer()->clear();
    this->palette.CopyFromOtherPalette(&this->original_palette, false);
    VGA.setPalette(&this->palette);
    FrameBuffer *fb = VGA.getFrameBuffer();
    fb->fillWithColor(0);
    int fpsupdate = 0;
    fpsupdate = (SDL_GetTicks() / 10) - fps_timer > 6;
    if (fpsupdate) {
        fps_timer = (SDL_GetTicks() / 10);
    }
    

    MIDGAME_SHOT *shot = this->midgames_shots[1][shot_counter];
    if (this->current_music != shot->music && shot->music != 255) {
        Mixer.switchBank(0);
        Mixer.playMusic(shot->music);
        this->current_music = shot->music;
    }
    if (shot->sound != nullptr && (fps == shot->sound_time_code || shot->sound_time_code == 0)) {
        Mixer.playSoundVoc(shot->sound->data, shot->sound->size);
        shot->sound = nullptr;
    }
    for (auto bg : shot->background) {
        if (bg->image->GetNumImages()>0) {
            RLEShape *shp = bg->image->GetShape(bg->shapeid);
            FrameBuffer *texture = new FrameBuffer(320, 200);
            texture->fillWithColor(255);
            texture->drawShape(shp);
            fb->blitWithMask(texture->framebuffer, bg->position_start.x, bg->position_start.y, 320, 200,255);
            if (fpsupdate && (bg->velocity.x != 0 || bg->velocity.y != 0)) {
                if (bg->position_start.x != bg->position_end.x) {
                    bg->position_start.x += bg->velocity.x;
                }  
                if (bg->velocity.y<0 && bg->position_start.y > bg->position_end.y) {
                    bg->position_start.y += bg->velocity.y;
                } else if (bg->velocity.y>0 && bg->position_start.y < bg->position_end.y) {
                    bg->position_start.y += bg->velocity.y;
                }
            }
            if (bg->palette != 0) {
                ByteStream paletteReader;
                paletteReader.Set(this->optPals.GetEntry(bg->palette)->data);
                this->palette.ReadPatch(&paletteReader);
                VGA.setPalette(&this->palette);
            }
            if (bg->pal != nullptr) {
                this->palette.CopyFromOtherPalette(bg->pal->GetColorPalette());
                VGA.setPalette(&this->palette);
            }
            delete texture;
        }
    }
    for (auto sprt: shot->sprites) {
        FrameBuffer *texture = new FrameBuffer(320, 200);
        texture->fillWithColor(255);
        if (sprt->keep_first_frame) {
            texture->drawShape(sprt->image->GetShape(1));
        }
        if (fps<sprt->image->GetNumImages()) {
            texture->drawShape(sprt->image->GetShape(fps));
        }
        int color = texture->framebuffer[0];
        fb->blitWithMask(texture->framebuffer, sprt->position_start.x, sprt->position_start.y, 320, 200,color);
        if (sprt->palette != 0) {
            ByteStream paletteReader;
            paletteReader.Set(this->optPals.GetEntry(sprt->palette)->data);
            this->palette.ReadPatch(&paletteReader);
            VGA.setPalette(&this->palette);
        }
        if (sprt->pal != nullptr) {
            this->palette.CopyFromOtherPalette(sprt->pal->GetColorPalette());
            VGA.setPalette(&this->palette);
        }
        delete texture;
    }
    for (auto bg : shot->foreground) {
        if (bg->image->GetNumImages()>0) {
            RLEShape *shp = bg->image->GetShape(bg->shapeid);
            FrameBuffer *texture = new FrameBuffer(320, 200);
            texture->fillWithColor(255);
            texture->drawShape(shp);
            fb->blitWithMask(texture->framebuffer, bg->position_start.x, bg->position_start.y, 320, 200,255);
            if (fpsupdate && (bg->velocity.x != 0 || bg->velocity.y != 0)) {
                if (bg->position_start.x != bg->position_end.x) {
                    bg->position_start.x += bg->velocity.x;
                }  
                if (bg->velocity.y<0 && bg->position_start.y > bg->position_end.y) {
                    bg->position_start.y += bg->velocity.y;
                } else if (bg->velocity.y>0 && bg->position_start.y < bg->position_end.y) {
                    bg->position_start.y += bg->velocity.y;
                }
            }
            if (bg->palette != 0) {
                ByteStream paletteReader;
                paletteReader.Set(this->optPals.GetEntry(bg->palette)->data);
                this->palette.ReadPatch(&paletteReader);
                VGA.setPalette(&this->palette);
            }
            if (bg->pal != nullptr) {
                this->palette.CopyFromOtherPalette(bg->pal->GetColorPalette());
                VGA.setPalette(&this->palette);
            }
            delete texture;
        }
    }
    if (!pause) {
        fps_counter++;
        fps+=fpsupdate;
    }
    if (fps > shot->nbframe) {
        fps = 1;
        shot_counter++;
        if (shot_counter>this->midgames_shots[1].size()-1) {
            shot_counter = 0;
            Game->stopTopActivity();
        }
    }
    for (size_t i = 0; i < CONV_TOP_BAR_HEIGHT; i++)
            VGA.getFrameBuffer()->fillLineColor(i, 0x00);

    for (size_t i = 0; i < CONV_BOTTOM_BAR_HEIGHT; i++)
        VGA.getFrameBuffer()->fillLineColor(199 - i, 0x00);
    VGA.vSync();
}
void SCAnimationPlayer::renderMenu() {
    ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
    ImGui::Text("Animation Player frame: %d, fps: %d, shot: %d", this->fps_counter, this->fps, this->shot_counter);
}
void SCAnimationPlayer::renderUI() {
    if (ImGui::BeginTabBar("Animations")) {
        if (ImGui::BeginTabItem("MidGames Data")) {
            if (ImGui::Button("Previous Shot")) {
                this->shot_counter--;
                if (this->shot_counter < 0) {
                    this->shot_counter = this->midgames_shots[1].size() - 1;
                }
                this->fps = 1;
                this->fps_counter = 0;
            }
            ImGui::SameLine();
            if (ImGui::Button("Previous Frame")) {
                this->fps--;
                if (this->fps < 1) {
                    this->fps = 1;
                }
                this->fps_counter--;
                if (this->fps_counter < 0) {
                    this->fps_counter = 0;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button(pause ? "Resume" : "Pause")) {
                this->pause = !this->pause;
            }
            ImGui::SameLine();
            if (ImGui::Button("Next Frame")) {
                this->fps++;
                if (this->fps > this->midgames_shots[1][this->shot_counter]->nbframe) {
                    this->fps = this->midgames_shots[1][this->shot_counter]->nbframe;
                }
                this->fps_counter++;
                if (this->fps_counter > this->midgames_shots[1][this->shot_counter]->nbframe) {
                    this->fps_counter = this->midgames_shots[1][this->shot_counter]->nbframe;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Next Shot")) {
                this->shot_counter++;
                if (this->shot_counter > this->midgames_shots[1].size() - 1) {
                    this->shot_counter = 0;
                }
                this->fps = 1;
                this->fps_counter = 0;
            }
            ImGui::Separator();
            ImGui::Text("Total shots: %d", this->midgames_shots[1].size());
            if (this->midgames_shots[1].size() > 0 && this->shot_counter < this->midgames_shots[1].size()) {
                MIDGAME_SHOT *shot = this->midgames_shots[1][this->shot_counter];
                ImGui::Text("Shot %d/%d", this->shot_counter + 1, this->midgames_shots[1].size());
                ImGui::Text("Number of frames: %d", shot->nbframe);
                ImGui::Text("Music ID: %d", shot->music);
                if (shot->sound != nullptr) {
                    ImGui::Text("Sound available to play at frame %d", shot->sound_time_code);
                } else {
                    ImGui::Text("No sound available");
                }
                ImGui::Text("Backgrounds: %d", shot->background.size());
                for (size_t i = 0; i < shot->background.size(); i++) {
                    MIDGAME_SHOT_BG *bg = shot->background[i];
                    if (ImGui::TreeNode(("Background " + std::to_string(i+1)).c_str())) {
                        ImGui::Text("Background %d:", i+1);
                        ImGui::Text("ShapeID: %d",bg->shapeid);
                        ImGui::Text("Pos Start: (%d,%d)",bg->position_start.x, bg->position_start.y);
                        ImGui::Text("Pos End: (%d,%d)", bg->position_end.x, bg->position_end.y);
                        ImGui::Text("Velocity: (%d,%d)",bg->velocity.x, bg->velocity.y);
                        ImGui::Text("PaletteID: %d, HasPal: %s", bg->palette, bg->pal != nullptr ? "Yes" : "No");
                        if (bg->pal != nullptr) {
                            ImGui::Text("Palette Colors:");
                            for (int c = 0; c < 256; c++) {
                                if (c % 12 == 0 && c != 0) {
                                    ImGui::NewLine();
                                } else if (c != 0) {
                                    ImGui::SameLine();
                                }
                                ImGui::ColorButton(("##bg" + std::to_string(i) + "color" + std::to_string(c)).c_str(), ImVec4(
                                    bg->pal->GetColorPalette()->colors[c].r / 255.0f,
                                    bg->pal->GetColorPalette()->colors[c].g / 255.0f,
                                    bg->pal->GetColorPalette()->colors[c].b / 255.0f,
                                    1.0f
                                ), ImGuiColorEditFlags_NoTooltip, ImVec2(20,20));
                                if (ImGui::IsItemHovered()) {
                                    ImGui::SetTooltip("Color %d: R:%d G:%d B:%d", c, bg->pal->GetColorPalette()->colors[c].r, bg->pal->GetColorPalette()->colors[c].g, bg->pal->GetColorPalette()->colors[c].b);
                                }
                            }
                        }
                        ImGui::TreePop();
                    }   
                    
                }
                ImGui::Text("Sprites: %d", shot->sprites.size());
                for (size_t i = 0; i < shot->sprites.size(); i++) {
                    MIDGAME_SHOT_SPRITE *sprt = shot->sprites[i];
                    if (ImGui::TreeNode(("Sprite " + std::to_string(i+1)).c_str())) {
                        ImGui::Text("Sprite %d:", i+1);
                        ImGui::Text("NumImages: %d", sprt->image->GetNumImages());
                        ImGui::Text("Pos Start: (%d,%d)", sprt->position_start.x, sprt->position_start.y);
                        ImGui::Text("Pos End: (%d,%d)", sprt->position_end.x, sprt->position_end.y);
                        ImGui::Text("Velocity: (%d,%d)", sprt->velocity.x, sprt->velocity.y);
                        ImGui::Text("Keep first frame: %s", sprt->keep_first_frame ? "Yes" : "No");
                        ImGui::Text("HasPal: %s", sprt->pal != nullptr ? "Yes" : "No");
                        if (sprt->pal != nullptr) {
                            ImGui::Text("Palette Colors:");
                            for (int c = 0; c < 256; c++) {
                                if (c % 12 == 0 && c != 0) {
                                    ImGui::NewLine();
                                } else if (c != 0) {
                                    ImGui::SameLine();
                                }
                                ImGui::ColorButton(("##sprt" + std::to_string(i) + "color" + std::to_string(c)).c_str(), ImVec4(
                                    sprt->pal->GetColorPalette()->colors[c].r / 255.0f,
                                    sprt->pal->GetColorPalette()->colors[c].g / 255.0f,
                                    sprt->pal->GetColorPalette()->colors[c].b / 255.0f,
                                    1.0f
                                ), ImGuiColorEditFlags_NoTooltip, ImVec2(20,20));
                                if (ImGui::IsItemHovered()) {
                                    ImGui::SetTooltip("Color %d: R:%d G:%d B:%d", c, sprt->pal->GetColorPalette()->colors[c].r, sprt->pal->GetColorPalette()->colors[c].g, sprt->pal->GetColorPalette()->colors[c].b);
                                }
                            }
                        }
                        ImGui::TreePop();
                    }
                }
            }
            ImGui::Separator();
            ImGui::Text("Current color palette");
            for (int i = 0; i < 256; i++) {
                if (i % 12 == 0 && i != 0) {
                    ImGui::NewLine();
                } else if (i != 0) {
                    ImGui::SameLine();
                }
                ImGui::ColorButton(("##" + std::to_string(i)).c_str(), ImVec4(
                    this->palette.colors[i].r / 255.0f,
                    this->palette.colors[i].g / 255.0f,
                    this->palette.colors[i].b / 255.0f,
                    1.0f
                ), ImGuiColorEditFlags_NoTooltip, ImVec2(20,20));
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Color %d: R:%d G:%d B:%d", i, this->palette.colors[i].r, this->palette.colors[i].g, this->palette.colors[i].b);
                }
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("VGA Pallette")) {
            if (ImGui::Button("Reset to original palette")) {
                this->palette.CopyFromOtherPalette(&this->original_palette, false);
                VGA.setPalette(&this->palette);
            }
            ImGui::Separator();
            ImGui::Text("Original color palette");
            for (int i = 0; i < 256; i++) {
                if (i % 12 == 0 && i != 0) {
                    ImGui::NewLine();
                } else if (i != 0) {
                    ImGui::SameLine();
                }
                ImGui::ColorButton(("##orig" + std::to_string(i)).c_str(), ImVec4(
                    this->original_palette.colors[i].r / 255.0f,
                    this->original_palette.colors[i].g / 255.0f,
                    this->original_palette.colors[i].b / 255.0f,
                    1.0f
                ), ImGuiColorEditFlags_NoTooltip, ImVec2(20,20));
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Color %d: R:%d G:%d B:%d", i, this->original_palette.colors[i].r, this->original_palette.colors[i].g, this->original_palette.colors[i].b);
                }
            }
            ImGui::Separator();
            ImGui::Text("Current color palette");
            for (int i = 0; i < 256; i++) {
                if (i % 12 == 0 && i != 0) {
                    ImGui::NewLine();
                } else if (i != 0) {
                    ImGui::SameLine();
                }
                ImGui::ColorButton(("##curr" + std::to_string(i)).c_str(), ImVec4(
                    this->palette.colors[i].r / 255.0f,
                    this->palette.colors[i].g / 255.0f,
                    this->palette.colors[i].b / 255.0f,
                    1.0f
                ), ImGuiColorEditFlags_NoTooltip, ImVec2(20,20));
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Color %d: R:%d G:%d B:%d", i, this->palette.colors[i].r, this->palette.colors[i].g, this->palette.colors[i].b);
                }
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}