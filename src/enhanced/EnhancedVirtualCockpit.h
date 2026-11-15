#pragma once
#include "../strike_commander/precomp.h"
#include "../strike_commander/SCJetVirtualCockpit.h"

/**
 * 3D cockpit of the jets with an optional brushed metal detail on the flat grey surfaces of the cockpit model
 * ([Video] cockpit_metal = 1, see EnhancedRenderer::setMetalDetail).
 */
class EnhancedVirtualCockpit : public SCJetVirtualCockpit {
public:
    EnhancedVirtualCockpit(SCCockpit *cockpit, const JetCockpitLayout &layout);

protected:
    void drawModel(const VirtualCockpitContext &ctx) override;

private:
    bool metal{false};
};
