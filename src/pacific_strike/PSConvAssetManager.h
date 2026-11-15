#pragma once
//
//  PSConvAssetManager.h
//  libRealSpace
//
//  Données des conversations de Pacific Strike : celles de Strike Commander, plus GAMEFLOW\PTR.IFF (PSConvCommanderPointer), les points
//  de la main du commandant à la carte.
//

#include "../realspace/ConvAssetManager.h"
#include "PSConvCommanderPointer.h"

class PSConvAssetManager : public ConvAssetManager {
protected:
    void BuildDB(void) override;

public:
    PSConvCommanderPointer pointer;
};
