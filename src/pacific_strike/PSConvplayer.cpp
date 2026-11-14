#include "PSConvplayer.h"
#include "PSConvAssetManager.h"

PSConvPlayer::PSConvPlayer() {
}

PSConvPlayer::~PSConvPlayer() {
}

bool PSConvPlayer::readByte(uint8_t &value) {
    if (this->pos + 1 > this->script_size) {
        return false;
    }
    value = this->script[this->pos];
    this->pos += 1;
    return true;
}

bool PSConvPlayer::readWord(uint16_t &value) {
    if (this->pos + 2 > this->script_size) {
        return false;
    }
    value = (uint16_t)(this->script[this->pos] | (this->script[this->pos + 1] << 8));
    this->pos += 2;
    return true;
}

bool PSConvPlayer::readSignedWord(int16_t &value) {
    uint16_t word = 0;
    if (!this->readWord(word)) {
        return false;
    }
    value = (int16_t)word;
    return true;
}

// Nom sur 9 octets, terminé par 0.
bool PSConvPlayer::readName(std::string &value) {
    if (this->pos + 9 > this->script_size) {
        return false;
    }
    value.clear();
    for (size_t i = 0; i < 9 && this->script[this->pos + i] != 0; i++) {
        value.push_back((char)this->script[this->pos + i]);
    }
    this->pos += 9;
    return true;
}

// Texte terminé par 0, sans numéro de voix devant.
bool PSConvPlayer::readText(ConvRecord &record) {
    record.text.clear();
    while (this->pos < this->script_size) {
        uint8_t c = this->script[this->pos];
        this->pos++;
        if (c == 0) {
            return true;
        }
        record.text.push_back((char)c);
    }
    return false;
}

// Un enregistrement d'un bloc IF_FLAG non retenu est lu sans être gardé.
void PSConvPlayer::keep(const ConvRecord &record) {
    if (!this->skipping) {
        this->records.push_back(record);
    }
}

// 1 GROUP_SHOT_ADD_CHARACTER : figure (9), x, y.
bool PSConvPlayer::readFigure(size_t at, int level) {
    ConvRecord figure;
    figure.offset = at;
    figure.op = CONV_GROUP_SHOT_ADD_CHARACTER;
    figure.level = level;
    if (!this->readName(figure.name) || !this->readSignedWord(figure.x) || !this->readSignedWord(figure.y)) {
        return false;
    }
    this->keep(figure);
    return true;
}

// 10 SHOW_TEXT : couleur, texte, voix (2 mots, non jouée). Après un CLOSEUP, la même personne continue de parler.
bool PSConvPlayer::readShowText(size_t at, uint8_t op, int level) {
    ConvRecord text;
    text.offset = at;
    text.op = op;
    text.level = level;
    uint16_t voice = 0;
    uint16_t voice_extra = 0;
    if (!this->readByte(text.color) || !this->readText(text) || !this->readWord(voice) || !this->readWord(voice_extra)) {
        return false;
    }
    text.value = voice;
    this->keep(text);
    return true;
}

// 17 GROUP_SHOT_COMMANDER : n, n numéros d'image, x, y de la cible ; gardé pour les plans de son GROUP_SHOT (le plan
// sans texte et chaque réplique), dessiné par drawCommander.
bool PSConvPlayer::readCommander(size_t at) {
    ConvCommander commander;
    uint16_t count = 0;
    if (!this->readWord(count)) {
        return false;
    }
    for (uint16_t k = 0; k < count; k++) {
        uint16_t image = 0;
        if (!this->readWord(image)) {
            return false;
        }
        commander.images.push_back(image);
    }
    if (!this->readSignedWord(commander.x) || !this->readSignedWord(commander.y)) {
        return false;
    }
    if (!this->skipping && !commander.images.empty()) {
        this->commanders[this->group_offset] = commander;
        this->open_commander = commander;
        this->commander_open = true;
    }
    printf("CONV %d: GROUP_SHOT_COMMANDER (octet %zu), %d images, cible (%d, %d)\n", this->conversationID, at, count, commander.x, commander.y);
    return true;
}

