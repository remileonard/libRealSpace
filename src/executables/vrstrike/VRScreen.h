//
//  Screen->h
//  libRealSpace
//
//  Created by Fabien Sanglard on 1/27/2014.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//

#pragma once
#include "../../engine/RSScreen.h"
#ifdef __cplusplus
extern "C" {
#endif
#include <stdlib.h>
#include <stdio.h>

#ifdef __cplusplus
}
#endif
#ifdef _WIN32
#include <Windows.h>
#endif
#include <GL/gl.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_opengl_glext.h>
#include <vector>

#include "../../commons/Matrix.h"

// OpenXR: types Win32 + OpenGL
#if defined(_WIN32)
#ifndef XR_USE_PLATFORM_WIN32
#define XR_USE_PLATFORM_WIN32
#endif
#ifndef XR_USE_GRAPHICS_API_OPENGL
#define XR_USE_GRAPHICS_API_OPENGL
#endif
#endif

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

class VRScreen: public RSScreen{
private:
    // --- OpenXR state ---
    XrInstance m_xrInstance{XR_NULL_HANDLE};
    XrSystemId m_xrSystemId{XR_NULL_SYSTEM_ID};
    XrSession m_xrSession{XR_NULL_HANDLE};
    XrSpace m_xrAppSpace{XR_NULL_HANDLE};
    XrSpace m_xrViewSpace{XR_NULL_HANDLE};
    XrSessionState m_sessionState{XR_SESSION_STATE_UNKNOWN};
    bool m_sessionRunning{false};

    XrEnvironmentBlendMode m_environmentBlendMode{XR_ENVIRONMENT_BLEND_MODE_OPAQUE};

    // "Écran cinéma" : pose fixe (en XR_REFERENCE_SPACE_TYPE_LOCAL)
    bool m_cinemaPoseValid{false};
    XrPosef m_cinemaPose{{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, -1.2f}};

    PFN_xrGetOpenGLGraphicsRequirementsKHR m_xrGetOpenGLGraphicsRequirementsKHR{nullptr};

    enum class PresentMode {
        CinemaQuad,
        StereoProjection,
    };
    PresentMode m_presentMode{PresentMode::CinemaQuad};

    struct Swapchain {
        XrSwapchain handle{XR_NULL_HANDLE};
        int32_t width{0};
        int32_t height{0};
        std::vector<XrSwapchainImageOpenGLKHR> images;
    };
    std::vector<XrViewConfigurationView> m_viewConfigs;
    std::vector<XrView> m_views;
    // [0] = swapchain QUAD (cinéma). Les swapchains stéréo sont séparés.
    std::vector<Swapchain> m_swapchains;
    std::vector<Swapchain> m_projectionSwapchains;

    bool m_hasGlFbo{false};

    // Rendu stéréo: FBO + depth partagés.
    GLuint m_stereoFbo{0};
    GLuint m_stereoDepthRb{0};
    int32_t m_stereoDepthW{0};
    int32_t m_stereoDepthH{0};

    // État de frame stéréo (wait/begin effectué dans prepareStereoFrame).
    bool m_stereoFrameBegun{false};
    bool m_stereoShouldRender{false};
    XrFrameState m_stereoFrameState{XR_TYPE_FRAME_STATE};
    std::vector<uint32_t> m_stereoAcquiredImageIndex;
    std::vector<bool> m_stereoImageAcquired;

    bool m_projectionLayerReady{false};
    XrCompositionLayerProjection m_projectionLayer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    std::vector<XrCompositionLayerProjectionView> m_projectionViews;

    int64_t m_colorSwapchainFormat{0};

    void pollXrEvents();
    bool initOpenXR();
    void shutdownOpenXR();
    void ensureSessionRunning();

    void ensureStereoFbo(int32_t w, int32_t h);

    static Matrix makeProjectionFromFov(const XrFovf& fov, float zNear, float zFar);
    static Matrix makeTransformFromPose(const XrPosef& pose, float metersToWorld);
    static Matrix mul(const Matrix& a, const Matrix& b);
    static Matrix invertRigidBody(const Matrix& m);

public:
    
    VRScreen();
    ~VRScreen() override;

    void init(int width, int height, bool fullscreen) override;
    void setTitle(const char* title) override;
    void refresh(void) override;

    void setPresentMode(PresentMode mode) { m_presentMode = mode; }
    PresentMode presentMode() const { return m_presentMode; }

    uint32_t vrStereoEyeCount() const override { return m_hasGlFbo ? (uint32_t)m_viewConfigs.size() : 0u; }
    bool vrPrepareStereoFrame(bool& outShouldRender) override;
    bool vrBeginStereoEye(uint32_t eye,
                          const Matrix& worldFromLocal,
                          float metersToWorld,
                          float zNear,
                          float zFar,
                          VRStereoEyeRenderInfo& out) override;
    void vrEndStereoFrame() override;

    // Stéréo: à appeler depuis l'activité (ex: SCStrike) avant de rendre les deux yeux.
    // refresh() se chargera du xrEndFrame correspondant.
    bool prepareStereoFrame();
    bool stereoShouldRender() const { return m_stereoFrameBegun && m_stereoShouldRender; }
    int32_t stereoEyeWidth(uint32_t eye) const;
    int32_t stereoEyeHeight(uint32_t eye) const;
    bool beginStereoEye(uint32_t eye,
                        const Matrix& worldFromLocal,
                        float metersToWorld,
                        float zNear,
                        float zFar,
                        Matrix& outProjection,
                        Matrix& outView);
    void endStereoFrame();
};
