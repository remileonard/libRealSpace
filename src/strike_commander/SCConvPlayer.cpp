//
//  SCConvPlayer.cpp
//  libRealSpace
//
//  Created by Fabien Sanglard on 1/31/2014.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//

#include "precomp.h"
#include "SCConvPlayer.h"

SCConvPlayer::SCConvPlayer() : conversationID(0), initialized(false) {
    this->Game = &GameEngine::instance();
}

SCConvPlayer::~SCConvPlayer() {}

// Taking animation
// 03 mouth anim
// 04 mouth anim
// 06 mouth anim
// 07 mouth anim
// 08 mouth anim
// 09 mouth pinched
// 10 mouth opened
// 11 mouth something
// 12 mouth something

// Eyes animation
// 13 eyes closed
// 14 eyes closed
// 15 eyes wide open
// 16 eagle eyes
// 16 left wink
// 17 upper left eyes
// 18 look right
// 19 look left
// 20 eyes straight
// 21 eyes blink closed
// 22 eyes blink mid-open

// 23 eye brows semi-raised
// 24 left eye brows semi-raised
// 25 right eye brows semi-raised
// 26 eye brows something

// General face expression
// 27 mouth heart
// 28 face tensed
// 29 face smile
// 30 right face tensed
// 31 right crooked
// 32 pinched lips
// 33 surprise
// 34 seducing face
// 35 look of desaproval face

// Cloth
// 35 civil clothes
// 36 pilot clothes
// 37 pilot clothes 2

// 38 sunglasses
// 39 pilot helmet (if drawing this, don't draw hairs
// 40 pilot helmet visor (if drawing this draw 39 too

// Images du gros plan pour chaque octet « image » de CLOSEUP, numéros libRealSpace (image du jeu + 1) : tête,
// yeux ou expression, bouche, coiffure, habits (CONV_SYSTEM.md § 7.4).
static const uint8_t kFaceMouthSlot = 255;
static const uint8_t kFaceHead = 1;
static const uint8_t kFaceHair = 2;
static const uint8_t kFaceClothes = 35;
static const uint8_t kFaceBlink = 13;

std::unordered_map<uint8_t, std::vector<uint8_t>> faces_shape = {
    { 13,
      std::vector<uint8_t>{
          kFaceHead,
          14,
          kFaceMouthSlot,
          kFaceHair,
          kFaceClothes,
      } },
    { 14,
      std::vector<uint8_t>{
          kFaceHead,
          15,
          kFaceMouthSlot,
          kFaceHair,
          kFaceClothes,
      } },
    { 15,
      std::vector<uint8_t>{
          kFaceHead,
          16,
          kFaceMouthSlot,
          kFaceHair,
          kFaceClothes,
      } },
    { 16,
      std::vector<uint8_t>{
          kFaceHead,
          17,
          kFaceMouthSlot,
          kFaceHair,
          kFaceClothes,
      } },
    { 17,
      std::vector<uint8_t>{
          kFaceHead,
          18,
          kFaceMouthSlot,
          kFaceHair,
          kFaceClothes,
      } },
    { 18,
      std::vector<uint8_t>{
          kFaceHead,
          19,
          kFaceMouthSlot,
          kFaceHair,
          kFaceClothes,
      } },
    { 19,
      std::vector<uint8_t>{
          kFaceHead,
          20,
          kFaceMouthSlot,
          kFaceHair,
          kFaceClothes,
      } },
    { 20,
      std::vector<uint8_t>{
          kFaceHead,
          21,
          kFaceMouthSlot,
          kFaceHair,
          kFaceClothes,
      } },
    { 21,
      std::vector<uint8_t>{
          kFaceHead,
          22,
          kFaceMouthSlot,
          kFaceHair,
          kFaceClothes,
      } },
    { 22,
      std::vector<uint8_t>{
          kFaceHead,
          23,
          kFaceMouthSlot,
          kFaceHair,
          kFaceClothes,
      } },
    { 23,
      std::vector<uint8_t>{
          kFaceHead,
          24,
          kFaceMouthSlot,
          kFaceHair,
          kFaceClothes,
      } },
    { 24,
      std::vector<uint8_t>{
          kFaceHead,
          25,
          kFaceMouthSlot,
          kFaceHair,
          kFaceClothes,
      } },
    { 25,
      std::vector<uint8_t>{
          kFaceHead,
          26,
          kFaceMouthSlot,
          kFaceHair,
          kFaceClothes,
      } },
    { 26,
      std::vector<uint8_t>{
          kFaceHead,
          27,
          kFaceMouthSlot,
          kFaceHair,
          kFaceClothes,
      } },
    { 27,
      std::vector<uint8_t>{
          kFaceHead,
          28,
          kFaceMouthSlot,
          kFaceHair,
          kFaceClothes,
      } },
    { 28,
      std::vector<uint8_t>{
          kFaceHead,
          29,
          kFaceMouthSlot,
          kFaceHair,
          kFaceClothes,
      } },
    { 29,
      std::vector<uint8_t>{
          kFaceHead,
          30,
          kFaceMouthSlot,
          kFaceHair,
          kFaceClothes,
      } },
    { 30,
      std::vector<uint8_t>{
          kFaceHead,
          31,
          kFaceMouthSlot,
          kFaceHair,
          kFaceClothes,
      } },
    { 31,
      std::vector<uint8_t>{
          kFaceHead,
          32,
          kFaceMouthSlot,
          kFaceHair,
          kFaceClothes,
      } },
    { 32,
      std::vector<uint8_t>{
          kFaceHead,
          33,
          kFaceMouthSlot,
          kFaceHair,
          kFaceClothes,
      } },
    { 33,
      std::vector<uint8_t>{
          kFaceHead,
          34,
          kFaceMouthSlot,
          kFaceHair,
          kFaceClothes,
      } },
    { 255,
      std::vector<uint8_t>{
          kFaceHead,
          kFaceMouthSlot,
          kFaceHair,
          kFaceClothes,
      } },
};

// Ancienne table, gardée pour les répliques de Pacific Strike (PSConvFrame).
std::unordered_map<uint8_t, std::vector<uint8_t>> legacy_faces_shape = {
    { 13,     std::vector<uint8_t>{1, 14, 2, 35}},
    { 14, std::vector<uint8_t>{1, 21, 28, 2, 35}},
    { 16,     std::vector<uint8_t>{1, 17, 2, 35}},
    { 17,     std::vector<uint8_t>{1, 18, 2, 35}},
    { 18,     std::vector<uint8_t>{1, 19, 2, 35}},
    { 19,         std::vector<uint8_t>{1, 2, 35}},
    { 20, std::vector<uint8_t>{1, 21, 28, 2, 35}},
    { 21, std::vector<uint8_t>{1, 21, 28, 2, 35}}, //todo check
    { 22,     std::vector<uint8_t>{1, 23, 2, 35}},
    { 23,     std::vector<uint8_t>{1, 24, 2, 35}},
    { 24,     std::vector<uint8_t>{1, 25, 2, 35}},
    { 25,     std::vector<uint8_t>{1, 26, 2, 35}},
    { 26,     std::vector<uint8_t>{1, 26, 2, 35}}, //todo check
    { 27,     std::vector<uint8_t>{1, 27, 2, 35}}, //todo check
    { 28,     std::vector<uint8_t>{1, 28, 2, 35}}, //todo check
    { 29,     std::vector<uint8_t>{1, 29, 2, 35}}, //todo check
    { 31,     std::vector<uint8_t>{1, 31, 2, 35}},
    { 32,     std::vector<uint8_t>{1, 33, 2, 35}}, //todo check
    {255,         std::vector<uint8_t>{1, 2, 35}},
};

static Point2D scriptPosition(int16_t x, int16_t y) {
    Point2D position;
    position.x = x;
    position.y = y;
    return position;
}

