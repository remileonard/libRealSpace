#pragma once
#include "../strike_commander/precomp.h"

// Terrain de Pacific Strike : même PAK que Strike Commander, sauf les régions de pistes
// (TRI_ParseOverlayGeometry_A7710 de PACIFIC.EXE, strike_commander_re/analysis/pacific/README.md § 7). La carte connaît
// aussi les porte-avions de la mission : sous un avion, le sol est le pont quand il y en a un.
class PSArea : public RSArea {
public:
    // Porte-avions de la mission (objets de classe CARR qui ont un chunk CARD), donnés par PSMission.
    std::vector<SCMissionActors *> carriers;
    float getY(float x, float z) override;
    bool carrierDeckAt(float x, float z, float &deck);

protected:
    void ParseTriFile(PakEntry *entry) override;
    void ParseBlocks(size_t lod, PakEntry *entry, size_t verticePerBlock) override;
    MapVertex *blockVertex(size_t lod, int block, int x, int y, int side);
    bool seaTriangle(MapVertex *tri0, MapVertex *tri1, MapVertex *tri2);
    void seaUV(MapVertex *tri0, MapVertex *tri1, MapVertex *tri2, float uv[3][2]);
    uint16_t waterImage();
    uint16_t water_image_id{ 0xFF };
};
