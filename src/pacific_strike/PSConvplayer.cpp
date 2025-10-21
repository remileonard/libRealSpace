#include "PSConvplayer.h"

PSConvFrame::PSConvFrame() {}

PSConvFrame::~PSConvFrame() {}

void PSConvFrame::parse_GROUP_SHOT(ByteStream *conv) {}

void PSConvFrame::parse_CLOSEUP(ByteStream *conv) {}

void PSConvFrame::parse_CLOSEUP_CONTINUATION(ByteStream *conv) {}

void PSConvFrame::parse_YESNOCHOICE_BRANCH1(ByteStream *conv, SCConvPlayer *player) {}

void PSConvFrame::parse_YESNOCHOICE_BRANCH2(ByteStream *conv) {}

void PSConvFrame::parse_GROUP_SHOT_ADD_CHARACTER(ByteStream *conv) {}

void PSConvFrame::parse_GROUP_SHOT_CHARACTER_TALK(ByteStream *conv) {}

void PSConvFrame::parse_SHOW_TEXT(ByteStream *conv) {}

void PSConvFrame::parse_UNKNOWN(ByteStream *conv) {}

void PSConvFrame::parse_CHOOSE_WINGMAN(ByteStream *conv, SCConvPlayer *player) {}

int PSConvFrame::SetSentenceFromConv(ByteStream *conv, int start_offset) { return 0; }

PSConvPlayer::PSConvPlayer() {}

PSConvPlayer::~PSConvPlayer() {}

void PSConvPlayer::ReadNextFrame(void) {}

void PSConvPlayer::parseConv(ConvFrame *tmp_frame) {}