// 0 GROUP_SHOT : décor (9), puis les commandes 1, 10 et 17 qui suivent.
bool PSConvPlayer::readGroupShot(size_t at) {
    ConvRecord shot;
    shot.offset = at;
    shot.op = CONV_GROUP_SHOT;
    shot.level = 0;
    if (!this->readName(shot.background)) {
        return false;
    }
    this->keep(shot);
    this->group_offset = at;
    this->commander_open = false;
    while (this->pos < this->script_size) {
        size_t sub = this->pos;
        uint8_t op = this->script[this->pos];
        bool ok = true;
        if (op == CONV_GROUP_SHOT_ADD_CHARACTER) {
            this->pos++;
            ok = this->readFigure(sub, 1);
        } else if (op == CONV_SHOW_TEXT) {
            this->pos++;
            ok = this->readShowText(sub, CONV_SHOW_TEXT, 1);
            if (ok && this->commander_open && !this->skipping) {
                this->commanders[sub] = this->open_commander;
            }
        } else if (op == PS_CONV_GROUP_SHOT_COMMANDER) {
            this->pos++;
            ok = this->readCommander(sub);
        } else {
            break;
        }
        if (!ok) {
            return false;
        }
    }
    this->commander_open = false;
    return true;
}

// 3 CLOSEUP : visage (9), image, décor (9), x, y, texte, couleur, voix (2 mots) ; puis les commandes 10 qui suivent.
bool PSConvPlayer::readCloseUp(size_t at) {
    ConvRecord line;
    line.offset = at;
    line.op = CONV_CLOSEUP;
    line.level = 0;
    uint16_t voice = 0;
    uint16_t voice_extra = 0;
    if (!this->readName(line.name) || !this->readByte(line.baseFrame) || !this->readName(line.background) || !this->readSignedWord(line.x) || !this->readSignedWord(line.y) || !this->readText(line) || !this->readByte(line.color) || !this->readWord(voice) || !this->readWord(voice_extra)) {
        return false;
    }
    line.value = voice;
    this->keep(line);
    while (this->pos < this->script_size && this->script[this->pos] == CONV_SHOW_TEXT) {
        size_t sub = this->pos;
        this->pos++;
        if (!this->readShowText(sub, CONV_CLOSEUP_CONTINUATION, 1)) {
            return false;
        }
    }
    return true;
}

void PSConvPlayer::SetArchive(PakEntry *convPakEntry) {
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
    this->script = convPakEntry->data;
    this->script_size = convPakEntry->size;
    this->pos = 0;
    this->skipping = false;
    this->records.clear();
    bool ok = true;
    while (ok && this->pos < this->script_size) {
        size_t at = this->pos;
        uint8_t op = this->script[this->pos];
        this->pos++;
        switch (op) {
        case CONV_GROUP_SHOT:
            ok = this->readGroupShot(at);
            break;
        case CONV_GROUP_SHOT_ADD_CHARACTER:
            ok = this->readFigure(at, 0);
            break;
        case CONV_CLOSEUP:
            ok = this->readCloseUp(at);
            break;
        case PS_CONV_IF_FLAG: {
            uint16_t number = 0;
            ok = this->readWord(number);
            this->skipping = PSGameState.getFlag(number) == 0;
            printf("CONV %d: IF_FLAG %d (octet %zu), valeur %d, %s\n", this->conversationID, number, at, PSGameState.getFlag(number), this->skipping ? "bloc saute" : "bloc joue");
        } break;
        case CONV_SHOW_TEXT:
            ok = this->readShowText(at, CONV_SHOW_TEXT, 0);
            break;
        case PS_CONV_ELSE:
            this->skipping = !this->skipping;
            printf("CONV %d: ELSE (octet %zu), %s\n", this->conversationID, at, this->skipping ? "bloc saute" : "bloc joue");
            break;
        case PS_CONV_ENDIF:
            this->skipping = false;
            break;
        case CONV_NOP_16:
            break;
        case PS_CONV_GROUP_SHOT_COMMANDER:
            ok = this->readCommander(at);
            break;
        default:
            printf("CONV %d: commande %d inconnue (octet %zu)\n", this->conversationID, op, at);
            ok = false;
            break;
        }
    }
    if (!ok) {
        printf("CONV %d: lecture arretee a l'octet %zu sur %zu\n", this->conversationID, this->pos, this->script_size);
    }
    printf("CONV %d: Pacific Strike, %zu enregistrements gardes\n", this->conversationID, this->records.size());
    this->buildFrames(this->records, ConvFormat::Floppy);
    initialized = true;
}

