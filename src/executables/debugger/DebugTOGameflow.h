#pragma once
#include "../../tactical_operation/TOGameflow.h"
#include "DebugGameFlow.h"

class DebugTOGameFlow : public DebugGameFlow, public TOGameFlow {
public:
    DebugTOGameFlow();
    ~DebugTOGameFlow();

protected:
    // Conversations de Tactical Operations (TOConvPlayer), sans la fenêtre de DebugConvPlayer.
    void playConv(uint8_t convId) override;
};