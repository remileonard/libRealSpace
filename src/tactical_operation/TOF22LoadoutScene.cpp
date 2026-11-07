//
//  TOF22LoadoutScene.cpp
//  libRealSpace
//

#include "TOF22LoadoutScene.h"

// Point d'emport 0 : AIM-9J (EXTR 2, image 1) ou AIM-9M (EXTR 3, image 3), zones de retrait 117 et 118.
// Point d'emport 1 : AIM-120 (EXTR 1, image 5), zones de retrait 119 et 120. EXTR 0 (image 7) : fond de la base.
TOF22LoadoutScene::TOF22LoadoutScene(PakArchive *optShps, PakArchive *optPals) : WeaponLoadoutScene(optShps, optPals) {
    this->optShps = optShps;
    this->optPals = optPals;
    this->weapon_pylones = {
        std::unordered_map<weapon_type_shp_id, std::vector<std::vector<uint8_t>>>{
            {
                AIM9J,
                std::vector<std::vector<uint8_t>>{
                    { 2 },
                    { 117,
                      118 },
                },
            },
            {
                AIM9M,
                std::vector<std::vector<uint8_t>>{
                    { 3 },
                    { 117,
                      118 },
                },
            },
        },
        std::unordered_map<weapon_type_shp_id, std::vector<std::vector<uint8_t>>>{
            {
                AIM120,
                std::vector<std::vector<uint8_t>>{
                    { 1 },
                    { 119,
                      120 },
                },
            },
        },
        std::unordered_map<weapon_type_shp_id, std::vector<std::vector<uint8_t>>>{},
        std::unordered_map<weapon_type_shp_id, std::vector<std::vector<uint8_t>>>{},
    };
}

RSImageSet *TOF22LoadoutScene::getShape(uint8_t shpid) {
    PakEntry *shapeEntry = this->optShps->GetEntry(shpid);
    PakArchive subPAK;

    subPAK.InitFromRAM("", shapeEntry->data, shapeEntry->size);
    RSImageSet *img = new RSImageSet();
    if (!subPAK.IsReady()) {
        img->InitFromPakEntry(this->optShps->GetEntry(shpid));

    } else {
        img->InitFromSubPakEntry(&subPAK);
    }
    return (img);
}
// Copie de WeaponLoadoutScene::init pour les images d'OPTSHPS2.PAK : image 0 vide dans libRealSpace, + 1.
std::vector<SCZone *> *TOF22LoadoutScene::init(GAMEFLOW_SCEN *gf, SCEN *sc_opts, std::function<void(bool fly)> onexit) {
    if (sc_opts == nullptr) {
        return nullptr;
    }
    this->gameflow_scene = gf;
    this->sceneOpts = sc_opts;
    this->zones.clear();
    GameState.weapon_load_out.clear();
    GameState.weapon_load_out[1] = 0;
    GameState.weapon_load_out[2] = 0;
    GameState.weapon_load_out[3] = 0;
    GameState.weapon_load_out[4] = 0;
    for (auto bg : sceneOpts->background->images) {
        background *tmpbg = new background();
        tmpbg->img = this->getShape(bg->ID);
        tmpbg->img->sequence[0] = 1;
        printf("F22 LOADOUT: fond, image %d de OPTSHPS2.PAK, %zu images\n", bg->ID, tmpbg->img->GetNumImages());
        this->layers.push_back(tmpbg);
    }
    uint8_t paltID = sceneOpts->background->palette->ID;
    uint8_t forPalTID = sceneOpts->foreground->palette->ID;

    this->rawPalette = this->optPals->GetEntry(paltID)->data;
    this->forPalette = this->optPals->GetEntry(forPalTID)->data;
    printf("F22 LOADOUT: palettes %d et %d de OPTPALS2.PAK\n", paltID, forPalTID);

    if (gameflow_scene->info.ID == 20) {
        background *tmpbg = new background();
        tmpbg->img = this->getShape(this->sceneOpts->extr[0]->SHAPE_ID);
        tmpbg->img->sequence[0] = 1;
        printf("F22 LOADOUT: fond de la base, image %d de OPTSHPS2.PAK\n", this->sceneOpts->extr[0]->SHAPE_ID);
        this->layers.push_back(tmpbg);
    }

    this->onexit = onexit;
    this->updateWeaponDisplay();
    return (&this->zones);
}

