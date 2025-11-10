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


void SCAnimationPlayer::initMid1(){
    this->loadShot("MID_1.IFF");
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
    this->mid.clear();
    this->mid.shrink_to_fit();
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
    if (this->midvoc.size() == 0) {

        for (int i = 0; i < midgames_files.size(); i++) {
            std::string file_path = "..\\..\\DATA\\MIDGAMES\\" + midgames_files[i] + "VOC.PAK";
            TreEntry *entry = Assets.GetEntryByName(file_path.c_str());
            if (entry == nullptr) {
                continue;
            }
            PakArchive *arch = new PakArchive();
            arch->InitFromRAM(file_path.c_str(), entry->data, entry->size);
            this->midvoc.push_back(arch);
        }
        std::string file_path = "..\\..\\DATA\\MIDGAMES\\MID33VOC.PAK";
        TreEntry *entry = Assets.GetEntryByName(file_path.c_str());
        if (entry != nullptr) {
            PakArchive *arch = new PakArchive();
            arch->InitFromRAM(file_path.c_str(), entry->data, entry->size);
            this->midvoc.push_back(arch);
        }
        file_path = "..\\..\\DATA\\MIDGAMES\\MID34VOC.PAK";
        entry = Assets.GetEntryByName(file_path.c_str());
        if (entry != nullptr) {
            PakArchive *arch = new PakArchive();
            arch->InitFromRAM(file_path.c_str(), entry->data, entry->size);
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
    if (!pause) {
        fpsupdate = (SDL_GetTicks() / 10) - fps_timer > 6;
        if (fpsupdate) {
            fps_timer = (SDL_GetTicks() / 10);
        }
    }
    
    

    MIDGAME_SHOT *shot = this->midgames_shots[1][shot_counter];
    if (this->current_music != shot->music && shot->music != 255) {
        Mixer.switchBank(0);
        Mixer.playMusic(shot->music);
        this->current_music = shot->music;
    }
    if (!shot->sound_played && shot->sound != nullptr && (fps == shot->sound_time_code || shot->sound_time_code == 0)) {
        Mixer.playSoundVoc(shot->sound->data, shot->sound->size);
        shot->sound_played = true;
    }
    for (auto bg : shot->background) {
        if (bg->palette != 0) {
            ByteStream paletteReader;
            if (bg->pak_palette == nullptr) {
                paletteReader.Set(this->optPals.GetEntry(bg->palette)->data, 772);
            } else {
                if (bg->palette >= bg->pak_palette->GetNumEntries()) {
                    continue;
                }
                paletteReader.Set(bg->pak_palette->GetEntry(bg->palette)->data, 772);
            }
            this->palette.ReadPatch(&paletteReader);
        }
        if (bg->pal != nullptr && !bg->use_external_palette) {
            this->palette.CopyFromOtherPalette(bg->pal->GetColorPalette());
        }
    }
    for (auto bg: shot->foreground) {
        if (bg->palette != 0) {
            ByteStream paletteReader;
            if (bg->pak_palette == nullptr) {
                paletteReader.Set(this->optPals.GetEntry(bg->palette)->data, 772);
            } else {
                if (bg->palette >= bg->pak_palette->GetNumEntries()) {
                    continue;
                }
                paletteReader.Set(bg->pak_palette->GetEntry(bg->palette)->data, 772);
            }
            
            this->palette.ReadPatch(&paletteReader);
        }
        if (bg->pal != nullptr && !bg->use_external_palette) {
            this->palette.CopyFromOtherPalette(bg->pal->GetColorPalette());
        }
    }
    for (auto sprt: shot->sprites) {
        if (sprt->palette != 0) {
            ByteStream paletteReader;
            if (sprt->pak_palette == nullptr) {
                paletteReader.Set(this->optPals.GetEntry(sprt->palette)->data, 772);
            } else {
                if (sprt->palette >= sprt->pak_palette->GetNumEntries()) {
                    continue;
                }
                paletteReader.Set(sprt->pak_palette->GetEntry(sprt->palette)->data, 772);
            }
            this->palette.ReadPatch(&paletteReader);
        }
        if (sprt->pal != nullptr && !sprt->use_external_palette) {
            this->palette.CopyFromOtherPalette(sprt->pal->GetColorPalette());
        }
    }
    for (auto chara: shot->characters) {
        if (chara->palette != 0) {
            ByteStream paletteReader;
            PakEntry *palEntry = nullptr;
            palEntry = ConvAssetManager::getInstance().convPals.GetEntry(chara->palette);
            if (palEntry == nullptr) {
                continue;
            }
            if (palEntry->size == 0) {
                continue;
            }
            paletteReader.Set(palEntry->data, 772);
            this->palette.ReadPatch(&paletteReader);
        }
    }
    
    
    VGA.setPalette(&this->palette);
    for (auto bg : shot->background) {
        if (bg->image == nullptr) {
            continue;
        }
        if (bg->image->GetNumImages()>0) {
            RLEShape *shp = bg->image->GetShape(bg->shapeid);
            FrameBuffer *texture = new FrameBuffer(320, 200);
            texture->fillWithColor(255);
            texture->drawShape(shp);
            fb->blitWithMask(texture->framebuffer, bg->current_position.x, bg->current_position.y, 320, 200,255);
            if (fpsupdate && (bg->velocity.x != 0 || bg->velocity.y != 0)) {
                if (bg->current_position.x != bg->position_end.x) {
                    bg->current_position.x += bg->velocity.x;
                }  
                if (bg->velocity.y<0 && bg->current_position.y > bg->position_end.y) {
                    bg->current_position.y += bg->velocity.y;
                } else if (bg->velocity.y>0 && bg->current_position.y < bg->position_end.y) {
                    bg->current_position.y += bg->velocity.y;
                }
            }
            delete texture;
        }
    }
    for (auto chara: shot->characters) {
        if (chara->image == nullptr) {
            continue;
        }
        if (chara->image->GetNumImages()>0) {
            RLEShape *shp = chara->image->GetShape(1);
            FrameBuffer *texture = new FrameBuffer(320, 200);
            texture->fillWithColor(255);
            texture->drawShape(shp);
            shp = chara->image->GetShape(chara->head_id);
            texture->drawShape(shp);
            shp = chara->image->GetShape(chara->cloth_id);
            texture->drawShape(shp);
            shp = chara->image->GetShape(chara->expression_id);
            texture->drawShape(shp);
            fb->blitWithMask(texture->framebuffer, chara->position_start.x, chara->position_start.y, 320, 200,255);
            delete texture;
        }
    }
    for (auto sprt: shot->sprites) {
        if (sprt->image == nullptr) {
            continue;
        }
        FrameBuffer *texture = new FrameBuffer(320, 200);
        texture->fillWithColor(255);
        if (sprt->keep_first_frame) {
            texture->drawShape(sprt->image->GetShape(1));
        }
        if (fpsupdate && (sprt->velocity.x != 0 || sprt->velocity.y != 0)) {
            if (sprt->current_position.x != sprt->position_end.x) {
                sprt->current_position.x += sprt->velocity.x;
            }  
            if (sprt->velocity.y<0 && sprt->current_position.y > sprt->position_end.y) {
                sprt->current_position.y += sprt->velocity.y;
            } else if (sprt->velocity.y>0 && sprt->current_position.y < sprt->position_end.y) {
                sprt->current_position.y += sprt->velocity.y;
            }
        }
        if (fpsupdate) {
            sprt->current_frame++;
            if (sprt->current_frame > sprt->image->GetNumImages()-1) {
                if (sprt->repeat_animation) {
                    sprt->current_frame = 1;
                } else {
                    sprt->current_frame = sprt->image->GetNumImages()-1;
                }
            }
        }
        
        texture->drawShape(sprt->image->GetShape(sprt->current_frame));
        
        int color = texture->framebuffer[0];
        fb->blitWithMask(texture->framebuffer, sprt->current_position.x, sprt->current_position.y, 320, 200,color);
        delete texture;
    }
    for (auto bg : shot->foreground) {
        if (bg->image == nullptr) {
            continue;
        }
        if (bg->image->GetNumImages()>0) {
            RLEShape *shp = bg->image->GetShape(bg->shapeid);
            FrameBuffer *texture = new FrameBuffer(320, 200);
            texture->fillWithColor(255);
            texture->drawShape(shp);
            fb->blitWithMask(texture->framebuffer, bg->current_position.x, bg->current_position.y, 320, 200,255);
            if (fpsupdate && (bg->velocity.x != 0 || bg->velocity.y != 0)) {
                if (bg->current_position.x != bg->position_end.x) {
                    bg->current_position.x += bg->velocity.x;
                }  
                if (bg->velocity.y<0 && bg->current_position.y > bg->position_end.y) {
                    bg->current_position.y += bg->velocity.y;
                } else if (bg->velocity.y>0 && bg->current_position.y < bg->position_end.y) {
                    bg->current_position.y += bg->velocity.y;
                }
            }
            delete texture;
        }
    }
    if (!pause) {
        fps_counter++;
        fps+=fpsupdate;
    } else if (pause && fps == 1) {
        for (auto bg : shot->background) {
            bg->current_position = {bg->position_start.x, bg->position_start.y};
        }
        for (auto sprt: shot->sprites) {
            sprt->current_frame = 1;
            sprt->current_position = {sprt->position_start.x, sprt->position_start.y};
        }
        for (auto bg : shot->foreground) {
            bg->current_position = {bg->position_start.x, bg->position_start.y};
        }
    }
    if (fps > shot->nbframe) {
        fps = 1;
        if (shot->sound_played) {
            shot->sound_played = false;
        }
        for (auto bg : shot->background) {
            bg->current_position = {bg->position_start.x, bg->position_start.y};
        }
        for (auto sprt: shot->sprites) {
            sprt->current_frame = 1;
            sprt->current_position = {sprt->position_start.x, sprt->position_start.y};
        }
        for (auto bg : shot->foreground) {
            bg->current_position = {bg->position_start.x, bg->position_start.y};
        }
        if (!stay_on_current_shot) {
            shot_counter++;
            if (shot_counter>this->midgames_shots[1].size()-1) {
                shot_counter = 0;
                if (auto_stop) {
                    Game->stopTopActivity();
                }
            }
            shot = this->midgames_shots[1][shot_counter];
            for (auto bg : shot->background) {
                bg->current_position = {bg->position_start.x, bg->position_start.y};
            }
            for (auto sprt: shot->sprites) {
                sprt->current_frame = 1;
                sprt->current_position = {sprt->position_start.x, sprt->position_start.y};
            }
            for (auto bg : shot->foreground) {
                bg->current_position = {bg->position_start.x, bg->position_start.y};
            }
        }
    }
    for (size_t i = 0; i < CONV_TOP_BAR_HEIGHT; i++)
            VGA.getFrameBuffer()->fillLineColor(i, 0x00);

    for (size_t i = 0; i < CONV_BOTTOM_BAR_HEIGHT; i++)
        VGA.getFrameBuffer()->fillLineColor(199 - i, 0x00);
    VGA.vSync();
}

void SCAnimationPlayer::loadShot(std::string filename) {
    SCAnimationArchive archive;
    this->midgames_shots[1].clear();
    archive.LoadFromFile(filename.c_str(), this->midgames_shots[1]);
    this->shot_counter = 0;
    this->fps = 1;  
    for (auto shot : this->midgames_shots[1]) {
        for (auto bg : shot->background) {
            bg->current_position = {bg->position_start.x, bg->position_start.y};
        }
        for (auto sprt: shot->sprites) {
            sprt->current_frame = 1;
            sprt->current_position = {sprt->position_start.x, sprt->position_start.y};
        }
        for (auto bg : shot->foreground) {
            bg->current_position = {bg->position_start.x, bg->position_start.y};
        }
        /*if (shot->sound == nullptr) {
            MIDGAME_SOUND *sound = new MIDGAME_SOUND();
            sound->data = shot->sound_pak->GetEntry(shot->sound_pak_entry_id)->data;
            sound->size = shot->sound_pak->GetEntry(shot->sound_pak_entry_id)->size;
            shot->sound = sound;
        }*/
    }
}