void ConvFrame::setGroupShot(const ConvRecord &record) {
    ConvBackGround *bg = ConvAssets.GetBackGround(record.background);

    this->mode = ConvFrame::CONV_WIDE;
    this->participants.clear();
    this->participants.shrink_to_fit();
    this->participant_positions.clear();
    this->bgLayers   = &bg->layers;
    this->bgPalettes = &bg->palettes;
    this->bgYs       = &bg->ys;
    this->face      = nullptr;
    this->group_scene = true;
    this->talker = -1;
}
void ConvFrame::addFigure(const ConvRecord &record) {
    CharFigure *participant = ConvAssets.GetFigure(record.name);
    this->participants.push_back(participant);
    this->participant_positions.push_back(scriptPosition(record.x, record.y));
}
void ConvFrame::setTalk(const ConvRecord &record) {
    this->textColor = record.color;
    this->setSentence(record);
}
void ConvFrame::copyScene(const ConvFrame *previous) {
    this->participants = previous->participants;
    this->participant_positions = previous->participant_positions;
    this->group_scene = previous->group_scene;
    this->figure_palette = previous->figure_palette;
    this->talker = -1;
}
void ConvFrame::setNarration(const ConvRecord &record) {
    this->mode = ConvFrame::CONV_SHOW_TEXT;
    this->textColor = record.color;
    this->setSentence(record);
}
void ConvFrame::setCloseUp(const ConvRecord &record) {
    this->face_expression = record.baseFrame;
    this->setSentence(record);
    this->participants.clear();
    this->participants.shrink_to_fit();
    this->participant_positions.clear();
    this->face_position = scriptPosition(record.x, record.y);
    this->game_positions = true;
    this->facePosition = static_cast<ConvFrame::FacePos>(static_cast<uint8_t>(record.x));

    this->mode         = ConvFrame::CONV_CLOSEUP;
    this->face         = ConvAssets.GetCharFace(record.name);
    ConvBackGround *bg = ConvAssets.GetBackGround(record.background);
    this->bgLayers     = &bg->layers;
    this->bgPalettes   = &bg->palettes;
    this->bgYs         = &bg->ys;

    this->textColor     = record.color;
    this->facePaletteID = ConvAssets.GetFacePaletteID("normal");
}
void ConvFrame::setSentence(const ConvRecord &record) {
    this->sound_file_name = nullptr;
    if (!record.voice.empty()) {
        this->sound_file_name = new std::string(record.voice);
    }
    this->lip_text = record.text;
    for (auto &c : this->lip_text) {
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }
    std::string *text = new std::string(record.text);
    this->text = (char *)text->c_str();
}
void ConvFrame::copyCloseUp(const ConvFrame *previous) {
    this->face            = previous->face;
    this->textColor       = previous->textColor;
    this->face_expression = previous->face_expression;
    this->facePaletteID   = previous->facePaletteID;
    this->facePosition    = previous->facePosition;
    this->face_position   = previous->face_position;
    this->game_positions  = previous->game_positions;
}
static const std::vector<std::string> kPilotNames = {
    "",
    "Billy",
    "Gwen",
    "Lyle",
    "Miguel",
    "Tex"
};
static const std::vector<std::string> kPilotFigures = {
    "red1",
    "billy2",
    "gwen2",
    "lyle2",
    "miguel2",
    "tex3"
};

static std::string pilotWingmanName(uint8_t pilot) {
    std::string name = pilot < kPilotNames.size() ? kPilotNames[pilot] : "";
    std::transform(name.begin(), name.end(), name.begin(), ::toupper);
    return name;
}

void SCConvPlayer::selectWing(const ConvRecord &record) {
    this->wing_screen = static_cast<uint8_t>(record.value & 255);
    this->wing_number = static_cast<uint8_t>(record.value >> 8);
    this->wing_pilots.clear();
    TreEntry *gameflowIFF = Assets.GetEntryByName(Assets.gameflow_filename);
    RSGameFlow flow;
    flow.InitFromRam(gameflowIFF->data, gameflowIFF->size);
    for (WING *wing : flow.game.wngs) {
        if (wing->info.ID == this->wing_number) {
            this->wing_pilots = wing->pilt;
            break;
        }
    }
    if (this->wing_pilots.empty()) {
        printf("CONV %d: SELECT_WING, escadre %d absente de GAMEFLOW.IFF ou sans PILT\n", this->conversationID,
               this->wing_number);
        return;
    }
    GameState.wingman = pilotWingmanName(this->wing_pilots[0]);
    printf("CONV %d: SELECT_WING, ecran %d, escadre %d, %zu pilotes, equipier par defaut %s\n", this->conversationID,
           this->wing_screen, this->wing_number, this->wing_pilots.size(), GameState.wingman.c_str());
}

void SCConvPlayer::setWingmanChoice(ConvFrame *frame) {
    ConvAssetManager &ConvAssets = ConvAssetManager::getInstance();
    ConvBackGround *bg = ConvAssets.GetBackGround(this->wing_screen == 0 ? "s_wing" : "w_wing");

    frame->bgLayers   = &bg->layers;
    frame->bgPalettes = &bg->palettes;
    frame->bgYs       = &bg->ys;
    frame->game_positions = true;

    frame->participants.clear();
    frame->participant_positions.clear();
    frame->pilots.clear();
    frame->mode = ConvFrame::CONV_WINGMAN_CHOICE;

    // Les pilots de p_wing sont des enfants du premier élément du décor : posés en (0, y de cet élément).
    int16_t parent_y = bg->ys.empty() ? 0 : bg->ys[0];
    for (uint8_t pilot = 0; pilot < kPilotFigures.size(); pilot++) {
        bool proposed = pilot == 0 ||
                        std::find(this->wing_pilots.begin(), this->wing_pilots.end(), pilot) != this->wing_pilots.end();
        if (!proposed) {
            continue;
        }
        frame->participants.push_back(ConvAssets.GetFigure(kPilotFigures[pilot]));
        frame->participant_positions.push_back(scriptPosition(0, parent_y));
        frame->pilots.push_back(pilot);
    }
    printf("CONV %d: CHOOSE_WINGMAN, decor %s, %zu pilotes proposes\n", this->conversationID,
           this->wing_screen == 0 ? "s_wing" : "w_wing", frame->pilots.size() - 1);
}

void SCConvPlayer::focus(void) { IActivity::focus(); }

void SCConvPlayer::answerQuestion(bool accepted) {
    GameState.mission_accepted = accepted;
    printf("CONV %d: reponse %s\n", this->conversationID, accepted ? "Accept Mission" : "Reject Mission");

    // Les répliques de la branche opposée sont retirées : refus = branche 1, acceptation = branche 2.
    int pathToRemove = accepted ? 2 : 1;
    auto it = conversation_frames.begin();
    if (it != conversation_frames.end() && *it == this->current_frame) {
        ++it;
    }
    while (it != conversation_frames.end()) {
        ConvFrame *f = *it;
        if (f && f->yes_no_path == pathToRemove) {
            delete f;
            it = conversation_frames.erase(it);
        } else {
            ++it;
        }
    }
    conversation_frames.shrink_to_fit();
}

static const char *convCommandName(uint8_t op) {
    switch (op) {
    case CONV_GROUP_SHOT:
        return "GROUP_SHOT";
    case CONV_GROUP_SHOT_ADD_CHARACTER:
        return "GROUP_SHOT_ADD_CHARACTER";
    case CONV_GROUP_SHOT_CHARACTER_TALK:
        return "GROUP_SHOT_CHARACTER_TALK";
    case CONV_CLOSEUP:
        return "CLOSEUP";
    case CONV_CLOSEUP_CONTINUATION:
        return "CLOSEUP_CONTINUATION";
    case CONV_SHOW_TEXT:
        return "SHOW_TEXT";
    case CONV_YESNOCHOICE_BRANCH1:
        return "YESNOCHOICE_BRANCH1";
    case CONV_YESNOCHOICE_BRANCH2:
        return "YESNOCHOICE_BRANCH2";
    case CONV_SELECT_WING:
        return "SELECT_WING";
    case CONV_CHOOSE_WINGMAN:
        return "CHOOSE_WINGMAN";
    case CONV_NOP_16:
        return "NOP_16";
    case CONV_NOP_18:
        return "NOP_18";
    default:
        return "?";
    }
}

