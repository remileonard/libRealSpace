//
//  TOConvAssetManager.cpp
//  libRealSpace
//

#include "TOConvAssetManager.h"
#include "TOConvPlayer.h"

void TOConvAssetManager::BuildDB(void) {
    std::string name = Assets.gameflow_root_path + "CONVSHP2.PAK";
    TreEntry *entry = Assets.GetEntryByName(name);
    this->convShps2.InitFromRAM("CONVSHP2.PAK", entry->data, entry->size);
    printf("TO CONV: %s, %zu images\n", name.c_str(), this->convShps2.GetNumEntries());
    ConvAssetManager::BuildDB();
    // Figure d'Ari : image de l'élément 6 de p_wing, comme les autres pilotes (red1 81 à tex3 86 = éléments 0 à 5).
    this->figures[TOConvPlayer::ARI_FIGURE] = this->makeFigure(TOConvPlayer::ARI_FIGURE, ARI_IMAGE);
    printf("TO CONV: figure %s, image %d ; %zu figures\n", TOConvPlayer::ARI_FIGURE.c_str(), ARI_IMAGE,
           this->figures.size());
}

PakEntry *TOConvAssetManager::convShape(uint8_t id) {
    if (id > LAST_SC_SHAPE) {
        return this->convShps2.GetEntry(id);
    }
    return ConvAssetManager::convShape(id);
}
