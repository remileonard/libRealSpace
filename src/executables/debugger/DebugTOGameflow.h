#pragma once
#include "../../tactical_operation/TOGameflow.h"
#include "DebugGameFlow.h"

class DebugTOGameFlow : public DebugGameFlow, public TOGameFlow {
public:
    DebugTOGameFlow();
    ~DebugTOGameFlow();
};