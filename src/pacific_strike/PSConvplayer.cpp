#include "PSConvplayer.h"

PSConvFrame::PSConvFrame() {}

PSConvFrame::~PSConvFrame() {}

void PSConvFrame::parse_GROUP_SHOT(ByteStream *conv) {
    std::string loc_str = conv->ReadString(8);
    ConvBackGround *bg = ConvAssets.GetBackGround(loc_str.c_str());

    this->mode = ConvFrame::CONV_WIDE;
    this->participants.clear();
    this->participants.shrink_to_fit();
    this->bgLayers   = &bg->layers;
    this->bgPalettes = &bg->palettes;
    this->face      = nullptr;

    while (conv->PeekByte() == 0x0 && !conv->IsEndOfStream()) {
        conv->MoveForward(1);
    }
    while (!isNextFrameIsConv((uint8_t) conv->PeekByte()) && !conv->IsEndOfStream()) {
        conv->MoveForward(1);
    }
    uint8_t next_type = conv->PeekByte();
    if (next_type == 0x0A) {
        conv->MoveForward(2);
        this->parse_SHOW_TEXT(conv);
    } else {
        conv->MoveForward(1);
    }
}

void PSConvFrame::parse_CLOSEUP(ByteStream *conv) {
    std::string speaker_name_str = conv->ReadString(8);
    conv->MoveForward(1);
    this->face_expression = conv->ReadByte();
    std::string location_set_name = conv->ReadString(8);
    conv->MoveForward(2);
    uint8_t pos = conv->ReadByte();
    conv->MoveForward(2);
    int next_frame_offset = SetSentenceFromConv(conv,0);
    this->participants.clear();
    this->participants.shrink_to_fit();
    
    this->facePosition = static_cast<ConvFrame::FacePos>(pos);

    this->mode         = ConvFrame::CONV_CLOSEUP;
    this->face         = ConvAssets.GetCharFace(speaker_name_str.c_str());
    ConvBackGround *bg        = ConvAssets.GetBackGround(location_set_name.c_str());
    this->bgLayers     = &bg->layers;
    this->bgPalettes   = &bg->palettes;

    conv->MoveForward(next_frame_offset);
    uint8_t color              = conv->ReadByte(); // Color ?
    this->textColor            = color;
    const char *pszExt         = "normal";
    this->facePaletteID = ConvAssets.GetFacePaletteID(const_cast<char *>(pszExt));
}

void PSConvFrame::parse_SHOW_TEXT(ByteStream *conv) {
    this->mode = ConvFrame::CONV_SHOW_TEXT;
    uint8_t color      = conv->ReadByte();
    int next_frame_offset = this->SetSentenceFromConv(conv, 0);
    this->textColor = color;

    conv->MoveForward(next_frame_offset);
    conv->PeekByte();
}

int PSConvFrame::SetSentenceFromConv(ByteStream *conv, int start_offset) {
    conv->MoveForward(start_offset);
    std::string sentence = conv->ReadStringNoSize(conv->GetSize() - conv->GetCurrentPosition());
    int sound_offset = 0;
    int decalage = 1;
    
    std::string *text = new std::string(sentence);
    std::vector<uint8_t> end_frame_mark = conv->ReadBytes(4);
    if (end_frame_mark[0] == 0xFF && end_frame_mark[0] == 0xFF) {
        
    } else if (end_frame_mark[0] == 0xF4 && end_frame_mark[0] == 0xFF) {

    }
    if (text->find("$N") != std::string::npos) {
        text->replace(text->find("$N"), 2, GameState.player_firstname);
    }
    if (text->find("$S") != std::string::npos) {
        text->replace(text->find("$S"), 2, GameState.player_name);
    }
    if (text->find("$C") != std::string::npos) {
        text->replace(text->find("$C"), 2, GameState.player_callsign);
    }
    if (text->find("$W") != std::string::npos) {
        text->replace(text->find("$W"), 2, GameState.wingman);
    }
    this->text         = (char *)text->c_str();
    return 0;
}

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
    if (this->conv.GetCurrentPosition() > this->size) {
        return;
    }
    uint8_t type = conv.ReadByte();
    switch (type) {
    case 0x00: // Group plan
    {
        uint8_t cub = conv.CurrentByte();
        if ((0x20 <= cub) && (cub <= 0x7E)) {
            tmp_frame->parse_GROUP_SHOT(&conv);
        } else if (cub == 0x3) {
            uint8_t next_b = conv.PeekByte();
            if ((0x20 <= next_b) && (next_b <= 0x7E)) {
                conv.MoveForward(1);
                tmp_frame->parse_CLOSEUP(&conv);
            } else {
                conv.MoveForward(1);
                tmp_frame->do_not_add = true;
            }
        }else {
            conv.MoveForward(1);
            tmp_frame->do_not_add = true;
        }
        break;
    }
    case 0x03: // Person talking
    {
        uint8_t cub = conv.CurrentByte();
        if ((0x20 <= cub) && (cub <= 0x7E)) {
            tmp_frame->parse_CLOSEUP(&conv);
        } else {
            conv.MoveForward(1);
            tmp_frame->do_not_add = true;
        }
        break;
    }
    case 0x0A: // Show text
    {
        uint8_t cub = conv.CurrentByte();
        if ((0x20 <= cub) && (cub <= 0x7E)) {
            tmp_frame->parse_SHOW_TEXT(&conv);
            tmp_frame->face = this->conversation_frames.back()->face;
            tmp_frame->textColor = this->conversation_frames.back()->textColor;
            tmp_frame->face_expression = this->conversation_frames.back()->face_expression;
            tmp_frame->facePaletteID = this->conversation_frames.back()->facePaletteID;
            tmp_frame->facePosition = this->conversation_frames.back()->facePosition;
        } else {
            conv.MoveForward(1);
            tmp_frame->do_not_add = true;
        }
        break;
    }
    default:
        tmp_frame->do_not_add = true;
        break;
    }
}
