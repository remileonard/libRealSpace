//
//  Screen->h
//  libRealSpace
//
//  Created by Fabien Sanglard on 1/27/2014.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//

#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include <stdlib.h>
#include <stdio.h>

#ifdef __cplusplus
}
#endif
#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif
#include <GL/gl.h>
#include <SDL.h>
#include <memory>

#include "../commons/Matrix.h"

struct VRStereoEyeRenderInfo {
    int32_t width{0};
    int32_t height{0};
    Matrix projection;
    Matrix view;
};

class RSScreen {
private:
    inline static std::unique_ptr<RSScreen> s_instance{};
public:
    RSScreen() = default;
    virtual ~RSScreen() = default;
    
    static RSScreen& getInstance() {
        if (!RSScreen::hasInstance()) {
            RSScreen::setInstance(std::make_unique<RSScreen>());
        }
        RSScreen& instance = RSScreen::instance();
        return instance;
    };
    static RSScreen& instance() {
        return *s_instance;
    }
    static void setInstance(std::unique_ptr<RSScreen> inst) {
        s_instance = std::move(inst);
    }

    // Optionnel : tester présence
    static bool hasInstance() { return (bool)s_instance; }

    
    virtual void init(int width, int height, bool fullscreen);
    virtual void openScreen(void);
    virtual void setTitle(const char* title);
    virtual void refresh(void);
    virtual void fxTurnOnTv();

    // --- VR stéréo (optionnel) ---
    // Par défaut: non supporté. Les implémentations VR (ex: VRScreen) surchargent.
    virtual uint32_t vrStereoEyeCount() const { return 0; }
    virtual bool vrPrepareStereoFrame(bool& outShouldRender) {
        outShouldRender = false;
        return false;
    }
    virtual bool vrBeginStereoEye(uint32_t /*eye*/,
                                  const Matrix& /*worldFromLocal*/,
                                  float /*metersToWorld*/,
                                  float /*zNear*/,
                                  float /*zFar*/,
                                  VRStereoEyeRenderInfo& /*out*/) {
        return false;
    }
    virtual void vrEndStereoFrame() {}

    SDL_Window* getSDLWindow() const { return m_window; }
    SDL_GLContext getGLContext() const { return m_glContext; }
    
    int32_t width;
    int32_t height;
    int32_t scale;
    bool is_spfx_finished{false};
    bool force_keyboard_capture{false};

protected:
    SDL_Window* m_window{nullptr};
    SDL_GLContext m_glContext{nullptr};
};
