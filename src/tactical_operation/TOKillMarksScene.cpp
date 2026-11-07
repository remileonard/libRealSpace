//
//  TOKillMarksScene.cpp
//  libRealSpace
//

#include "TOKillMarksScene.h"

TOKillMarksScene::TOKillMarksScene(PakArchive *optShps2, PakArchive *optPals2) : EndMissionScene(nullptr, nullptr) {
    this->optShps2 = optShps2;
    this->optPals2 = optPals2;
}

// Copie d'EndMissionScene::init : OPTSHPS.PAK 141 et 142, OPTPALS.PAK 19 ; OPTSHPS2.PAK 13 et OPTPALS2.PAK 1.
void TOKillMarksScene::init() {
    SCShot::init();
    this->scoringSprites = this->getShape(142);
    this->layers.clear();
    shotBackground *tmpbg = new shotBackground();
    PakEntry *background = this->optShps2->GetEntry(13);
    PakArchive subPAK;
    subPAK.InitFromRAM("", background->data, background->size);
    tmpbg->img = new RSImageSet();
    if (!subPAK.IsReady()) {
        tmpbg->img->InitFromPakEntry(background);
    } else {
        tmpbg->img->InitFromSubPakEntry(&subPAK);
    }
    this->layers.push_back(tmpbg);
    tmpbg = new shotBackground();
    tmpbg->img = this->getShape(141);
    if (tmpbg->img != nullptr) {
        this->layers.push_back(tmpbg);
    }
    this->rawPalette = this->optPals2->GetEntry(1)->data;
    PakEntry *paletteEntry2 = this->optPals.GetEntry(19);
    if (paletteEntry2 != nullptr) {
        this->p2 = new VGAPalette();
        ByteStream paletteReader;
        paletteReader.Set(paletteEntry2->data, 772);
        this->p2->ReadPatch(&paletteReader);
    }
    printf("TO KILL MARKS: fond 13 d'OPTSHPS2.PAK, %zu images ; palette 1 d'OPTPALS2.PAK\n", this->layers[0]->img->GetNumImages());
}
