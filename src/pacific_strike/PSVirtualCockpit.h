#pragma once
#include "../strike_commander/precomp.h"
#include "PSCockpit.h"

class PSVirtualCockpit : public SCVirtualCockpit {
public:
    explicit PSVirtualCockpit(PSCockpit *cockpit);
    ~PSVirtualCockpit() override;

protected:
    PSCockpit *ps_cockpit{ nullptr };
    std::unordered_map<PSCockpitInstrument *, FrameBuffer *> instrument_framebuffers;
    bool traced{ false };

    Vector3D modelOffset(const VirtualCockpitContext &ctx);
    void drawModel(const VirtualCockpitContext &ctx) override;
    void updateInstruments(const VirtualCockpitContext &ctx) override;
};
