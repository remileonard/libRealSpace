#include "ConvData.h"
#include <set>

ConvData::ConvData(const uint8_t *data, size_t size) : data(data), size(size) {}

int ConvData::parse() {
    recordList.clear();
    errorCode = ConvParseError::None;
    errorAt = 0;
    errorText.clear();
    pos = 0;
    scriptFormat = detectFormat();
    if (scriptFormat == ConvFormat::Unknown) {
        fail(0, ConvParseError::UnknownFormat, "format de conversation non reconnu");
        return static_cast<int>(errorCode);
    }
    while (pos < size) {
        size_t at = pos;
        uint8_t op = data[pos];
        pos++;
        bool ok = false;
        switch (op) {
        case CONV_GROUP_SHOT:
            ok = parseGroupShot(at);
            break;
        case CONV_CLOSEUP:
            ok = parseCloseUp(at);
            break;
        case CONV_SHOW_TEXT:
            if (scriptFormat == ConvFormat::Cd) {
                ok = parseNarration(at, 0);
            } else {
                ok = fail(at, ConvParseError::NotACommand, "octet qui n'est pas une commande");
            }
            break;
        case CONV_YESNOCHOICE_BRANCH2:
            ok = parseJump(at, op, 0);
            break;
        case CONV_SELECT_WING:
            ok = parseWing(at);
            break;
        case CONV_CHOOSE_WINGMAN:
        case CONV_NOP_16:
        case CONV_NOP_18:
            ok = addSimple(at, op);
            break;
        default:
            ok = fail(at, ConvParseError::NotACommand, "octet qui n'est pas une commande");
            break;
        }
        if (!ok) {
            return static_cast<int>(errorCode);
        }
    }
    if (!checkJumps()) {
        return static_cast<int>(errorCode);
    }
    return 0;
}

ConvFormat ConvData::format() const {
    return scriptFormat;
}

const std::vector<ConvRecord> &ConvData::records() const {
    return recordList;
}

size_t ConvData::errorOffset() const {
    return errorAt;
}

const std::string &ConvData::errorMessage() const {
    return errorText;
}

ConvFormat ConvData::detectFormat() {
    size_t text = 0;
    uint8_t kind = 0;
    if (!findFirstText(text, kind)) {
        return ConvFormat::Unknown;
    }
    size_t firstEnd = 0;
    if (!stringEnd(text, firstEnd)) {
        return ConvFormat::Unknown;
    }
    bool floppy = followsText(firstEnd + 1, kind);
    bool cd = false;
    size_t secondEnd = 0;
    if (looksLikeVoice(text, firstEnd) && stringEnd(firstEnd + 1, secondEnd)) {
        cd = followsText(secondEnd + 1, kind);
    }
    if (cd && !floppy) {
        return ConvFormat::Cd;
    }
    if (floppy && !cd) {
        return ConvFormat::Floppy;
    }
    return ConvFormat::Unknown;
}

bool ConvData::findFirstText(size_t &text, uint8_t &kind) {
    size_t at = 0;
    while (at < size) {
        uint8_t op = data[at];
        at++;
        switch (op) {
        case CONV_CLOSEUP:
            text = at + 23;
            kind = CONV_CLOSEUP;
            return text < size;
        case CONV_SHOW_TEXT:
            text = at + 1;
            kind = CONV_SHOW_TEXT;
            return text < size;
        case CONV_GROUP_SHOT:
            at += 9;
            while (at < size && data[at] == CONV_GROUP_SHOT_ADD_CHARACTER) {
                at += 14;
            }
            if (at < size && data[at] == CONV_GROUP_SHOT_CHARACTER_TALK) {
                text = at + 15;
                kind = CONV_GROUP_SHOT_CHARACTER_TALK;
                return text < size;
            }
            if (at < size && data[at] == CONV_SHOW_TEXT) {
                text = at + 2;
                kind = CONV_SHOW_TEXT;
                return text < size;
            }
            break;
        case CONV_YESNOCHOICE_BRANCH2:
        case CONV_SELECT_WING:
            at += 2;
            break;
        case CONV_CHOOSE_WINGMAN:
        case CONV_NOP_16:
        case CONV_NOP_18:
            break;
        default:
            return false;
        }
    }
    return false;
}

