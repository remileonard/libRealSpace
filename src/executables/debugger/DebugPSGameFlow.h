#pragma once
#include "../../strike_commander/precomp.h"
#include "DebugGameFlow.h"

class DebugPSGameFlow : public DebugGameFlow {
private:
    // Ajoutez ces membres pour gérer l'état des popups
    bool m_showMidGamePopup = false;
    bool m_showShowMapPopup = false;
    bool m_showEndMissionPopup = false;
    bool m_showFlyMissionPopup = false;
    bool m_showFlyMission2Popup = false;
    bool m_showLedgerPopup = false;
    bool m_showKillBoardPopup = false;
    bool m_showCatalogPopup = false;
    void renderPopups();
    
protected:
    void playMapShot(std::vector<MAP_POINT *> *points) override;
    void flyMission(uint8_t missionid) override;
    void playShot(uint8_t shotId) override;
    void playConv(uint8_t convId) override;
    void playMidGame(uint8_t midGameId) override;
    void playShowMap(uint8_t mapId) override;
    void playEndMission() override;
    bool playFlyMission(uint8_t flyMissionId) override;
    bool playFlyMission2(uint8_t flyMissionId) override;
    void lookAtLedger() override;
    void lookAtKillBoard() override;
    void viewCatalog() override;
    void renderConvPlayer() override;
    void renderUI() override;
public:
    void init() override;
    DebugPSGameFlow();
    ~DebugPSGameFlow();
};
