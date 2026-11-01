//
//  Screen->h
//  libRealSpace
//
//  Created by Fabien Sanglard on 1/27/2014.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//

#pragma once
#include "../engine/RSScreen.h"

class EnhancedScreen : public RSScreen {
    void init(int width, int height, bool fullscreen) override;
    
};