bool ConvData::stringEnd(size_t start, size_t &end) {
    for (size_t i = start; i < size; i++) {
        if (data[i] == 0) {
            end = i;
            return true;
        }
    }
    return false;
}

bool ConvData::looksLikeVoice(size_t start, size_t end) {
    if (end - start > 8) {
        return false;
    }
    for (size_t i = start; i < end; i++) {
        uint8_t c = data[i];
        bool digit = c >= '0' && c <= '9';
        bool letter = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
        if (!digit && !letter) {
            return false;
        }
    }
    return true;
}

bool ConvData::isTopLevel(uint8_t op) {
    switch (op) {
    case CONV_GROUP_SHOT:
    case CONV_CLOSEUP:
    case CONV_SHOW_TEXT:
    case CONV_YESNOCHOICE_BRANCH2:
    case CONV_SELECT_WING:
    case CONV_CHOOSE_WINGMAN:
    case CONV_NOP_16:
    case CONV_NOP_18:
        return true;
    default:
        return false;
    }
}

bool ConvData::followsText(size_t at, uint8_t kind) {
    if (kind == CONV_CLOSEUP) {
        at++;
    }
    if (at == size) {
        return true;
    }
    if (at > size) {
        return false;
    }
    uint8_t next = data[at];
    if (isTopLevel(next)) {
        return true;
    }
    if (kind == CONV_GROUP_SHOT_CHARACTER_TALK) {
        return next == CONV_GROUP_SHOT_CHARACTER_TALK;
    }
    if (kind == CONV_CLOSEUP) {
        return next == CONV_CLOSEUP_CONTINUATION || next == CONV_YESNOCHOICE_BRANCH1;
    }
    return next == CONV_GROUP_SHOT_CHARACTER_TALK || next == CONV_CLOSEUP_CONTINUATION ||
           next == CONV_YESNOCHOICE_BRANCH1;
}

bool ConvData::fail(size_t at, ConvParseError code, const char *message) {
    errorCode = code;
    errorAt = at;
    errorText = message;
    return false;
}

bool ConvData::need(size_t count) {
    if (pos + count > size) {
        return fail(pos, ConvParseError::PastEnd, "champ au-delà de la fin du script");
    }
    return true;
}

bool ConvData::readByte(uint8_t &value) {
    if (!need(1)) {
        return false;
    }
    value = data[pos];
    pos++;
    return true;
}

bool ConvData::readWord(uint16_t &value) {
    if (!need(2)) {
        return false;
    }
    value = static_cast<uint16_t>(data[pos] | (data[pos + 1] << 8));
    pos += 2;
    return true;
}

bool ConvData::readSignedWord(int16_t &value) {
    uint16_t raw = 0;
    if (!readWord(raw)) {
        return false;
    }
    value = static_cast<int16_t>(raw);
    return true;
}

bool ConvData::readName(std::string &value) {
    if (!need(9)) {
        return false;
    }
    value.clear();
    for (size_t i = 0; i < 9 && data[pos + i] != 0; i++) {
        value.push_back(static_cast<char>(data[pos + i]));
    }
    pos += 9;
    return true;
}

bool ConvData::readString(std::string &value) {
    value.clear();
    while (pos < size) {
        uint8_t c = data[pos];
        pos++;
        if (c == 0) {
            return true;
        }
        value.push_back(static_cast<char>(c));
    }
    return fail(pos, ConvParseError::UnterminatedString, "chaîne sans 0 final");
}

bool ConvData::readText(ConvRecord &record) {
    if (scriptFormat == ConvFormat::Cd && !readString(record.voice)) {
        return false;
    }
    return readString(record.text);
}

bool ConvData::peekIs(uint8_t value) {
    return pos < size && data[pos] == value;
}

void ConvData::begin(ConvRecord &record, size_t at, uint8_t op, int level) {
    record.offset = at;
    record.op = op;
    record.level = level;
}

bool ConvData::addSimple(size_t at, uint8_t op) {
    ConvRecord record;
    begin(record, at, op, 0);
    recordList.push_back(record);
    return true;
}

