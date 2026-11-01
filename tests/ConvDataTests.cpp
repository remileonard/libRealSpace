#include <gtest/gtest.h>
#include "realspace/ConvData.h"
#include <string>
#include <vector>

class ConvScript {
public:
    std::vector<uint8_t> bytes;

    ConvScript &op(uint8_t value) {
        bytes.push_back(value);
        return *this;
    }
    ConvScript &name(const std::string &value) {
        for (size_t i = 0; i < 9; i++) {
            bytes.push_back(i < value.size() ? static_cast<uint8_t>(value[i]) : 0);
        }
        return *this;
    }
    ConvScript &word(int16_t value) {
        uint16_t raw = static_cast<uint16_t>(value);
        bytes.push_back(static_cast<uint8_t>(raw & 255));
        bytes.push_back(static_cast<uint8_t>(raw >> 8));
        return *this;
    }
    ConvScript &text(const std::string &value) {
        for (char c : value) {
            bytes.push_back(static_cast<uint8_t>(c));
        }
        bytes.push_back(0);
        return *this;
    }
};

static ConvScript closeUp(const std::string *voice, const std::string &line, uint8_t color) {
    ConvScript script;
    script.op(CONV_CLOSEUP).name("billy").op(22).name("bar").word(160).word(128);
    if (voice != nullptr) {
        script.text(*voice);
    }
    script.text(line).op(color);
    return script;
}

TEST(ConvDataTest, CloseUpFloppy_IsReadWithoutVoice) {
    ConvScript script = closeUp(nullptr, "Hello, $C.", 224);
    script.op(CONV_CLOSEUP_CONTINUATION).text("Second line.");
    ConvData conv(script.bytes.data(), script.bytes.size());

    ASSERT_EQ(0, conv.parse());
    EXPECT_EQ(ConvFormat::Floppy, conv.format());
    ASSERT_EQ(2u, conv.records().size());
    const ConvRecord &line = conv.records()[0];
    EXPECT_EQ(CONV_CLOSEUP, line.op);
    EXPECT_EQ("billy", line.name);
    EXPECT_EQ(22, line.baseFrame);
    EXPECT_EQ("bar", line.background);
    EXPECT_EQ(160, line.x);
    EXPECT_EQ(128, line.y);
    EXPECT_EQ("Hello, $C.", line.text);
    EXPECT_EQ(224, line.color);
    EXPECT_TRUE(line.voice.empty());
    EXPECT_EQ(CONV_CLOSEUP_CONTINUATION, conv.records()[1].op);
    EXPECT_EQ(1, conv.records()[1].level);
    EXPECT_EQ("Second line.", conv.records()[1].text);
}

TEST(ConvDataTest, CloseUpCd_ReadsVoiceThenText) {
    std::string voice = "3gw009";
    std::string empty;
    ConvScript script = closeUp(&voice, "Hello, $C.", 224);
    script.op(CONV_CLOSEUP_CONTINUATION).text(empty).text("");
    ConvData conv(script.bytes.data(), script.bytes.size());

    ASSERT_EQ(0, conv.parse());
    EXPECT_EQ(ConvFormat::Cd, conv.format());
    ASSERT_EQ(2u, conv.records().size());
    EXPECT_EQ("3gw009", conv.records()[0].voice);
    EXPECT_EQ("Hello, $C.", conv.records()[0].text);
    EXPECT_EQ(224, conv.records()[0].color);
    EXPECT_TRUE(conv.records()[1].voice.empty());
    EXPECT_TRUE(conv.records()[1].text.empty());
}

