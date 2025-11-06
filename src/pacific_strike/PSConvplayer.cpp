#include "PSConvplayer.h"

PSConvFrame::PSConvFrame() {}

PSConvFrame::~PSConvFrame() {}

void PSConvFrame::parse_GROUP_SHOT(ByteStream *conv) {
    std::string loc_str = conv->ReadString(8);
    ConvBackGround *bg = ConvAssets.GetBackGround(loc_str.c_str());

    this->mode = ConvFrame::CONV_WIDE;
    this->bgLayers   = &bg->layers;
    this->bgPalettes = &bg->palettes;
    this->participants.clear();
    this->participants.shrink_to_fit();
    while (conv->PeekByte(0) != 0x0A && !conv->IsEndOfStream()) {
        conv->MoveForward(1);
    }
    uint8_t x = conv->ReadByte();
    uint8_t y = conv->ReadByte();
    this->SetSentenceFromConv(conv, 0);
    this->textColor = 160;
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
    this->textColor            = 160;
    const char *pszExt         = "normal";
    this->facePaletteID = ConvAssets.GetFacePaletteID(const_cast<char *>(pszExt));
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


bool psIsNextFrameIsConv(uint8_t type) {
    switch (type) {
        case 0x00:
        case 0x03:
            return true;
        default:
            return false;
    }
    return false;
}

void PSConvPlayer::SetArchive(PakEntry *convPakEntry) {

    if (convPakEntry->size == 0) {
        Game->log("Conversation entry is empty: Unable to load it.\n");
        stop();
        return;
    }

    this->size = convPakEntry->size;

    this->conv.Set(convPakEntry->data, convPakEntry->size);
    end = convPakEntry->data + convPakEntry->size;

    if (this->conversation_frames.size() > 0) {
        for (auto frame : this->conversation_frames) {
            delete frame;
        }
        this->conversation_frames.clear();
        this->conversation_frames.shrink_to_fit();
    }
    std::vector<uint8_t> *conv_frames_data;
    uint16_t end_of_frames_marker = 65535;
    int read_bytes = 0;
    int conv_size = 0;
    bool between_frames = false;
    std::vector<uint8_t> *between_frame_bytes;
    std::vector<std::vector<uint8_t> *> all_frames_data;
    while (read_bytes < conv.GetSize()) {
        if (!between_frames) {
            if (conv_size == 0) {
                conv_frames_data = new std::vector<uint8_t>();
            }
            conv_frames_data->push_back(conv.ReadByte());
            read_bytes++;
            conv_size++;
            if (conv_frames_data->size() >= 2) {
                uint16_t possible_end_marker = conv.PeekUShort();
                if (possible_end_marker == end_of_frames_marker) {
                    conv_frames_data->push_back(conv.ReadByte());
                    conv_frames_data->push_back(conv.ReadByte());
                    between_frames = true;
                    between_frame_bytes = new std::vector<uint8_t>();
                    conv_size = 0;
                }
            }
        } else {
            uint8_t b = conv.PeekByte(0);
            uint8_t b1 = conv.PeekByte(1);
            if (psIsNextFrameIsConv(b) && !psIsNextFrameIsConv(b1) && between_frame_bytes->size()>=2) {
                all_frames_data.push_back(conv_frames_data);
                between_frames = false;
            } else {
                between_frame_bytes->push_back(conv.ReadByte());
                read_bytes++;
            }
        }
    }
    for (auto frame_data : all_frames_data) {
        PSConvFrame *tmp_frame = new PSConvFrame();
        tmp_frame->creationTime       = SDL_GetTicks();
        tmp_frame->text               = nullptr;
        tmp_frame->sound_file_name    = nullptr;
        tmp_frame->facePaletteID      = 0;
        tmp_frame->font = this->font;
        tmp_frame->conversationID = this->conversationID;
        tmp_frame->SetExpired(false);
        ByteStream frame_stream;
        frame_stream.Set(frame_data->data(), frame_data->size());
        uint8_t frame_type = frame_stream.ReadByte();
        switch (frame_type) {
        case 0x00: // Group plan
            tmp_frame->parse_GROUP_SHOT(&frame_stream);
        break;
        case 0x03: // Person talking
            tmp_frame->parse_CLOSEUP(&frame_stream);
        break;
        default:
            break;
        }
        this->conversation_frames.push_back(tmp_frame);
    }
    initialized = true;
}