// Une paire par point d'emport : AIM-9J ou AIM-9M sur le point 0, AIM-120 sur le point 1.
void TOF22LoadoutScene::placeWeapon(uint8_t id) {
    int slot = 0;
    if (id == AIM9J || id == AIM9M) {
        slot = 1;
    } else if (id == AIM120) {
        slot = 2;
    }
    if (slot == 0 || GameState.weapon_load_out[slot] != 0) {
        printf("F22 LOADOUT: arme %d non posee, point d'emport %d occupe ou absent\n", id, slot - 1);
        return;
    }
    GameState.weapon_load_out[slot] = id;
    GameState.weapon_load_out[id] += 2;
    printf("F22 LOADOUT: arme %d sur le point d'emport %d, %d\n", id, slot - 1, GameState.weapon_load_out[id]);
}

// Gameflow_ApplyDefaultLoadoutAlt_58223 : AIM-9J et AIM-9M (0, 1) point 0, AIM-120 (8) point 1, une par côté ;
// autres armes : points 0 à 255, 0 par côté.
void TOF22LoadoutScene::loadoutRange(uint8_t weapon, int &first_hp, int &last_hp, int &max_per_side) {
    first_hp = 0;
    last_hp = 255;
    max_per_side = 0;
    if (weapon == 0 || weapon == 1) {
        first_hp = 0;
        last_hp = 0;
        max_per_side = 1;
    } else if (weapon == 8) {
        first_hp = 1;
        last_hp = 1;
        max_per_side = 1;
    }
}

void TOF22LoadoutScene::dropEmptyFirstImage(RSImageSet *img) {
    if (img == nullptr || img->shapes.size() < 2) {
        return;
    }
    delete img->shapes[0];
    img->shapes.erase(img->shapes.begin());
}

void TOF22LoadoutScene::addSpriteZone(uint8_t sprite, std::function<void(std::vector<EFCT *> *, uint8_t)> action) {
    SCZone *z = this->createZone(this->sceneOpts->foreground->sprites[sprite], sprite);
    z->active = true;
    z->onclick = action;
    this->dropEmptyFirstImage(z->sprite->img);
    this->zones.push_back(z);
}

// Copie de WeaponLoadoutScene::updateWeaponDisplay pour les trois armes du F-22 (points d'emport 0 et 1).
void TOF22LoadoutScene::updateWeaponDisplay() {
    this->zones.clear();
    this->zones.shrink_to_fit();
    this->addSpriteZone(this->gameflow_scene->info.ID, std::bind(&TOF22LoadoutScene::exitClicked, this, std::placeholders::_1, std::placeholders::_2));
    this->addSpriteZone(FLY_SPRITE, std::bind(&TOF22LoadoutScene::exitClicked, this, std::placeholders::_1, std::placeholders::_2));
    for (auto weap : GameState.weapon_inventory) {
        uint8_t sprite = weapon_inv_to_loadout[weapon_ids(weap.first)];
        if (weap.second != 0 && this->sceneOpts->foreground->sprites.count(sprite) > 0) {
            this->addSpriteZone(sprite, std::bind(&TOF22LoadoutScene::addWeapon, this, std::placeholders::_1, std::placeholders::_2));
        }
    }
    for (int i = 1; i < 3; i++) {
        int loaded = GameState.weapon_load_out[i];
        if (loaded != AIM9J && loaded != AIM9M && loaded != AIM120) {
            continue;
        }
        weapon_type_shp_id weapon = weapon_type_shp_id(loaded);
        const char *name = "AIM120";
        if (weapon == AIM9J) {
            name = "AIM9J";
        } else if (weapon == AIM9M) {
            name = "AIM9M";
        }
        char label[32];
        snprintf(label, sizeof(label), "%s(%d)", name, GameState.weapon_load_out[weapon]);
        SCZone *extra = this->addExtraZone(this->weapon_pylones[i - 1][weapon][0][0]);
        this->dropEmptyFirstImage(extra->sprite->img);
        this->extra_zones.push_back(extra);
        for (auto nid : this->weapon_pylones[i - 1][weapon][1]) {
            this->zones.push_back(this->addNoGfXZone(nid, label, weapon));
        }
    }
    printf("F22 LOADOUT: %zu zones, %zu armes posees sur l'avion\n", this->zones.size(), this->extra_zones.size());
}
