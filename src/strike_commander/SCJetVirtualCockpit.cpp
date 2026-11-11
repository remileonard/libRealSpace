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

void SCJetVirtualCockpit::drawInstruments(const VirtualCockpitContext &ctx) {
    this->renderHUD(ctx);
    if (this->cockpit->hud != nullptr) {
        this->drawPanel(this->cockpit->hud_framebuffer, this->layout.hud);
    }
    if (this->cockpit->cockpit->MONI.SHAP.data != nullptr && this->cockpit->mfd_right_framebuffer != nullptr &&
        this->cockpit->mfd_left_framebuffer != nullptr) {
        this->renderMFDRight();
        this->drawPanel(this->cockpit->mfd_right_framebuffer, this->layout.mfd_right);
        this->renderMFDLeft();
        this->drawPanel(this->cockpit->mfd_left_framebuffer, this->layout.mfd_left);
    }
    if (this->cockpit->cockpit->MONI.INST.RAWS.NORM.data != nullptr && this->cockpit->raws_framebuffer != nullptr) {
        this->renderRAWS();
        this->drawPanel(this->cockpit->raws_framebuffer, this->layout.raws);
    }
    this->drawTargetCamera(ctx);
    if (!this->layout.alti.empty() && this->cockpit->alti_framebuffer != nullptr) {
        this->renderAlti();
        this->drawPanel(this->cockpit->alti_framebuffer, this->layout.alti);
    }
    if (!this->layout.speed.empty() && this->cockpit->speed_framebuffer != nullptr) {
        this->renderSpeed();
        this->drawPanel(this->cockpit->speed_framebuffer, this->layout.speed);
    }
    if (this->cockpit->comm_framebuffer != nullptr && this->renderComm()) {
        this->drawPanel(this->cockpit->comm_framebuffer, this->layout.comm);
    }
    if (ctx.mouse_control) {
        this->renderMouse();
        this->drawPanel(this->mouse_framebuffer, this->layout.mouse);
    }
}
