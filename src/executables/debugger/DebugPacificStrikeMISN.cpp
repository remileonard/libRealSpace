#include "DebugPacificStrikeMISN.h"
#include <imgui.h>
#include <imgui_impl_opengl2.h>
#include <imgui_impl_sdl2.h>

DebugPacificStrikeMISN::DebugPacificStrikeMISN() {

}

DebugPacificStrikeMISN::~DebugPacificStrikeMISN() {
    
}

void DebugPacificStrikeMISN::setMission(std::string mission_name) { 
    PSStrike::setMission(mission_name.c_str());
    DebugStrike::target = nullptr;
    DebugStrike::current_mission = PSStrike::current_mission;
    DebugStrike::player_plane = PSStrike::player_plane;
    DebugStrike::cockpit = PSStrike::cockpit;
    DebugStrike::area = PSStrike::area;
    DebugStrike::camera = PSStrike::camera;
    
}

void DebugPacificStrikeMISN::renderMissionLoadingDialog(bool &show_mission) {
    ImGui::Begin("Mission", &show_mission);
    std::vector<const char *> ps_mission_list = {
        "CRLS-M1.IFF", "CRLS-M2.IFF", "CRLS-M3.IFF", "CRLS-M4.IFF", "CRLS-ML.IFF", "ENDG-M1.IFF",
        "ENDG-M2.IFF", "GDLC-M1.IFF", "GDLC-M2.IFF", "GDLC-M3.IFF", "GDLC-M4.IFF", "GDLC-ML.IFF",
        "IJOK-M1.IFF", "IJOK-M2.IFF", "IJOK-M3.IFF", "IJOK-M4.IFF", "INST-M1.IFF", "INST-M1J.IFF",
        "INST-M2.IFF", "INST-M2J.IFF", "INST-MG.IFF", "INST-MGJ.IFF", "LOSE-M1A.IFF", "LOSE-M1B.IFF",
        "LOSE-M1C.IFF", "LOSE-M2A.IFF", "LOSE-M2B.IFF", "LOSE-M2C.IFF", "LYTG-M1.IFF", "LYTG-M2.IFF",
        "LYTG-M3.IFF", "LYTG-M4.IFF", "LYTG-ML.IFF",
        "MDWY-M1.IFF", "MDWY-M2.IFF", "MDWY-M3.IFF", "MDWY-M4.IFF", "MDWY-ML.IFF",
        "PRLH-M1.IFF", "PRLH-M2.IFF", "PRLH-M3.IFF",
        "SLMN-M1.IFF", "SLMN-M2.IFF", "SLMN-M3.IFF", "SLMN-M4.IFF", "SLMN-ML.IFF",
    };
    static ImGuiComboFlags flags = 0;
    static int ps_mission_idx = 0;
    ImGui::PushItemWidth(120.0f);
    if (ImGui::BeginCombo("Pacific Strike", ps_mission_list[ps_mission_idx], flags)) {
        int n = 0;
        for (auto ps_mission : ps_mission_list) {
            const bool is_selected = (ps_mission_idx == n);
            if (ImGui::Selectable(ps_mission, is_selected))
                ps_mission_idx = n;
            if (is_selected)
                ImGui::SetItemDefaultFocus();
            n++;
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine(300);
    if (ImGui::Button("Load PS Mission")) {
        DebugStrike::Assets.navmap_filename = "..\\..\\DATA\\COCKPITS\\NAVMAP.IFF";
        DebugStrike::Assets.navmap_add_filename = "";
        this->setMission((char *)ps_mission_list[ps_mission_idx]);
    }
    ImGui::PopItemWidth();
    ImGui::End();
}
