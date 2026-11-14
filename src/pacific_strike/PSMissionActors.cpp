#include "PSMissionActors.h"
#include "PSBrain.h"

// Radio_SendMessage_EA0E forcé : envoyé même en silence radio
void PSMissionActors::forcedMessage(uint8_t message) {
    bool talkative = this->talkative;
    this->talkative = true;
    this->setMessage(message);
    this->talkative = talkative;
}

// AI_PlayerRadioCommand_E3CA : message_id = rang de l'ordre dans OPTS, lettre du chunk ASKS de PLAYER.IFF (d = 100 ... n = 110)
bool PSMissionActors::respondToRadioMessage(int message_id, SCMission *mission, SCMissionActors *sender) {
    if (message_id < 1 || message_id > (int)this->profile->radi.opts.size()) {
        return false;
    }
    char ask = this->profile->radi.opts[message_id - 1];
    PSBrain *brain = dynamic_cast<PSBrain *>(this->brain);
    printf("PSMissionActors %s#%d ordre radio %d '%c'\n", this->actor_name.c_str(), this->actor_id, message_id, ask);
    if (brain == nullptr) {
        return false;
    }
    uint8_t status = brain->statusReply();
    bool unable = status == 4 || status == 29;
    switch (ask) {
    case 'd':
        this->forcedMessage(status);
        break;
    case 'g':
        if (brain->isFleeing()) {
            this->forcedMessage(8);
        } else if (!brain->enemiesActive()) {
            this->forcedMessage(1);
            brain->leaveFormation();
        } else if (unable) {
            this->forcedMessage(status);
        } else {
            brain->leaveFormation();
            this->forcedMessage(1);
        }
        break;
    case 'h':
        if (brain->isFleeing()) {
            this->forcedMessage(8);
        } else {
            this->forcedMessage(1);
            brain->joinFormation();
        }
        break;
    case 'i': {
        SCMissionActors *target = sender != nullptr ? sender->target : nullptr;
        if (brain->isFleeing()) {
            this->forcedMessage(8);
        } else if (unable) {
            this->forcedMessage(status);
        } else if (target != nullptr && target->plane == nullptr && target->team_id != this->team_id) {
            brain->ground_target = target;
            brain->objective_locked = true;
            brain->in_formation = false;
            this->forcedMessage(18);
            printf("PSMissionActors %s#%d attaque la cible au sol %s#%d\n", this->actor_name.c_str(), this->actor_id, target->actor_name.c_str(), target->actor_id);
        } else if (target != nullptr && target->plane != nullptr && target->team_id != this->team_id) {
            brain->air_target = target;
            brain->objective_locked = true;
            brain->in_formation = false;
            this->forcedMessage(1);
            printf("PSMissionActors %s#%d attaque l'avion %s#%d\n", this->actor_name.c_str(), this->actor_id, target->actor_name.c_str(), target->actor_id);
        } else {
            this->forcedMessage(0);
        }
        break;
    }
    case 'j':
        if (brain->lowFuel() || brain->seriouslyDamaged() || brain->unarmed()) {
            this->forcedMessage(8);
            brain->fleeHome();
        } else {
            this->forcedMessage(0);
        }
        break;
    case 'k':
        this->forcedMessage(14);
        break;
    case 'l':
        this->forcedMessage(1);
        this->talkative = false;
        break;
    case 'm':
        this->talkative = true;
        this->forcedMessage(1);
        break;
    default:
        this->forcedMessage(0);
        break;
    }
    return true;
}
