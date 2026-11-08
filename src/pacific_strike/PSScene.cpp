//
//  PSScene.cpp
//  libRealSpace
//

#include "PSScene.h"

std::vector<SCZone *> *PSScene::init(GAMEFLOW_SCEN *gf, SCEN *sc_opts, std::function<void(std::vector<EFCT *> *script, uint8_t id)> onclick) {
    std::vector<SCZone *> *zones = SCScene::init(gf, sc_opts, onclick);
    for (auto zone : this->zones) {
        zone->active = this->zoneActive(zone);
    }
    return zones;
}

// Copie de SCZone::isActive sur la table des variables : un numéro jamais posé vaut 0 (GameVar_Get_56414).
// 0 et 2 : drapeau non posé ; 1 et 3 : drapeau posé ; 2 et 3 : la condition suivante est en « ou ».
bool PSScene::zoneActive(SCZone *zone) {
    if (zone->sprite == nullptr || zone->sprite->requ == nullptr) {
        return true;
    }
    bool active = true;
    bool next_is_or = false;
    bool first = true;
    for (auto requ : *zone->sprite->requ) {
        bool set = GameState.getGameVar(requ->value) != 0;
        bool result = set;
        if (requ->op == 0 || requ->op == 2) {
            result = !set;
        }
        if (first) {
            active = result;
            first = false;
        } else if (next_is_or) {
            active = active || result;
        } else {
            active = active && result;
        }
        next_is_or = requ->op == 2 || requ->op == 3;
    }
    printf("PS SCENE: zone %d, %zu conditions, %s\n", zone->id, zone->sprite->requ->size(), active ? "active" : "inactive");
    return active;
}
