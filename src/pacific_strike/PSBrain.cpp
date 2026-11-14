#include "../strike_commander/precomp.h"
#include "PSBrain.h"
#include "PSEntity.h"

// AI_FleeHome_E8E5 : vitesse voulue (125, 100, 0) en axes asm
static const Vector3D PS_FLEE_VELOCITY(125.0f, 0.0f, -100.0f);

PSBrain::PSBrain(SCMissionActors *owner) : SCAIBrain(owner) {
    for (uint8_t value : owner->profile->ai.goal) {
        if ((PSGoalSelector)value == PS_GOAL_FORMATION_TARGETING) {
            this->formation_targeting = true;
        }
    }
}

// AI_TopLevelThink_B0C4 : au sol Goal_ExecuteObjective_C9E4, en vol les gestionnaires GOAL du profil
bool PSBrain::runGoalSelectors() {
    if (owner->plane->on_ground) {
        this->decide("au sol");
        return this->executeGoalAction();
    }
    for (uint8_t rawSelector : owner->profile->ai.goal) {
        size_t mark = strlen(decision);
        bool acted = false;
        switch ((PSGoalSelector)rawSelector) {
        case PS_GOAL_EXECUTE_OBJECTIVE:
            this->decide("GOAL %d action", rawSelector);
            acted = this->executeGoalAction();
            break;
        case PS_GOAL_WANDER_RANDOM:
            this->decide("GOAL %d errance", rawSelector);
            acted = this->tryWanderRandom();
            break;
        case PS_GOAL_BEHAVIOR_STATE_MACHINE:
            this->decide("GOAL %d combat", rawSelector);
            acted = this->combatStep(false);
            break;
        case PS_GOAL_MORALE_REACTION:
            this->decide("GOAL %d moral", rawSelector);
            acted = this->moraleReaction();
            break;
        case PS_GOAL_FOLLOW_LEADER_IN_FORMATION:
            this->decide("GOAL %d chef en formation", rawSelector);
            acted = this->followLeaderInFormation();
            break;
        case PS_GOAL_FOLLOW_LEADER:
            this->decide("GOAL %d chef", rawSelector);
            acted = this->followLeader();
            break;
        default:
            continue;
        }
        if (acted) {
            return true;
        }
        decision[mark] = '\0';
        this->decide("GOAL %d sans effet", rawSelector);
    }
    return false;
}

// Goal_FollowLeaderInFormation_C91C
bool PSBrain::followLeaderInFormation() {
    if (!in_formation) {
        return false;
    }
    return this->followLeader();
}

// Goal_FollowLeader_C93E : chef = allié par défaut du PART (ordre 179 passé au chargement)
bool PSBrain::followLeader() {
    MISN_PART *part = owner->object;
    if (part != nullptr && part->follow_ally != formation.leader_arg) {
        formation.leader_arg = part->follow_ally;
        formation.leader = nullptr;
        if (part->follow_ally != 255) {
            for (auto actor : owner->mission->actors) {
                if (actor != owner && actor->actor_id == part->follow_ally) {
                    formation.leader = actor;
                    break;
                }
            }
        }
        printf("PSBrain %s#%d chef %s (allié par défaut %d)\n", owner->actor_name.c_str(), owner->actor_id, formation.leader != nullptr ? formation.leader->actor_name.c_str() : "aucun", part->follow_ally);
    }
    SCMissionActors *leader = formation.leader;
    if (leader == nullptr) {
        this->decide("pas de chef");
        return false;
    }
    if (leader->is_destroyed) {
        printf("PSBrain %s#%d chef %s#%d détruit, effacé\n", owner->actor_name.c_str(), owner->actor_id, leader->actor_name.c_str(), leader->actor_id);
        formation.leader = nullptr;
        this->decide("chef détruit");
        return false;
    }
    if (this->behaviorRunning()) {
        this->decide("%s en cours", this->behaviorLabel().c_str());
        this->tickBehavior();
        return true;
    }
    this->formationFlight(leader);
    return true;
}

// Goal_FormationFlight_D2AC
void PSBrain::formationFlight(SCMissionActors *leader) {
    SCPlane *lead = leader->plane;
    if (lead == nullptr) {
        return;
    }
    if (lead->y - lead->groundlevel > 333.0f) {
        this->decide("vers le chef %s#%d : manoeuvre 7", leader->actor_name.c_str(), leader->actor_id);
        this->startFollowLeader(leader);
        return;
    }
    Vector3D point = lead->position + Vector3D(0.0f, 1000.0f, 0.0f);
    Vector3D velocity = lead->forward * 200.0f + lead->worldVelocity();
    this->decide("vers le chef %s#%d : 1000 m au-dessus", leader->actor_name.c_str(), leader->actor_id);
    this->applyNavigation(point, velocity, 2.0f);
}

