//
//  RSWorld.h
//  libRealSpace
//
//  Created by Rémi LEONARD on 02/12/2024.
//  Copyright (c) 2013 Fabien Sanglard. All rights reserved.
//
#pragma once

#include "../commons/IFFSaxLexer.h"
#include "RSCameraCOMP.h"

struct RSCameraDef {
    std::string          name;         // nom interne : "CHASECAM"/"COCKPIT"/"VICTIM"/"AUTOTRAC"/"WEAPON"/"ROTATCAM"
    uint8_t              typeCode = 0x14; // CHAS=3 CKPT=4 VICT=7 ROTA=8 TARG=9 WEAP=0x0B
    std::string          subject;      // entité porteuse, ex. "PLAYER"
    std::string          cockpitArt;   // CKPT seul, ex. "F16-CKPT"
    uint32_t             farClip  = 0; // 50000
    uint16_t             fov      = 0; // 40 (CKPT lit 35 ici — pas sûrement un FOV)
    uint16_t             nearClip = 0; // 10
    uint16_t             viewW = 0, viewH = 0;  // 319, 199
    std::vector<int32_t> params;       // table i32 de fin (VICT/WEAP) : offsets 24.8 (÷256 = pieds) + petits int
};



class RSWorld {
private:
    void parseWRLD(uint8_t *data, size_t size);
    void parseWRLD_INFO(uint8_t *data, size_t size);
    void parseWRLD_HORZ(uint8_t *data, size_t size);
    void parseWRLD_WTCH(uint8_t *data, size_t size);
    void parseWRLD_PALT(uint8_t *data, size_t size);
    void parseWRLD_TERA(uint8_t *data, size_t size);
    void parseWRLD_SKYS(uint8_t *data, size_t size);
    void parseWRLD_GLNT(uint8_t *data, size_t size);
    void parseWRLD_SMOK(uint8_t *data, size_t size);
    void parseWRLD_LGHT(uint8_t *data, size_t size);
    void parseWRLD_CAMR(uint8_t *data, size_t size);
    void parseWRLD_CAMR_STRT(uint8_t *data, size_t size);
    void parseWRLD_CAMR_CHAS(uint8_t *data, size_t size);
    void parseWRLD_CAMR_CKPT(uint8_t *data, size_t size);
    void parseWRLD_CAMR_VICT(uint8_t *data, size_t size);
    void parseWRLD_CAMR_TARG(uint8_t *data, size_t size);
    void parseWRLD_CAMR_WEAP(uint8_t *data, size_t size);
    void parseWRLD_CAMR_ROTA(uint8_t *data, size_t size);
    void parseWRLD_CAMR_COMP(uint8_t *data, size_t size);

public:
    std::string tera;
    std::string cameraSetName;
    std::vector<RSCameraDef> cameras;
    std::vector<RSCameraSequence> cameraSequences;
    RSWorld();
    ~RSWorld();
    void InitFromRAM(uint8_t *data, size_t size);
};