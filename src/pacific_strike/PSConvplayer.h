#pragma once
#include "../strike_commander/precomp.h"
#include "PSState.h"

class PSConvPlayer : public virtual SCConvPlayer {
protected:
    PSState &PSGameState = PSState::getInstance();

protected:
    // Conv_PlayScript_C2ED0 de PACIFIC.EXE (strike_commander_re/analysis/pacific/README.md § 4.2) : la grammaire de
    // Pacific Strike est lue en ConvRecord, affichés ensuite par SCConvPlayer::buildFrames.
    enum PSConvCommand : uint8_t {
        PS_CONV_IF_FLAG = 5,
        PS_CONV_ELSE = 12,
        PS_CONV_ENDIF = 13,
        PS_CONV_GROUP_SHOT_COMMANDER = 17
    };
    void SetArchive(PakEntry *convPakEntry) override;

private:
    const uint8_t *script{ nullptr };
    size_t script_size{ 0 };
    size_t pos{ 0 };
    bool skipping{ false };
    std::vector<ConvRecord> records;

    bool readName(std::string &value);
    bool readWord(uint16_t &value);
    bool readSignedWord(int16_t &value);
    bool readByte(uint8_t &value);
    bool readText(ConvRecord &record);
    void keep(const ConvRecord &record);
    bool readFigure(size_t at, int level);
    bool readShowText(size_t at, uint8_t op, int level);
    bool readCommander(size_t at);
    bool readGroupShot(size_t at);
    bool readCloseUp(size_t at);

public:
    PSConvPlayer();
    ~PSConvPlayer() override;
};
