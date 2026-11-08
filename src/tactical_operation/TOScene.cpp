//
//  TOScene.cpp
//  libRealSpace
//

#include "TOScene.h"

TOScene::TOScene(PakArchive *optShps, PakArchive *optPals, PakArchive *optShps2, PakArchive *optPals2) : SCScene(optShps, optPals) {
    this->optShps2 = optShps2;
    this->optPals2 = optPals2;
}

// Image numéro shpid, dans OPTSHPS2.PAK si second, sinon dans OPTSHPS.PAK (SCScene::getShape). Une forme simple
// d'OPTSHPS2.PAK suivie d'une palette est lue avec une image vide en 0, retirée ici.
RSImageSet *TOScene::screenShape(uint8_t shpid, bool second) {
    if (!second) {
        return this->getShape(shpid);
    }
    PakEntry *shapeEntry = this->optShps2->GetEntry(shpid);
    PakArchive subPAK;
    subPAK.InitFromRAM("", shapeEntry->data, shapeEntry->size);
    RSImageSet *img = new RSImageSet();
    if (!subPAK.IsReady()) {
        img->InitFromPakEntry(shapeEntry);
    } else {
        img->InitFromSubPakEntry(&subPAK);
    }
    img->removeFirstEmptyShape();
    printf("TO SCENE: image %d de OPTSHPS2.PAK, %zu images\n", shpid, img->GetNumImages());
    return img;
}

// OptionScreen_Load_870B : palette de numéro > 48 dans OPTPALS2.PAK (cmp [bp+var_2], 30h / jle).
uint8_t *TOScene::screenPalette(uint16_t id) {
    if (id > LAST_SC_PALETTE) {
        printf("TO SCENE: palette %d de OPTPALS2.PAK\n", id);
        return this->optPals2->GetEntry(id)->data;
    }
    return this->optPals->GetEntry(id)->data;
}

// Copie de SCScene::init avec les règles d'OptionScreen_Load_870B : fond d'un écran de numéro > 136 et image d'un
// sprite de numéro > 136 dans OPTSHPS2.PAK (cmp ..., 88h / jbe) ; palettes par screenPalette.
std::vector<SCZone *> *TOScene::init(GAMEFLOW_SCEN *gf, SCEN *sc_opts, std::function<void(std::vector<EFCT *> *script, uint8_t id)> onclick) {
    this->gameflow_scene = gf;
    this->sceneOpts = sc_opts;

    this->zones.clear();
    this->zones.shrink_to_fit();

    GameState.aircraftHooks.clear();
    for (auto bg : sceneOpts->background->images) {
        background *tmpbg = new background();
        tmpbg->img = this->screenShape(bg->ID, gf->info.ID > LAST_SC_SCREEN);
        this->layers.push_back(tmpbg);
    }
    uint8_t paltID = sceneOpts->background->palette->ID;
    uint8_t forPalTID = sceneOpts->foreground->palette->ID;

    this->rawPalette = this->screenPalette(paltID);
    this->forPalette = this->screenPalette(forPalTID);

    uint8_t optionScenID = gameflow_scene->info.ID;
    chk = nullptr;
    this->scene_id = optionScenID;
    frame_counter = 1000;
    frame_animation = 0;
    if (optionScenID == 107) {
        AssetManager &ast = AssetManager::getInstance();
        TreEntry *entry = ast.GetEntryByName("..\\..\\DATA\\MIDGAMES\\CHICK.SHP");
        if (entry != nullptr) {
            PakEntry *subPAK;
            subPAK = new PakEntry();
            subPAK->data = entry->data;
            subPAK->size = entry->size;
            chk = new RSImageSet();
            chk->InitFromPakEntry(subPAK);
        }
    }
    for (auto sprite : this->gameflow_scene->sprt) {
        uint8_t sprtId = sprite->info.ID;
        // le clck dans sprite semble indiquer qu'il faut jouer l'animation après avoir cliquer et donc executer le
        // efect à la fin de l'animation.
        uint8_t zone_id = 0;
        if (this->sceneOpts->foreground->sprites.count(sprtId) > 0) {
            uint8_t optsprtId = this->sceneOpts->foreground->sprites[sprtId]->sprite.SHP_ID;
            animatedSprites *sprt = new animatedSprites();
            SCZone *z = new SCZone();
            if (this->sceneOpts->foreground->sprites[sprtId]->zone != nullptr) {
                sprt->rect = new sprtRect();
                sprt->rect->x1 = this->sceneOpts->foreground->sprites[sprtId]->zone->X1;
                sprt->rect->y1 = this->sceneOpts->foreground->sprites[sprtId]->zone->Y1;
                sprt->rect->x2 = this->sceneOpts->foreground->sprites[sprtId]->zone->X2;
                sprt->rect->y2 = this->sceneOpts->foreground->sprites[sprtId]->zone->Y2;

                z->id = sprtId;
                z->position.x = sprt->rect->x1;
                z->position.y = sprt->rect->y1;
                z->dimension.x = sprt->rect->x2 - sprt->rect->x1;
                z->dimension.y = sprt->rect->y2 - sprt->rect->y1;
                z->label = this->sceneOpts->foreground->sprites[sprtId]->label;
                z->onclick = onclick;
                z->quad = nullptr;
            }
            if (sceneOpts->foreground->sprites[sprtId]->quad != nullptr) {
                sprt->quad = new std::vector<Point2D *>();

                Point2D *p;

                p = new Point2D();
                p->x = sceneOpts->foreground->sprites[sprtId]->quad->xa1;
                p->y = sceneOpts->foreground->sprites[sprtId]->quad->ya1;
                sprt->quad->push_back(p);

                p = new Point2D();
                p->x = sceneOpts->foreground->sprites[sprtId]->quad->xa2;
                p->y = sceneOpts->foreground->sprites[sprtId]->quad->ya2;
                sprt->quad->push_back(p);

                p = new Point2D();
                p->x = sceneOpts->foreground->sprites[sprtId]->quad->xb1;
                p->y = sceneOpts->foreground->sprites[sprtId]->quad->yb1;
                sprt->quad->push_back(p);

                p = new Point2D();
                p->x = sceneOpts->foreground->sprites[sprtId]->quad->xb2;
                p->y = sceneOpts->foreground->sprites[sprtId]->quad->yb2;
                sprt->quad->push_back(p);

                z->id = sprtId;
                z->quad = sprt->quad;
                z->label = sceneOpts->foreground->sprites[sprtId]->label;
                z->onclick = onclick;
            }

            sprt->id = sprite->info.ID;

            sprt->shapid = optsprtId;
            sprt->unkown = sprite->info.UNKOWN;
            sprt->efect = sprite->efct;
            sprt->img = this->screenShape(optsprtId, sprtId > LAST_SC_SCREEN);
            sprt->img->removeFirstEmptyShape();
            sprt->frameCounter = 0;
            if (this->sceneOpts->foreground->sprites[sprtId]->CLCK == 1) {
                sprt->cliked = true;
                sprt->frameCounter = 1;
            }
            if (this->sceneOpts->foreground->sprites[sprtId]->SEQU != nullptr) {
                sprt->frames = this->sceneOpts->foreground->sprites[sprtId]->SEQU;
                sprt->frameCounter = 0;
            }
            sprt->requ = sprite->requ;
            z->sprite = sprt;
            z->id = zone_id;
            zone_id++;
            this->zones.push_back(z);
        } else {
            printf("%d, ID Sprite not found !!\n", sprtId);
        }
    }
    for (auto zn : this->zones) {
        zn->isActive(&GameState.requierd_flags);
    }
    return (&this->zones);
}
