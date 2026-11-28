#pragma once
#include "precomp.h"

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

    void render();
    SCExplosion(RSEntity *obj, Vector3D position);
};