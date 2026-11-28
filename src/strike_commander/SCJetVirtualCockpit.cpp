//
//  SCJetVirtualCockpit.cpp
//  libRealSpace
//
#include "precomp.h"

const JetCockpitLayout &SCJetVirtualCockpit::f16Layout() {
    static const JetCockpitLayout layout = {
        // hud
        {{5.8f, 2.0f, -1.22f}, {5.8f, 2.0f, 1.35f}, {6.0f, -0.8f, 1.35f}, {6.0f, -0.8f, -1.22f}},
        // mfd_right
        {{6.2f, -2.5f, -3.0f}, {6.2f, -2.5f, -1.15f}, {6.1f, -4.4f, -1.15f}, {6.1f, -4.4f, -3.0f}},
        // mfd_left
        {{6.2f, -2.5f, 1.15f}, {6.2f, -2.5f, 3.0f}, {6.1f, -4.4f, 3.0f}, {6.1f, -4.4f, 1.15f}},
        // raws
        {{6.2f, -1.38f, -2.10f}, {6.2f, -1.38f, -1.15f}, {6.1f, -2.45f, -1.15f}, {6.1f, -2.45f, -2.10f}},
        // alti
        {{6.1f, -3.15f, 0.020f}, {6.1f, -3.15f, 0.90f}, {5.7f, -3.92f, 0.90f}, {5.7f, -3.92f, 0.020f}},
        // speed
        {{6.1f, -3.15f, -0.90f}, {6.1f, -3.15f, -0.020f}, {5.7f, -3.92f, -0.020f}, {5.7f, -3.92f, -0.90f}},
        // comm
        {{6.1f, -0.95f, -1.90f}, {6.1f, -0.95f, 2.020f}, {6.1f, -1.10f, 2.020f}, {6.1f, -1.10f, -1.90f}},
        // mouse
        {{6.05f, -1.10f, -1.0f}, {6.05f, -1.10f, 1.0f}, {6.05f, -2.10f, 1.0f}, {6.05f, -2.10f, -1.0f}},
    };
    return layout;
}

const JetCockpitLayout &SCJetVirtualCockpit::f22Layout() {
    static const JetCockpitLayout layout = {
        // hud
        {{5.8f, 2.0f, -1.22f}, {5.8f, 2.0f, 1.35f}, {6.0f, -0.8f, 1.35f}, {6.0f, -0.8f, -1.22f}},
        // mfd_right
        {{8.2f, -2.95f, -3.3f}, {8.2f, -2.95f, -1.35f}, {8.1f, -5.0f, -1.35f}, {8.1f, -5.0f, -3.3f}},
        // mfd_left
        {{8.2f, -2.95f, 1.35f}, {8.2f, -2.95f, 3.3f}, {8.1f, -5.0f, 3.3f}, {8.1f, -5.0f, 1.35f}},
        // raws
        {{5.8f, -0.95f, -0.54f}, {5.8f, -0.95f, 0.64f}, {5.8f, -1.835f, 0.64f}, {5.8f, -1.835f, -0.54f}},
        // alti, speed: no analog gauges in the F-22 cockpit
        {},
        {},
        // comm
        {{6.1f, -0.95f, -1.90f}, {6.1f, -0.95f, 2.020f}, {6.1f, -1.10f, 2.020f}, {6.1f, -1.10f, -1.90f}},
        // mouse
        {{6.05f, -1.10f, -1.0f}, {6.05f, -1.10f, 1.0f}, {6.05f, -2.10f, 1.0f}, {6.05f, -2.10f, -1.0f}},
    };
    return layout;
}

SCJetVirtualCockpit::SCJetVirtualCockpit(SCCockpit *cockpit, const JetCockpitLayout &layout)
    : SCVirtualCockpit(cockpit), layout(layout) {}

