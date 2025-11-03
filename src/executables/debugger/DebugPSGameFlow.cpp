#include "DebugPSConvPlayer.h"
#include "DebugPSGameFlow.h"
#include "DebugPacificStrikeMISN.h"
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
void DebugPSGameFlow::playMapShot(std::vector<MAP_POINT *> *points) {}
DebugPSGameFlow::DebugPSGameFlow() { printf("DebugPSGameFlow constructor\n"); }

DebugPSGameFlow::~DebugPSGameFlow() {}

void DebugPSGameFlow::renderUI() {
    DebugGameFlow::renderUI();
    renderPopups();
}

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
void DebugPSGameFlow::playMidGame(uint8_t midGameId) {
    m_showMidGamePopup = true;
}

void DebugPSGameFlow::playShowMap(uint8_t mapId) {
    m_showShowMapPopup = true;
}

void DebugPSGameFlow::playEndMission() {
    if (std::string(this->missionToFly).length() > 0) {
        DebugPacificStrikeMISN *fly = new DebugPacificStrikeMISN();
        fly->init();
        fly->setMission(this->missionToFly);
        this->missionToFly = nullptr;
        fly_mission.push(static_cast<DebugStrike*>(fly));   
    }
    this->next_miss = GameState.mission_id;
}

bool DebugPSGameFlow::playFlyMission(uint8_t flyMissionId) {
    uint8_t flymID = flyMissionId;
    GameState.mission_flyed = flymID;
    this->missionToFly = (char *)malloc(13);
    sprintf(this->missionToFly, "%s.IFF", this->gameFlowParser.game.mlst->data[flymID]->c_str());
    strtoupper(this->missionToFly, this->missionToFly);
    return true;
}

bool DebugPSGameFlow::playFlyMission2(uint8_t flyMissionId) {
    m_showFlyMission2Popup = true;
    return false;
}

void DebugPSGameFlow::lookAtLedger() {
    m_showLedgerPopup = true;
}

void DebugPSGameFlow::lookAtKillBoard() {
    m_showKillBoardPopup = true;
}

void DebugPSGameFlow::viewCatalog() {
    m_showCatalogPopup = true;
}

void DebugPSGameFlow::flyMission(uint8_t missionid) {
    
    DebugPacificStrikeMISN *fly = new DebugPacificStrikeMISN();
    fly->init();
    fly->setMission(this->missionToFly);
    this->missionToFly = nullptr;
    fly_mission.push(static_cast<DebugStrike*>(fly));
    this->next_miss = GameState.mission_id;
}

// Nouvelle méthode à appeler dans votre boucle de rendu principale
void DebugPSGameFlow::renderPopups() {
    if (m_showMidGamePopup) {
        ImGui::OpenPopup("Not Implemented##MidGame");
        m_showMidGamePopup = false;
    }
    if (ImGui::BeginPopupModal("Not Implemented##MidGame", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("MidGame playback is not implemented in DebugPSGameFlow yet.");
        if (ImGui::Button("OK")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (m_showShowMapPopup) {
        ImGui::OpenPopup("Not Implemented##ShowMap");
        m_showShowMapPopup = false;
    }
    if (ImGui::BeginPopupModal("Not Implemented##ShowMap", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("ShowMap playback is not implemented in DebugPSGameFlow yet.");
        if (ImGui::Button("OK")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (m_showEndMissionPopup) {
        ImGui::OpenPopup("Not Implemented##EndMission");
        m_showEndMissionPopup = false;
    }
    if (ImGui::BeginPopupModal("Not Implemented##EndMission", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("EndMission playback is not implemented in DebugPSGameFlow yet.");
        if (ImGui::Button("OK")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (m_showFlyMissionPopup) {
        ImGui::OpenPopup("Not Implemented##FlyMission");
        m_showFlyMissionPopup = false;
    }
    if (ImGui::BeginPopupModal("Not Implemented##FlyMission", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("FlyMission playback is not implemented in DebugPSGameFlow yet.");
        if (ImGui::Button("OK")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (m_showFlyMission2Popup) {
        ImGui::OpenPopup("Not Implemented##FlyMission2");
        m_showFlyMission2Popup = false;
    }
    if (ImGui::BeginPopupModal("Not Implemented##FlyMission2", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("FlyMission2 playback is not implemented in DebugPSGameFlow yet.");
        if (ImGui::Button("OK")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (m_showLedgerPopup) {
        ImGui::OpenPopup("Not Implemented##Ledger");
        m_showLedgerPopup = false;
    }
    if (ImGui::BeginPopupModal("Not Implemented##Ledger", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Ledger viewing is not implemented in DebugPSGameFlow yet.");
        if (ImGui::Button("OK")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (m_showKillBoardPopup) {
        ImGui::OpenPopup("Not Implemented##KillBoard");
        m_showKillBoardPopup = false;
    }
    if (ImGui::BeginPopupModal("Not Implemented##KillBoard", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("KillBoard viewing is not implemented in DebugPSGameFlow yet.");
        if (ImGui::Button("OK")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (m_showCatalogPopup) {
        ImGui::OpenPopup("Not Implemented##Catalog");
        m_showCatalogPopup = false;
    }
    if (ImGui::BeginPopupModal("Not Implemented##Catalog", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Catalog viewing is not implemented in DebugPSGameFlow yet.");
        if (ImGui::Button("OK")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}