#include "PSStrike.h"
#include "PSVirtualCockpit.h"

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
    this->placePlayerOnGround();
    this->selectFirstWeapon();
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

// Cockpit 3D de Pacific Strike ; PSStrike::createCockpit rend toujours un PSCockpit.
SCVirtualCockpit *PSStrike::createVirtualCockpit(SCCockpit *cockpit) {
    return new PSVirtualCockpit(static_cast<PSCockpit *>(cockpit));
}

// Tir de l'arme choisie ; un canon ou une bombe n'ont pas besoin de cible.
void PSStrike::firePrimary() {
    if (this->player_plane->selected_weapon < 0 || (size_t)this->player_plane->selected_weapon >= this->player_plane->weaps_load.size()) {
        return;
    }
    if (this->player_plane->weaps_load[this->player_plane->selected_weapon] == nullptr) {
        return;
    }
    this->player_plane->Shoot(this->player_plane->selected_weapon, this->target, this->current_mission);
    this->cockpit->is_shooting = true;
}

// Premier point d'emport chargé.
void PSStrike::selectFirstWeapon() {
    for (size_t i = 0; i < this->player_plane->weaps_load.size(); i++) {
        if (this->player_plane->weaps_load[i] != nullptr) {
            this->player_plane->selected_weapon = (int)i;
            printf("PSStrike: arme choisie %s (point %zu)\n", this->player_plane->weaps_load[i]->name.c_str(), i);
            return;
        }
    }
}

// Arme suivante : le premier point d'emport, après celui choisi, qui porte une autre arme que l'arme choisie (deux
// points qui portent le même objet d'arme comptent pour une seule arme).
void PSStrike::selectNextWeapon() {
    std::vector<SCWeaponLoadoutHardPoint *> &stations = this->player_plane->weaps_load;
    if (stations.empty()) {
        return;
    }
    int current = this->player_plane->selected_weapon;
    RSEntity *current_weapon = nullptr;
    if (current >= 0 && (size_t)current < stations.size() && stations[current] != nullptr) {
        current_weapon = stations[current]->objct;
    }
    for (size_t k = 1; k <= stations.size(); k++) {
        size_t candidate = ((size_t)current + k) % stations.size();
        if (stations[candidate] == nullptr || stations[candidate]->objct == current_weapon) {
            continue;
        }
        this->player_plane->selected_weapon = (int)candidate;
        printf("PSStrike: arme choisie %s (point %zu), %d restant sur ce point\n", stations[candidate]->name.c_str(), candidate, stations[candidate]->nb_weap);
        return;
    }
}

// Joueur parti à l'arrêt (vitesse 0 dans son PART) : posé sur le sol sous lui, pont d'un porte-avions compris, train
// sorti et manette à 0. SCStrike::setMission ne connaît que le terrain et le fait partir en vol.
void PSStrike::placePlayerOnGround() {
    MISN_PART *part = this->current_mission->player->object;
    if (part->speed != 0) {
        return;
    }
    float ground = this->area->getY(this->player_plane->x, this->player_plane->z);
    printf("PSStrike: joueur a l'arret, altitude %.1f, sol %.1f\n", this->player_plane->y, ground);
    if (fabsf(ground - this->player_plane->y) > 10.0f) {
        return;
    }
    this->player_plane->y = ground;
    part->position.y = ground;
    this->new_position.y = ground;
    this->player_plane->on_ground = true;
    this->player_plane->SetThrottle(0);
    this->player_plane->vz = 0;
    this->player_plane->velocity = Vector3D(0.0f, 0.0f, 0.0f);
    if (this->player_plane->GetWheel() == 0) {
        this->player_plane->SetWheel();
    }
}
