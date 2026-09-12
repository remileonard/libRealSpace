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

// Sous-chunk CAMR (caméra simple). Layout confirmé sur les constructeurs ASM
// (sub_85790 CHAS, sub_82CDC VICT, sub_85ACB CKPT ...) :
//   +0x00 char[8] nom
//   +0x08 slot look-at : 14 o (CHAS) ou 2 o (autres) — nuls sur les WRLD livrés
//   +..   char[8] sujet ("PLAYER")   [CKPT insère ici char[8] cockpitArt]
//   +..   u32     farClip   (le moteur applique <<8 -> 24.8)
//   +..   u16     fov       (<<8)
//   +..   u32     nearClip / flags   (le moteur lit UN dword ici, pas pad+octet)
//   +..   u16 x4  rect viewport : x, y, w, h
//   +..   (VICT/WEAP seulement) payload étendue -> params
// Aucune coordonnée d'offset : la position de la caméra externe est calculée
// côté moteur (sous-composant de corps rigide attaché à l'avion).
struct RSCameraDef {
    std::string          name;         // "CHASECAM"/"COCKPIT"/"VICTIM"/"AUTOTRAC"/"WEAPON"/"ROTATCAM"
    uint8_t              typeCode = 0x14; // CHAS=3 CKPT=4 CONT=6 VICT=7 ROTA=8 TARG=9 WEAP=0x0B
    std::string          subject;      // entité porteuse, ex. "PLAYER"
    std::string          cockpitArt;   // CKPT seul, ex. "F16-CKPT"
    uint32_t             farClip  = 0; // 50000
    uint16_t             fov      = 0; // 40
    uint32_t             nearClip = 0; // dword ; contient ~10
    uint16_t             viewX = 0;
    uint16_t             viewY = 0;
    uint16_t             viewW = 0;
    uint16_t             viewH = 0;  // rect de rendu (0, 0, 319, 199)
    std::vector<int32_t> params;       // VICT/WEAP : payload étendue (offsets 24.8 relatifs au sujet)
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