# Hard-Coded Midgame Data

The midgame animations in the original codebase are defined using hard-coded data structures within the `SCAnimationPlayer::initMid1()` function. This function initializes a series of midgame sequences, each represented by a `MIDGAME_DATA` structure. Each sequence contains multiple frames, and each frame specifies the graphical elements to be displayed, their positions, and associated sound effects.

I am keeping the original hard-coded data here for reference, as it may be useful for understanding the structure and flow of the midgame animations. Below is the complete hard-coded data from the `initMid1()` function:

```cpp
#ifdef OLD_CODE
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
                    { &this->optShps, OptionShapeID::MOUTAINS_BG, 0, 24, {0,16}, {0,-16}, {0,-1} ,0 },
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
                    { &this->optShps, MOUTAINS_BG, 0, 24, {0,-16}, {0,0}, {0,0} ,0 },
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
                    { &this->optShps, MOUTAINS_BG, 0, 24, {0,-16}, {0,0}, {0,0} ,0 },
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
                    { &this->optShps, MOUTAINS_BG, 0, 24, {0,-16}, {0,0}, {0,0} ,0 },
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
                    { &this->optShps, MOUTAINS_BG, 0, 24, {0,-16}, {0,0}, {0,0} ,0 },
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
                    { &this->optShps, MOUTAINS_BG, 0, 24, {0,-16}, {0,0}, {0,0} ,0 },
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
                    { &this->optShps, MOUTAINS_BG, 0, 24, {0,-16}, {0,0}, {0,0} ,0 },
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
                    { &this->optShps, MOUTAINS_BG, 0, 24, {0,-16}, {0,0}, {0,0} ,0 },
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
                    { &this->optShps, MOUTAINS_BG, 0, 24, {0,-16}, {0,0}, {0,1} ,0 },
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
                    { &this->optShps, MOUTAINS_BG, 0, 24, {0,0}, {0,0}, {0,0} ,0 },
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
                    { &this->optShps, MOUTAINS_BG, 0, 24, {0,0}, {0,0}, {0,0} ,0 },
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
                    { &this->optShps, MOUTAINS_BG, 0, 24, {0,0}, {0,0}, {0,0} ,0 },
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
                    { &this->optShps, MOUTAINS_BG, 0, 24, {0,0}, {0,0}, {0,0} ,0 },
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
#endif


/*for (auto mid_data: this->midgames_data) {
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
                bg->pak = shot_bg.pak;
                bg->pak_entry_id = shot_bg.shape_id;
                bg->palette = shot_bg.pal_id;
                bg->image = tmp_img;
                if (tmp_img->palettes.size()>0) {
                    bg->pal = tmp_img->palettes[0];
                }
                bg->shapeid = shot_bg.sub_shape_id;
                bg->position_start = shot_bg.start;
                bg->position_end = shot_bg.end;
                bg->current_position = {shot_bg.start.x, shot_bg.start.y};
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
                sprite->pak = sprt.pak;
                sprite->pak_entry_id = sprt.shape_id;
                sprite->shapeid = sprt.shape_id;
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
                bg->pak = shot_bg.pak;
                bg->pak_entry_id = shot_bg.shape_id;
                bg->palette = shot_bg.pal_id;
                bg->image = tmp_img;
                if (tmp_img->palettes.size()>0) {
                    bg->pal = tmp_img->palettes[0];
                }
                bg->shapeid = shot_bg.sub_shape_id;
                bg->position_start = shot_bg.start;
                bg->position_end = shot_bg.end;
                bg->current_position = {shot_bg.start.x, shot_bg.start.y};
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
    }*/
```