#pragma once
#include "../strike_commander/precomp.h"

class PSConvPlayer;
class PSConvFrame : public ConvFrame {
public:
    PSConvFrame();
    ~PSConvFrame();

    void parse_GROUP_SHOT(ByteStream *conv);
    void parse_CLOSEUP(ByteStream *conv);
    void parse_CLOSEUP_CONTINUATION(ByteStream *conv) override;
    void parse_YESNOCHOICE_BRANCH1(ByteStream *conv, SCConvPlayer *player) override;
    void parse_YESNOCHOICE_BRANCH2(ByteStream *conv) override;
    void parse_GROUP_SHOT_ADD_CHARACTER(ByteStream *conv) override;
    void parse_GROUP_SHOT_CHARACTER_TALK(ByteStream *conv) override;
    void parse_SHOW_TEXT(ByteStream *conv) override;
    void parse_UNKNOWN(ByteStream *conv) override;
    void parse_CHOOSE_WINGMAN(ByteStream *conv, SCConvPlayer *player) override;
    int SetSentenceFromConv(ByteStream *conv, int start_offset) override;

};
class PSConvPlayer: public SCConvPlayer {
protected:

public:
    PSConvPlayer();
    ~PSConvPlayer() override;
    virtual void ReadNextFrame(void) override;
    virtual void parseConv(ConvFrame *tmp_frame) override;
};