void PSConvPlayer::applyGroupPalette(void) {
    VGA.setPalette(&this->palette);
}

void PSConvPlayer::pushFrame(ConvFrame *frame, const std::vector<ConvRecord> &records, const ConvRecord &record) {
    SCConvPlayer::pushFrame(frame, records, record);
    auto found = this->commanders.find(record.offset);
    if (found != this->commanders.end()) {
        this->frame_commanders[frame] = found->second;
    }
}

// image qui couvre l'écran (en-tête 0 à 319 x 0 à 199), placée comme un élément de décor
void PSConvPlayer::drawFullScreenImage(RLEShape *shape) {
    if (shape == nullptr) {
        return;
    }
    Point2D saved = shape->position;
    Point2D position;
    position.x = -shape->GetLeft();
    position.y = CONV_TOP_BAR_HEIGHT + 1 - shape->GetTop();
    shape->SetPosition(&position);
    VGA.getFrameBuffer()->drawShape(shape);
    shape->SetPosition(&saved);
}

// PointerLayer_Draw_62EF2 : la baguette (PointerLayer_DrawStick_630DA), l'image 0, puis l'image courante. Scène à 10
// images par seconde (LayerGroup_Construct_66650) : l'image courante avance d'une étape toutes les 6 tics
// (PointerLayer_Advance_62F57) et reste sur la dernière.
void PSConvPlayer::drawCommander(const ConvCommander &commander) {
    std::vector<Point2D> &hand_points = static_cast<PSConvAssetManager &>(ConvAssetManager::getInstance()).pointer.hand_points;
    CharFigure *figure = ConvAssetManager::getInstance().GetFigure(PSGameState.getFlag(2128) != 0 ? "yucmndr" : "commndr");
    if (figure == nullptr || figure->appearances == nullptr) {
        return;
    }
    uint32_t ticks = (SDL_GetTicks() - this->current_frame->creationTime) * 60 / 1000;
    size_t last = commander.images.size() - 1;
    size_t step = (std::min)((size_t)(ticks / 6), last);
    uint16_t image = commander.images[step];
    int top = CONV_TOP_BAR_HEIGHT + 1;
    if (image < hand_points.size() && commander.images[last] < hand_points.size()) {
        Point2D hand = hand_points[image];
        Point2D last_hand = hand_points[commander.images[last]];
        int end_x = commander.x;
        int end_y = commander.y;
        if (step != last) {
            end_x = hand.x - std::abs(last_hand.x - commander.x);
            end_y = hand.y - std::abs(last_hand.y - commander.y);
        }
        VGA.getFrameBuffer()->line(hand.x, hand.y + top, end_x, end_y + top, 15);
    }
    // image n du jeu = entrée n + image_offset du jeu d'images (entrée 0 vide quand il porte une palette)
    this->drawFullScreenImage(figure->appearances->GetShape(figure->image_offset));
    this->drawFullScreenImage(figure->appearances->GetShape(image + figure->image_offset));
}

void PSConvPlayer::drawGroupScene(void) {
    SCConvPlayer::drawGroupScene();
    auto found = this->frame_commanders.find(this->current_frame);
    if (found == this->frame_commanders.end()) {
        return;
    }
    this->drawCommander(found->second);
    for (size_t i = 0; i < CONV_TOP_BAR_HEIGHT + 1; i++) {
        VGA.getFrameBuffer()->fillLineColor(i, 0x00);
    }
    for (size_t i = 0; i < CONV_BOTTOM_BAR_HEIGHT; i++) {
        VGA.getFrameBuffer()->fillLineColor(199 - i, 0x00);
    }
}
