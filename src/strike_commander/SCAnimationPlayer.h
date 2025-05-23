//
//  SCAnimationPlayer.h
//  libRealSpace
//
//  Created by Fabien Sanglard on 1/31/2014.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//

#ifndef __libRealSpace__SCAnimationPlayer__
#define __libRealSpace__SCAnimationPlayer__

typedef struct MIDGAME_SHOT_BG {
    RSImageSet *image;
    uint8_t palette;
    RSPalette *pal;
    Point2D position_start;
    Point2D position_end;
    Point2D velocity;
    int shapeid;
} MIDGAME_SHOT_BG;

typedef struct MIDGAME_SHAPE_DATA {
    PakArchive *pak;
    uint8_t shape_id;
    uint8_t sub_shape_id;
    uint8_t pal_id;
    Point2D start;
    Point2D end;
    Point2D velocity;
    uint8_t keep_first_frame;
} MIDGAME_SHAPE_DATA;

typedef struct MIDGAME_SHOT_SPRITE {
    RSImageSet *image;
    RSPalette *pal;
    uint8_t palette{0};
    Point2D position_start;
    Point2D position_end;
    Point2D velocity;
    int shapeid;
    uint8_t keep_first_frame;
} MIDGAME_SHOT_SPRITE;

typedef struct MIDGAME_SOUND {
    uint8_t *data;
    size_t size;
} MIDGAME_SOUND;

typedef struct MIDGAME_SHOT {
    std::vector<MIDGAME_SHOT_BG *>background;
    std::vector<MIDGAME_SHOT_SPRITE *>sprites;
    int nbframe{0};
    int music{255};
    MIDGAME_SOUND *sound{nullptr};
    int sound_time_code{0};
} MIDGAME_SHOT;

typedef struct MIDGAME_DATA_SHOT {
    std::vector<MIDGAME_SHAPE_DATA> background;
    std::vector<MIDGAME_SHAPE_DATA> forground;
    std::vector<MIDGAME_SHAPE_DATA> sprites;
    int nbframe;
    uint8_t music{255};
    PakEntry *sound{nullptr};
    int sound_time_code{0};
} MIDGAME_DATA_SHOT;

typedef struct MIDGAME_DATA {
    std::vector<MIDGAME_DATA_SHOT> shots;
} MIDGAME_DATA;


class SCAnimationPlayer: public IActivity {
private:
    PakArchive midgames;
    PakArchive optShps;
    PakArchive optPals;
    std::vector<PakArchive*> mid;
    std::vector<PakArchive*> midvoc;
    std::map<uint8_t, std::vector<MIDGAME_SHOT *>> midgames_shots;
    int shot_counter{0};
    int fps_counter{0};
    int fps{1};
    int fps_timer{0};
    int current_music{255};
public:
    SCAnimationPlayer();
    ~SCAnimationPlayer();
    void checkKeyboard(void);
    void init();
    void runFrame(void);
    void renderMenu();
};

#endif /* defined(__libRealSpace__SCAnimationPlayer__) */
