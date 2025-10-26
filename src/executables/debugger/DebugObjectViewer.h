#pragma once
#include "../../strike_commander/precomp.h"

class DebugObjectViewer : public SCObjectViewer {
private:
    std::vector<Vector3D> vertices;
    bool moovingLight{true};
    bool autoRotate{true};
    int lodLevel{0};
public:
    DebugObjectViewer();
    ~DebugObjectViewer();

    void runFrame(void) override;
    void renderMenu() override;
    void renderUI() override;
};