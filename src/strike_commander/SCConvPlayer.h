//
//  SCConvPlayer.h
//  libRealSpace
//
//  Created by Fabien Sanglard on 1/31/2014.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//

#pragma once
#include "../realspace/ConvAssetManager.h"
#include "../realspace/ConvData.h"

class SCConvPlayer;
class ConvFrame {

public:
    RSFont *font{nullptr};
    char *text{nullptr};
    uint8_t textColor;
    uint8_t conversationID{0};
    enum ConvMode { CONV_WIDE, CONV_CLOSEUP, CONV_WINGMAN_CHOICE, CONV_CONTRACT_CHOICE, CONV_SHOW_TEXT };
    ConvMode mode;

    enum FacePos { FACE_DEF = 0x0, FACE_LEFT = 0x82, FACE_RIGHT = 0xBE, FACE_CENTER = 0xA0 };
    FacePos facePosition;
    uint8_t face_expression;
    Point2D face_position{};
    // If we are in a wide of chose wingman mode
    std::vector<CharFigure *> participants;
    std::vector<Point2D> participant_positions;
    bool group_scene{false};
    int talker{-1};
    std::string lip_text;
    int figure_palette{-1};
    uint32_t duration{0};
    bool game_positions{false};
    std::string *sound_file_name{nullptr};
    // If we are in close up mode
    CharFace *face{nullptr};
    int8_t facePaletteID;
    bool do_not_add{false};
    std::vector<RLEShape *> *bgLayers{nullptr};
    std::vector<uint8_t *> *bgPalettes{nullptr};
    std::vector<int16_t> *bgYs{nullptr};
    std::vector<uint8_t> pilots;
    int choice{-1};
    uint32_t choice_time{0};

    uint32_t creationTime; // Used to check when a frame expires.

    inline void SetExpired(bool exp) { this->expired = exp; }
    inline bool IsExpired(void) { return this->expired; }
    void setGroupShot(const ConvRecord &record);
    void addFigure(const ConvRecord &record);
    void setTalk(const ConvRecord &record);
    void copyScene(const ConvFrame *previous);
    void setNarration(const ConvRecord &record);
    void setCloseUp(const ConvRecord &record);
    void setSentence(const ConvRecord &record);
    void copyCloseUp(const ConvFrame *previous);
    int yes_no_path{0}; // current path taken for yes/no choice
protected:
    bool expired;
    SCState &GameState = SCState::getInstance();
    ConvAssetManager &ConvAssets = ConvAssetManager::getInstance();
};

class SCConvPlayer : public IActivity {
protected:
    int32_t conversationID;
    bool initialized;
    ByteStream conv;
    size_t size;  // In bytes
    uint8_t *end; // In bytes
    RSFont *font{nullptr};
    uint8_t wing_screen{0};
    uint8_t wing_number{0};
    PakArchive convPak;
    PakArchive convPals;
    ConvFrame *current_frame{nullptr};

    virtual void CheckFrameExpired(void);
    virtual void SetArchive(PakEntry *conv);
    void buildFrames(const std::vector<ConvRecord> &records, ConvFormat format);
    size_t buildGroupShot(const std::vector<ConvRecord> &records, size_t index, ConvFormat format);
    ConvFrame *newFrame(void);
    void addFrame(ConvFrame *frame);
    void pushFrame(ConvFrame *frame, const std::vector<ConvRecord> &records, const ConvRecord &record);
    void applyGroupPalette(void);
    void drawGroupScene(void);
    void applyFacePalette(void);
    void startVoice(void);
    void drawCloseUpFace(int mouth);
    void drawCloseUp(bool lips);
    uint32_t blink_tick{0};
    bool blinking{false};
    void selectWing(const ConvRecord &record);
    void setWingmanChoice(ConvFrame *frame);
    void answerQuestion(bool accepted);
    int layerOffset(size_t index);
    void runChoice(void);
    void clearZones(void);
    void addContractChoiceZones(void);
    void addWingmanZones(ConvFrame *frame);
    void prepareZones(void);
    SCZone *CheckZones(void);
    std::vector<uint8_t> wing_pilots;
    void DrawText(void);
    int txt_color{0};
    SCState &GameState = SCState::getInstance();
public:
    std::vector<SCZone *> zones;
    std::vector<ConvFrame *> conversation_frames;
    SCConvPlayer();
    ~SCConvPlayer();

    void init();
    void runFrame(void);
    void SetID(int32_t id);
    virtual void focus(void);
    void clicked(void *none, uint8_t id);
    void selectWingMan(void *none, uint8_t id);
};
