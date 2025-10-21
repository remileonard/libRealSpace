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

void PSConvPlayer::ReadNextFrame(void) {
    PSConvFrame *tmp_frame = new PSConvFrame();

    tmp_frame->creationTime       = SDL_GetTicks();
    tmp_frame->text               = nullptr;
    tmp_frame->sound_file_name    = nullptr;
    tmp_frame->facePaletteID      = 0;
    tmp_frame->font = this->font;
    tmp_frame->conversationID = this->conversationID;
    // tmp_frame->face = nullptr;

    this->parseConv(tmp_frame);

    tmp_frame->SetExpired(false);
    if (this->conversation_frames.size() > 0) {
        if (tmp_frame->bgLayers == nullptr) {
            tmp_frame->bgLayers = this->conversation_frames.back()->bgLayers;
            tmp_frame->bgPalettes = this->conversation_frames.back()->bgPalettes;
        }
    }
    if (this->txt_color != 0 && tmp_frame->textColor == 0) {
        tmp_frame->textColor = this->txt_color;
    } else if (tmp_frame->textColor != 0) {
        this->txt_color = tmp_frame->textColor;
    }
    if (this->yes_no_path == 0 && tmp_frame->yes_no_path != 0) {
        this->yes_no_path = tmp_frame->yes_no_path;
    } else if (this->yes_no_path == 1 && tmp_frame->yes_no_path != 0) {
        this->yes_no_path = tmp_frame->yes_no_path;
    } else if (tmp_frame->yes_no_path == 0) {
        tmp_frame->yes_no_path = this->yes_no_path;
    }
    if (!tmp_frame->do_not_add) {
        this->conversation_frames.push_back(tmp_frame);
    } else {
        delete tmp_frame;
    }
}

void PSConvPlayer::parseConv(ConvFrame *tmp_frame) {
    printf("PSConvPlayer::parseConv not implemented yet.\n");
    conv.ReadByte(); // Just to advance the stream
}
