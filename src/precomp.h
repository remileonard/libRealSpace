//
//  precomp.h
//  iff
//
//  Created by Fabien Sanglard on 12/27/2013.
//  Copyright (c) 2013 Fabien Sanglard. All rights reserved.
//

#ifndef iff_precomp_h
#define iff_precomp_h

#include "block_def.h"

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

    #include "Maths.h"
    #include "Matrix.h"
    #include "Quaternion.h"


    // Reusable Realspace Classes
    #include "Base.h"
    #include "LZBuffer.h"
    #include "ByteStream.h"
    #include "PakArchive.h"
    #include "TreArchive.h"

    #include "AssetManager.h"
    
    #include "Camera.h"
    #include "Texture.h"
    #include "RSImage.h"

//Fonts stuff
    #include "RSFont.h"
    #include "RSFontManager.h"
    #include "RSImageSet.h"
    #include "SCRenderer.h"
    #include "RSPalette.h"
    #include "RSSmokeSet.h"
    #include "RSEntity.h"
    #include "RSProf.h"
    #include "RSMusic.h"
    #include "RSSound.h"
    #include "RSMapTextureSet.h"
    #include "RSArea.h"
    #include "RLEShape.h"
    #include "RSOption.h"
    #include "RSGameflow.h"
    #include "RSHud.h"
    #include "RSCockpit.h"
    #include "RSNavMap.h"
    #include "RSWorld.h"
    #include "RSMission.h"
    
    // Music Stuff
    #include "RSMusic.h"
    #include "RSMixer.h"
    
    //VGA and Glide
    #include "RSScreen.h"
    #include "FrameBuffer.h"
    #include "RSVGA.h"
    extern SCRenderer Renderer;
#endif

#endif
