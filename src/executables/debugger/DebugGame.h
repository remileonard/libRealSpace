#pragma once


#include <stack>
#include <vector>
#include <string>
#include <SDL.h>
#include "../../commons/Maths.h"
#include "../../engine/GameEngine.h"
#include "../../engine/SCMouse.h"
#include "../../engine/RSScreen.h"
#include "../../engine/RSVGA.h"
#include "../../engine/SCRenderer.h"
#include "../../engine/IActivity.h"
#include "../../engine/Loader.h"
#include "../../strike_commander/SCState.h"

class DebugGame : public GameEngine {

public:
    DebugGame();
    ~DebugGame();

    void init(void) override;
    void run(void) override;
    void loadSC();
    void loadSCCD();
    void loadTO();
    void loadPacific();
    void testMissionSC();
    void testObjects();
    void testController();
};
