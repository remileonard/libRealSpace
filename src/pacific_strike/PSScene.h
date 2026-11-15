#pragma once
//
//  PSScene.h
//  libRealSpace
//
//  Écran du déroulement de Pacific Strike : celui de Strike Commander (SCScene), dont les conditions REQU portent sur
//  des numéros de 2 octets de la table des variables et drapeaux (drapeaux de PSState).
//

#include "../strike_commander/precomp.h"
#include "PSState.h"

class PSScene : public SCScene {
protected:
    PSState &PSGameState = PSState::getInstance();

public:
    PSScene(PakArchive *optShps, PakArchive *optPals) : SCScene(optShps, optPals) {};
    std::vector<SCZone *> *init(GAMEFLOW_SCEN *gf, SCEN *sc_opts, std::function<void(std::vector<EFCT *> *script, uint8_t id)> onclick) override;

private:
    bool zoneActive(SCZone *zone);
};
