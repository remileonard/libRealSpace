#pragma once
#include "precomp.h"
#include "../realspace/RSAirdens.h"

class SCExplosion {
private:
    SCRenderer &Renderer = SCRenderer::getInstance();
public:
    RSEntity *obj{nullptr};
    // temps passé sur l'image courante ; une image toutes les 7 images à 60 images/s
    float image_time{0.0f};
    int current_frame{0};
    Vector3D position{0, 0, 0};
    bool is_finished{false};
    // Débris (chunk DEBR, Debris_SpawnFromDebrList_3844F) : l'image animée tourne en boucle et tombe (gravité, frottement de l'air
    // ATMO, masse DYNM) jusqu'au sol de la carte.
    bool falling{false};
    // temps restant d'un débris, en secondes (durée de vie DEBR INFO × 1/25 s)
    float life_left{0.0f};
    Vector3D velocity{0, 0, 0};
    RSArea *area{nullptr};
    RSAirdens airdens;

    void render();
    void fall(float dt);
    SCExplosion(RSEntity *obj, Vector3D position);
};