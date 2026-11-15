#include "../strike_commander/precomp.h"
#include "PSBrain.h"
#include "PSEntity.h"
#include "PSPlane.h"

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
        if (!this->orderInProgress()) {
            this->decide("au sol, ordre termine");
            return false;
        }
        this->decide("au sol");
        return this->executeGoalAction();
    }
    for (uint8_t rawSelector : owner->profile->ai.goal) {
        size_t mark = strlen(decision);
        bool acted = false;
        switch ((PSGoalSelector)rawSelector) {
        case PS_GOAL_EXECUTE_OBJECTIVE:
            if (!this->orderInProgress()) {
                this->decide("GOAL %d ordre termine", rawSelector);
                break;
            }
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
    if (maneuver.id == 22) {
        return this->tickGunAttack();
    }
    if (maneuver.id == 14 || maneuver.id == 15) {
        return this->tickRecovery();
    }
    if (maneuver.id >= 3 && maneuver.id <= 6) {
        return this->tickCombatManeuver();
    }
    if (maneuver.id == 19) {
        return this->tickGroundAttack();
    }
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

bool PSBrain::isFleeing() {
    return formation.leader_state == 2;
}

// AI_JoinFormation_E36E
void PSBrain::joinFormation() {
    in_formation = true;
    formation.leader_state = 0;
    objective_locked = false;
    owner->target = nullptr;
    printf("PSBrain %s#%d rejoint la formation\n", owner->actor_name.c_str(), owner->actor_id);
}

// AI_LeaveFormation_E39C
void PSBrain::leaveFormation() {
    in_formation = false;
    formation.leader_state = 0;
    objective_locked = false;
    owner->target = nullptr;
    printf("PSBrain %s#%d quitte la formation\n", owner->actor_name.c_str(), owner->actor_id);
}

// AI_FuelPercent_EFF3 : moins de 55 %
bool PSBrain::lowFuel() {
    float capacity = (float)owner->object->entity->jdyn->fuel_capacity;
    if (capacity <= 0.0f) {
        return false;
    }
    return 100.0f * owner->plane->GetFuel() / capacity < 55.0f;
}

// AI_StatusReportMessage_EDC7 : la dernière condition vraie l'emporte
uint8_t PSBrain::statusReply() {
    uint8_t reply = 2;
    if (this->seriouslyDamaged()) {
        reply = 29;
    }
    if (this->lowFuel()) {
        reply = 4;
    }
    if (this->isFleeing()) {
        reply = 8;
    }
    return reply;
}

bool PSBrain::enemiesActive() {
    return mood.enemies_active;
}

bool PSBrain::unarmed() {
    return this->loadedWeaponMask() == 0;
}

// Goal_IsComplete_C73C : l'ordre en cours est-il encore à exécuter
bool PSBrain::orderInProgress() {
    SCMissionActors *target = owner->target;
    std::vector<SPOT *> &spots = owner->mission->mission->mission_data.spots;
    switch (owner->current_command) {
    case OP_SET_OBJ_TAKE_OFF:
        return this->isRunning(BEHAVIOR_TAKEOFF) || owner->plane == nullptr || owner->plane->on_ground || !owner->taken_off;
    case OP_SET_OBJ_LAND:
        return last_finished_behavior != 0x12;
    case OP_SET_OBJ_FLY_TO_WP: {
        if (owner->plane == nullptr || owner->current_command_arg >= spots.size()) {
            return false;
        }
        Vector3D delta = spots[owner->current_command_arg]->position - owner->plane->position;
        delta.y = 0.0f;
        return delta.Length() > 500.0f;
    }
    case OP_SET_OBJ_DESTROY_TARGET:
    case OP_SET_OBJ_DEFEND_TARGET:
        return target != nullptr && !target->is_destroyed;
    case OP_SET_OBJ_DEFEND_AREA:
        return this->opposingCampAlive() == 0;
    case OP_SET_OBJ_FOLLOW_ALLY:
        return false;
    case OP_SET_OBJ_PATROL_POSITION:
        return true;
    default:
        return !owner->current_command_executed;
    }
}

// GroundAttack_PhaseDispatch_13C9B : le sol est-il au-dessus du point où sera l'avion dans 40 s en ligne droite
bool PSBrain::groundAhead() {
    Vector3D ahead = owner->plane->position + owner->plane->worldVelocity() * 40.0f;
    float ground = owner->plane->area->getY(ahead.x, ahead.z);
    return ground >= ahead.y;
}

// AI_BehaviorStateMachine_B968 : tournoi sur la liste MVRS de l'entité, construite au chargement (ovr236) :
// 5 nœuds fixes de poids 0 (19 attaque au sol, 22, 14 évitement du sol, 15 sortie de décrochage, 7 vol vers la cible),
// puis les manœuvres du profil que sub_84B24 (fonction pas encore nommée) sait créer (3, 4, 5, 6), 10 au plus.
// total = note + poids ± 1, le plus grand au-dessus de −1000 gagne ; le nœud 19 vise la cible au sol.
bool PSBrain::runTournament() {
    tournament_scores[0] = '\0';
    int ids[10];
    int weights[10];
    int count = 0;
    static const int fixed_ids[5] = {
        19,
        22,
        14,
        15,
        7
    };
    for (int i = 0; i < 5; i++) {
        ids[count] = fixed_ids[i];
        weights[count] = 0;
        count++;
    }
    for (auto &entry : owner->profile->ai.mvrs) {
        if (count >= 10) {
            break;
        }
        if (entry.node_id < 3 || entry.node_id > 6) {
            continue;
        }
        ids[count] = entry.node_id;
        weights[count] = entry.value;
        count++;
    }
    int best = -1000;
    int best_id = 0;
    int best_hint = 0;
    uint8_t best_bits = 0;
    Vector3D best_point = maneuver.point;
    for (int i = 0; i < count; i++) {
        SCMissionActors *target = ids[i] == 19 ? this->groundAttackTarget() : air_target;
        maneuver.phase_hint = 0;
        maneuver.bits = 0;
        int score = 0;
        if (ids[i] == 22) {
            score = this->scoreGunAttack(target);
        } else if (ids[i] == 7) {
            score = this->scoreFollowTarget(target);
        } else if (ids[i] == 19) {
            score = this->scoreGroundAttack(target);
        } else if (ids[i] == 3) {
            score = this->scoreJink(target);
        } else if (ids[i] == 4) {
            score = this->scoreClimbAway(target);
        } else if (ids[i] == 5) {
            score = this->scoreImmelmann(target);
        } else if (ids[i] == 6) {
            score = this->scoreSplitS(target);
        } else if (ids[i] == 14) {
            score = this->scoreGroundAvoid();
        } else if (ids[i] == 15) {
            score = this->scoreStallRecovery();
        } else {
            score = this->scoreManeuver(ids[i], target);
        }
        if (score == 0) {
            continue;
        }
        int total = score + weights[i] + ((std::rand() & 1) ? 1 : -1);
        size_t used = strlen(tournament_scores);
        snprintf(tournament_scores + used, sizeof(tournament_scores) - used, "%s%d:%d", used > 0 ? " " : "", ids[i], total);
        if (total > best) {
            best = total;
            best_id = ids[i];
            best_hint = maneuver.phase_hint;
            best_bits = maneuver.bits;
            best_point = maneuver.point;
        }
    }
    if (best_id == 0) {
        return false;
    }
    maneuver.phase_hint = best_hint;
    maneuver.bits = best_bits;
    maneuver.point = best_point;
    SCMissionActors *target = best_id == 19 ? this->groundAttackTarget() : air_target;
    if (best_id == 19) {
        this->startGroundAttack(target);
        return true;
    }
    printf("PSBrain %s#%d tournoi : manoeuvre %d gagne (%s)\n", owner->actor_name.c_str(), owner->actor_id, best_id, tournament_scores);
    if (best_id == 22) {
        this->startGunAttack(target);
        return true;
    }
    if (best_id >= 3 && best_id <= 6) {
        this->startCombatManeuver(best_id, target);
        return true;
    }
    if (best_id == 7 && target != nullptr && target->plane != nullptr) {
        this->startFollowLeader(target);
        return true;
    }
    this->applyManeuver(best_id, target, REACT_NONE);
    return true;
}

// sub_A1A8 (fonction pas encore nommée), note du nœud 7 : 5 si la cible est un avion
int PSBrain::scoreFollowTarget(SCMissionActors *target) {
    if (target == nullptr || target->plane == nullptr || target->is_destroyed) {
        return 0;
    }
    return 5;
}

// MVRS_ID22_AimQuality_1125E : error = angle entre la direction voulue et le nez ; pendant la note, la direction voulue
// est le nez (MVRS_SetupOwnState_9578), l'écart vaut 0
int PSBrain::gunAimQuality(SCMissionActors *target, float error) {
    if (target == nullptr || target->plane == nullptr) {
        return 0;
    }
    Vector3D delta = target->plane->position - owner->plane->position;
    float distance = delta.Length();
    float off_nose = owner->plane->forward.AngleBetween(delta);
    int quality = 0;
    if (distance < 2000.0f) {
        quality = 100 - (int)(error * error);
    } else if (distance < 4000.0f && error <= 1.0f) {
        quality = 15;
    }
    if (gun_bursts != 0) {
        quality += (off_nose > 30.0f ? 20 : 10) - gun_bursts;
    }
    if (distance < 500.0f) {
        quality += (int)((500.0f - distance) / 20.0f);
    }
    quality += owner->profile->ai.atrb.TH;
    return (std::max)(quality, 0);
}

// MVRS_ID22_ScoreGunAttack_10C89 : attaque au canon contre un avion
int PSBrain::scoreGunAttack(SCMissionActors *target) {
    const char *refused = nullptr;
    float height = owner->plane->y - owner->plane->groundlevel;
    if (target == nullptr || target->plane == nullptr || target->is_destroyed) {
        refused = "pas de cible avion";
    } else if (owner->pilot->NosePitch() < 0.0f && height < this->levelFloor()) {
        refused = "pique trop bas";
    } else if (static_cast<PSPlane *>(owner->plane)->gunHardpoint() < 0) {
        refused = "pas de canon charge";
    }
    if (refused != nullptr) {
        if (refused != last_gun_refusal) {
            printf("PSBrain %s#%d manoeuvre 22 notee 0 : %s (hauteur %.0f m, assiette %.0f)\n", owner->actor_name.c_str(), owner->actor_id, refused, height, owner->pilot->NosePitch());
            last_gun_refusal = refused;
        }
        return 0;
    }
    last_gun_refusal = nullptr;
    gun_bursts = 0;
    int quality = this->gunAimQuality(target, 0.0f) + 2 * owner->profile->ai.atrb.AR;
    int score = quality >= 100 ? 10 : quality / 10;
    printf("PSBrain %s#%d manoeuvre 22 : distance %.0f m, qualite %d, note %d\n", owner->actor_name.c_str(), owner->actor_id, (target->plane->position - owner->plane->position).Length(), quality, score);
    return score;
}

// MVRS_ID22_ApplyGunAttack_10D3C : 2 s d'attaque, compteur de rafales à 0
void PSBrain::startGunAttack(SCMissionActors *target) {
    this->applyManeuver(22, target, REACT_NONE);
    maneuver.timer = 2.0f;
    gun_bursts = 0;
    this->tickGunAttack();
}

// MVRS_ID22_TickGunAttack_10D82 : visée de libRealSpace (gunSnap, sinon vers la cible), vitesses de Pacific Strike,
// tir au canon quand la qualité de visée dépasse 75
bool PSBrain::tickGunAttack() {
    maneuver.timer -= TICK_DURATION;
    SCMissionActors *target = maneuver.target;
    bool has_target = target != nullptr && target->plane != nullptr && !target->is_destroyed;
    if (has_target) {
        SCPilot *pilot = owner->pilot;
        pilot->BeginManual();
        Vector3D delta = target->plane->position - owner->plane->position;
        int distance = (int)delta.Length();
        pilot->target_waypoint = target->plane->position;
        if (!this->gunSnap(target)) {
            pilot->CmdGuidance(delta);
        }
        float target_speed = target->plane->worldVelocity().Length();
        float wanted = (float)owner->object->entity->jdyn->ai_speed_max;
        if ((int)this->headingDelta(target->plane->forward) < 90 && 2000 / 2 > distance) {
            wanted = this->pilotLevel() != 0 ? target_speed : target_speed + 30.0f;
        }
        this->speedThrottle(wanted);
        int quality = this->gunAimQuality(target, owner->plane->forward.AngleBetween(delta));
        if (quality > 75) {
            int hardpoint = static_cast<PSPlane *>(owner->plane)->gunHardpoint();
            if (hardpoint >= 0) {
                PlaneFireEvent fire;
                fire.plane = owner->plane;
                fire.hardpoint = hardpoint;
                fire.target = target;
                fire.mission = owner->mission;
                MessageBus::getInstance().publish(std::make_unique<PlaneFireEvent>(fire));
            }
            gun_bursts++;
        }
        this->decide("manoeuvre 22 : d %d m, visee %d, rafales %d", distance, quality, gun_bursts);
    }
    if (!has_target || maneuver.timer < 0.0f) {
        maneuver.end_reason = has_target ? "minuteur" : "sans cible";
        printf("PSBrain %s#%d manoeuvre 22 finie (%s), %d rafales\n", owner->actor_name.c_str(), owner->actor_id, maneuver.end_reason, gun_bursts);
        this->endManeuver(true);
        owner->pilot->ClearGuidance();
        return false;
    }
    return true;
}

// AI_BehaviorStateMachine_B968 (GOAL 4) : le comportement en cours tourne ; sinon cible (aérienne, au sol, ou cherchée
// par Targeting_SelectBestTarget_BEB6), puis le tournoi. Pas de tir hors des manœuvres : en combat aérien, seul le
// nœud 22 tire.
bool PSBrain::behaviorStateMachine() {
    if (this->behaviorRunning()) {
        this->decide("%s en cours", this->behaviorLabel().c_str());
        this->tickBehavior();
        return true;
    }
    if (air_target == nullptr && ground_target == nullptr) {
        this->acquireBestThreat(true);
    }
    if (air_target == nullptr && ground_target == nullptr) {
        this->decide("aucune cible");
        return false;
    }
    bool won = this->runTournament();
    if (won) {
        this->decide("tournoi : ID %d (%s)", maneuver.id, tournament_scores);
    } else {
        this->decide("tournoi sans gagnant (%s)", tournament_scores);
    }
    return won;
}

// les ordres du script (Goal_ExecuteObjective_C9E4, cas 167) et les réactions passent par le GOAL 4 de Pacific Strike
bool PSBrain::combatStep(bool ground_allowed) {
    return this->behaviorStateMachine();
}

// plancher de hauteur selon le niveau du pilote [entité+178] (word_7C804 : 250, 200, 150 m)
float PSBrain::levelFloor() {
    static const float floor_by_level[3] = {
        250.0f,
        200.0f,
        150.0f
    };
    return floor_by_level[this->pilotLevel()];
}

// AI_TopLevelThink_B0C4 : réflexes 14 et 15 avant le reste de la réflexion, même si un comportement tourne
void PSBrain::topLevelThink() {
    if (this->pacificReflexes()) {
        return;
    }
    SCAIBrain::topLevelThink();
}

// les réflexes de Strike Commander (manœuvres 14 et 15 de SCAIBrain) sont remplacés par pacificReflexes
bool PSBrain::runReflexes() {
    return false;
}

// AI_TopLevelThink_B0C4 : sous 1000 m, hors attaque au sol, sol devant (40 s en ligne droite) : nœud 14 ; sinon
// décrochage : nœud 15. Le comportement en cours est abandonné et la réflexion s'arrête pour ce tick.
bool PSBrain::pacificReflexes() {
    if (owner->plane->on_ground || owner->plane->ejected) {
        return false;
    }
    float height = owner->plane->y - owner->plane->groundlevel;
    int id = 0;
    if (height < 1000.0f && !(maneuver.id == 19 && this->behaviorRunning()) && this->groundAhead()) {
        id = 14;
    } else if (this->stallCondition()) {
        id = 15;
    }
    if (id == 0) {
        return false;
    }
    if (maneuver.id == id && this->behaviorRunning()) {
        maneuver.timer = id == 14 ? 4.0f : 8.0f;
        this->tickManeuver();
    } else {
        printf("PSBrain %s#%d reflexe %d : hauteur %.0f m, vitesse %.0f m/s, assiette %.0f, vitesse verticale %.1f\n", owner->actor_name.c_str(), owner->actor_id, id, height, owner->plane->worldVelocity().Length(), owner->pilot->NosePitch(), owner->plane->worldVelocity().y);
        this->abandonBehavior();
        this->startRecovery(id);
    }
    this->decide("reflexe %d", id);
    return true;
}

// sub_A215 : monte plus lentement que le champ 24 du JDYN
bool PSBrain::stallCondition() {
    float speed = owner->plane->worldVelocity().Length();
    return (float)owner->object->entity->jdyn->ai_speed_min > speed && owner->pilot->NosePitch() > 0.0f;
}

// sub_A1BA (fonction pas encore nommée) : note du nœud 14
int PSBrain::scoreGroundAvoid() {
    if (last_finished_behavior == 19 || owner->pilot->autopilotActive()) {
        return 0;
    }
    float height = owner->plane->y - owner->plane->groundlevel;
    if (height <= this->levelFloor() && owner->pilot->NosePitch() < 0.0f) {
        return 10;
    }
    return 0;
}

// sub_A215 (fonction pas encore nommée) : note du nœud 15
int PSBrain::scoreStallRecovery() {
    if (owner->pilot->autopilotActive()) {
        return 0;
    }
    return this->stallCondition() ? 10 : 0;
}

// sub_10B1B / sub_10BD2 (fonctions pas encore nommées) : lancement des nœuds 14 (4 s) et 15 (8 s), puis le tick
void PSBrain::startRecovery(int id) {
    uint8_t level = id == 14 ? REACT_GROUND_AVOID : REACT_STALL_RECOVERY;
    this->applyManeuver(id, nullptr, level);
    reaction_level = level;
    maneuver.timer = id == 14 ? 4.0f : 8.0f;
    this->tickManeuver();
}

// sub_10B52 / sub_10C09 (fonctions pas encore nommées) : nez à +20° (14) ou +5° (15), ailes à plat, vitesse du champ
// 23 du JDYN ; à moins de 200 m et sérieusement endommagé : éjection (sub_A58C)
bool PSBrain::tickRecovery() {
    maneuver.timer -= TICK_DURATION;
    float height = owner->plane->y - owner->plane->groundlevel;
    if (height < 200.0f && this->seriouslyDamaged() && !owner->plane->ejected) {
        this->ejectPilot();
    }
    float pitch = maneuver.id == 14 ? 20.0f : 5.0f;
    Vector3D direction(owner->plane->forward.x, 0.0f, owner->plane->forward.z);
    direction.Normalize();
    direction.y = tanf(degreeToRad(pitch));
    owner->pilot->SetGuidanceDirection(direction);
    this->speedThrottle((float)owner->object->entity->jdyn->ai_speed_max);
    this->decide("manoeuvre %d : hauteur %.0f m, assiette %.0f, reste %.1f s", maneuver.id, height, owner->pilot->NosePitch(), maneuver.timer);
    if (maneuver.timer < 0.0f) {
        maneuver.end_reason = "minuteur";
        printf("PSBrain %s#%d manoeuvre %d finie, hauteur %.0f m\n", owner->actor_name.c_str(), owner->actor_id, maneuver.id, height);
        this->endManeuver(true);
        owner->pilot->ClearGuidance();
        return false;
    }
    return true;
}

// sub_A58C (fonction pas encore nommée) : réplique 9 envoyée même en silence radio, puis éjection
void PSBrain::ejectPilot() {
    bool talkative = owner->talkative;
    owner->talkative = true;
    owner->setMessage(9);
    owner->talkative = talkative;
    owner->plane->ejected = true;
    owner->plane->object->alive = false;
    printf("PSBrain %s#%d s'ejecte (hauteur %.0f m, serieusement endommage)\n", owner->actor_name.c_str(), owner->actor_id, owner->plane->y - owner->plane->groundlevel);
}

// Math_HeadingDelta_A41D : écart de cap horizontal entre deux vecteurs, en degrés (0 à 180)
float PSBrain::headingBetween(Vector3D a, Vector3D b) {
    return std::fabs(signed180(a.Azimuth() - b.Azimuth()));
}

// byte_7C7FB : valeur de manche des manœuvres 5 et 6 selon le niveau du pilote (14, 15, 16)
int PSBrain::combatStick() {
    static const int stick_by_level[3] = {
        14,
        15,
        16
    };
    return stick_by_level[this->pilotLevel()];
}

// sub_A03C (fonction pas encore nommée) : note de la manœuvre 3, esquive contre une cible derrière qui vise l'avion
int PSBrain::scoreJink(SCMissionActors *target) {
    if (target == nullptr || target->plane == nullptr || target->is_destroyed) {
        return 0;
    }
    Vector3D delta = target->plane->position - owner->plane->position;
    float distance = delta.Length();
    float aspect = this->headingBetween(target->plane->forward, -delta);
    if (aspect > 20.0f || this->headingDelta(delta) < 150.0f || distance > 2000.0f) {
        return 0;
    }
    if (this->pilotLevel() == 2) {
        if (last_finished_behavior == 3 || distance >= 1000.0f) {
            return 0;
        }
        return 4;
    }
    return distance < 1000.0f ? 5 : 4;
}

// sub_A277 (fonction pas encore nommée) : note de la manœuvre 4, dégagement en montée
int PSBrain::scoreClimbAway(SCMissionActors *target) {
    if (target == nullptr || target->plane == nullptr || target->is_destroyed) {
        return 0;
    }
    float height = owner->plane->y - owner->plane->groundlevel;
    if (height > 8333.0f) {
        return 0;
    }
    if (last_finished_behavior == 6) {
        return 9;
    }
    Vector3D delta = target->plane->position - owner->plane->position;
    float distance = delta.Length();
    float aspect = this->headingBetween(target->plane->forward, -delta);
    if (this->headingDelta(delta) < 90.0f) {
        return 0;
    }
    if (aspect < 90.0f && distance < 2000.0f) {
        return 0;
    }
    int score = 0;
    if (last_finished_behavior == 4) {
        if (height < 1000.0f) {
            return 9;
        }
        score += 3;
    }
    score += aspect < 155.0f ? 1 : 3;
    if (height < this->levelFloor()) {
        score += 4;
    } else if (height < 1000.0f) {
        score += 2;
    }
    return (std::min)(score, 8);
}

// sub_A0BD (fonction pas encore nommée) : note de la manœuvre 5, Immelmann
int PSBrain::scoreImmelmann(SCMissionActors *target) {
    if (target == nullptr || target->plane == nullptr || target->is_destroyed) {
        return 0;
    }
    float height = owner->plane->y - owner->plane->groundlevel;
    float speed = owner->plane->worldVelocity().Length();
    float pitch = owner->pilot->NosePitch();
    JDYN *jdyn = owner->object->entity->jdyn;
    if (height > 8333.0f || (float)(jdyn->ai_speed_max / 2 + 2 * jdyn->ai_speed_min) > speed || pitch < -10.0f || pitch > 20.0f) {
        return 0;
    }
    if (this->systemDamagePercent("ENGINE") + this->systemDamagePercent("ELEVATOR") + this->systemDamagePercent("AILERON") > 0) {
        return 0;
    }
    Vector3D delta = target->plane->position - owner->plane->position;
    float off_nose = this->headingDelta(delta);
    if (delta.Length() < 200.0f && off_nose < 20.0f && this->headingDelta(target->plane->forward) > 160.0f) {
        return 7;
    }
    int score = 0;
    if (height < 1000.0f) {
        score += 3;
    }
    if (off_nose > 150.0f) {
        score += 3;
    } else if (off_nose > 90.0f) {
        score += 1;
    }
    return score;
}

// sub_A333 (fonction pas encore nommée) : note de la manœuvre 6, split-S, après un vol vers la cible (7)
int PSBrain::scoreSplitS(SCMissionActors *target) {
    if (target == nullptr || target->plane == nullptr || target->is_destroyed) {
        return 0;
    }
    Vector3D delta = target->plane->position - owner->plane->position;
    float distance = delta.Length();
    float aspect = this->headingBetween(target->plane->forward, -delta);
    float pitch = owner->pilot->NosePitch();
    float height = owner->plane->y - owner->plane->groundlevel;
    if (aspect > 30.0f || distance > 1800.0f || pitch < -12.0f || pitch > 20.0f || last_finished_behavior != 7 || height < 2.0f * this->levelFloor()) {
        return 0;
    }
    JDYN *jdyn = owner->object->entity->jdyn;
    int score = 0;
    if ((float)(2 * jdyn->ai_speed_min) > owner->plane->worldVelocity().Length()) {
        score += 2;
    }
    if (this->headingDelta(delta) > 135.0f) {
        score += 2;
    }
    if (distance < 4000.0f) {
        score += 4;
    }
    if (jdyn->ai_unknown_8a >= 3) {
        score += 1;
    }
    if (jdyn->ai_decision_weight >= 3) {
        score -= 1;
    }
    return (std::min)(score, 8);
}

// lancements sub_1047B (3), sub_114E1 (4), sub_105C1 (5), sub_1157B (6) : 2 s pour 3 et 4, 8 s pour 5 et 6 ;
// la manœuvre 3 tire sa variante, parmi 5 sous le plancher, sinon parmi 8
void PSBrain::startCombatManeuver(int id, SCMissionActors *target) {
    this->applyManeuver(id, target, REACT_NONE);
    maneuver.timer = id <= 4 ? 2.0f : 8.0f;
    maneuver.phase = 0;
    if (id == 3) {
        float height = owner->plane->y - owner->plane->groundlevel;
        maneuver.phase = std::rand() % (height < this->levelFloor() ? 5 : 8);
    }
    printf("PSBrain %s#%d manoeuvre %d lancee, phase %d\n", owner->actor_name.c_str(), owner->actor_id, id, maneuver.phase);
    this->tickCombatManeuver();
}

// ticks sub_10503 (3), sub_1151D (4), sub_105FE (5), sub_115B7 (6)
bool PSBrain::tickCombatManeuver() {
    static const float jink_pitch[8] = {
        30.0f,
        30.0f,
        30.0f,
        0.0f,
        0.0f,
        -30.0f,
        -30.0f,
        -30.0f
    };
    static const float jink_turn[8] = {
        -30.0f,
        0.0f,
        30.0f,
        -30.0f,
        -30.0f,
        -30.0f,
        0.0f,
        30.0f
    };
    maneuver.timer -= TICK_DURATION;
    SCPilot *pilot = owner->pilot;
    JDYN *jdyn = owner->object->entity->jdyn;
    float pitch = pilot->NosePitch();
    int phase_before = maneuver.phase;
    bool finished = false;
    float wanted_speed = (float)jdyn->ai_speed_max;
    Vector3D level(owner->plane->forward.x, 0.0f, owner->plane->forward.z);
    level.Normalize();
    if (maneuver.id == 3) {
        Vector3D direction = level.RotateAzimuth(jink_turn[maneuver.phase]);
        direction.y = tanf(degreeToRad(jink_pitch[maneuver.phase]));
        pilot->SetGuidanceDirection(direction);
    } else if (maneuver.id == 4) {
        float climb = (float)jdyn->ai_speed_min > owner->plane->worldVelocity().Length() ? 0.0f : 30.0f;
        Vector3D direction = level;
        direction.y = tanf(degreeToRad(climb));
        pilot->SetGuidanceDirection(direction);
    } else if (maneuver.id == 5) {
        pilot->BeginManual();
        if (maneuver.phase == 0) {
            pilot->CmdRollTo(0.0f, 5.0f);
            if (std::fabs(pilot->BankAngle()) < 20.0f) {
                maneuver.phase = 1;
            }
        } else if (maneuver.phase == 1) {
            pilot->CmdRollTo(0.0f, 5.0f);
            pilot->CmdPitchStick((float)this->combatStick());
            if (pitch > 35.0f) {
                maneuver.phase = 2;
            }
        } else if (maneuver.phase == 2) {
            pilot->CmdPitchStick((float)this->combatStick());
            if (pitch < 30.0f) {
                maneuver.phase = 3;
            }
        } else {
            pilot->CmdPitchTo(0.0f, 5.0f);
            finished = pitch < 15.0f;
        }
    } else {
        pilot->BeginManual();
        if (maneuver.phase == 0) {
            pilot->CmdRollTo(180.0f, 5.0f);
            if (std::fabs(pilot->BankAngle()) > 160.0f) {
                maneuver.phase = 1;
            }
        } else if (maneuver.phase == 1) {
            wanted_speed = (float)(2 * jdyn->ai_speed_min);
            pilot->CmdRollTo(180.0f, 5.0f);
            pilot->CmdPitchStick((float)this->combatStick());
            if (pitch < -35.0f) {
                maneuver.phase = 2;
            }
        } else if (maneuver.phase == 2) {
            wanted_speed = (float)(2 * jdyn->ai_speed_min);
            pilot->CmdPitchStick((float)this->combatStick());
            if (pitch > -35.0f) {
                maneuver.phase = 3;
            }
        } else {
            pilot->CmdPitchTo(0.0f, 5.0f);
        }
    }
    this->speedThrottle(wanted_speed);
    if (maneuver.id >= 5 && maneuver.phase != phase_before) {
        printf("PSBrain %s#%d manoeuvre %d phase %d -> %d (assiette %.0f, roulis %.0f)\n", owner->actor_name.c_str(), owner->actor_id, maneuver.id, phase_before, maneuver.phase, pitch, pilot->BankAngle());
    }
    this->decide("manoeuvre %d phase %d : assiette %.0f, reste %.1f s", maneuver.id, maneuver.phase, pitch, maneuver.timer);
    if (finished || maneuver.timer < 0.0f) {
        maneuver.end_reason = finished ? "fin de la manoeuvre" : "minuteur";
        printf("PSBrain %s#%d manoeuvre %d finie (%s)\n", owner->actor_name.c_str(), owner->actor_id, maneuver.id, maneuver.end_reason);
        this->endManeuver(true);
        pilot->ClearGuidance();
        return false;
    }
    return true;
}

// sub_FBE7 (fonction pas encore nommée) : collision à moins de 50 m (même camp) ou 25 m (ennemi), ou passage à moins de
// 50 m dans les 4 s ; sub_BD4C : évitement = écart × vitesse de l'autre × 4, sans descendre sous 600 m d'altitude,
// puis le nœud 20 pendant 2 s
bool PSBrain::collisionCourse(SCMissionActors *other) {
    Vector3D away = owner->plane->position - other->plane->position;
    float distance = away.Length();
    Vector3D other_velocity = other->plane->worldVelocity();
    Vector3D closing = other_velocity - owner->plane->worldVelocity();
    float closing_speed = closing.Length();
    float immediate = other->team_id == owner->team_id ? 50.0f : 25.0f;
    bool collision = distance < immediate;
    if (!collision && closing_speed > 0.0f) {
        float angle = closing.AngleBetween(away);
        if (angle < 90.0f) {
            float miss = distance * sinf(degreeToRad(angle));
            float along = distance * cosf(degreeToRad(angle));
            collision = miss < 50.0f && along <= 4.0f * closing_speed;
        }
    }
    if (!collision) {
        return false;
    }
    Vector3D side = Vector3D::Cross(away, other_velocity * 4.0f);
    if (owner->plane->y < 600.0f && side.y < 0.0f) {
        side.y = 0.0f;
    }
    this->abandonBehavior();
    this->applyManeuver(20, nullptr, REACT_COLLISION);
    maneuver.segment = side;
    maneuver.segment_timer = 2.0f;
    reaction_level = REACT_COLLISION;
    printf("PSBrain %s#%d evitement de collision avec %s#%d, d %.0f m, seuil %.0f m\n", owner->actor_name.c_str(), owner->actor_id, other->actor_name.c_str(), other->actor_id, distance, immediate);
    this->tickManeuver();
    return true;
}

// sub_10839 (fonction pas encore nommée) : celui de SCAIBrain, avec la vitesse voulue = champ 23 du JDYN tant que le
// pilote automatique n'est pas engagé
void PSBrain::tickNavigation() {
    nav.timer -= TICK_DURATION;
    owner->pilot->target_waypoint = nav.point;
    if (!owner->pilot->autopilotActive()) {
        this->speedThrottle((float)owner->object->entity->jdyn->ai_speed_max);
        if (std::fabs(owner->pilot->NosePitch()) < 15.0f) {
            owner->pilot->engageAutopilot(nav.point, nav.velocity);
        } else {
            owner->pilot->SetPitchCommand(0.0f, 5.0f);
        }
    }
    nav.reached = owner->pilot->autopilotReached();
    if (nav.timer < 0.0f || nav.reached) {
        owner->pilot->disengageAutopilot();
        this->endBehavior(BEHAVIOR_NAVIGATION, true);
    }
}

// GroundAttack_CanEngage_137A1 : la cible au sol, sinon celle de l'ordre du script si elle est au sol
SCMissionActors *PSBrain::groundAttackTarget() {
    if (ground_target != nullptr && !ground_target->is_destroyed) {
        return ground_target;
    }
    SCMissionActors *target = owner->target;
    if (target != nullptr && !target->is_destroyed && target->object != nullptr && target->object->entity != nullptr && target->object->entity->target_type == 2) {
        return target;
    }
    return nullptr;
}

// GroundAttack_CanEngage_137A1 : navire, bombe / torpille / roquette 9, canon 7 ; à terre, arme autre que canon et
// torpille 9, canon 7, sinon 1 ; sans arme, 10 pour un kamikaze
int PSBrain::scoreGroundAttack(SCMissionActors *target) {
    if (target == nullptr || target->object == nullptr || target->object->entity == nullptr || target->object->entity->target_type != 2) {
        return 0;
    }
    PSPlane *plane = static_cast<PSPlane *>(owner->plane);
    bool kamikaze = (PSCombatClass)owner->object->entity->combat_class == PS_COMBAT_CLASS_KAMIKAZE;
    bool ship = target->object->entity->entity_type == EntityType::ship || target->object->entity->entity_type == EntityType::carr;
    if (ship) {
        if (plane->hardpointForMask(31) >= 0) {
            return 9;
        }
        if (plane->hardpointForMask(64) >= 0) {
            return 7;
        }
        return kamikaze ? 10 : 0;
    }
    if (plane->hardpointForMask(119) < 0) {
        return kamikaze ? 10 : 0;
    }
    if (plane->hardpointForMask(183) >= 0) {
        return 9;
    }
    if (plane->hardpointForMask(64) >= 0) {
        return 7;
    }
    return 1;
}

void PSBrain::setGroundPhase(int phase, float timer) {
    if (phase != maneuver.phase) {
        printf("PSBrain %s#%d attaque au sol : phase %d -> %d, %.0f s\n", owner->actor_name.c_str(), owner->actor_id, maneuver.phase, phase, timer);
    }
    maneuver.phase = phase;
    maneuver.timer = timer;
}

void PSBrain::fireHardpoint(int hardpoint, SCMissionActors *target) {
    if (hardpoint < 0) {
        return;
    }
    PlaneFireEvent fire;
    fire.plane = owner->plane;
    fire.hardpoint = hardpoint;
    fire.target = target;
    fire.mission = owner->mission;
    MessageBus::getInstance().publish(std::make_unique<PlaneFireEvent>(fire));
}

// GroundAttack_Start_13956 : phase d'après l'arme et le rôle ; altitude d'approche
void PSBrain::startGroundAttack(SCMissionActors *target) {
    PSPlane *plane = static_cast<PSPlane *>(owner->plane);
    PSCombatClass role = (PSCombatClass)owner->object->entity->combat_class;
    bool ship = target->object->entity->entity_type == EntityType::ship || target->object->entity->entity_type == EntityType::carr;
    this->applyManeuver(19, target, REACT_NONE);
    maneuver.phase = -1;
    float my_y = owner->plane->y;
    float target_y = target->object->position.y;
    float above = my_y - target_y;
    approach_altitude = my_y;
    ground_hardpoint = plane->hardpointForMask(ship ? 31 : 183);
    if (ground_hardpoint < 0) {
        ground_hardpoint = plane->hardpointForMask(64);
    }
    uint8_t mask = plane->hardpointMask(ground_hardpoint);
    if (role == PS_COMBAT_CLASS_KAMIKAZE || ground_hardpoint < 0) {
        this->setGroundPhase(role == PS_COMBAT_CLASS_KAMIKAZE ? 6 : 9, 4.0f);
    } else if (mask == 64) {
        approach_altitude = target_y + 500.0f;
        this->setGroundPhase(0, 2.0f);
    } else if (mask == 16) {
        this->setGroundPhase(1, 2.0f);
    } else if (mask == 8) {
        if (above > 350.0f) {
            approach_altitude = target_y + 350.0f;
        } else if (above < 250.0f) {
            approach_altitude = target_y + 250.0f;
        }
        this->setGroundPhase(5, 3.0f);
    } else {
        bool dive = false;
        if (role == PS_COMBAT_CLASS_DIVE_BOMBER) {
            approach_altitude = above < 3300.0f ? target_y + 3300.0f : my_y;
            dive = approach_altitude - 500.0f <= my_y;
        }
        if (dive) {
            this->setGroundPhase(3, 2.0f);
        } else {
            approach_altitude = above < 500.0f ? target_y + 500.0f : my_y;
            this->setGroundPhase(2, 3.0f);
        }
    }
    printf("PSBrain %s#%d attaque au sol de %s#%d : point d'emport %d (masque %d), phase %d, altitude d'approche %.0f m\n", owner->actor_name.c_str(), owner->actor_id, target->actor_name.c_str(), target->actor_id, ground_hardpoint, mask, maneuver.phase, approach_altitude);
    this->tickGroundAttack();
}

// GroundAttack_PhaseDispatch_13C9B et ses phases
bool PSBrain::tickGroundAttack() {
    SCPilot *pilot = owner->pilot;
    JDYN *jdyn = owner->object->entity->jdyn;
    SCMissionActors *target = maneuver.target;
    maneuver.timer -= TICK_DURATION;
    if (maneuver.phase != 4 && maneuver.phase != 7 && maneuver.phase != 10 && this->groundAhead()) {
        this->setGroundPhase(10, 4.0f);
    }
    if (maneuver.timer < 0.0f) {
        if (maneuver.phase == 3 || maneuver.phase == 4 || maneuver.phase == 5 || maneuver.phase == 7) {
            maneuver.timer = 5.0f;
        } else {
            maneuver.phase = 9;
        }
    }
    bool has_target = target != nullptr && !target->is_destroyed && target->object != nullptr;
    if (!has_target && maneuver.phase != 9 && maneuver.phase != 10 && maneuver.phase != 11) {
        this->setGroundPhase(10, 3.0f);
    }
    Vector3D delta(0.0f, 0.0f, 0.0f);
    float distance = 0.0f;
    if (has_target) {
        delta = target->object->position - owner->plane->position;
        distance = delta.Length();
    }
    float height = owner->plane->y - owner->plane->groundlevel;
    float off_nose = owner->plane->forward.AngleBetween(delta);
    Vector3D level(owner->plane->forward.x, 0.0f, owner->plane->forward.z);
    level.Normalize();
    float max_speed = (float)jdyn->ai_speed_max;
    switch (maneuver.phase) {
    case 0:
    case 1: {
        float range = 2500.0f;
        float close = maneuver.phase == 0 ? 600.0f : 1000.0f;
        if (off_nose < 5.0f) {
            pilot->SetGuidanceDirection(delta);
            if (distance <= range) {
                this->fireHardpoint(ground_hardpoint, target);
                maneuver.timer = 2.0f;
            }
            if (distance < close) {
                this->setGroundPhase(10, 10.0f);
            }
        } else if (off_nose > 35.0f && distance < (maneuver.phase == 0 ? 2000.0f : 2500.0f)) {
            Vector3D away(-delta.x, 0.0f, -delta.z);
            away.Normalize();
            away.y = 0.3f;
            pilot->SetGuidanceDirection(away);
        } else {
            pilot->SetGuidanceDirection(delta);
        }
        this->speedThrottle(owner->plane->worldVelocity().Length());
        break;
    }
    case 2:
    case 5: {
        Vector3D approach(delta.x, approach_altitude - owner->plane->y, delta.z);
        pilot->SetGuidanceDirection(approach);
        this->speedThrottle((float)jdyn->ai_speed_cruise);
        if (this->headingDelta(delta) >= 70.0f) {
            break;
        }
        bool release = false;
        if (maneuver.phase == 5) {
            release = distance >= 700.0f && distance <= 1525.0f;
        } else if (ground_hardpoint >= 0) {
            Vector3D impact = this->predictBombImpact(owner->plane->weaps_load[ground_hardpoint]->objct);
            Vector3D miss = impact - target->object->position;
            miss.y = 0.0f;
            release = miss.Length() < 20.0f + owner->plane->worldVelocity().Length() * TICK_DURATION;
        }
        if (release) {
            printf("PSBrain %s#%d largage %s a %.0f m de %s#%d\n", owner->actor_name.c_str(), owner->actor_id, maneuver.phase == 5 ? "torpille" : "bombe", distance, target->actor_name.c_str(), target->actor_id);
            this->fireHardpoint(ground_hardpoint, target);
            this->setGroundPhase(10, maneuver.phase == 5 ? 2.0f : 4.0f);
        }
        break;
    }
    case 3:
    case 6: {
        Vector3D horizontal(delta.x, 0.0f, delta.z);
        float above = owner->plane->y - target->object->position.y;
        Vector3D approach(delta.x, approach_altitude - owner->plane->y, delta.z);
        pilot->SetGuidanceDirection(approach);
        this->speedThrottle((float)jdyn->ai_speed_cruise);
        if (this->headingDelta(delta) < 5.0f && horizontal.Length() <= above) {
            this->setGroundPhase(maneuver.phase == 3 ? 4 : 7, 25.0f);
        }
        break;
    }
    case 4:
    case 7:
        pilot->SetGuidanceDirection(delta);
        this->speedThrottle(max_speed);
        if (maneuver.phase == 4 && height < 666.0f) {
            printf("PSBrain %s#%d largage en pique a %.0f m du sol\n", owner->actor_name.c_str(), owner->actor_id, height);
            this->fireHardpoint(ground_hardpoint, target);
            this->setGroundPhase(10, height < 600.0f ? 8.0f : 15.0f);
        }
        break;
    case 10: {
        Vector3D climb = level;
        climb.y = tanf(degreeToRad(8.0f));
        pilot->SetGuidanceDirection(climb);
        this->speedThrottle(max_speed);
        break;
    }
    case 11: {
        Vector3D away(-delta.x, 0.0f, -delta.z);
        pilot->SetGuidanceDirection(away);
        this->speedThrottle(max_speed);
        break;
    }
    default:
        break;
    }
    this->decide("attaque au sol phase %d : d %.0f m, hauteur %.0f m, reste %.1f s", maneuver.phase, distance, height, maneuver.timer);
    if (maneuver.phase == 9 || (maneuver.phase == 10 && maneuver.timer < 0.0f)) {
        maneuver.end_reason = "attaque au sol terminee";
        printf("PSBrain %s#%d attaque au sol finie\n", owner->actor_name.c_str(), owner->actor_id);
        this->endManeuver(true);
        pilot->ClearGuidance();
        return false;
    }
    return true;
}