ConvFrame *SCConvPlayer::newFrame(void) {
    ConvFrame *frame = new ConvFrame();

    frame->creationTime    = SDL_GetTicks();
    frame->text            = nullptr;
    frame->sound_file_name = nullptr;
    frame->facePaletteID   = 0;
    frame->font            = this->font;
    frame->conversationID  = this->conversationID;
    return frame;
}

void SCConvPlayer::addFrame(ConvFrame *frame) {
    frame->SetExpired(false);
    if (this->conversation_frames.size() > 0) {
        if (frame->bgLayers == nullptr) {
            frame->bgLayers = this->conversation_frames.back()->bgLayers;
            frame->bgPalettes = this->conversation_frames.back()->bgPalettes;
            frame->bgYs = this->conversation_frames.back()->bgYs;
        }
    }
    if (this->txt_color != 0 && frame->textColor == 0) {
        frame->textColor = this->txt_color;
    } else if (frame->textColor != 0) {
        this->txt_color = frame->textColor;
    }
    if (!frame->do_not_add) {
        this->conversation_frames.push_back(frame);
    } else {
        delete frame;
    }
}

static int answerBranch(const std::vector<ConvRecord> &records, size_t offset) {
    for (const ConvRecord &question : records) {
        if (question.op != CONV_YESNOCHOICE_BRANCH1) {
            continue;
        }
        size_t refuse_start = question.offset + 3 + question.value;
        if (offset >= question.offset && offset < refuse_start) {
            return 1;
        }
        for (const ConvRecord &jump : records) {
            if (jump.op != CONV_YESNOCHOICE_BRANCH2 || jump.offset + 3 != refuse_start) {
                continue;
            }
            size_t refuse_end = jump.offset + 3 + jump.value;
            if (offset >= refuse_start && offset < refuse_end) {
                return 2;
            }
        }
    }
    return 0;
}

static int advanceCloseUpMouth(const std::string &text, size_t &cursor) {
    static const char *lip_table = "06891599098769362999252999";
    if (cursor >= text.size()) {
        return -1;
    }
    int mouth = -1;
    char c = text[cursor];
    cursor++;
    char next = cursor < text.size() ? text[cursor] : '\0';
    if (c == 'e') {
        if (next < 'a' || next > 'z') {
            c = next;
            cursor++;
        } else if (next == 'r') {
            mouth = 6;
            cursor++;
        }
    } else if (c == 'h') {
        cursor++;
    } else if (c == 't') {
        if (next == 'h') {
            mouth = 9;
            cursor++;
        }
    } else if (c == 'c') {
        if (next == 'h') {
            mouth = 6;
            cursor++;
        }
    }
    if (mouth == -1 && c >= 'a' && c <= 'z') {
        mouth = lip_table[c - 'a'] - '0' + 2;
    }
    return mouth;
}

// Bouche du gros plan à la lecture numéro reads (une lecture par image de scène, 10 par seconde), -1 = aucune.
static int closeUpMouth(const std::string &text, uint32_t reads) {
    size_t cursor = 0;
    int mouth = -1;
    for (uint32_t r = 0; r <= reads; r++) {
        if (cursor >= text.size()) {
            return -1;
        }
        mouth = advanceCloseUpMouth(text, cursor);
    }
    return mouth;
}

// Durée d'une réplique de gros plan sans voix, en ms (§ 7.4 et § 8.7 de CONV_SYSTEM.md).
static uint32_t closeUpDuration(const ConvFrame *frame, uint8_t op, ConvFormat format) {
    if (format == ConvFormat::Cd) {
        return frame->lip_text.empty() ? 800 : 5000;
    }
    if (op == CONV_SHOW_TEXT) {
        return 2000;
    }
    if (frame->lip_text.empty()) {
        return 1500;
    }
    size_t cursor = 0;
    uint32_t reads = 0;
    while (cursor < frame->lip_text.size()) {
        advanceCloseUpMouth(frame->lip_text, cursor);
        reads++;
    }
    return (reads + 40) * 100;
}

// Durée d'une réplique de scène de groupe sans voix, en ms (§ 7.2 et § 8.6 de CONV_SYSTEM.md).
static uint32_t groupReplyDuration(const ConvRecord &record, ConvFormat format) {
    if (format == ConvFormat::Cd) {
        return record.text.empty() ? 800 : 5000;
    }
    if (record.op == CONV_SHOW_TEXT) {
        return 4000;
    }
    size_t letters = record.text.size() > 0 ? record.text.size() : 1;
    return static_cast<uint32_t>(letters * 100);
}

void SCConvPlayer::pushFrame(ConvFrame *frame, const std::vector<ConvRecord> &records, const ConvRecord &record) {
    frame->yes_no_path = answerBranch(records, record.offset);
    printf("CONV %d: replique %zu, %s (octet %zu), mode %d, branche %d, couleur %d, figures %zu, parle %d, duree %u, "
           "voix \"%s\", texte \"%s\"\n",
           this->conversationID, this->conversation_frames.size(), convCommandName(record.op), record.offset,
           frame->mode, frame->yes_no_path, frame->textColor, frame->participants.size(), frame->talker,
           frame->duration, frame->sound_file_name != nullptr ? frame->sound_file_name->c_str() : "",
           frame->text != nullptr ? frame->text : "");
    this->addFrame(frame);
}

size_t SCConvPlayer::buildGroupShot(const std::vector<ConvRecord> &records, size_t index, ConvFormat format) {
    const ConvRecord &shot = records[index];
    size_t i = index + 1;
    std::vector<const ConvRecord *> figurants;
    while (i < records.size() && records[i].op == CONV_GROUP_SHOT_ADD_CHARACTER) {
        figurants.push_back(&records[i]);
        i++;
    }
    std::vector<const ConvRecord *> replies;
    while (i < records.size() && records[i].level == 1 &&
           (records[i].op == CONV_GROUP_SHOT_CHARACTER_TALK || records[i].op == CONV_SHOW_TEXT)) {
        replies.push_back(&records[i]);
        i++;
    }
    int figure_palette = -1;
    if (!figurants.empty()) {
        figure_palette = static_cast<int>(ConvAssetManager::getInstance().GetFigure(figurants[0]->name)->paletteID);
    }
    printf("CONV %d: GROUP_SHOT %s, %zu figurants, %zu repliques, palette du premier figurant %d\n",
           this->conversationID, shot.background.c_str(), figurants.size(), replies.size(), figure_palette);
    // Version CD : 7 attentes de 10 tics à 60 Hz avant la première réplique, ou à la place des répliques s'il n'y en
    // a pas (CONV_SYSTEM.md § 8.6) ; le premier personnage qui parle est déjà construit.
    static const uint32_t kCdSceneWait = 7 * 10 * 1000 / 60;
    if (replies.empty() || format == ConvFormat::Cd) {
        ConvFrame *frame = newFrame();
        frame->setGroupShot(shot);
        for (const ConvRecord *figurant : figurants) {
            frame->addFigure(*figurant);
        }
        frame->figure_palette = figure_palette;
        frame->cd_format = format == ConvFormat::Cd;
        if (format == ConvFormat::Cd) {
            frame->duration = kCdSceneWait;
            if (!replies.empty() && replies[0]->op == CONV_GROUP_SHOT_CHARACTER_TALK) {
                frame->addFigure(*replies[0]);
            }
        }
        this->pushFrame(frame, records, shot);
        if (replies.empty()) {
            return i;
        }
    }
    for (size_t k = 0; k < replies.size(); k++) {
        const ConvRecord &reply = *replies[k];
        ConvFrame *frame = newFrame();
        frame->setGroupShot(shot);
        for (const ConvRecord *figurant : figurants) {
            frame->addFigure(*figurant);
        }
        frame->figure_palette = figure_palette;
        frame->cd_format = format == ConvFormat::Cd;
        // Disquette : tous les personnages qui parlent sont là dès le début de la scène ; CD : chacun apparaît à
        // sa première réplique et reste.
        for (size_t t = 0; t < replies.size(); t++) {
            if (replies[t]->op != CONV_GROUP_SHOT_CHARACTER_TALK) {
                continue;
            }
            if (format == ConvFormat::Cd && t > k) {
                continue;
            }
            if (t == k) {
                frame->talker = static_cast<int>(frame->participants.size());
            }
            frame->addFigure(*replies[t]);
        }
        if (reply.op == CONV_GROUP_SHOT_CHARACTER_TALK) {
            frame->setTalk(reply);
        } else {
            frame->setNarration(reply);
        }
        frame->duration = groupReplyDuration(reply, format);
        this->pushFrame(frame, records, reply);
    }
    return i;
}