// Goal_MoraleReaction_B712 : seule la fuite, renvoie toujours 0
bool PSBrain::moraleReaction() {
    if (mood.timer > 0.0f) {
        return false;
    }
    mood.timer = 5.0f;
    if (morale != 5) {
        return false;
    }
    int roll = std::rand() % 36;
    if (roll != 1) {
        this->decide("panique, tirage %d", roll);
        return false;
    }
    if (formation.leader_state == 2) {
        return false;
    }
    owner->setMessage(8);
    this->fleeHome();
    return false;
}

// AI_FleeHome_E8E5
void PSBrain::fleeHome() {
    in_formation = false;
    objective_locked = true;
    owner->target = nullptr;
    formation.leader_state = 2;
    mood.fleeing = true;
    mood.destination = mood.home;
    mood.destination.y += 1000.0f;
    this->abandonBehavior();
    this->applyNavigation(mood.destination, PS_FLEE_VELOCITY, 2.0f);
    printf("PSBrain %s#%d moral %d : fuite vers (%.0f, %.0f, %.0f)\n", owner->actor_name.c_str(), owner->actor_id, morale, mood.destination.x, mood.destination.y, mood.destination.z);
}

// Targeting_AcquireTarget_FF8B : un avion qui n'est pas un chasseur, en formation avec le GOAL 9, rompt la formation
bool PSBrain::acquireBestThreat(bool allow_new_target) {
    if (!this->isFighter() && formation_targeting && in_formation && this->canTargetAircraft()) {
        in_formation = false;
        owner->setMessage(18);
        printf("PSBrain %s#%d touché en formation : quitte la formation\n", owner->actor_name.c_str(), owner->actor_id);
        this->decide("quitte la formation");
        return false;
    }
    return SCAIBrain::acquireBestThreat(allow_new_target);
}

// Targeting_MayAcquireTarget_FF05
bool PSBrain::canTargetAircraft() {
    bool formation_rule = formation_targeting && in_formation;
    if (this->isFighter()) {
        return !formation_rule;
    }
    if (just_hit && formation_rule) {
        return true;
    }
    if (just_hit && this->seriouslyDamaged()) {
        return true;
    }
    return false;
}

bool PSBrain::isFighter() {
    return (PSCombatClass)owner->object->entity->combat_class == PS_COMBAT_CLASS_FIGHTER;
}

// DamageSys_IsSeriouslyDamaged_41612 : systèmes 0, 4 et 3 du chunk SYSN d'EXTENTS.IFF
bool PSBrain::seriouslyDamaged() {
    if (this->systemDamagePercent("ENGINE") > 30) {
        return true;
    }
    if (this->systemDamagePercent("AILERON") > 50) {
        return true;
    }
    return this->systemDamagePercent("ELEVATOR") > 50;
}

// DamageSys_PercentOfSystem_41595 : 100 * dégâts / points du système
int PSBrain::systemDamagePercent(const std::string &name) {
    for (auto &system : owner->object->entity->sysm) {
        auto sub_system = system.second.find(name);
        if (sub_system == system.second.end() || sub_system->second == 0) {
            continue;
        }
        auto health_system = owner->plane->system_health.find(system.first);
        if (health_system == owner->plane->system_health.end()) {
            return 0;
        }
        auto health = health_system->second.find(name);
        if (health == health_system->second.end()) {
            return 0;
        }
        int damage = (int)sub_system->second - (int)health->second;
        return 100 * damage / (int)sub_system->second;
    }
    return 0;
}

// [entité+178] : niveau du pilote d'après l'octet 5 d'ATRB (FL)
int PSBrain::pilotLevel() {
    int flying = owner->profile->ai.atrb.FL;
    if (flying > 12) {
        return 2;
    }
    if (flying > 6) {
        return 1;
    }
    return 0;
}

// MVRS_ID7_ApplyFollowLeader_108DD : vers le chef, 2 s
void PSBrain::startFollowLeader(SCMissionActors *leader) {
    this->applyManeuver(7, leader, REACT_NONE);
    maneuver.timer = 2.0f;
    owner->pilot->BeginManual();
    Vector3D direction = leader->plane->position - owner->plane->position;
    int distance = (int)direction.Length();
    float leader_speed = leader->plane->worldVelocity().Length();
    float wanted = (float)owner->object->entity->jdyn->ai_speed_max;
    if ((int)this->headingDelta(direction) < 30) {
        if (2000 / 3 > distance && this->pilotLevel() != 0) {
            wanted = leader_speed;
        } else {
            wanted = leader_speed + 30.0f;
        }
    }
    owner->pilot->target_waypoint = leader->plane->position;
    owner->pilot->CmdGuidance(direction);
    this->speedThrottle(wanted);
    printf("PSBrain %s#%d manoeuvre 7 vers %s#%d : distance %d m, chef %.0f m/s, vitesse voulue %.0f m/s\n", owner->actor_name.c_str(), owner->actor_id, leader->actor_name.c_str(), leader->actor_id, distance, leader_speed, wanted);
}

bool PSBrain::tickManeuver() {
    if (maneuver.id != 7) {
        return SCAIBrain::tickManeuver();
    }
    return this->tickFollowLeader();
}

