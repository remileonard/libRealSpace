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
    // Scène de groupe : les palettes du décor seules ; la palette FGPL « normal » est complète dans Pacific Strike et
    // écraserait celles du décor (strike_commander_re/tools/ps_doc_images.py).
    void applyGroupPalette(void) override;
    // GROUP_SHOT_COMMANDER (17) : calque du commandant à la carte (PointerLayer_Construct_62E1B).
    struct ConvCommander {
        std::vector<uint16_t> images;
        int16_t x{ 0 };
        int16_t y{ 0 };
    };
    void pushFrame(ConvFrame *frame, const std::vector<ConvRecord> &records, const ConvRecord &record) override;
    void drawGroupScene(void) override;

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
    size_t group_offset{ 0 };
    bool commander_open{ false };
    ConvCommander open_commander;
    std::unordered_map<size_t, ConvCommander> commanders;
    std::unordered_map<ConvFrame *, ConvCommander> frame_commanders;
    void drawFullScreenImage(RLEShape *shape);
    void drawCommander(const ConvCommander &commander);

public:
    PSConvPlayer();
    ~PSConvPlayer() override;
};