TEST(ConvDataTest, GroupShot_ReadsSignedPositions) {
    ConvScript script;
    script.op(CONV_GROUP_SHOT).name("str_s_ex");
    script.op(CONV_GROUP_SHOT_ADD_CHARACTER).name("airwing1").word(0).word(-20);
    script.op(CONV_GROUP_SHOT_CHARACTER_TALK).name("air1").word(-5).word(300).op(224).text("Go.");
    script.op(CONV_SHOW_TEXT).op(15).text("Later.");
    ConvData conv(script.bytes.data(), script.bytes.size());

    ASSERT_EQ(0, conv.parse());
    EXPECT_EQ(ConvFormat::Floppy, conv.format());
    ASSERT_EQ(4u, conv.records().size());
    EXPECT_EQ("str_s_ex", conv.records()[0].background);
    EXPECT_EQ("airwing1", conv.records()[1].name);
    EXPECT_EQ(0, conv.records()[1].x);
    EXPECT_EQ(-20, conv.records()[1].y);
    EXPECT_EQ(-5, conv.records()[2].x);
    EXPECT_EQ(300, conv.records()[2].y);
    EXPECT_EQ(224, conv.records()[2].color);
    EXPECT_EQ("Go.", conv.records()[2].text);
    EXPECT_EQ(CONV_SHOW_TEXT, conv.records()[3].op);
    EXPECT_EQ(1, conv.records()[3].level);
    EXPECT_EQ(15, conv.records()[3].color);
}

TEST(ConvDataTest, ByteThatIsNotACommand_IsAnError) {
    ConvScript script = closeUp(nullptr, "Hello.", 224);
    script.op(CONV_CLOSEUP_CONTINUATION).text("Again.");
    script.op(224);
    ConvData conv(script.bytes.data(), script.bytes.size());

    EXPECT_EQ(static_cast<int>(ConvParseError::NotACommand), conv.parse());
    EXPECT_EQ(script.bytes.size() - 1, conv.errorOffset());
}

TEST(ConvDataTest, FieldPastEnd_IsAnError) {
    ConvScript script = closeUp(nullptr, "Hello.", 224);
    script.op(CONV_SELECT_WING).op(0);
    ConvData conv(script.bytes.data(), script.bytes.size());

    EXPECT_EQ(static_cast<int>(ConvParseError::PastEnd), conv.parse());
}

TEST(ConvDataTest, JumpToTheStartOfACommand_IsAccepted) {
    ConvScript script = closeUp(nullptr, "Accept?", 224);
    size_t refuse_jump = script.bytes.size();
    script.op(CONV_YESNOCHOICE_BRANCH1).word(0);
    script.op(CONV_CLOSEUP_CONTINUATION).text("Good.");
    size_t accept_end = script.bytes.size();
    script.op(CONV_YESNOCHOICE_BRANCH2).word(0);
    size_t refuse_start = script.bytes.size();
    script.op(CONV_CLOSEUP).name("billy").op(22).name("bar").word(160).word(128).text("Too bad.").op(224);
    script.bytes[refuse_jump + 1] = static_cast<uint8_t>(refuse_start - (refuse_jump + 3));
    script.bytes[accept_end + 1] = static_cast<uint8_t>(script.bytes.size() - (accept_end + 3));
    ConvData conv(script.bytes.data(), script.bytes.size());

    ASSERT_EQ(0, conv.parse()) << conv.errorMessage();
    EXPECT_EQ(5u, conv.records().size());
}

TEST(ConvDataTest, JumpIntoACommand_IsAnError) {
    ConvScript script = closeUp(nullptr, "Accept?", 224);
    script.op(CONV_YESNOCHOICE_BRANCH1).word(2);
    script.op(CONV_CLOSEUP_CONTINUATION).text("Good.");
    ConvData conv(script.bytes.data(), script.bytes.size());

    EXPECT_EQ(static_cast<int>(ConvParseError::BadJump), conv.parse());
}

TEST(ConvDataTest, ScriptWithoutText_HasUnknownFormat) {
    ConvScript script;
    script.op(CONV_CHOOSE_WINGMAN);
    ConvData conv(script.bytes.data(), script.bytes.size());

    EXPECT_EQ(static_cast<int>(ConvParseError::UnknownFormat), conv.parse());
}