void SCConvPlayer::buildFrames(const std::vector<ConvRecord> &records, ConvFormat format) {
    size_t i = 0;
    while (i < records.size()) {
        const ConvRecord &record = records[i];
        if (record.op == CONV_GROUP_SHOT) {
            i = this->buildGroupShot(records, i, format);
            continue;
        }
        i++;
        ConvFrame *frame = nullptr;
        switch (record.op) {
        case CONV_SHOW_TEXT:
            frame = newFrame();
            frame->setNarration(record);
            if (!this->conversation_frames.empty()) {
                frame->copyCloseUp(this->conversation_frames.back());
                frame->copyScene(this->conversation_frames.back());
            }
            if (frame->group_scene) {
                frame->duration = groupReplyDuration(record, format);
            } else if (frame->game_positions) {
                frame->duration = closeUpDuration(frame, record.op, format);
            }
            break;
        case CONV_CLOSEUP:
            frame = newFrame();
            frame->setCloseUp(record);
            frame->duration = closeUpDuration(frame, record.op, format);
            break;
        case CONV_CLOSEUP_CONTINUATION:
            frame = newFrame();
            frame->setSentence(record);
            if (!this->conversation_frames.empty()) {
                frame->copyCloseUp(this->conversation_frames.back());
                frame->mode = this->conversation_frames.back()->mode;
            }
            if (frame->game_positions) {
                frame->duration = closeUpDuration(frame, record.op, format);
            }
            break;
        case CONV_YESNOCHOICE_BRANCH1:
            frame = newFrame();
            frame->mode = ConvFrame::CONV_CONTRACT_CHOICE;
            if (!this->conversation_frames.empty()) {
                frame->copyCloseUp(this->conversation_frames.back());
            }
            printf("CONV %d: YESNOCHOICE_BRANCH1 (octet %zu), refus a l'octet %zu\n", this->conversationID,
                   record.offset, record.offset + 3 + record.value);
            break;
        case CONV_YESNOCHOICE_BRANCH2:
            printf("CONV %d: YESNOCHOICE_BRANCH2 (octet %zu), saut a l'octet %zu\n", this->conversationID,
                   record.offset, record.offset + 3 + record.value);
            break;
        case CONV_SELECT_WING:
            this->selectWing(record);
            break;
        case CONV_CHOOSE_WINGMAN:
            frame = newFrame();
            this->setWingmanChoice(frame);
            break;
        default:
            printf("CONV %d: %s, rien a faire\n", this->conversationID, convCommandName(record.op));
            break;
        }
        if (frame != nullptr) {
            this->pushFrame(frame, records, record);
        }
    }
}

void SCConvPlayer::SetArchive(PakEntry *convPakEntry) {

    if (convPakEntry->size == 0) {
        Game->log("Conversation entry is empty: Unable to load it.\n");
        stop();
        return;
    }

    if (this->conversation_frames.size() > 0) {
        for (auto frame : this->conversation_frames) {
            delete frame;
        }
        this->conversation_frames.clear();
        this->conversation_frames.shrink_to_fit();
    }

    ConvData script(convPakEntry->data, convPakEntry->size);
    int error = script.parse();
    if (error != 0) {
        printf("CONV %d: erreur de lecture %d a l'octet %zu : %s\n", this->conversationID, error,
               script.errorOffset(), script.errorMessage().c_str());
        Game->log("Conv ID %d: parse error %d at byte %zu.\n", this->conversationID, error, script.errorOffset());
        stop();
        return;
    }
    printf("CONV %d: format %s, %zu enregistrements\n", this->conversationID,
           script.format() == ConvFormat::Cd ? "CD" : "disquette", script.records().size());
    this->buildFrames(script.records(), script.format());

    initialized = true;
}

void SCConvPlayer::SetID(int32_t id) {

    this->conversationID = id;

    if (convPak.GetNumEntries() <= id) {
        stop();
        Game->log("Cannot load conversation id (max convID is %lu).", convPak.GetNumEntries() - 1);
        return;
    }
    
    PakEntry *entry = convPak.GetEntry(id);
    if (entry->size == 0) {
        Game->log("Conversation entry is empty: Unable to load it.\n");
        return;
    }
    if (entry->data[5] == 0x06 && entry->data[4] == 0x00) {
        PKWareDecompressor decompressor;
        uint8_t *compressed = entry->data + 4;
        size_t compressedSize = entry->size - 4;
        size_t decompressedSize = 0;
        uint8_t *decompressed = decompressor.DecompressPKWare (compressed, compressedSize, decompressedSize);
        entry->data = decompressed;
        entry->size = decompressedSize;
    }
    SetArchive(entry);
}

void SCConvPlayer::init() {
    VGAPalette *rendererPalette = VGA.getPalette();
    this->palette               = *rendererPalette;
    TreEntry *convEntry = Assets.GetEntryByName(Assets.convpak_filename);

    convPak.InitFromRAM("CONV.PAK", convEntry->data, convEntry->size);
    this->font = FontManager.GetFont("..\\..\\DATA\\FONTS\\CONVFONT.SHP");

    TreEntry *convPalettesEntry = Assets.GetEntryByName(Assets.conv_pal_filename);
    convPals.InitFromRAM("CONVPALS.PAK", convPalettesEntry->data, convPalettesEntry->size);
}

/**
 * @brief Check if the current frame has expired, either because of user input or
 *        because a certain amount of time has passed.
 *
 * If the user has pressed a key or clicked, the current frame is marked as
 * expired. If the frame has been displayed for longer than 5 seconds, the
 * current frame is also marked as expired.
 */
void SCConvPlayer::CheckFrameExpired(void) {
    if (this->current_frame->mode != ConvFrame::CONV_CONTRACT_CHOICE && this->current_frame->mode != ConvFrame::CONV_WINGMAN_CHOICE) {
        Keyboard* kb = Game->getKeyboard();
        if (!kb)
            return;
        if (kb->isActionJustPressed(InputAction::KEY_ESCAPE)) {
            Game->stopTopActivity();
            Mixer.stopSound(6);
        }
        if (Mouse.buttons[MouseButton::LEFT].event == MouseButton::RELEASED) {
            Mouse.buttons[MouseButton::LEFT].event = MouseButton::NONE;
            this->current_frame->SetExpired(true);
            Mixer.stopSound(6);
            return;
        }

    
        int32_t currentTime = SDL_GetTicks();

        if (this->current_frame->sound_file_name != nullptr && this->current_frame->sound_file_name->length()>0) {
            if (Mixer.isSoundPlaying(6) == false) {
                this->current_frame->SetExpired(true);
            }
        } else {
            uint32_t limit = this->current_frame->duration > 0 ? this->current_frame->duration : 5000;
            if (currentTime - this->current_frame->creationTime > limit) {
                this->current_frame->SetExpired(true);
            }
        }
        
    }
}

