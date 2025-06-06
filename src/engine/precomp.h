#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
    
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
    #include <string.h>
    #include <vector>
    #include <stack>
    #include <map>
    #include <math.h>
    #include <stdint.h>
    #include <float.h>
    #include <algorithm> 

#ifndef _WIN32
    #include <unistd.h>
#endif
#ifdef _WIN32
	#include <Windows.h>
#endif

    #include <SDL.h>
    #include <GL/gl.h>
    #include <GL/glu.h>

    #include "../commons/Maths.h"
    #include "../commons/Matrix.h"
    #include "../commons/Quaternion.h"
    #include "Camera.h"
    #include "Texture.h"
    

    #include "../realspace/AssetManager.h"
    #include "../realspace/RLEShape.h"
    #include "../realspace/RSFont.h"
    #include "../realspace/RSMusic.h"

    #include "RSMixer.h"
    #include "RSScreen.h"
    #include "FrameBuffer.h"
    #include "RSVGA.h"
#endif

