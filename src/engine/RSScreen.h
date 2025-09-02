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
#include <Windows.h>
#endif
#include <GL/gl.h>
#include <SDL.h>
#include <memory>

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
    
    int32_t width;
    int32_t height;
    int32_t scale;
    bool is_spfx_finished{false};
};
