#pragma once
#include "../strike_commander/precomp.h"
#include "PSCockpit.h"

// Cockpit 3D de Pacific Strike : l'objet du FORM REAL du fichier de cockpit, placé par son chunk INFO, et l'image de
// chaque instrument posée au point de son chunk 3SPC (CockpitInstrument_DrawIn3D_185B9 et sub_237B0 de PACIFIC.EXE).
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