// MVRS_ID7_TickFollowLeader_109A4
bool PSBrain::tickFollowLeader() {
    maneuver.timer -= TICK_DURATION;
    SCMissionActors *leader = maneuver.target;
    if (leader != nullptr && leader->plane != nullptr) {
        owner->pilot->BeginManual();
        Vector3D direction = leader->plane->position - owner->plane->position;
        int distance = (int)direction.Length();
        float leader_speed = leader->plane->worldVelocity().Length();
        float wanted = (float)owner->object->entity->jdyn->ai_speed_max;
        if ((int)this->headingDelta(leader->plane->forward) < 90) {
            if (2000 / 2 > distance) {
                if (this->pilotLevel() != 0) {
                    wanted = leader_speed;
                } else {
                    wanted = leader_speed + 30.0f;
                }
            } else {
                wanted = leader_speed + 25.0f;
            }
        }
        owner->pilot->target_waypoint = leader->plane->position;
        owner->pilot->CmdGuidance(direction);
        this->speedThrottle(wanted);
        this->decide("manoeuvre 7 : d %d m, chef %.0f m/s, voulue %.0f m/s", distance, leader_speed, wanted);
    }
    if (leader == nullptr || maneuver.timer < 0.0f) {
        maneuver.end_reason = leader == nullptr ? "sans chef" : "minuteur";
        this->endManeuver(true);
        owner->pilot->ClearGuidance();
        return false;
    }
    return true;
}

// sub_9C3D : manette 1, 8 ou 10 selon l'écart de vitesse
void PSBrain::speedThrottle(float wanted) {
    int error = (int)wanted - (int)owner->plane->worldVelocity().Length();
    PSSpeedState measured = PS_SPEED_MUCH_TOO_FAST;
    if (error > 20) {
        measured = PS_SPEED_MUCH_TOO_SLOW;
    } else if (error > 1) {
        measured = PS_SPEED_TOO_SLOW;
    } else if (error >= 0) {
        measured = PS_SPEED_ON_TARGET;
    } else if (error > -2) {
        measured = PS_SPEED_SLIGHTLY_FAST;
    } else if (error > -21) {
        measured = PS_SPEED_TOO_FAST;
    }
    PSSpeedState previous = speed_state;
    speed_state = this->nextSpeedState(measured, previous);
    int throttle = -1;
    switch (speed_state) {
    case PS_SPEED_MUCH_TOO_FAST:
    case PS_SPEED_TOO_FAST:
        throttle = 1;
        break;
    case PS_SPEED_MUCH_TOO_SLOW:
    case PS_SPEED_TOO_SLOW:
        throttle = 10;
        break;
    case PS_SPEED_SLIGHTLY_FAST:
    case PS_SPEED_ON_TARGET:
        throttle = 8;
        break;
    default:
        break;
    }
    if (speed_state != previous) {
        printf("PSBrain %s#%d vitesse voulue %.0f m/s, écart %d : état %d -> %d, manette %d\n", owner->actor_name.c_str(), owner->actor_id, wanted, error, previous, speed_state, throttle);
    }
    if (throttle >= 0) {
        owner->pilot->CmdThrottle(throttle);
    }
}

// sub_97D3 : selon l'état précédent, l'état mesuré le remplace ou non (maintien de la manette)
PSSpeedState PSBrain::nextSpeedState(PSSpeedState measured, PSSpeedState previous) {
    switch (previous) {
    case PS_SPEED_NONE:
        return measured;
    case PS_SPEED_MUCH_TOO_FAST:
        switch (measured) {
        case PS_SPEED_MUCH_TOO_SLOW:
        case PS_SPEED_TOO_SLOW:
        case PS_SPEED_ON_TARGET:
            return measured;
        case PS_SPEED_SLIGHTLY_FAST:
            return PS_SPEED_TOO_FAST;
        default:
            return previous;
        }
    case PS_SPEED_MUCH_TOO_SLOW:
        switch (measured) {
        case PS_SPEED_MUCH_TOO_FAST:
        case PS_SPEED_TOO_FAST:
        case PS_SPEED_SLIGHTLY_FAST:
            return measured;
        case PS_SPEED_ON_TARGET:
            return PS_SPEED_TOO_SLOW;
        default:
            return previous;
        }
    case PS_SPEED_TOO_FAST:
        switch (measured) {
        case PS_SPEED_MUCH_TOO_FAST:
        case PS_SPEED_MUCH_TOO_SLOW:
        case PS_SPEED_TOO_SLOW:
        case PS_SPEED_ON_TARGET:
            return measured;
        default:
            return previous;
        }
    case PS_SPEED_TOO_SLOW:
        switch (measured) {
        case PS_SPEED_MUCH_TOO_FAST:
        case PS_SPEED_MUCH_TOO_SLOW:
        case PS_SPEED_TOO_FAST:
        case PS_SPEED_SLIGHTLY_FAST:
            return measured;
        default:
            return previous;
        }
    default:
        return measured;
    }
}