// Codes du texte (Conv_DrawText_59F54), remplacés à chaque occurrence. $L et $O : vides pour les escadres de type 0,
// les seules des données. $A, $B, $P, $T : état absent de libRealSpace, laissés tels quels comme un code inconnu.
std::string SCConvPlayer::expandTextCodes(const std::string &raw) {
    std::string text;
    size_t i = 0;
    while (i < raw.size()) {
        if (raw[i] != '$' || i + 1 >= raw.size()) {
            text.push_back(raw[i]);
            i++;
            continue;
        }
        char code = raw[i + 1];
        if (code == 'S') {
            text += GameState.player_name;
        } else if (code == 'N') {
            text += GameState.player_firstname;
        } else if (code == 'C') {
            text += GameState.player_callsign;
        } else if (code == 'W') {
            text += GameState.wingman;
        } else if (code != 'L' && code != 'O') {
            text.push_back('$');
            text.push_back(code);
        }
        i += 2;
    }
    return text;
}

// Texte d'une réplique (Conv_DrawText_59F54) : lignes de 30 caractères au plus, coupées à l'espace précédent, chacune
// centrée sur l'écran, haut de la ligne n à y = 156 + 15 × n (zone de texte en 152, y = 4 + 15 × n).
void SCConvPlayer::DrawText(void) {
    if (this->current_frame->text == NULL) {
        return;
    }
    std::string rest = this->expandTextCodes(this->current_frame->text);
    int line = 0;
    while (!rest.empty()) {
        std::string current;
        if (rest.size() > 30) {
            size_t cut = 30;
            while (cut > 0 && rest[cut] != ' ') {
                cut--;
            }
            if (cut == 0) {
                cut = 30;
                current = rest.substr(0, cut);
                rest = rest.substr(cut);
            } else {
                current = rest.substr(0, cut);
                rest = rest.substr(cut + 1);
            }
        } else {
            current = rest;
            rest.clear();
        }
        Point2D origin;
        origin.x = 0;
        origin.y = 156 + 15 * line;
        VGA.getFrameBuffer()->printText(this->font, &origin, (char *)current.c_str(), this->current_frame->textColor, 0,
                                        static_cast<uint32_t>(current.size()), 2, 0, TEXT_JUSTIFY_CENTER, 320);
        line++;
    }
}
void SCConvPlayer::applyGroupPalette(void) {
    ConvAssetManager &ConvAssets = ConvAssetManager::getInstance();
    ByteStream paletteReader;
    PakEntry *normal = convPals.GetEntry(ConvAssets.GetFigurePaletteID("normal"));
    paletteReader.Set(normal->data, normal->size);
    this->palette.ReadPatch(&paletteReader);
    if (this->current_frame->figure_palette >= 0) {
        PakEntry *figure = convPals.GetEntry(this->current_frame->figure_palette);
        ByteStream figureReader;
        figureReader.Set(figure->data, figure->size);
        this->palette.ReadPatch(&figureReader);
    }
    VGA.setPalette(&this->palette);
}

static int groupMouth(const std::string &lip_text, uint32_t elapsed) {
    static const char *lip_table = "04302500000340143000353000";
    size_t index = elapsed / 100;
    if (index >= lip_text.size()) {
        return -1;
    }
    char c = lip_text[index];
    if (c < 'a' || c > 'z') {
        return -1;
    }
    return lip_table[c - 'a'] - '0' + 1;
}

// Table des bouches d'une voix (VoicePlayer_LoadWithLipSync_7D52, MIDGAME_SYSTEM.md § 6.6) : le VOC (premier bloc,
// octets non signés à partir de l'octet 32) est découpé en tranches d'une image de scène ; valeur d'une tranche =
// moyenne × 1000 − 128 000 ; référence = plus grande valeur absolue du maximum et du minimum pris sans l'entrée
// maximale ni l'entrée minimale ; p = (référence − |valeur sur 16 bits|) × 100 / référence, puis une bouche du jeu.
static std::vector<int16_t> voiceLipTable(const uint8_t *voc, size_t size, int cadence) {
    std::vector<int16_t> mouths;
    if (size <= 32 || voc[30] == 0) {
        return mouths;
    }
    int32_t frequency = 1000000 / (256 - voc[30]);
    uint32_t block = static_cast<uint32_t>(voc[26]) | (static_cast<uint32_t>(voc[27]) << 8) |
                     (static_cast<uint32_t>(voc[28]) << 16) | (static_cast<uint32_t>(voc[29]) << 24);
    int32_t samples = static_cast<int32_t>(block >> 8) - 2;
    if (samples > static_cast<int32_t>(size) - 32) {
        samples = static_cast<int32_t>(size) - 32;
    }
    int32_t per_frame = frequency / cadence;
    std::vector<int32_t> values;
    for (int32_t k = 0; values.size() < 512; k++) {
        int32_t first = static_cast<int32_t>(static_cast<int64_t>(k) * frequency / cadence);
        if (first >= samples) {
            break;
        }
        int32_t count = per_frame + 1;
        if (first + count > samples) {
            count = samples - first;
        }
        int64_t sum = 0;
        for (int32_t i = 0; i < count; i++) {
            sum += voc[32 + first + i];
        }
        values.push_back(static_cast<int32_t>(sum * 1000 / count) - 128000);
    }
    if (values.size() < 3) {
        return mouths;
    }
    size_t highest = 0;
    size_t lowest = 0;
    for (size_t i = 1; i < values.size(); i++) {
        if (values[i] > values[highest]) {
            highest = i;
        }
        if (values[i] < values[lowest]) {
            lowest = i;
        }
    }
    int32_t maximum = INT32_MIN;
    int32_t minimum = INT32_MAX;
    for (size_t i = 0; i < values.size(); i++) {
        if (i == highest || i == lowest) {
            continue;
        }
        maximum = (std::max)(maximum, values[i]);
        minimum = (std::min)(minimum, values[i]);
    }
    int32_t reference = (std::max)(std::abs(maximum), std::abs(minimum));
    for (int32_t value : values) {
        int16_t mouth = -1;
        if (reference != 0) {
            int32_t p = (reference - std::abs(static_cast<int32_t>(static_cast<int16_t>(value)))) * 100 / reference;
            if (p >= 95) {
                mouth = -1;
            } else if (p >= 78) {
                mouth = 8;
            } else if (p >= 65) {
                mouth = 3;
            } else if (p >= 42) {
                mouth = 2;
            } else if (p >= 25) {
                mouth = 10;
            } else {
                mouth = 11;
            }
        }
        mouths.push_back(mouth);
    }
    return mouths;
}

// Bouche de la voix à l'image de scène courante (10 par seconde), numéro du jeu, -1 = aucune. Gros plan (filtre de
// la version CD, 1116E) : 10 ou 11 après un 10 ou un 11 devient -1.
static int voiceMouth(ConvFrame *frame, bool closeup) {
    int index = static_cast<int>((SDL_GetTicks() - frame->creationTime) / 100);
    while (frame->voice_index < index) {
        frame->voice_index++;
        int mouth = -1;
        if (frame->voice_index < static_cast<int>(frame->voice_mouths.size())) {
            mouth = frame->voice_mouths[frame->voice_index];
        }
        if (closeup && (mouth == 10 || mouth == 11) && (frame->voice_mouth == 10 || frame->voice_mouth == 11)) {
            mouth = -1;
        }
        frame->voice_mouth = mouth;
    }
    return frame->voice_mouth;
}

