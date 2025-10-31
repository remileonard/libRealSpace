#include "DebugPSConvPlayer.h"
#include "DebugPSGameFlow.h"
#include <imgui.h>
#include <imgui_impl_opengl2.h>
#include <imgui_impl_sdl2.h>

void DebugPSGameFlow::playConv(uint8_t convId) {
    SCConvPlayer *conv = new DebugPSConvPlayer();
    conv->init();
    conv->SetID(convId);
    DebugGameFlow::convs.push(conv);
}
DebugPSGameFlow::DebugPSGameFlow() {
    printf("DebugPSGameFlow constructor\n");
}

DebugPSGameFlow::~DebugPSGameFlow() {}


void DebugPSGameFlow::renderConvPlayer() {
    ImGui::Begin("Convert Player", &conv_player);
    static ImGuiComboFlags flags = 0;
    if (ImGui::BeginCombo("List des conversations", nullptr, flags)) {
        for (int i = 0; i < 313; i++) {
            if (ImGui::Selectable(std::to_string(i).c_str(), false)) {
                SCConvPlayer *conv = new DebugPSConvPlayer();
                conv->init();
                conv->SetID(i);
                DebugGameFlow::convs.push(conv);
            }
        }
        ImGui::EndCombo();
    }
    ImGui::End();
}