//
//  RSMusic.cpp
//  libRealSpace
//
//  Created by Fabien Sanglard on 12/30/2013.
//  Copyright (c) 2013 Fabien Sanglard. All rights reserved.
//

#include "precomp.h"
void RSMusic::init() {
    PakArchive *pak = new PakArchive();
    TreEntry *entry = assetManager.GetEntryByName("..\\..\\DATA\\MIDGAMES\\AMUSIC.PAK");
    if (entry == NULL) {
        printf("RSMusic::init: Could not find ..\\..\\DATA\\MIDGAMES\\AMUSIC.PAK\n");
        return;
    }
    pak->InitFromRAM("..\\..\\DATA\\MIDGAMES\\AMUSIC.PAK", entry->data, entry->size);

    for (size_t i = 0; i < pak->GetNumEntries(); i++) {
        PakEntry *e = pak->GetEntry(i);
        uint8_t *data = new uint8_t[e->size];
        memcpy(data, e->data, e->size);
        MemMusic *music = new MemMusic();
        music->data = data;
        music->size = e->size;
        musics[0].push_back(music);
        midgames_musics[0].push_back(music);
    }
    TreEntry *gameflow = assetManager.GetEntryByName("..\\..\\DATA\\SOUND\\GAMEFLOW.ADL");
    if (gameflow == NULL) {
        printf("RSMusic::init: Could not find ..\\..\\DATA\\SOUND\\GAMEFLOW.ADL\n");
        return;
    }
    pak = new PakArchive();
    pak->InitFromRAM("..\\..\\DATA\\SOUND\\GAMEFLOW.ADL", gameflow->data, gameflow->size);
    for (size_t i = 0; i < pak->GetNumEntries(); i++) {
        PakEntry *e = pak->GetEntry(i);
        PakArchive *subpak = new PakArchive();
        subpak->InitFromRAM("..\\..\\DATA\\SOUND\\GAMEFLOW.ADL", e->data, e->size);
        if (i > 0) {
            for (size_t t = 0; t < gameflow_musics[0].size(); t++) {
                gameflow_musics[i].push_back(gameflow_musics[0][t]);
            }
        }
        for (size_t j = 0; j < subpak->GetNumEntries(); j++) {
            PakEntry *sub = subpak->GetEntry(j);
            uint8_t *data = new uint8_t[sub->size];
            if (sub->data[0] != 'F') {
                continue;
            } else {
                memcpy(data, sub->data, sub->size);
                MemMusic *music = new MemMusic();
                music->data = data;
                music->size = sub->size;
                musics[1].push_back(music);
                gameflow_musics[i].push_back(music);
            }
            
        }
    }

    TreEntry *combat = assetManager.GetEntryByName("..\\..\\DATA\\SOUND\\COMBAT.ADL");
    pak = new PakArchive();
    pak->InitFromRAM("..\\..\\DATA\\SOUND\\COMBAT.ADL", combat->data, combat->size);
    for (size_t i = 0; i < pak->GetNumEntries(); i++) {
        PakEntry *e = pak->GetEntry(i);
        PakArchive *subpak = new PakArchive();
        subpak->InitFromRAM("..\\..\\DATA\\SOUND\\COMBAT.ADL", e->data, e->size);
        for (size_t j = 0; j < subpak->GetNumEntries(); j++) {
            PakEntry *sub = subpak->GetEntry(j);
            PakArchive *subsubpak = new PakArchive();
            if (sub->data[0] == 'F')
                continue;
            subsubpak->InitFromRAM("..\\..\\DATA\\SOUND\\COMBAT.ADL", sub->data, sub->size);
            for (size_t k = 0; k < subsubpak->GetNumEntries(); k++) {
                PakEntry *subsub = subpak->GetEntry(k);
                if (subsub != NULL) {
                    uint8_t *data = new uint8_t[subsub->size];
                    memcpy(data, subsub->data, subsub->size);
                    MemMusic *music = new MemMusic();
                    music->data = data;
                    music->size = subsub->size;
                    musics[2].push_back(music);
                    combat_musics[i].push_back(music); 
                }
            }
        }
    }
}

void RSMusic::SwitchBank(uint8_t bank) {
    if (this->bank == bank) {
        return;
    }
    this->bank = bank;
}
MemMusic *RSMusic::GetMusic(uint32_t index) { 
    if (musics.find(bank) == musics.end()) {
        return NULL;
    }
    if (index >= musics[bank].size()) {
        return NULL;
    }
    return musics[bank][index]; 
}