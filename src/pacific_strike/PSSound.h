#pragma once
//
//  PSSound.h
//  libRealSpace
//
//  Sons de Pacific Strike : mêmes fichiers que Strike Commander, effets associés aux sons de DSOUNDFX.PAK d'après la
//  table de DigitalSFX_Construct_5AE86.
//

#include "../realspace/RSSound.h"

class PSSound : public RSSound {
protected:
    void initEffects() override;
};