static void drawAnchored(FrameBuffer *fb, RLEShape *shape, Point2D anchor) {
    Point2D saved = shape->position;
    Point2D position;
    position.x = anchor.x - shape->GetLeft();
    position.y = anchor.y + CONV_TOP_BAR_HEIGHT + 1 - shape->GetTop();
    shape->SetPosition(&position);
    fb->drawShape(shape);
    shape->SetPosition(&saved);
}

void SCConvPlayer::drawGroupScene(void) {
    ConvFrame *frame = this->current_frame;
    for (size_t i = 0; i < frame->participants.size(); i++) {
        CharFigure *participant = frame->participants[i];
        Point2D anchor = frame->participant_positions[i];
        drawAnchored(VGA.getFrameBuffer(), participant->appearances->GetShape(participant->image_offset), anchor);
        if (static_cast<int>(i) != frame->talker) {
            continue;
        }
        int mouth = -1;
        // Sans voix, la version CD laisse la bouche immobile (CONV_SYSTEM.md § 8.5).
        if (!frame->voice_mouths.empty()) {
            mouth = voiceMouth(frame, false);
        } else if (!frame->cd_format) {
            mouth = groupMouth(frame->lip_text, SDL_GetTicks() - frame->creationTime);
        }
        if (mouth >= 0) {
            drawAnchored(VGA.getFrameBuffer(), participant->appearances->GetShape(participant->image_offset + mouth),
                         anchor);
        }
    }
    for (size_t i = 0; i < CONV_TOP_BAR_HEIGHT + 1; i++) {
        VGA.getFrameBuffer()->fillLineColor(i, 0x00);
    }
    for (size_t i = 0; i < CONV_BOTTOM_BAR_HEIGHT; i++) {
        VGA.getFrameBuffer()->fillLineColor(199 - i, 0x00);
    }
}

void SCConvPlayer::applyFacePalette(void) {
    PakEntry *entry = convPals.GetEntry(this->current_frame->facePaletteID);
    ByteStream paletteReader;
    paletteReader.Set(entry->data, entry->size);
    this->palette.ReadPatch(&paletteReader);
    VGA.setPalette(&this->palette);
}

void SCConvPlayer::drawCloseUpFace(int mouth) {
    ConvFrame *frame = this->current_frame;
    auto found = faces_shape.find(frame->face_expression);
    if (found == faces_shape.end()) {
        return;
    }
    bool blink = this->blinking && (frame->face_expression <= 20 || frame->face_expression == 255);
    for (size_t k = 0; k < found->second.size(); k++) {
        uint8_t image = found->second[k];
        if (image == kFaceMouthSlot) {
            if (blink && frame->face_expression == 255) {
                drawAnchored(VGA.getFrameBuffer(), frame->face->appearances->GetShape(kFaceBlink), frame->face_position);
            }
            if (mouth >= 0) {
                drawAnchored(VGA.getFrameBuffer(), frame->face->appearances->GetShape(mouth + 1), frame->face_position);
            }
            continue;
        }
        if (k == 1 && blink) {
            image = kFaceBlink;
        }
        drawAnchored(VGA.getFrameBuffer(), frame->face->appearances->GetShape(image), frame->face_position);
    }
}

void SCConvPlayer::drawCloseUp(bool lips) {
    ConvFrame *frame = this->current_frame;
    if (frame->face != nullptr) {
        int mouth = -1;
        if (lips) {
            if (!frame->voice_mouths.empty()) {
                mouth = voiceMouth(frame, true);
            } else {
                mouth = closeUpMouth(frame->lip_text, (SDL_GetTicks() - frame->creationTime) / 100);
            }
        }
        this->drawCloseUpFace(mouth);
    }
    for (size_t i = 0; i < CONV_TOP_BAR_HEIGHT + 1; i++) {
        VGA.getFrameBuffer()->fillLineColor(i, 0x00);
    }
    for (size_t i = 0; i < CONV_BOTTOM_BAR_HEIGHT; i++) {
        VGA.getFrameBuffer()->fillLineColor(199 - i, 0x00);
    }
}

// Voix de la réplique courante. Fichier introuvable : réplique sans voix, comme le jeu quand les voix sont
// absentes (CONV_SYSTEM.md § 8.2) ; le texte reste affiché pendant sa durée.
void SCConvPlayer::startVoice(void) {
    ConvFrame *frame = this->current_frame;
    if (frame->sound_file_name == nullptr || frame->sound_file_name->length() == 0) {
        return;
    }
    std::string filename = "F:" + *frame->sound_file_name + ".VOC";
    std::transform(filename.begin(), filename.end(), filename.begin(), ::toupper);
    TreEntry *sound_entry = Assets.GetEntryByName(filename.c_str());
    if (sound_entry == nullptr) {
        printf("CONV %d: voix %s introuvable, replique sans voix (%u ms)\n", this->conversationID, filename.c_str(),
               frame->duration > 0 ? frame->duration : 5000);
        delete frame->sound_file_name;
        frame->sound_file_name = nullptr;
        return;
    }
    Mixer.playSoundVoc(sound_entry->data, sound_entry->size, 6);
    frame->voice_mouths = voiceLipTable(sound_entry->data, sound_entry->size, 10);
    frame->voice_index = -1;
    frame->voice_mouth = -1;
    printf("CONV %d: voix %s, %zu bouches calculees sur le son\n", this->conversationID, filename.c_str(),
           frame->voice_mouths.size());
}

int SCConvPlayer::layerOffset(size_t index) {
    ConvFrame *frame = this->current_frame;
    if (!frame->game_positions || frame->bgYs == nullptr || frame->bgYs->empty()) {
        return 0;
    }
    // Question : la scène passe en plein écran, seuls le dernier élément du décor et la tête descendent de 24
    // lignes (ils restent en place à l'écran). Équipiers : les éléments suivants sont des enfants du premier.
    if (frame->mode == ConvFrame::CONV_CONTRACT_CHOICE && index + 1 < frame->bgLayers->size()) {
        return -(CONV_TOP_BAR_HEIGHT + 1);
    }
    if (frame->mode == ConvFrame::CONV_WINGMAN_CHOICE && index > 0) {
        return (*frame->bgYs)[0];
    }
    return 0;
}

static SCZone *rectangleZone(uint8_t id, const std::string &label, int x1, int y1, int x2, int y2) {
    SCZone *zone = new SCZone();
    zone->id = id;
    zone->label = new std::string(label);
    zone->quad = new std::vector<Point2D *>();
    Point2D *p1 = new Point2D();
    p1->x = x1;
    p1->y = y1;
    Point2D *p2 = new Point2D();
    p2->x = x2 + 1;
    p2->y = y1;
    Point2D *p3 = new Point2D();
    p3->x = x2 + 1;
    p3->y = y2 + 1;
    Point2D *p4 = new Point2D();
    p4->x = x1;
    p4->y = y2 + 1;
    zone->quad->push_back(p1);
    zone->quad->push_back(p2);
    zone->quad->push_back(p3);
    zone->quad->push_back(p4);
    return zone;
}

void SCConvPlayer::clearZones(void) {
    for (SCZone *zone : this->zones) {
        if (zone->quad != nullptr) {
            for (Point2D *point : *zone->quad) {
                delete point;
            }
            delete zone->quad;
        }
        delete zone->label;
        delete zone;
    }
    this->zones.clear();
}

// Zones de la question (CONV_SYSTEM.md § 7.5) : les bornes du jeu sont en unités de souris (x de 0 à 639), soit
// x 59 à 139 à l'écran, sur la main ; partout ailleurs, refus. CheckZones s'arrête à la première zone trouvée.
void SCConvPlayer::addContractChoiceZones(void) {
    SCZone *accept = rectangleZone(1, "Accept Mission", 59, 113, 139, 199);
    accept->onclick = std::bind(&SCConvPlayer::clicked, this, std::placeholders::_1, std::placeholders::_2);
    this->zones.push_back(accept);
    SCZone *reject = rectangleZone(2, "Reject Mission", 0, 0, 319, 199);
    reject->onclick = std::bind(&SCConvPlayer::clicked, this, std::placeholders::_1, std::placeholders::_2);
    this->zones.push_back(reject);
}

