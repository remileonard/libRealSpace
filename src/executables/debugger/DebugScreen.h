//
//  Screen->h
//  libRealSpace
//
//  Created by Fabien Sanglard on 1/27/2014.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//

#pragma once
#include "../../engine/RSScreen.h"
#include "../../engine/GameEngine.h"
#include "../../engine/RSVGA.h"
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

class DebugScreen: public RSScreen{
private:
    GLuint screen_texture;
    bool show_ui{false};
    bool show_mouse{true};
public:
    
    DebugScreen();
    ~DebugScreen() override;
    
    void init(int width, int height, bool fullscreen) override;
    void setTitle(const char* title) override;
    void refresh(void) override;
};
