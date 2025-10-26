#include "DebugPSConvPlayer.h"

DebugPSConvPlayer::DebugPSConvPlayer() {}

DebugPSConvPlayer::~DebugPSConvPlayer() {}

void DebugPSConvPlayer::SetID(int conv_id) {
    DebugConvPlayer::init();
    PSConvPlayer::SetID(conv_id);
    DebugConvPlayer::conversation_frames = PSConvPlayer::conversation_frames;
    DebugConvPlayer::initialized = true;
}