void SCJetVirtualCockpit::updateInstruments(const VirtualCockpitContext &ctx) {
    this->renderHUD(ctx);
    if (this->cockpit->hud != nullptr) {
        this->addPanel(this->cockpit->hud_framebuffer, this->layout.hud);
    }
    if (this->cockpit->cockpit->MONI.SHAP.data != nullptr && this->cockpit->mfd_right_framebuffer != nullptr &&
        this->cockpit->mfd_left_framebuffer != nullptr) {
        this->renderMFDRight();
        this->addPanel(this->cockpit->mfd_right_framebuffer, this->layout.mfd_right);
        this->renderMFDLeft();
        this->addPanel(this->cockpit->mfd_left_framebuffer, this->layout.mfd_left);
    }
    if (this->cockpit->cockpit->MONI.INST.RAWS.NORM.data != nullptr && this->cockpit->raws_framebuffer != nullptr) {
        this->renderRAWS();
        this->addPanel(this->cockpit->raws_framebuffer, this->layout.raws);
    }
    this->addTargetCamera(ctx);
    if (!this->layout.alti.empty() && this->cockpit->alti_framebuffer != nullptr) {
        this->renderAlti();
        this->addPanel(this->cockpit->alti_framebuffer, this->layout.alti);
    }
    if (!this->layout.speed.empty() && this->cockpit->speed_framebuffer != nullptr) {
        this->renderSpeed();
        this->addPanel(this->cockpit->speed_framebuffer, this->layout.speed);
    }
    if (this->cockpit->comm_framebuffer != nullptr && this->renderComm()) {
        this->addPanel(this->cockpit->comm_framebuffer, this->layout.comm);
    }
    this->addMasterWarning();
    if (ctx.mouse_control) {
        this->renderMouse();
        this->addPanel(this->mouse_framebuffer, this->layout.mouse);
    }
}

// Voyant d'alerte (MWRN) : images de la vue agrandie (plaque 0, alerte IR 1, radar 2), comme le RAWS du cockpit virtuel. Sa place
// est celle qu'il a dans la vue agrandie du cockpit 2D par rapport au RAWS, reportée sur le panneau du RAWS.
void SCJetVirtualCockpit::addMasterWarning() {
    Moni &moni = this->cockpit->cockpit->MONI;
    RAWSShape &raws = moni.INST.RAWS;
    if (this->layout.raws.size() != 4 || moni.INST.MWRN.ARTS.GetNumImages() < 3 || raws.ZOOM.data == nullptr) {
        return;
    }
    RLEShape *plate = moni.INST.MWRN.ARTS.GetShape(0);
    int width = plate->GetWidth();
    int height = plate->GetHeight();
    int raws_width = raws.ZOOM.GetWidth();
    int raws_height = raws.ZOOM.GetHeight();
    if (width <= 0 || height <= 0 || raws_width <= 0 || raws_height <= 0) {
        return;
    }
    if (this->mwrn_framebuffer == nullptr) {
        this->mwrn_framebuffer = new FrameBuffer(width, height);
    }
    this->mwrn_framebuffer->fillWithColor(0);
    this->cockpit->RenderMasterWarningShapes({0, 0}, this->mwrn_framebuffer, plate, 1, 2);
    // coin haut gauche de la plaque dans la vue agrandie (ancre en bas à droite, comme RenderMasterWarning) ; RAWS en (84, 112)
    // sans position propre, comme RenderRAWSBig
    int raws_x = raws.zoom_x != 0 && raws.zoom_y != 0 ? raws.zoom_x : 84;
    int raws_y = raws.zoom_x != 0 && raws.zoom_y != 0 ? raws.zoom_y : 112;
    float left = (float)(moni.INST.MWRN.zoom_x - width + 1 - raws_x) / (float)raws_width;
    float top = (float)(moni.INST.MWRN.zoom_y - height + 1 - raws_y) / (float)raws_height;
    if (!this->mwrn_traced) {
        this->mwrn_traced = true;
        printf("VCOCKPIT: voyant d'alerte %dx%d en (%d, %d), RAWS %dx%d en (%d, %d) : panneau (%.2f, %.2f) a (%.2f, %.2f) du RAWS\n", width, height, moni.INST.MWRN.zoom_x - width + 1, moni.INST.MWRN.zoom_y - height + 1, raws_width, raws_height, raws_x, raws_y, left, top, left + (float)width / (float)raws_width, top + (float)height / (float)raws_height);
    }
    float right = left + (float)width / (float)raws_width;
    float bottom = top + (float)height / (float)raws_height;
    const std::vector<Vector3D> &q = this->layout.raws;
    Vector3D across = q[1] - q[0];
    Vector3D down = q[3] - q[0];
    std::vector<Vector3D> quad;
    quad.push_back(q[0] + across * left + down * top);
    quad.push_back(q[0] + across * right + down * top);
    quad.push_back(q[0] + across * right + down * bottom);
    quad.push_back(q[0] + across * left + down * bottom);
    this->addPanel(this->mwrn_framebuffer, quad);
}
