//
//  TOConvPlayer.cpp
//  libRealSpace
//

#include "TOConvPlayer.h"

const std::string TOConvPlayer::ARI_FIGURE = "ari";

static const std::vector<std::string> kTOPilotNames = {
    "",
    "Billy",
    "Gwen",
    "Lyle",
    "Miguel",
    "Tex",
    "Ari"
};
static const std::vector<std::string> kTOPilotFigures = {
    "red1",
    "billy2",
    "gwen2",
    "lyle2",
    "miguel2",
    "tex3",
    TOConvPlayer::ARI_FIGURE
};

TOConvPlayer::TOConvPlayer() {
}

TOConvPlayer::~TOConvPlayer() {
}

const std::vector<std::string> &TOConvPlayer::pilotNames(void) {
    return kTOPilotNames;
}

const std::vector<std::string> &TOConvPlayer::pilotFigures(void) {
    return kTOPilotFigures;
}

// Ari prend l'emplacement de Lyle sur l'écran de choix.
uint8_t TOConvPlayer::pilotSlot(uint8_t pilot) {
    if (pilot == TO_PILOT_ARI) {
        return TO_PILOT_LYLE;
    }
    return pilot;
}

// CHOOSE_WINGMAN : pilotes de l'escadre, Lyle jamais (Ari à sa place) ; CHOOSE_WINGMAN_NO_LYLE : sans le 6e pilote de
// l'escadre ni Lyle ; CHOOSE_WINGMAN_ORIGINAL : les cinq pilotes d'origine de l'escadre.
std::vector<uint8_t> TOConvPlayer::wingmanCandidates(uint8_t command) {
    std::vector<uint8_t> candidates;
    for (size_t k = 0; k < this->wing_pilots.size(); k++) {
        uint8_t pilot = this->wing_pilots[k];
        bool keep = true;
        if (command == CONV_CHOOSE_WINGMAN) {
            keep = pilot != TO_PILOT_LYLE;
        } else if (command == TO_CHOOSE_WINGMAN_NO_LYLE) {
            keep = k != 5 && pilot != TO_PILOT_LYLE;
        } else if (command == TO_CHOOSE_WINGMAN_ORIGINAL) {
            keep = pilot != TO_PILOT_ARI;
        }
        if (keep) {
            candidates.push_back(pilot);
        }
    }
    printf("CONV %d: commande %d, %zu pilotes proposes sur %zu de l'escadre\n", this->conversationID, command,
           candidates.size(), this->wing_pilots.size());
    return candidates;
}

ConvFrame *TOConvPlayer::buildOtherCommand(const ConvRecord &record) {
    if (record.op != TO_CHOOSE_WINGMAN_NO_LYLE && record.op != TO_CHOOSE_WINGMAN_ORIGINAL) {
        return SCConvPlayer::buildOtherCommand(record);
    }
    ConvFrame *frame = this->newFrame();
    this->setWingmanChoice(frame, this->wingmanCandidates(record.op));
    return frame;
}
