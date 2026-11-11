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
 * family provides its own subclass by implementing updateInstruments().
 *
 * The instruments are updated once per frame (first render() after
 * beginFrame()) and their GPU textures are reused, so rendering the cockpit
 * for several views (VR eyes) only draws the geometry again.
 */
class SCVirtualCockpit {
public:
    explicit SCVirtualCockpit(SCCockpit *cockpit);
    virtual ~SCVirtualCockpit();

    // Call once per frame, before the first render().
    void beginFrame();
    void render(const VirtualCockpitContext &ctx);
    void releaseTextures();

protected:
    SCRenderer &Renderer = SCRenderer::getInstance();
    SCMouse &Mouse = SCMouse::getInstance();

    SCCockpit *cockpit{nullptr};
    FrameBuffer *mouse_framebuffer{nullptr};
    // Cockpit frame of the panels, valid during render().
    Vector3D cockpit_pos{0.0f, 0.0f, 0.0f};
    Vector3D cockpit_rot{0.0f, 0.0f, 0.0f};

    virtual void drawModel(const VirtualCockpitContext &ctx);
    // Fills the instrument framebuffers and declares the panels to draw with addPanel().
    virtual void updateInstruments(const VirtualCockpitContext &ctx) = 0;

    // Instrument framebuffer fillers, shared by the cockpit implementations.
    void renderHUD(const VirtualCockpitContext &ctx);
    void renderMFDRight();
    void renderMFDLeft();
    void renderRAWS();
    void renderAlti();
    void renderSpeed();
    bool renderComm();
    void renderMouse();

    // Uploads fb to its texture and draws it this frame on quad, given in the cockpit frame.
    void addPanel(FrameBuffer *fb, const std::vector<Vector3D> &quad);
    // Target camera view, drawn as a billboard in front of the camera.
    void addTargetCamera(const VirtualCockpitContext &ctx);

private:
    enum class PanelSpace { COCKPIT, CAMERA_BILLBOARD };
    struct Panel {
        Texture *texture;
        PanelSpace space;
        std::vector<Vector3D> quad;
    };
    std::unordered_map<FrameBuffer *, Texture *> textures;
    std::vector<Panel> panels;
    bool instruments_dirty{true};

    Texture *uploadFramebuffer(FrameBuffer *fb);
    void drawPanel(const Panel &panel, const VirtualCockpitContext &ctx);
};

#endif
