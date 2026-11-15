#include "EnhancedVirtualCockpit.h"
#include "EnhancedRenderer.h"
#include "../engine/Config.hpp"

EnhancedVirtualCockpit::EnhancedVirtualCockpit(SCCockpit *cockpit, const JetCockpitLayout &layout)
    : SCJetVirtualCockpit(cockpit, layout) {
    this->metal = Config::getInstance().getString("Video", "cockpit_metal", "0") == "1";
    printf("[enhanced] cockpit metal detail: %s\n", this->metal ? "on" : "off");
}

void EnhancedVirtualCockpit::drawModel(const VirtualCockpitContext &ctx) {
    if (!this->metal) {
        SCJetVirtualCockpit::drawModel(ctx);
        return;
    }
    EnhancedRenderer::setMetalDetail(true);
    SCJetVirtualCockpit::drawModel(ctx);
    EnhancedRenderer::setMetalDetail(false);
}
