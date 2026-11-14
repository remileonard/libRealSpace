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

// GroundAttack_PhaseDispatch_13C9B : avant chaque phase de l'attaque au sol, si le sol est devant, phase 10 (sub_14325,
// fonction pas encore nommée) pendant 4 s : nez à +8°, ailes à plat, vitesse du champ 23 du JDYN ; puis l'attaque s'arrête
void PSBrain::updateGroundAttack(SCMissionActors *target) {
    if (pull_up_timer <= 0.0f && this->groundAhead()) {
        pull_up_timer = 4.0f;
        owner->pilot->disengageAutopilot();
        printf("PSBrain %s#%d attaque au sol : sol devant (altitude %.0f, sol %.0f, vitesse verticale %.1f), ressource 4 s\n", owner->actor_name.c_str(), owner->actor_id, owner->plane->y, owner->plane->groundlevel, owner->plane->worldVelocity().y);
    }
    if (pull_up_timer <= 0.0f) {
        SCAIBrain::updateGroundAttack(target);
        return;
    }
    pull_up_timer -= TICK_DURATION;
    Vector3D direction(owner->plane->forward.x, 0.0f, owner->plane->forward.z);
    direction.Normalize();
    direction.y = tanf(degreeToRad(8.0f));
    owner->pilot->SetGuidanceDirection(direction);
    this->speedThrottle((float)owner->object->entity->jdyn->ai_speed_max);
    ground_attack_active = true;
    this->decide("attaque au sol : ressource %.1f s", pull_up_timer);
    if (pull_up_timer <= 0.0f) {
        printf("PSBrain %s#%d attaque au sol : fin de la ressource, attaque arretee\n", owner->actor_name.c_str(), owner->actor_id);
        this->resetGroundAttack(false);
    }
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
        SCMissionActors *target = ids[i] == 19 ? ground_target : air_target;
        maneuver.phase_hint = 0;
        maneuver.bits = 0;
        int score = ids[i] == 22 ? this->scoreGunAttack(target) : this->scoreManeuver(ids[i], target);
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
    SCMissionActors *target = best_id == 19 ? ground_target : air_target;
    printf("PSBrain %s#%d tournoi : manoeuvre %d gagne (%s)\n", owner->actor_name.c_str(), owner->actor_id, best_id, tournament_scores);
    if (best_id == 22) {
        this->startGunAttack(target);
        return true;
    }
    if (best_id == 7 && target != nullptr && target->plane != nullptr) {
        this->startFollowLeader(target);
        return true;
    }
    this->applyManeuver(best_id, target, REACT_NONE);
    return true;
}

// MVRS_ID22_AimQuality_1125E, avec la visée de libRealSpace : écart = angle entre le nez et la direction de la cible
int PSBrain::gunAimQuality(SCMissionActors *target) {
    if (target == nullptr || target->plane == nullptr) {
        return 0;
    }
    Vector3D delta = target->plane->position - owner->plane->position;
    float distance = delta.Length();
    float error = owner->plane->forward.AngleBetween(delta);
    int quality = 0;
    if (distance < 2000.0f) {
        quality = 100 - (int)(error * error);
    } else if (distance < 4000.0f && error <= 1.0f) {
        quality = 15;
    }
    if (gun_bursts != 0) {
        quality += (error > 30.0f ? 20 : 10) - gun_bursts;
    }
    if (distance < 500.0f) {
        quality += (int)((500.0f - distance) / 20.0f);
    }
    quality += owner->profile->ai.atrb.TH;
    return (std::max)(quality, 0);
}

// MVRS_ID22_ScoreGunAttack_10C89 : attaque au canon contre un avion
int PSBrain::scoreGunAttack(SCMissionActors *target) {
    static const float floor_by_level[3] = {
        250.0f,
        200.0f,
        150.0f
    };
    if (target == nullptr || target->plane == nullptr || target->is_destroyed) {
        return 0;
    }
    float height = owner->plane->y - owner->plane->groundlevel;
    if (owner->pilot->NosePitch() < 0.0f && height < floor_by_level[this->pilotLevel()]) {
        return 0;
    }
    if ((this->loadedWeaponMask() & (1 << (PS_WEAPON_GUN - 1))) == 0) {
        return 0;
    }
    int quality = this->gunAimQuality(target) + 2 * owner->profile->ai.atrb.AR;
    return quality >= 100 ? 10 : quality / 10;
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
        int quality = this->gunAimQuality(target);
        if (quality > 75) {
            pilot->Fire(1 << (PS_WEAPON_GUN - 1), target);
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
