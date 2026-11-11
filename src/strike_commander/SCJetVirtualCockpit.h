//
//  SCJetVirtualCockpit.h
//  libRealSpace
//
//  Virtual cockpit of the Strike Commander jets (F-16, F-22): same instruments,
//  only their position in the cockpit changes.
//
#pragma once
#ifndef __libRealSpace__SCJetVirtualCockpit__
#define __libRealSpace__SCJetVirtualCockpit__

#include "precomp.h"

/**
 * @brief Instrument quads in the cockpit frame. An empty quad means the
 *        instrument is not displayed in this cockpit.
 */
struct JetCockpitLayout {
    std::vector<Vector3D> hud;
    std::vector<Vector3D> mfd_right;
    std::vector<Vector3D> mfd_left;
    std::vector<Vector3D> raws;
    std::vector<Vector3D> alti;
    std::vector<Vector3D> speed;
    std::vector<Vector3D> comm;
    std::vector<Vector3D> mouse;
};

class SCJetVirtualCockpit : public SCVirtualCockpit {
public:
    static const JetCockpitLayout &f16Layout();
    static const JetCockpitLayout &f22Layout();

    SCJetVirtualCockpit(SCCockpit *cockpit, const JetCockpitLayout &layout);

protected:
    const JetCockpitLayout &layout;
    void drawInstruments(const VirtualCockpitContext &ctx) override;
};

#endif
