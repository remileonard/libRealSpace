//
//  PSConvAssetManager.cpp
//  libRealSpace
//

#include "PSConvAssetManager.h"

void PSConvAssetManager::BuildDB(void) {
    ConvAssetManager::BuildDB();
    TreEntry *entry = Assets.GetEntryByName(Assets.gameflow_root_path + "PTR.IFF");
    if (entry == nullptr) {
        printf("PS CONV: PTR.IFF introuvable\n");
        return;
    }
    this->pointer.InitFromRam(entry->data, entry->size);
    printf("PS CONV: PTR.IFF, %zu points de la main\n", this->pointer.hand_points.size());
}
