//
//  R4Strike.h
//  libRealSpace
//
//  The view of a flight on R4nkine. The graph belongs to the mission (R4Mission, made by createMission); R4Strike runs its APP step
//  once a frame, after the camera, and puts it in the pass of the renderer with the effects (smoke, explosions, tracers) and the
//  cockpit of the game.
//
#pragma once

#include "R4Mission.h"

class R4Strike : public SCStrike {
public:
    R4Strike();
    ~R4Strike() override;
    void runFrame() override;

protected:
    SCMission *createMission(char const *missionName) override;
    void afterCameraPlaced() override;
    void renderScene(int32_t viewportW, int32_t viewportH, float projVerticalOffset, bool forceVirtualCockpit) override;
    void renderWorld() override;
    void renderPlayerPlane() override;

private:
    // the mission of the flight when it is a R4Mission, else null
    R4Mission *mission() const;
    bool playerPlaneShown() const;
};
