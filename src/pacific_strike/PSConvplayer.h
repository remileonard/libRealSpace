#pragma once
#include "../strike_commander/precomp.h"

class PSConvPlayer;
class PSConvFrame : public ConvFrame {
public:
    PSConvFrame();
    ~PSConvFrame();

    void parse_GROUP_SHOT(ByteStream *conv);
    void parse_CLOSEUP(ByteStream *conv);
    void parse_SHOW_TEXT(ByteStream *conv);
    int SetSentenceFromConv(ByteStream *conv, int start_offset);

};
class PSConvPlayer: public SCConvPlayer {
protected:

public:
    PSConvPlayer();
    ~PSConvPlayer() override;
    virtual void ReadNextFrame(void) override;
    virtual void parseConv(ConvFrame *tmp_frame) override;
};
