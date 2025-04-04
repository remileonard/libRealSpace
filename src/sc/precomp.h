//
//  precomp.h
//  libRealSpace
//
//  Created by Fabien Sanglard on 1/25/2014.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//

#ifndef libRealSpace_precomp_h
#define libRealSpace_precomp_h

//libspace include
#include "../precomp.h"

#include "SCenums.h"
#include "PaletteIDs.h"
#include "ShapeIDs.h"



// Core of the game
#include "SCState.h"
#include "SCMouse.h"
#include "SCButton.h"
#include "SCZone.h"
#include "IActivity.h"
#include "GameEngine.h"
#include "ConvAssetManager.h"
#include "main.h"

// Simulation
#include "SCSmokeSet.h"
#include "SCSimulatedObject.h"
#include "SCPlane.h"
#include "SCPilot.h"
#include "SCMissionActors.h"
#include "SCMission.h"
#include "SCProg.h"

#include "SCFileRequester.h"

//Activities
#include "SCConvPlayer.h"
#include "SCAnimationPlayer.h"
#include "SCRegister.h"

#include "SCNavMap.h"
#include "SCCockpit.h"
#include "SCStrike.h"
#include "SCObjectViewer.h"
#include "SCDogFightMenu.h"
#include "SCTrainingMenu.h"
#include "SCMainMenu.h"
#include "SCShot.h"
#include "SCScene.h"
#include "SCGameFlow.h"

#endif
