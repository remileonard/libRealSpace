//
//  SCVirtualCockpit.h
//  libRealSpace
//
//  3D (virtual) cockpit rendering: draws the cockpit model and maps the
//  SCCockpit instrument framebuffers onto quads placed in the cockpit frame.
//
#pragma once
#ifndef __libRealSpace__SCVirtualCockpit__
#define __libRealSpace__SCVirtualCockpit__

#include "precomp.h"

/**
 * @brief Per-frame data provided by the activity to render the virtual cockpit.
 */
struct VirtualCockpitContext {
    Camera *camera{nullptr};
    SCPlane *plane{nullptr};
    int eye_y{0};
    Vector2D gunsight_hud_offset{0.0f, 0.0f};
    bool mouse_control{false};
};

/**
 * @brief Base class for the 3D cockpits.
 *
 * SCCockpit fills the instrument framebuffers (HUD, MFDs, ...), a virtual
 * cockpit decides where those instruments are displayed in 3D. Each aircraft
 * family provides its own subclass by implementing drawInstruments().
 */
class SCVirtualCockpit {
public:
    explicit SCVirtualCockpit(SCCockpit *cockpit);
    virtual ~SCVirtualCockpit();

    void render(const VirtualCockpitContext &ctx);
    void releaseTextures();

protected:
    SCRenderer &Renderer = SCRenderer::getInstance();
    SCMouse &Mouse = SCMouse::getInstance();

    SCCockpit *cockpit{nullptr};
    FrameBuffer *mouse_framebuffer{nullptr};
    // Cockpit frame used by drawPanel(), valid during render().
    Vector3D cockpit_pos{0.0f, 0.0f, 0.0f};
    Vector3D cockpit_rot{0.0f, 0.0f, 0.0f};

    virtual void drawModel(const VirtualCockpitContext &ctx);
    virtual void drawInstruments(const VirtualCockpitContext &ctx) = 0;

    // Instrument framebuffer fillers, shared by the cockpit implementations.
    void renderHUD(const VirtualCockpitContext &ctx);
    void renderMFDRight();
    void renderMFDLeft();
    void renderRAWS();
    void renderAlti();
    void renderSpeed();
    bool renderComm();
    void renderMouse();

    // Uploads fb as a texture and draws it on quad, given in the cockpit frame.
    void drawPanel(FrameBuffer *fb, const std::vector<Vector3D> &quad);
    void drawPanel(FrameBuffer *fb, Vector3D position, Vector3D orientation, const std::vector<Vector3D> &quad);
    // Target camera view, drawn as a billboard in front of the camera.
    void drawTargetCamera(const VirtualCockpitContext &ctx);

private:
    // Textures are kept one extra render so the GPU is done with them before deletion.
    std::vector<Texture *> previous_textures;
    std::vector<Texture *> current_textures;
};

#endif
