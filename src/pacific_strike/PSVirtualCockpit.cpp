#include "PSVirtualCockpit.h"

PSVirtualCockpit::PSVirtualCockpit(PSCockpit *cockpit) : SCVirtualCockpit(cockpit) {
    this->ps_cockpit = cockpit;
}

PSVirtualCockpit::~PSVirtualCockpit() {
    for (auto &entry : this->instrument_framebuffers) {
        delete entry.second;
    }
    this->instrument_framebuffers.clear();
}

// L'objet du cockpit a son origine au point de l'oeil : le tableau de bord de CORPIT est 2,5 m devant et 0,8 m sous
// l'origine, là où les chunks 3SPC placent les instruments. Il se dessine donc à la caméra, sans décalage. Les 3
// entiers du chunk INFO du FORM REAL (Corsair : 2,40 m vers le nez, 3,30 m vers le haut) placent ce point par rapport
// à l'avion dans le jeu (Cockpit_UpdateView_1699B) ; libRealSpace place sa caméra à eye_y au-dessus de l'avion.
Vector3D PSVirtualCockpit::modelOffset(const VirtualCockpitContext &ctx) {
    return Vector3D(0.0f, 0.0f, 0.0f);
}

void PSVirtualCockpit::drawModel(const VirtualCockpitContext &ctx) {
    PSCockpitDef *def = this->ps_cockpit->definition();
    if (def == nullptr || def->REAL.OBJS == nullptr) {
        return;
    }
    Renderer.drawModel(def->REAL.OBJS, Renderer.lodLevel, this->cockpit_pos, this->cockpit_rot, this->modelOffset(ctx));
}

// Un instrument avec un chunk 3SPC et une image 1 : sa texture a la taille de cette image ; son quadrilatère part du
// point (x, y, z) du 3SPC dans le repère de l'objet du cockpit, large de w vers la droite et haut de h vers le bas
// (sub_237B0). Valeurs en 24.8.
void PSVirtualCockpit::updateInstruments(const VirtualCockpitContext &ctx) {
    PSCockpitDef *def = this->ps_cockpit->definition();
    if (def == nullptr) {
        return;
    }
    Vector3D offset = this->modelOffset(ctx);
    for (auto instrument : def->instruments) {
        if (!instrument->has_space || instrument->SHAP.GetNumImages() < 2) {
            continue;
        }
        if (instrument->type == PS_INST_BANK || instrument->type == PS_INST_AMMO || instrument->type == PS_INST_BCOM) {
            continue;
        }
        FrameBuffer *fb = nullptr;
        auto found = this->instrument_framebuffers.find(instrument);
        if (found == this->instrument_framebuffers.end()) {
            RLEShape *shape = instrument->SHAP.GetShape(1);
            fb = new FrameBuffer(shape->GetWidth(), shape->GetHeight());
            this->instrument_framebuffers[instrument] = fb;
        } else {
            fb = found->second;
        }
        this->ps_cockpit->RenderInstrument3D(instrument, fb);
        float span = (float)instrument->space[0] / 256.0f;
        float nose = (float)instrument->space[1] / 256.0f;
        float up = (float)instrument->space[2] / 256.0f;
        float width = (float)instrument->space[3] / 256.0f;
        float height = (float)instrument->space[4] / 256.0f;
        std::vector<Vector3D> quad;
        quad.push_back(Vector3D(offset.x + nose, offset.y + up, offset.z + span));
        quad.push_back(Vector3D(offset.x + nose, offset.y + up, offset.z + span + width));
        quad.push_back(Vector3D(offset.x + nose, offset.y + up - height, offset.z + span + width));
        quad.push_back(Vector3D(offset.x + nose, offset.y + up - height, offset.z + span));
        if (!this->traced) {
            printf("PSVirtualCockpit: instrument %d, texture %d x %d, coin (nez %.3f, haut %.3f, envergure %.3f), %.3f x %.3f\n", (int)instrument->type, fb->width, fb->height, quad[0].x, quad[0].y, quad[0].z, width, height);
        }
        this->addPanel(fb, quad);
    }
    this->traced = true;
}
