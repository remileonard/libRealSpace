#pragma once
#include "../../strike_commander/precomp.h"


class DebugEmptyActivity : public IActivity {
public:
    DebugEmptyActivity();
    ~DebugEmptyActivity();
    void init(void) override;
    void runFrame(void) override;
    void renderMenu(void) override;
    void renderUI(void) override;

private:
    
};