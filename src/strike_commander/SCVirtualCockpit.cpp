//
//  SCVirtualCockpit.cpp
//  libRealSpace
//
#include "precomp.h"

SCVirtualCockpit::SCVirtualCockpit(SCCockpit *cockpit) : cockpit(cockpit) {
    this->mouse_framebuffer = new FrameBuffer(320, 200);
}

SCVirtualCockpit::~SCVirtualCockpit() {
    this->releaseTextures();
    delete this->mouse_framebuffer;
    this->mouse_framebuffer = nullptr;
}

void SCVirtualCockpit::releaseTextures() {
    for (Texture *tex : this->previous_textures) {
        delete tex;
    }
    this->previous_textures.clear();
    this->previous_textures.shrink_to_fit();
    for (Texture *tex : this->current_textures) {
        delete tex;
    }
    this->current_textures.clear();
    this->current_textures.shrink_to_fit();
}

void SCVirtualCockpit::render(const VirtualCockpitContext &ctx) {
    if (this->cockpit == nullptr || this->cockpit->cockpit == nullptr || ctx.camera == nullptr || ctx.plane == nullptr) {
        return;
    }
    for (Texture *tex : this->previous_textures) {
        delete tex;
    }
    this->previous_textures.clear();
    this->previous_textures.swap(this->current_textures);

    const Vector3D camPos = {
        ctx.camera->getPosition().x,
        ctx.camera->getPosition().y,
        ctx.camera->getPosition().z
    };
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glTranslatef(camPos.x, camPos.y, camPos.z);

    this->cockpit_pos = {0.0f, 0.0f, 0.0f};
    this->cockpit_rot = {(ctx.plane->azimuthf + 900) / 10.0f,
                         ctx.plane->elevationf / 10.0f,
                         -ctx.plane->twist / 10.0f};

    this->drawModel(ctx);
    this->drawInstruments(ctx);

    glPopMatrix();
}

void SCVirtualCockpit::drawModel(const VirtualCockpitContext &ctx) {
    Vector3D ajustement = {0.0f, -(float)ctx.eye_y, 0.0f};
    if (ctx.camera->isUsingCustomMatrices()) {
        const float cockpitScale = 1.0f; // ajuste: 0.7 .. 0.95
        ajustement = {-0.55f, -(float)ctx.eye_y, 0.0f};
        Renderer.drawModel(this->cockpit->cockpit->REAL.OBJS, Renderer.lodLevel, this->cockpit_pos, this->cockpit_rot, ajustement, cockpitScale);
    } else {
        Renderer.drawModel(this->cockpit->cockpit->REAL.OBJS, Renderer.lodLevel, this->cockpit_pos, this->cockpit_rot, ajustement);
    }
}

void SCVirtualCockpit::renderHUD(const VirtualCockpitContext &ctx) {
    this->cockpit->cannonAngularOffset = ctx.gunsight_hud_offset;
    CockpitFace prev_face = this->cockpit->face;
    this->cockpit->face = CockpitFace::CP_BIG;
    this->cockpit->RenderHUD();
    this->cockpit->face = prev_face;
}

void SCVirtualCockpit::renderMFDRight() {
    if (this->cockpit->show_cam) {
        this->cockpit->RenderMFDSCamera({0, 0}, this->cockpit->mfd_right_framebuffer);
    } else if (this->cockpit->show_damage) {
        this->cockpit->RenderMFDSDamage({0, 0}, this->cockpit->mfd_right_framebuffer);
    } else {
        this->cockpit->RenderMFDSWeapon({0, 0}, this->cockpit->mfd_right_framebuffer);
    }
}

