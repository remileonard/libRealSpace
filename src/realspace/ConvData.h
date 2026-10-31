#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

enum ConvCommand : uint8_t {
    CONV_GROUP_SHOT = 0,
    CONV_GROUP_SHOT_ADD_CHARACTER = 1,
    CONV_GROUP_SHOT_CHARACTER_TALK = 2,
    CONV_CLOSEUP = 3,
    CONV_CLOSEUP_CONTINUATION = 4,
    CONV_SHOW_TEXT = 10,
    CONV_YESNOCHOICE_BRANCH1 = 11,
    CONV_YESNOCHOICE_BRANCH2 = 12,
    CONV_SELECT_WING = 14,
    CONV_CHOOSE_WINGMAN = 15,
    CONV_NOP_16 = 16,
    CONV_NOP_18 = 18
};

enum class ConvFormat {
    Floppy,
    Cd,
    Unknown
};

enum class ConvParseError {
    None = 0,
    UnknownFormat = 1,
    NotACommand = 2,
    PastEnd = 3,
    UnterminatedString = 4,
    BadJump = 5
};

struct ConvRecord {
    size_t offset{0};
    uint8_t op{0};
    int level{0};
    std::string name;
    std::string background;
    uint8_t baseFrame{0};
    int16_t x{0};
    int16_t y{0};
    uint8_t color{0};
    std::string voice;
    std::string text;
    uint16_t value{0};
};

class ConvData {
public:
    ConvData(const uint8_t *data, size_t size);

    int parse();
    ConvFormat format() const;
    const std::vector<ConvRecord> &records() const;
    size_t errorOffset() const;
    const std::string &errorMessage() const;

private:
    const uint8_t *data;
    size_t size;
    ConvFormat scriptFormat{ConvFormat::Unknown};
    size_t pos{0};
    std::vector<ConvRecord> recordList;
    ConvParseError errorCode{ConvParseError::None};
    size_t errorAt{0};
    std::string errorText;

    ConvFormat detectFormat();
    bool findFirstText(size_t &text, uint8_t &kind);
    bool stringEnd(size_t start, size_t &end);
    bool looksLikeVoice(size_t start, size_t end);
    static bool isTopLevel(uint8_t op);
    bool followsText(size_t at, uint8_t kind);

    bool fail(size_t at, ConvParseError code, const char *message);
    bool need(size_t count);
    bool readByte(uint8_t &value);
    bool readWord(uint16_t &value);
    bool readSignedWord(int16_t &value);
    bool readName(std::string &value);
    bool readString(std::string &value);
    bool readText(ConvRecord &record);
    bool peekIs(uint8_t value);
    void begin(ConvRecord &record, size_t at, uint8_t op, int level);
    bool addSimple(size_t at, uint8_t op);
    bool parseJump(size_t at, uint8_t op, int level);
    bool parseWing(size_t at);
    bool parseNarration(size_t at, int level);
    bool parseGroupShot(size_t at);
    bool parseCloseUp(size_t at);
    bool checkJumps();
};
