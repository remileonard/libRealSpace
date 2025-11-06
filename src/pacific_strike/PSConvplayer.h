#pragma once
#include "../strike_commander/precomp.h"

class PSConvPlayer;
class PSConvFrame : public ConvFrame {
public:
    PSConvFrame();
    ~PSConvFrame();

    void parse_GROUP_SHOT(ByteStream *conv);
    void parse_CLOSEUP(ByteStream *conv);
    int SetSentenceFromConv(ByteStream *conv, int start_offset);

};
class PSConvPlayer: public virtual SCConvPlayer {
protected:
    void SetArchive(PakEntry *convPakEntry) override; 
public:
    PSConvPlayer();
    ~PSConvPlayer() override;
};