void SCVirtualCockpit::renderMFDLeft() {
    if (this->cockpit->show_comm) {
        this->cockpit->mfd_left_framebuffer->fillWithColor(0);
        this->cockpit->RenderMFDSComm({0, 0}, this->cockpit->comm_target, this->cockpit->mfd_left_framebuffer);
    } else {
        this->cockpit->RenderMFDSRadar({0, 0}, this->cockpit->radar_zoom * 20000.0f, this->cockpit->radar_mode, this->cockpit->mfd_left_framebuffer);
    }
}

void SCVirtualCockpit::renderRAWS() {
    this->cockpit->raws_framebuffer->fillWithColor(0);
    this->cockpit->RenderRAWSBig({0, 0}, this->cockpit->raws_framebuffer);
}

void SCVirtualCockpit::renderAlti() {
    this->cockpit->alti_framebuffer->fillWithColor(0);
    this->cockpit->RenderAlti({0, 0}, this->cockpit->alti_framebuffer);
}

void SCVirtualCockpit::renderSpeed() {
    this->cockpit->speed_framebuffer->fillWithColor(0);
    this->cockpit->RenderSpeedOmetter({0, 0}, this->cockpit->speed_framebuffer);
}

bool SCVirtualCockpit::renderComm() {
    this->cockpit->comm_framebuffer->fillWithColor(0);
    return this->cockpit->RenderCommMessages({0, 13}, this->cockpit->comm_framebuffer);
}

void SCVirtualCockpit::renderMouse() {
    this->mouse_framebuffer->fillWithColor(0);
    this->mouse_framebuffer->line(160, 0, 160, 200, 223);
    this->mouse_framebuffer->line(0, 100, 320, 100, 223);
    this->mouse_framebuffer->rect_slow(1, 1, 319, 199, 223);
    Point2D cursorPos = Mouse.position;
    cursorPos.x -= 4;
    cursorPos.y -= 4;
    Mouse.appearances[SCMouse::CURSOR]->SetPosition(&cursorPos);
    this->mouse_framebuffer->drawShape(Mouse.appearances[SCMouse::CURSOR]);
}

void SCVirtualCockpit::drawPanel(FrameBuffer *fb, const std::vector<Vector3D> &quad) {
    this->drawPanel(fb, this->cockpit_pos, this->cockpit_rot, quad);
}

void SCVirtualCockpit::drawPanel(FrameBuffer *fb, Vector3D position, Vector3D orientation, const std::vector<Vector3D> &quad) {
    if (fb == nullptr || quad.size() != 4) {
        return;
    }
    Texture *texture = new Texture();
    texture->animated = true;
    RSImage image;
    image.name[0] = '\0';
    image.flags = 0;
    image.palette = &this->cockpit->palette;
    image.data = fb->framebuffer;
    image.width = fb->width;
    image.height = fb->height;
    texture->set(&image);
    texture->updateContent(&image);
    Renderer.drawTexturedQuad(position, orientation, quad, texture);
    // The pixels belong to the framebuffer, not to the image.
    image.data = nullptr;
    this->current_textures.push_back(texture);
}

void SCVirtualCockpit::drawTargetCamera(const VirtualCockpitContext &ctx) {
    FrameBuffer *fb = this->cockpit->target_framebuffer;
    if (fb == nullptr || ctx.camera->isUsingCustomMatrices()) {
        return;
    }
    fb->fillWithColor(255);
    this->cockpit->RenderTargetWithCam({0, 0}, fb);

    Vector3D right = ctx.camera->getRight();
    Vector3D up = ctx.camera->getUp();
    Vector3D quadPos = ctx.camera->getPosition() + ctx.camera->getForward() * 8.0f;

    const float width = 4.92f;
    const float height = 3.22f;
    // Compensates the vertical projection offset of the view.
    const float projectionYOffset = -1.5f;
    Vector3D center = quadPos + up * projectionYOffset;

    this->drawPanel(fb, {0, 0, 0}, {0, 0, 0}, {
        center + (up * height) - (right * width),
        center + (up * height) + (right * width),
        center - (up * height) + (right * width),
        center - (up * height) - (right * width)
    });
}
