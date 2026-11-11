#include "PSStrike.h"

// Pacific Strike choisit sa musique de mission dans setMission (numéro + 1).
void PSStrike::start(void) {
    IActivity::start();
}

SCMission *PSStrike::createMission(char const *missionName) {
    return new PSMission(missionName, &object_cache);
}

// Chargement de SCStrike (mission créée par createMission, caméra d'ouverture STARTCAM), puis la musique de
// Pacific Strike : numéro TUNE + 1.
void PSStrike::setMission(char const *missionName) {
    if (Mixer.isSoundPlaying(5)) {
        Mixer.stopSound(5);
    }
    Mixer.stopMusic();
    SCStrike::setMission(missionName);
    Mixer.switchBank(2);
    Mixer.playMusic(this->current_mission->mission->mission_data.tune + 1);
    printf("PSStrike: mission %s, musique %d\n", missionName, this->current_mission->mission->mission_data.tune + 1);
    verticalOffset = 0.0f;
    SCStrike::verticalOffset = 0.0f;
    SCRenderer::instance().verticalOffset = verticalOffset;
    SCRenderer::instance().initRenderCameraView();
}
PSStrike::PSStrike() {
}

PSStrike::~PSStrike() {
}

SCCockpit *PSStrike::createCockpit() {
    return new PSCockpit();
}
