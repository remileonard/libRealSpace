//
//  AILMidi.h
//  libRealSpace
//
//  Constantes MIDI / XMIDI communes au pilote AdLib et a l'interpreteur XMIDI.
//  Les noms de controleurs sont ceux d'AIL 2.0 (analysis/ail_sources/AIL.INC).
//
#pragma once

namespace AILMidi {

// Octet d'etat d'un message MIDI : type (4 bits hauts) | canal (4 bits bas)
enum Status {
    NOTE_OFF = 0x80,
    NOTE_ON = 0x90, // XMIDI : suivi de la duree de la note (VLN)
    POLY_PRESSURE = 0xA0,
    CONTROL_CHANGE = 0xB0,
    PROGRAM_CHANGE = 0xC0,
    CHANNEL_PRESSURE = 0xD0,
    PITCH_BEND = 0xE0,
    SYSEX = 0xF0,
    META = 0xFF,
};
const int STATUS_MASK = 0xF0;
const int CHANNEL_MASK = 0x0F;

// Evenements meta (0xFF type longueur donnees)
enum Meta {
    META_END_OF_TRACK = 0x2F,
    META_TEMPO = 0x51,          // 3 octets : microsecondes par noire
    META_TIME_SIGNATURE = 0x58, // numerateur, log2(denominateur), ...
};

// Controleurs (AIL.INC)
enum Controller {
    MODULATION = 1,
    PART_VOLUME = 7,
    PANPOT = 10,
    EXPRESSION = 11,
    SUSTAIN = 64,
    CHAN_LOCK = 110, // controleurs XMIDI generaux
    CHAN_PROTECT = 111,
    VOICE_PROTECT = 112,
    TIMBRE_PROTECT = 113,
    PATCH_BANK_SEL = 114,
    INDIRECT_C_PFX = 115,
    FOR_LOOP = 116,
    NEXT_LOOP = 117,
    CLEAR_BEAT_BAR = 118,
    CALLBACK_TRIG = 119,
    SEQ_INDEX = 120,
    RESET_ALL_CTRLS = 121, // messages de mode de canal
    ALL_NOTES_OFF = 123,
};

const int SWITCH_ON = 64;        // controleur interrupteur : >= 64 = actif
const int PITCH_CENTER_L = 0x00; // pitch-bend au centre : 0x2000 (DEF_PITCH_L / DEF_PITCH_H)
const int PITCH_CENTER_H = 0x40;
const int RHYTHM_CHANNEL = 9; // canal 10 MIDI : percussions
const int RHYTHM_BANK = 0x7F; // banque des timbres de percussion (patch = note)

inline int status(int type, int channel) {
    return type | (channel & CHANNEL_MASK);
}

} // namespace AILMidi
