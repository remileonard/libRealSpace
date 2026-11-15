#include "PSMissionActors.h"
#include "PSBrain.h"
#include "PSEntity.h"

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

// Goal_SetObjective_C2A8 : ordres refusés après un ordre radio ou en fuite ; OP_SET_OBJ_FOLLOW_ALLY ne change pas l'ordre
// en cours ; la réponse au script est celle de Goal_IsComplete_C73C sur l'ordre en cours
bool PSMissionActors::setObjective(prog_op command, uint8_t arg) {
    PSBrain *brain = dynamic_cast<PSBrain *>(this->brain);
    if (brain == nullptr) {
        return SCMissionActors::setObjective(command, arg);
    }
    if (brain->objective_locked) {
        brain->onObjectiveSet(false);
        return true;
    }
    if (command == OP_SET_OBJ_FLY_TO_AREA) {
        return false;
    }
    if (command != OP_SET_OBJ_FOLLOW_ALLY) {
        SCMissionActors::setObjective(command, arg);
    }
    bool in_progress = brain->orderInProgress();
    if (in_progress != this->last_in_progress) {
        printf("PSMissionActors %s#%d ordre %d (%d) : %s\n", this->actor_name.c_str(), this->actor_id, (int)this->current_command, this->current_command_arg, in_progress ? "en cours" : "termine");
        this->last_in_progress = in_progress;
    }
    return in_progress;
}

// Swpn_TurretUpdate_403A8 : la tourelle se tait quand son porteur est détruit ou, pour un avion, que son pilote s'est
// éjecté.
bool PSMissionActorsTurret::parentActive() {
    SCMissionActors *owner = this->parent;
    bool active = owner->is_active || owner == this->mission->player;
    if (!active || owner->is_destroyed || !owner->object->alive) {
        return false;
    }
    if (owner->plane != nullptr) {
        return !owner->plane->ejected;
    }
    return true;
}

// Vecteur du repère du modèle du porteur (nez, haut, droite) vers le monde, avec l'orientation que le rendu donne à son
// objet : (360 - cap + 90) autour de Y, puis tangage autour de Z, puis -roulis autour de X.
Vector3D PSMissionActorsTurret::parentToWorld(Vector3D model) {
    MISN_PART *part = this->parent->object;
    float heading = (360.0f - (float)part->azymuth + 90.0f) * (float)M_PI / 180.0f;
    float pitch = (float)(int16_t)part->pitch * (float)M_PI / 180.0f;
    float roll = -(float)(int16_t)part->roll * (float)M_PI / 180.0f;
    Vector3D rolled = {
        model.x,
        model.y * cosf(roll) - model.z * sinf(roll),
        model.y * sinf(roll) + model.z * cosf(roll)
    };
    Vector3D pitched = {
        rolled.x * cosf(pitch) - rolled.y * sinf(pitch),
        rolled.x * sinf(pitch) + rolled.y * cosf(pitch),
        rolled.z
    };
    Vector3D world = {
        pitched.x * cosf(heading) + pitched.z * sinf(heading),
        pitched.y,
        -pitched.x * sinf(heading) + pitched.z * cosf(heading)
    };
    return world;
}

void PSMissionActorsTurret::onMissionUpdate(const MissionUpdateEvent &event) {
    bool active = this->parentActive();
    if (active != this->is_active) {
        printf("TURRET %s de %s#%d : %s\n", this->actor_name.c_str(), this->parent->actor_name.c_str(), this->parent->actor_id, active ? "active" : "inactive");
        this->is_active = active;
        if (!active) {
            this->target = nullptr;
        }
    }
    if (active) {
        Vector3D offset = {
            this->mount->z,
            this->mount->y,
            this->mount->x
        };
        this->object->position = this->parent->object->position + this->parentToWorld(offset);
        this->team_id = this->parent->team_id;
    }
    SCMissionActorsSwpn::onMissionUpdate(event);
    this->giveShotsToParent();
}

