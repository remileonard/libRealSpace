#pragma once
#include "../strike_commander/precomp.h"

class PSGameFlowParser : public RSGameFlow {
protected:
    void parseOpCode(uint8_t *data, size_t size, std::vector<EFCT *>* efct_list) override;
public:
    PSGameFlowParser();
    ~PSGameFlowParser();
};