// Zones des pilotes (CONV_SYSTEM.md § 7.6), dans l'ordre de test du jeu, pour les pilotes proposés seulement.
// Survolé : l'image 1 du pilote (ses yeux) est dessinée par-dessus son image 0.
void SCConvPlayer::addWingmanZones(ConvFrame *frame) {
    static const int bounds[5][5] = {
        {
            1,
            187,
            72,
            232,
            191,
        },
        {
            2,
            145,
            75,
            187,
            197,
        },
        {
            3,
            29,
            68,
            75,
            195,
        },
        {
            4,
            84,
            72,
            132,
            193,
        },
        {
            5,
            242,
            70,
            281,
            199,
        },
    };
    for (const auto &bound : bounds) {
        uint8_t pilot = static_cast<uint8_t>(bound[0]);
        auto found = std::find(frame->pilots.begin(), frame->pilots.end(), pilot);
        if (found == frame->pilots.end()) {
            continue;
        }
        size_t index = static_cast<size_t>(found - frame->pilots.begin());
        CharFigure *figure = frame->participants[index];
        SCZone *zone = rectangleZone(pilot, "Select " + kPilotNames[pilot], bound[1], bound[2], bound[3], bound[4]);
        zone->onclick = std::bind(&SCConvPlayer::selectWingMan, this, std::placeholders::_1, std::placeholders::_2);
        zone->hover_shape = figure->appearances->GetShape(figure->image_offset + 1);
        zone->hover_shape->uncompressed = false;
        Point2D anchor = frame->participant_positions[index];
        zone->hover_position.x = anchor.x - zone->hover_shape->GetLeft();
        zone->hover_position.y = anchor.y + CONV_TOP_BAR_HEIGHT + 1 - zone->hover_shape->GetTop();
        this->zones.push_back(zone);
    }
}

void SCConvPlayer::prepareZones(void) {
    this->clearZones();
    if (this->current_frame->mode == ConvFrame::CONV_CONTRACT_CHOICE) {
        this->addContractChoiceZones();
    } else if (this->current_frame->mode == ConvFrame::CONV_WINGMAN_CHOICE) {
        this->addWingmanZones(this->current_frame);
    }
}

void SCConvPlayer::clicked(void *none, uint8_t id) {
    if (this->current_frame->choice >= 0) {
        return;
    }
    this->current_frame->choice = id == 1 ? 1 : 0;
    this->current_frame->choice_time = SDL_GetTicks();
    printf("CONV %d: clic, %s\n", this->conversationID, id == 1 ? "Accept Mission" : "Reject Mission");
}

void SCConvPlayer::selectWingMan(void *none, uint8_t id) {
    if (this->current_frame->choice >= 0) {
        return;
    }
    this->current_frame->choice = id;
    this->current_frame->choice_time = SDL_GetTicks();
    printf("CONV %d: clic, pilote %d\n", this->conversationID, id);
}

SCZone *SCConvPlayer::CheckZones(void) {
    SCZone *hit = nullptr;
    for (SCZone *zone : this->zones) {
        zone->hovered = false;
        if (hit == nullptr && zone->quad != nullptr && isPointInQuad(Mouse.getPosition(), zone->quad)) {
            hit = zone;
        }
    }
    if (hit == nullptr) {
        Mouse.setMode(SCMouse::CURSOR);
        return nullptr;
    }
    hit->hovered = true;
    Mouse.setMode(SCMouse::VISOR);
    if (Mouse.buttons[MouseButton::LEFT].event == MouseButton::RELEASED) {
        Mouse.buttons[MouseButton::LEFT].event = MouseButton::NONE;
        hit->onAction();
    }
    // Libellé : police de la conversation, couleur de la dernière réplique, centré sur l'écran à y = 175.
    Point2D origin;
    origin.x = 0;
    origin.y = 175;
    VGA.getFrameBuffer()->printText(this->font, &origin, (char *)hit->label->c_str(), this->current_frame->textColor, 0,
                                    static_cast<uint32_t>(hit->label->length()), 2, 0, TEXT_JUSTIFY_CENTER, 320);
    return hit;
}

void SCConvPlayer::runChoice(void) {
    ConvFrame *frame = this->current_frame;
    uint32_t now = SDL_GetTicks();
    uint32_t ticks = (now - frame->creationTime) * 60 / 1000;
    bool opened = ticks >= 24;

    if (frame->mode == ConvFrame::CONV_CONTRACT_CHOICE) {
        this->applyFacePalette();
        if (frame->face != nullptr) {
            static const uint8_t kQuestionImages[4] = {
                kFaceHead,
                54,
                kFaceHair,
                kFaceClothes,
            };
            for (uint8_t image : kQuestionImages) {
                drawAnchored(VGA.getFrameBuffer(), frame->face->appearances->GetShape(image), frame->face_position);
            }
        }
    } else {
        if (!frame->participants.empty()) {
            PakEntry *entry = convPals.GetEntry(frame->participants[0]->paletteID);
            ByteStream paletteReader;
            paletteReader.Set(entry->data, entry->size);
            this->palette.ReadPatch(&paletteReader);
            VGA.setPalette(&this->palette);
        }
        for (size_t i = 0; i < frame->participants.size(); i++) {
            CharFigure *participant = frame->participants[i];
            RLEShape *body = participant->appearances->GetShape(participant->image_offset);
            body->uncompressed = false;
            drawAnchored(VGA.getFrameBuffer(), body, frame->participant_positions[i]);
        }
    }

    if (frame->choice < 0 && opened) {
        this->CheckZones();
    } else {
        for (SCZone *zone : this->zones) {
            zone->hovered = false;
        }
    }
    for (SCZone *zone : this->zones) {
        zone->draw();
    }

    int top = 0;
    int bottom = 0;
    if (frame->choice >= 0) {
        uint32_t closing = (now - frame->choice_time) * 60 / 1000;
        if (closing >= 24) {
            if (frame->mode == ConvFrame::CONV_CONTRACT_CHOICE) {
                this->answerQuestion(frame->choice == 1);
            } else {
                GameState.wingman = pilotWingmanName(static_cast<uint8_t>(frame->choice));
                printf("CONV %d: equipier choisi %s\n", this->conversationID, GameState.wingman.c_str());
            }
            frame->SetExpired(true);
            closing = 24;
        }
        top = static_cast<int>(closing);
        bottom = 2 * static_cast<int>(closing);
    } else if (!opened) {
        top = 24 - static_cast<int>(ticks);
        bottom = 48 - 2 * static_cast<int>(ticks);
    }
    for (int i = 0; i < top; i++) {
        VGA.getFrameBuffer()->fillLineColor(i, 0x00);
    }
    for (int i = 0; i < bottom; i++) {
        VGA.getFrameBuffer()->fillLineColor(199 - i, 0x00);
    }
    Mouse.draw();
}

/**
 * Runs a single frame of the conversation player, updating the game state and rendering the graphics.
 *
 * This function is called by the game engine to update the state of the conversation player and to
 * render the graphics. It checks if the current frame needs to be updated and if so, reads the next
 * frame from the conversation data. It also checks if the buttons need to be drawn and if so, draws
 * them.
 *
 * @param None
 *
 * @return None
 *
 */