// Les tirs de la tourelle sont ceux de son porteur : c'est lui qui les simule et les dessine, l'avion dans weaps_object,
// un autre objet dans weapons_shooted. La tourelle reste le tireur (camp, coups portés).
void PSMissionActorsTurret::giveShotsToParent() {
    for (auto shot : this->weapons_shooted) {
        Vector3D delta = this->aiming_vector;
        printf("TURRET %s de %s#%d : tire sur %s, a %.0f m, %.0f m au-dessus de la tourelle, altitudes porteur %.1f tourelle %.1f cible %.1f, munitions %d\n", this->actor_name.c_str(), this->parent->actor_name.c_str(), this->parent->actor_id, this->target != nullptr ? this->target->actor_name.c_str() : "none", delta.Length(), delta.y, this->parent->object->position.y, this->object->position.y, this->object->position.y + this->aiming_vector.y, this->object->entity->swpn_data->weapons_round);
        if (this->parent->plane != nullptr) {
            this->parent->plane->weaps_object.push_back(shot);
        } else {
            this->parent->weapons_shooted.push_back(shot);
        }
    }
    this->weapons_shooted.clear();
}

// Swpn_TurretUpdate_403A8 et Swpn_TurretSlewClamp_40C5B : direction de la cible dans le repère du montage, élévation
// et gisement (positif vers la gauche) bornés par le chunk GARC ; un axe dont une borne atteint 180 ou -180 est libre.
// Axes du montage dans le modèle du porteur : lacet autour du haut, puis tangage autour de la droite ainsi tournée.
bool PSMissionActorsTurret::inFiringArc(const Vector3D &to_target) {
    PSEntity *entity = static_cast<PSEntity *>(this->object->entity);
    float yaw = this->mount->yaw * (float)M_PI / 180.0f;
    float pitch = this->mount->pitch * (float)M_PI / 180.0f;
    Vector3D yawed_forward = {
        cosf(yaw),
        0.0f,
        sinf(yaw)
    };
    Vector3D mount_right = {
        -sinf(yaw),
        0.0f,
        cosf(yaw)
    };
    Vector3D mount_forward = {
        yawed_forward.x * cosf(pitch),
        sinf(pitch),
        yawed_forward.z * cosf(pitch)
    };
    Vector3D mount_up = {
        -yawed_forward.x * sinf(pitch),
        cosf(pitch),
        -yawed_forward.z * sinf(pitch)
    };
    float forward = Vector3D::Dot(to_target, this->parentToWorld(mount_forward));
    float right = Vector3D::Dot(to_target, this->parentToWorld(mount_right));
    float up = Vector3D::Dot(to_target, this->parentToWorld(mount_up));
    float azimuth = atan2f(-right, forward) * 180.0f / (float)M_PI;
    float elevation = atan2f(up, sqrtf(forward * forward + right * right)) * 180.0f / (float)M_PI;
    bool elevation_free = entity->arc_elevation_max >= 180.0f || entity->arc_elevation_min <= -180.0f;
    bool azimuth_free = entity->arc_azimuth_max >= 180.0f || entity->arc_azimuth_min <= -180.0f;
    bool elevation_ok = elevation_free || (elevation <= entity->arc_elevation_max && elevation >= entity->arc_elevation_min);
    bool azimuth_ok = azimuth_free || (azimuth <= entity->arc_azimuth_max && azimuth >= entity->arc_azimuth_min);
    if (!elevation_ok || !azimuth_ok) {
        printf("TURRET %s de %s#%d : %s hors secteur, elevation %.1f [%.0f, %.0f], gisement %.1f [%.0f, %.0f], altitudes porteur %.1f tourelle %.1f cible %.1f, porteur cap %d tangage %d roulis %d\n", this->actor_name.c_str(), this->parent->actor_name.c_str(), this->parent->actor_id, this->target != nullptr ? this->target->actor_name.c_str() : "none", elevation, entity->arc_elevation_min, entity->arc_elevation_max, azimuth, entity->arc_azimuth_min, entity->arc_azimuth_max, this->parent->object->position.y, this->object->position.y, this->aiming_vector.y + this->object->position.y, (int)this->parent->object->azymuth, (int)(int16_t)this->parent->object->pitch, (int)(int16_t)this->parent->object->roll);
        return false;
    }
    return true;
}
