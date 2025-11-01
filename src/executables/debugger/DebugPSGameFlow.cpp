#include "DebugPSConvPlayer.h"
#include "DebugPSGameFlow.h"
#include "../pacific_strike/PSGameFlowParser.h"
#include <imgui.h>
#include <imgui_impl_opengl2.h>
#include <imgui_impl_sdl2.h>

void DebugPSGameFlow::init() {
    TreEntry *gameflowIFF = Assets.GetEntryByName(Assets.gameflow_filename);
    TreEntry *optionIFF = Assets.GetEntryByName(Assets.option_filename);
    this->optionParser.InitFromRam(optionIFF->data, optionIFF->size);
    PSGameFlowParser psParser;
    psParser.InitFromRam(gameflowIFF->data, gameflowIFF->size);
    this->gameFlowParser = psParser;
    //this->gameFlowParser.InitFromRam(gameflowIFF->data, gameflowIFF->size);

    TreEntry *optShapEntry = Assets.GetEntryByName(Assets.optshps_filename);
    this->optShps.InitFromRAM("OPTSHPS.PAK", optShapEntry->data, optShapEntry->size);
    TreEntry *optPalettesEntry = Assets.GetEntryByName(Assets.optpals_filename);
    this->optPals.InitFromRAM("OPTPALS.PAK", optPalettesEntry->data, optPalettesEntry->size);
    this->efect = nullptr;
    this->createMiss();
}
void DebugPSGameFlow::playConv(uint8_t convId) {
    SCConvPlayer *conv = new DebugPSConvPlayer();
    conv->init();
    conv->SetID(convId);
    DebugGameFlow::convs.push(conv);
}
void DebugPSGameFlow::playShot(uint8_t shotId) {}
void DebugPSGameFlow::flyMission() {}
DebugPSGameFlow::DebugPSGameFlow() { printf("DebugPSGameFlow constructor\n"); }

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

void DebugPSGameFlow::playMapShot(std::vector<MAP_POINT *> *points) {}