void SCConvPlayer::runFrame(void) {

    if (!initialized) {
        stop();
        Game->log("Conv ID %d was not initialized: Stopping.\n", this->conversationID);
        return;
    }
    if (this->conversation_frames.size() == 0) {
        stop();
        Game->log("Conv ID %d has no frames: Stopping.\n", this->conversationID);
        return;
    }
    if (this->current_frame == nullptr) {
        this->current_frame = this->conversation_frames[0];
        this->current_frame->creationTime       = SDL_GetTicks();
        this->current_frame->SetExpired(false);
        this->startVoice();
        this->prepareZones();
    }
    // If frame needs to be update
    if (this->current_frame->mode != ConvFrame::CONV_CONTRACT_CHOICE && this->current_frame->mode != ConvFrame::CONV_WINGMAN_CHOICE) {
        CheckFrameExpired();
    }

    if (this->current_frame->IsExpired()) {
        this->conversation_frames.erase(this->conversation_frames.begin());
        this->conversation_frames.shrink_to_fit();
        if (this->conversation_frames.size() == 0) {
            stop();
            Game->log("Conv ID %d has no frames: Stopping.\n", this->conversationID);
            return;
        }
        this->current_frame = this->conversation_frames[0];
        this->current_frame->creationTime       = SDL_GetTicks();
        this->startVoice();
        this->prepareZones();
    }
    
    checkButtons();

    VGA.activate();
    VGA.getFrameBuffer()->fillWithColor(255);
    // Update the palette for the current background
    if (this->current_frame->bgLayers != nullptr && this->current_frame->bgPalettes != nullptr) {
        for (size_t i = 0; i < this->current_frame->bgLayers->size(); i++) {
            ByteStream paletteReader;
            paletteReader.Set((*this->current_frame->bgPalettes)[i], 772);
            this->palette.ReadPatch(&paletteReader);
        }
        VGA.setPalette(&this->palette);
        for (size_t i = 0; i < this->current_frame->bgLayers->size(); i++) {
            RLEShape *shape = (*this->current_frame->bgLayers)[i];
            int offset = this->layerOffset(i);
            Point2D saved = shape->position;
            if (offset != 0) {
                Point2D moved = saved;
                moved.y += offset;
                shape->SetPosition(&moved);
            }
            VGA.getFrameBuffer()->drawShape(shape);
            shape->SetPosition(&saved);
        }
    }
    
    
    // Draw static
    
    

    uint32_t scene_tick = SDL_GetTicks() / 100;
    if (scene_tick != this->blink_tick) {
        this->blink_tick = scene_tick;
        this->blinking = std::rand() % 30 == 0;
    }
    switch (this->current_frame->mode) {
        case ConvFrame::CONV_SHOW_TEXT:
        {
            if (this->current_frame->face == NULL && this->current_frame->group_scene) {
                this->applyGroupPalette();
                this->drawGroupScene();
                break;
            }
            if (this->current_frame->game_positions) {
                this->applyFacePalette();
                this->drawCloseUp(false);
                break;
            }
            for (size_t i = 0; i < CONV_TOP_BAR_HEIGHT + 1; i++)
            VGA.getFrameBuffer()->fillLineColor(i, 0x00);

            for (size_t i = 0; i < CONV_BOTTOM_BAR_HEIGHT; i++)
                VGA.getFrameBuffer()->fillLineColor(199 - i, 0x00);
            ByteStream paletteReader;

            paletteReader.Set(convPals.GetEntry(this->current_frame->facePaletteID)->data, convPals.GetEntry(this->current_frame->facePaletteID)->size);
            this->palette.ReadPatch(&paletteReader);

            int32_t pos = 0;

            if (this->current_frame->facePosition == ConvFrame::FACE_LEFT)
                pos = -30;

            if (this->current_frame->facePosition == ConvFrame::FACE_RIGHT)
                pos = 30;

            if (this->current_frame->face != NULL) {
                RLEShape *s = nullptr;
                if (legacy_faces_shape.find(this->current_frame->face_expression) != legacy_faces_shape.end()) {
                    for (auto shape : legacy_faces_shape[this->current_frame->face_expression]) {
                        s = this->current_frame->face->appearances->GetShape(shape);
                        s->SetPositionX(pos);
                        VGA.getFrameBuffer()->drawShape(s);
                    }
                }
            } else {
                for (size_t i = 0; i < this->current_frame->participants.size(); i++) {
                    CharFigure *participant = this->current_frame->participants[i];
                    RLEShape *s = participant->appearances->GetShape(0);
                    Point2D position = {participant->x, participant->y};
                    s->SetPosition(&position);
                    VGA.getFrameBuffer()->drawShape(s);
                    if (participant->appearances->GetNumImages() > 1) {
                        s = participant->appearances->GetShape(1);
                        s->SetPosition(&position);
                        VGA.getFrameBuffer()->drawShape(s);
                    }
                    position.x += s->GetWidth();
                }
            }
            for (size_t i = 0; i < CONV_TOP_BAR_HEIGHT; i++)
                VGA.getFrameBuffer()->fillLineColor(i, 0x00);

            for (size_t i = 0; i < CONV_BOTTOM_BAR_HEIGHT; i++)
                VGA.getFrameBuffer()->fillLineColor(199 - i, 0x00);

        }
        break;
        case ConvFrame::CONV_CLOSEUP:
        {
            if (this->current_frame->game_positions) {
                this->applyFacePalette();
                this->drawCloseUp(true);
                break;
            }
            for (size_t i = 0; i < CONV_TOP_BAR_HEIGHT + 1; i++)
            VGA.getFrameBuffer()->fillLineColor(i, 0x00);

            for (size_t i = 0; i < CONV_BOTTOM_BAR_HEIGHT; i++)
                VGA.getFrameBuffer()->fillLineColor(199 - i, 0x00);
            ByteStream paletteReader;

            paletteReader.Set(convPals.GetEntry(this->current_frame->facePaletteID)->data, convPals.GetEntry(this->current_frame->facePaletteID)->size);
            this->palette.ReadPatch(&paletteReader);
            VGA.setPalette(&this->palette);
            int32_t pos = 0;

            if (this->current_frame->mode == ConvFrame::CONV_CLOSEUP) {
                if (this->current_frame->facePosition == ConvFrame::FACE_LEFT)
                    pos = -30;

                if (this->current_frame->facePosition == ConvFrame::FACE_RIGHT)
                    pos = 30;
            }

            if (this->current_frame->face != NULL) {
                RLEShape *s = nullptr;
                if (legacy_faces_shape.find(this->current_frame->face_expression) != legacy_faces_shape.end()) {
                    for (auto shape : legacy_faces_shape[this->current_frame->face_expression]) {
                        s = this->current_frame->face->appearances->GetShape(shape);
                        s->SetPositionX(pos);
                        VGA.getFrameBuffer()->drawShape(s);
                    }
                }
                for (size_t i = 03; i < 11 && this->current_frame->text != nullptr && this->current_frame->text[0] != '\0'; i++) {
                    RLEShape *s = this->current_frame->face->appearances->GetShape(3 + (SDL_GetTicks() / 100) % 10);
                    s->SetPositionX(pos);
                    VGA.getFrameBuffer()->drawShape(s);
                }
            }
        }
        break;
        case ConvFrame::CONV_WIDE:
        {
            if (this->current_frame->group_scene) {
                this->applyGroupPalette();
                this->drawGroupScene();
                break;
            }
            for (size_t i = 0; i < CONV_TOP_BAR_HEIGHT; i++) {
                VGA.getFrameBuffer()->fillLineColor(i, 0x00);
            }
            for (size_t i = 0; i < CONV_BOTTOM_BAR_HEIGHT; i++) {
                VGA.getFrameBuffer()->fillLineColor(199 - i, 0x00);
            }
        }
        break;
        case ConvFrame::CONV_WINGMAN_CHOICE:
        case ConvFrame::CONV_CONTRACT_CHOICE:
        {
            this->runChoice();
        }
        break;
    }
    
    this->DrawText();
    
    VGA.vSync();
}
