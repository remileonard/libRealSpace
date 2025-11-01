#pragma once
#include "../strike_commander/precomp.h"

class PSGameFlowParser : public RSGameFlow {
protected:
    void parseMISS_EFCT(uint8_t* data, size_t size) override;
    void parseMISS(uint8_t* data, size_t size) override;
    void parseGAME(uint8_t* data, size_t size) override;
public:
    void InitFromRam(uint8_t* data, size_t size) override;
    PSGameFlowParser();
    ~PSGameFlowParser();
};