bool ConvData::parseJump(size_t at, uint8_t op, int level) {
    ConvRecord record;
    begin(record, at, op, level);
    if (!readWord(record.value)) {
        return false;
    }
    recordList.push_back(record);
    return true;
}

bool ConvData::parseWing(size_t at) {
    ConvRecord record;
    begin(record, at, CONV_SELECT_WING, 0);
    uint8_t screen = 0;
    uint8_t wing = 0;
    if (!readByte(screen) || !readByte(wing)) {
        return false;
    }
    record.value = static_cast<uint16_t>(screen | (wing << 8));
    recordList.push_back(record);
    return true;
}

bool ConvData::parseNarration(size_t at, int level) {
    ConvRecord record;
    begin(record, at, CONV_SHOW_TEXT, level);
    if (!readByte(record.color) || !readText(record)) {
        return false;
    }
    recordList.push_back(record);
    return true;
}

bool ConvData::parseGroupShot(size_t at) {
    ConvRecord shot;
    begin(shot, at, CONV_GROUP_SHOT, 0);
    if (!readName(shot.background)) {
        return false;
    }
    recordList.push_back(shot);
    while (peekIs(CONV_GROUP_SHOT_ADD_CHARACTER)) {
        ConvRecord extra;
        begin(extra, pos, CONV_GROUP_SHOT_ADD_CHARACTER, 1);
        pos++;
        if (!readName(extra.name) || !readSignedWord(extra.x) || !readSignedWord(extra.y)) {
            return false;
        }
        recordList.push_back(extra);
    }
    while (peekIs(CONV_GROUP_SHOT_CHARACTER_TALK) || peekIs(CONV_SHOW_TEXT)) {
        size_t sub = pos;
        uint8_t op = data[pos];
        pos++;
        if (op == CONV_SHOW_TEXT) {
            if (!parseNarration(sub, 1)) {
                return false;
            }
            continue;
        }
        ConvRecord talk;
        begin(talk, sub, CONV_GROUP_SHOT_CHARACTER_TALK, 1);
        if (!readName(talk.name) || !readSignedWord(talk.x) || !readSignedWord(talk.y) || !readByte(talk.color) ||
            !readText(talk)) {
            return false;
        }
        recordList.push_back(talk);
    }
    return true;
}

bool ConvData::parseCloseUp(size_t at) {
    ConvRecord line;
    begin(line, at, CONV_CLOSEUP, 0);
    if (!readName(line.name) || !readByte(line.baseFrame) || !readName(line.background) ||
        !readSignedWord(line.x) || !readSignedWord(line.y) || !readText(line) || !readByte(line.color)) {
        return false;
    }
    recordList.push_back(line);
    while (peekIs(CONV_CLOSEUP_CONTINUATION) || peekIs(CONV_SHOW_TEXT) || peekIs(CONV_YESNOCHOICE_BRANCH1)) {
        size_t sub = pos;
        uint8_t op = data[pos];
        pos++;
        bool ok = false;
        if (op == CONV_CLOSEUP_CONTINUATION) {
            ConvRecord next;
            begin(next, sub, CONV_CLOSEUP_CONTINUATION, 1);
            ok = readText(next);
            if (ok) {
                recordList.push_back(next);
            }
        } else if (op == CONV_SHOW_TEXT) {
            ok = parseNarration(sub, 1);
        } else {
            ok = parseJump(sub, CONV_YESNOCHOICE_BRANCH1, 1);
        }
        if (!ok) {
            return false;
        }
    }
    return true;
}

bool ConvData::checkJumps() {
    std::set<size_t> starts;
    for (const auto &record : recordList) {
        starts.insert(record.offset);
    }
    for (const auto &record : recordList) {
        if (record.op != CONV_YESNOCHOICE_BRANCH1 && record.op != CONV_YESNOCHOICE_BRANCH2) {
            continue;
        }
        size_t target = record.offset + 3 + record.value;
        if (target != size && starts.count(target) == 0) {
            return fail(record.offset, ConvParseError::BadJump, "saut hors d'une commande");
        }
    }
    return true;
}
