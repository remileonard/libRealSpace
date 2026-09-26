#include "precomp.h"
#include "SCAIBrain.h"

SCAIBrain::SCAIBrain(SCMissionActors *owner) {
    this->owner = owner;
}

void SCAIBrain::tick() {
    if (owner->pilot != nullptr) {
        owner->pilot->ClearGuidance();
    }
    if (!home_set) {
        home_position = owner->plane->position;
        home_set = true;
    }
    if (brain_orders_enabled) {
        this->tryActiveWingman();
    }
    debug_ticks++;
    int flying = owner->profile->ai.atrb.FL;
    int retarget_mask = flying < 4 ? 15 : flying < 11 ? 7 : 3;
    if (retarget_clock == 0.0f) {
        retarget_clock = 0.098f * (float) (owner->actor_id + 1);
    }
    retarget_clock += TICK_DURATION;
    int retarget_second = (int) floorf(retarget_clock);
    bool slow_window = false;
    if ((retarget_second & retarget_mask) == 0) {
        slow_window = !retarget_fired;
        retarget_fired = true;
    } else {
        retarget_fired = false;
    }
    if (flying >= 12 || air_target == nullptr || slow_window || just_hit) {
        this->acquireBestThreat(true);
    }
    if (mutiny) {
        air_target = this->playerActor();
    }
    morale = this->computeMorale();
    discipline_timer -= TICK_DURATION;
    if (discipline_timer <= 0.0f) {
        static const int adjust[4] = {7, 4, -3, -5};
        disciplined = owner->profile->ai.atrb.LY + adjust[morale - 2] > 7;
        discipline_timer = 3.0f;
    }
    morale_timer -= TICK_DURATION;
    weapon_mask = this->selectWeaponMask();
    if (weapon_mask != last_weapon_mask) {
        printf("AI %s#%d weapon_mask=0x%X\n", owner->actor_name.c_str(), owner->actor_id, weapon_mask);
        last_weapon_mask = weapon_mask;
    }
    fire_solution_quality = this->computeFireSolutionQuality();
    fire_request = false;
    if (weapon_mask != 0 && debug_ticks % 25 == 0) {
        printf("AI %s#%d weapon_mask=0x%X quality=%d\n", owner->actor_name.c_str(), owner->actor_id, weapon_mask, fire_solution_quality);
    }
    evasion_active = false;
    pursuit_active = false;
    ground_attack_active = false;
    ground_attack_seen = false;
    combat_step_called = false;
    // AI_TopLevelThink : menaces sautees en 0xA1/0xA2 ; reflexes et esquive seulement sans comportement en cours
    bool ground_order = owner->current_command == OP_SET_OBJ_TAKE_OFF || owner->current_command == OP_SET_OBJ_LAND;
    bool behavior_running = ground_op != GROUND_OP_NONE;
    if (owner->pilot != nullptr && !owner->plane->on_ground) {
        if (!ground_order) {
            this->incomingThreatWarning();
        }
        if (!behavior_running) {
            this->runReflexes();
        }
    }
    if (evasion_enabled && !behavior_running && (reaction_level == REACT_NONE || reaction_level == REACT_ENGAGED || reaction_level == REACT_MISSILE)) {
        this->reactToMissile();
    }
    if (evasion_active) {
        reaction_level = REACT_MISSILE;
    } else if (reaction_level == REACT_MISSILE) {
        reaction_level = REACT_NONE;
    }
    nav_requested = false;
    bool reacted = false;
    if (!evasion_active && reaction_level <= REACT_ENGAGED && owner->pilot != nullptr && !owner->plane->on_ground) {
        reacted = this->engageAttackerReaction();
    }
    if (!evasion_active && !reacted) {
        if (maneuver_id != 0 && reaction_level != REACT_NONE && maneuver_level != REACT_NONE) {
            this->tickManeuver();
        } else {
            this->runGoalSelectors();
        }
    }
    if (!combat_step_called && maneuver_id != 0 && maneuver_level == REACT_NONE) {
        this->endManeuver();
    }
    if (!ground_attack_seen) {
        this->resetGroundAttack();
    }
    if (!nav_requested) {
        this->stopNavigation();
    }
    just_hit = false;
}

bool SCAIBrain::engageAttackerReaction() {
    bool was_engaged = reaction_level == REACT_ENGAGED;
    if (reaction_level == REACT_ENGAGED) {
        reaction_level = REACT_NONE;
    }
    if (air_target == nullptr || air_target->plane == nullptr || reaction_level != REACT_NONE) {
        return false;
    }
    Vector3D to_target = air_target->plane->position - owner->plane->position;
    float nose_vs_target_nose = owner->plane->forward.AngleBetween(air_target->plane->forward);
    float nose_vs_direction = owner->plane->forward.AngleBetween(to_target);
    bool on_my_six = nose_vs_direction > 150.0f && nose_vs_target_nose < 30.0f && to_target.Length() < (float) owner->mission->intel.range_close;
    if (!just_hit && !on_my_six) {
        return false;
    }
    reaction_level = REACT_ENGAGED;
    if (!was_engaged) printf("AI %s#%d engage attacker=%s hit=%d six=%d command=%d leader_state=%d\n", owner->actor_name.c_str(), owner->actor_id, air_target->actor_name.c_str(), just_hit ? 1 : 0, on_my_six ? 1 : 0, owner->current_command, leader_state);
    if (owner->current_command == OP_SET_OBJ_FOLLOW_ALLY) {
        if (leader_state != 0) {
            return false;
        }
        bool leader_is_player = false;
        for (auto actor : owner->mission->actors) {
            if (actor->actor_id == owner->current_command_arg) {
                leader_is_player = actor->actor_name == "PLAYER";
                break;
            }
        }
        bool aims_at_me = air_target->brain != nullptr && air_target->brain->air_target == owner;
        if (!leader_is_player && !aims_at_me) {
            return false;
        }
        leader_state = 3;
        owner->target = air_target;
        if (leader_is_player) {
            owner->setMessage(0x12);
        }
        return false;
    }
    this->stopNavigation();
    return this->combatStep(false);
}

bool SCAIBrain::followAllyOrder(uint8_t arg) {
    if (leader_state == 2) {
        objective_locked = true;
        if (!this->navigateToPoint(brain_destination, 2000.0f)) {
            this->wander();
        }
        return true;
    }
    if (leader_state == 0) {
        objective_locked = false;
    }
    if (leader_state == 3) {
        SCMissionActors *engaged = owner->target;
        if (engaged == nullptr || engaged->is_destroyed || (engaged->plane != nullptr && engaged->plane->object->alive == 0)) {
            leader_state = 0;
            owner->target = nullptr;
            owner->current_target = SCMissionActors::NO_TARGET;
        }
    }
    owner->current_command_executed = owner->followAllyFormation(arg);
    if (leader_state == 3) {
        this->combatStep(false);
    } else if (leader_state == 1) {
        objective_locked = true;
        this->combatStep(!mutiny);
    }
    return true;
}

SCMissionActors *SCAIBrain::playerActor() {
    for (auto actor : owner->mission->actors) {
        if (actor->actor_name == "PLAYER") {
            return actor;
        }
    }
    return nullptr;
}

SCMissionActors *SCAIBrain::leaderActor() {
    if (owner->current_command != OP_SET_OBJ_FOLLOW_ALLY) {
        return nullptr;
    }
    for (auto actor : owner->mission->actors) {
        if (actor->actor_id == owner->current_command_arg) {
            return actor;
        }
    }
    return nullptr;
}

bool SCAIBrain::canHoldOrder() {
    uint16_t loaded = this->loadedWeaponMask();
    if (owner->current_command == OP_SET_OBJ_DESTROY_TARGET) {
        SCMissionActors *target = owner->target;
        if (target == nullptr) {
            return true;
        }
        return (loaded & (target->plane == nullptr ? 0xFC : 0xF03)) != 0;
    }
    if (owner->current_command == OP_SET_OBJ_DEFEND_TARGET) {
        return (loaded & 0xF03) != 0;
    }
    return true;
}

int SCAIBrain::computeMorale() {
    int score = 100;
    bool damaged = false;
    for (auto &system : owner->object->entity->sysm) {
        auto health_system = owner->plane->system_health.find(system.first);
        if (health_system == owner->plane->system_health.end()) {
            continue;
        }
        for (auto &sub_system : system.second) {
            auto health = health_system->second.find(sub_system.first);
            if (health != health_system->second.end() && health->second < sub_system.second) {
                damaged = true;
            }
        }
    }
    if (damaged) {
        score -= 100;
    }
    float capacity = (float) owner->object->entity->jdyn->fuel_capacity;
    if (capacity > 0.0f) {
        score += (int) (51.0f * (float) owner->plane->GetFuel() / capacity) - 50;
    }
    if (!this->canHoldOrder()) {
        score -= 50;
    }
    enemies_alive = 0;
    own_losses = 0;
    enemies_active = false;
    float radar_range = (float) owner->mission->intel.range_far;
    for (auto actor : owner->mission->actors) {
        if (actor->plane == nullptr) {
            continue;
        }
        if (actor->team_id == owner->team_id) {
            if (actor->is_destroyed) {
                own_losses++;
            }
        } else if (!actor->is_destroyed && actor->is_active) {
            enemies_alive++;
            if ((actor->plane->position - owner->plane->position).Length() < radar_range) {
                enemies_active = true;
            }
        }
    }
    if (enemies_alive > 0) {
        score += -8 * enemies_alive - 32 * own_losses;
    }
    if (reaction_level != REACT_NONE) {
        score -= 50;
    }
    int confidence = owner->profile->ai.atrb.CN;
    score += confidence < 3 ? 0 : confidence < 6 ? 15 : confidence < 12 ? 30 : confidence < 15 ? 50 : 75;
    if (enemies_alive > 0 && score >= 80) {
        score = 79;
    }
    if (confidence > 9 && score < 25) {
        score = 25;
    }
    if (confidence <= 0) {
        score = 0;
    }
    return score < 25 ? 5 : score < 50 ? 4 : score < 80 ? 3 : 2;
}

void SCAIBrain::leaveFight() {
    owner->setMessage(8);
    leader_state = 2;
    objective_locked = true;
    brain_destination = home_position;
    brain_destination.y += 1000.0f;
    this->stopNavigation();
    printf("AI %s#%d morale=%d leaves the fight\n", owner->actor_name.c_str(), owner->actor_id, morale);
}

bool SCAIBrain::moraleReaction() {
    if (morale_timer > 0.0f) {
        return false;
    }
    morale_timer = 5.0f;
    SCMissionActors *player = this->playerActor();
    bool player_side = player != nullptr && owner->team_id == player->team_id;
    SCMissionActors *leader = this->leaderActor();
    bool leader_is_player = leader != nullptr && leader == player;
    bool following = owner->current_command == OP_SET_OBJ_FOLLOW_ALLY;
    if (morale >= 4) {
        if (!player_side) {
            if (fleeing) {
                return false;
            }
            owner->setMessage(8);
            fleeing = true;
            objective_locked = true;
            brain_destination = home_position;
            brain_destination.y += 1000.0f;
            this->stopNavigation();
            printf("AI %s#%d morale=%d flees\n", owner->actor_name.c_str(), owner->actor_id, morale);
            return true;
        }
        if (leader_is_player && !disciplined && following) {
            if (last_attacker != nullptr && last_attacker == player && !enemies_active) {
                mutiny = true;
                air_target = player;
                leader_state = 1;
                objective_locked = true;
                owner->setMessage(0x20);
                printf("AI %s#%d morale=%d turns on the player\n", owner->actor_name.c_str(), owner->actor_id, morale);
                this->combatStep(false);
                return true;
            }
            if (leader_state != 2) {
                this->leaveFight();
                return true;
            }
            return false;
        }
        if (leader_is_player && (reaction_level == REACT_ENGAGED || reaction_level == REACT_MISSILE)) {
            owner->setMessage(6);
        }
        return false;
    }
    if (player_side && leader_is_player && !disciplined && leader_state != 1 && leader_state != 2 && following && enemies_active) {
        owner->setMessage(0x12);
        leader_state = 1;
        objective_locked = true;
        printf("AI %s#%d morale=%d engages on its own\n", owner->actor_name.c_str(), owner->actor_id, morale);
        return true;
    }
    return false;
}

bool SCAIBrain::combatStep(bool ground_allowed) {
    combat_step_called = true;
    if (air_target == nullptr) {
        if (maneuver_id != 0 && maneuver_level == REACT_NONE) {
            this->endManeuver();
        }
        if (ground_allowed && ground_target != nullptr && ground_attack_enabled) {
            this->updateGroundAttack(ground_target);
            return ground_attack_active;
        }
        return false;
    }
    owner->current_target = air_target->actor_id;
    air_target->attacker = owner;
    if (reaction_level <= REACT_ENGAGED) {
        if (fire_control_enabled) {
            this->updateFireControl();
        }
        if (fire_request || fire_solution_quality > 0) {
            if (maneuver_id != 0 && maneuver_level == REACT_NONE) {
                this->endManeuver();
            }
            if (pursuit_enabled) {
                this->updatePursuit();
            }
            return true;
        }
    }
    if (maneuver_id != 0) {
        this->tickManeuver();
        return true;
    }
    return this->runTournament();
}

bool SCAIBrain::navigateToPoint(Vector3D point, float radius) {
    static const float CRUISE_SPEED = 250.0f;
    Vector3D delta = point - owner->plane->position;
    if (delta.Length() <= radius) {
        this->stopNavigation();
        return false;
    }
    Vector3D velocity = delta;
    velocity.Normalize();
    velocity = velocity * CRUISE_SPEED;
    if (owner->pilot->autopilotActive()) {
        owner->pilot->setAutopilotTarget(point, velocity);
    } else {
        float nose_pitch = radToDegree(asinf(std::max(-1.0f, std::min(1.0f, owner->plane->forward.y))));
        if (std::fabs(nose_pitch) < 15.0f) {
            owner->pilot->engageAutopilot(point, velocity);
        } else {
            owner->pilot->SetPitchCommand(0.0f, 5.0f);
        }
    }
    nav_active = true;
    nav_requested = true;
    return true;
}

void SCAIBrain::navigateToPilotWaypoint() {
    if (!brain_orders_enabled || owner->plane->on_ground || owner->pilot->land || !owner->pilot->has_waypoint) {
        return;
    }
    Vector3D point = owner->pilot->target_waypoint;
    point.y = (float) owner->pilot->target_climb;
    this->navigateToPoint(point, 0.0f);
}

void SCAIBrain::stopNavigation() {
    if (nav_active) {
        owner->pilot->disengageAutopilot();
        nav_active = false;
    }
}

void SCAIBrain::wander() {
    Vector3D own_position = owner->plane->position;
    Vector3D delta = wander_point - own_position;
    delta.y = 0.0f;
    if (!wander_point_set || delta.Length() < 2000.0f) {
        float angle = degreeToRad((float) (std::rand() % 360));
        wander_point = own_position + Vector3D(sinf(angle) * 30000.0f, 0.0f, cosf(angle) * 30000.0f);
        wander_point_set = true;
    }
    Vector3D direction = wander_point - own_position;
    direction.y = 0.0f;
    owner->pilot->SetGuidanceDirection(direction);
}

bool SCAIBrain::defendTargetOrder(uint8_t arg) {
    owner->current_command_executed = false;
    SCMissionActors *defended = nullptr;
    for (auto actor : owner->mission->actors) {
        if (actor->actor_id == arg) {
            defended = actor;
            break;
        }
    }
    if (defended == nullptr || defended->is_destroyed || (defended->plane != nullptr && defended->plane->object->alive == 0)) {
        this->stopNavigation();
        owner->current_command_executed = true;
        return false;
    }
    owner->current_objective = OP_SET_OBJ_DEFEND_TARGET;
    Vector3D point = defended->plane != nullptr ? defended->plane->position : defended->object->position;
    int state = 2;
    bool acted = true;
    if (this->navigateToPoint(point, 30000.0f)) {
        state = 0;
    } else if (this->combatStep(false)) {
        state = 1;
    } else {
        this->wander();
    }
    if (state != 1) {
        owner->current_target = SCMissionActors::NO_TARGET;
    }
    if (state != defend_state) {
        printf("AI %s#%d defend target=%s state=%s d=%.0f\n", owner->actor_name.c_str(), owner->actor_id, defended->actor_name.c_str(), state == 0 ? "navigate" : state == 1 ? "combat" : "wander", (point - owner->plane->position).Length());
        defend_state = state;
    }
    return acted;
}

bool SCAIBrain::destroyTargetOrder(uint8_t arg) {
    owner->current_command_executed = false;
    if (owner->is_destroyed || owner->plane == nullptr || owner->plane->object->alive == 0) {
        owner->current_command_executed = true;
        return false;
    }
    bool is_new_target = owner->target == nullptr || owner->target->actor_id != arg;
    if (is_new_target) {
        owner->target = nullptr;
        for (auto actor : owner->mission->actors) {
            if (actor->actor_id == arg) {
                owner->target = actor;
                actor->attacker = owner;
                if (std::rand() % 16 >= owner->profile->ai.atrb.VB) {
                    owner->setMessage(actor->plane == nullptr ? 11 : 12);
                }
                break;
            }
        }
    }
    SCMissionActors *target = owner->target;
    if (target == nullptr) {
        return false;
    }
    if (target->is_destroyed || (target->plane != nullptr && target->plane->object->alive == 0)) {
        owner->current_target = SCMissionActors::NO_TARGET;
        target->attacker = nullptr;
        owner->target = nullptr;
        if (!is_new_target && std::rand() % 16 >= owner->profile->ai.atrb.VB) {
            owner->setMessage(13);
        }
        owner->current_command_executed = true;
        return false;
    }
    owner->current_objective = OP_SET_OBJ_DESTROY_TARGET;
    owner->current_target = arg;
    if (target->plane == nullptr) {
        bool ground = target->object != nullptr && target->object->entity != nullptr && target->object->entity->target_type == 2;
        if (ground_attack_enabled && ground) {
            this->updateGroundAttack(target);
            if (ground_phase != 6) {
                return ground_attack_active;
            }
        }
        owner->current_command_executed = owner->destroyTarget(arg);
        return false;
    }
    return this->combatStep(false);
}

SCMissionActors *SCAIBrain::acquireBestThreat(bool allow_new_target) {
    int air_candidates = 0;
    int ground_candidates = 0;
    int missile_candidates = 0;
    std::string air_names;
    int best_score = -5000;
    SCMissionActors *best_candidate = nullptr;
    SCSimulatedObject *best_missile = nullptr;
    if (threat_state == 2) {
        threat_state = 0;
    }
    if (air_target != nullptr && air_target->is_destroyed) {
        air_target = nullptr;
    }
    if (ground_target != nullptr && ground_target->is_destroyed) {
        ground_target = nullptr;
    }
    uint16_t weapon_mask = this->loadedWeaponMask();
    int weight_a = owner->profile->ai.atrb.AR - owner->profile->ai.atrb.FL + 16;
    int weight_b = owner->profile->ai.atrb.FL - owner->profile->ai.atrb.AR + 16;
    for (auto actor : owner->mission->actors) {
        if (actor == owner || actor->is_destroyed) {
            continue;
        }
        if (!actor->is_active && actor != owner->mission->player) {
            continue;
        }
        if (actor->team_id == owner->team_id) {
            continue;
        }
        if (actor->object == nullptr || actor->object->entity == nullptr) {
            continue;
        }
        Vector3D candidate_position = actor->object->position;
        if (actor->plane != nullptr) {
            candidate_position = actor->plane->position;
        }
        Vector3D delta = candidate_position - owner->plane->position;
        char geometry[192];
        if (actor->object->entity->entity_type == EntityType::jet) {
            air_candidates++;
            if (actor->plane != nullptr) {
                ThreatScore score = this->scoreAirCandidate(actor, delta);
                bool accepted = this->skillCheck(owner->profile->ai.atrb.FL, score.aptitude);
                int total = weight_a * score.a + weight_b * score.b;
                if (accepted && total > best_score) {
                    best_score = total;
                    best_candidate = actor;
                    best_missile = nullptr;
                }
                snprintf(geometry, sizeof(geometry), " d=%.0f ahead=%.0f aims=%.0f heading=%.0f A=%d B=%d si=%d score=%d %s", delta.Length(), owner->plane->forward.AngleBetween(delta), actor->plane->forward.AngleBetween(-delta), owner->plane->forward.AngleBetween(actor->plane->forward), score.a, score.b, score.aptitude, total, accepted ? "ok" : "skip");
            } else {
                snprintf(geometry, sizeof(geometry), " d=%.0f ahead=%.0f", delta.Length(), owner->plane->forward.AngleBetween(delta));
            }
            air_names += " " + actor->actor_name + "#" + std::to_string(actor->actor_id) + "(" + std::to_string(actor->team_id) + geometry + ")";
        } else if (allow_new_target && actor->object->entity->target_type == 2 && (weapon_mask & 0x83C) != 0) {
            ground_candidates++;
            ThreatScore score = this->scoreGroundCandidate(actor, delta);
            bool accepted = this->skillCheck(owner->profile->ai.atrb.FL, score.aptitude);
            int total = weight_a * score.a + weight_b * score.b;
            if (accepted && total > best_score) {
                best_score = total;
                best_candidate = actor;
                best_missile = nullptr;
            }
        }
    }
    SCSimulatedObject *missile = owner->weapon_shooted_at_me;
    if (missile != nullptr && missile->alive && missile->target == owner && missile->obj->entity_type == EntityType::missiles && missile->obj->wdat->target_domain == 1) {
        missile_candidates++;
        ThreatScore score = this->scoreMissile(missile);
        bool accepted = this->skillCheck(owner->profile->ai.atrb.FL, score.aptitude);
        int total = weight_a * score.a + weight_b * score.b;
        if (accepted) {
            missile_threat = missile;
            if (total > best_score) {
                best_score = total;
                best_candidate = nullptr;
                best_missile = missile;
            }
        }
    }
    if (best_missile != nullptr) {
        missile_threat = best_missile;
        air_target = nullptr;
        ground_target = nullptr;
        threat_state = 2;
    } else if (best_candidate != nullptr) {
        if (best_candidate->object->entity->target_type == 2) {
            ground_target = best_candidate;
            air_target = nullptr;
        } else {
            air_target = best_candidate;
            ground_target = nullptr;
        }
    }
    if (debug_ticks % 100 == 0 || air_candidates != last_air_candidates || ground_candidates != last_ground_candidates || missile_candidates != last_missile_candidates) {
        const char *target_name = "none";
        if (air_target != nullptr) {
            target_name = air_target->actor_name.c_str();
        } else if (ground_target != nullptr) {
            target_name = ground_target->actor_name.c_str();
        }
        printf("AI %s#%d(team %d) command=%d current_target=%d candidates: air=%d ground=%d missile=%d target=%s threat_state=%d\n  air:%s\n", owner->actor_name.c_str(), owner->actor_id, owner->team_id, (int) owner->current_command, owner->current_target, air_candidates, ground_candidates, missile_candidates, target_name, threat_state, air_names.c_str());
        last_air_candidates = air_candidates;
        last_ground_candidates = ground_candidates;
        last_missile_candidates = missile_candidates;
    }
    return best_candidate;
}

bool SCAIBrain::skillCheck(uint8_t stat, int modifier) {
    int roll = (std::rand() & 15) + 1;
    return roll <= stat + modifier;
}

uint16_t SCAIBrain::loadedWeaponMask() {
    uint16_t mask = 0;
    for (auto weap : owner->plane->weaps_load) {
        if (weap == nullptr || weap->nb_weap <= 0) {
            continue;
        }
        int weapon_id = weap->objct->wdat->weapon_id;
        if (weapon_id >= 1) {
            mask |= (1 << (weapon_id - 1));
        }
    }
    return mask;
}

ThreatScore SCAIBrain::scoreAirCandidate(SCMissionActors *candidate, Vector3D delta) {
    ThreatScore score;
    float distance = delta.Length();
    uint16_t mask = this->loadedWeaponMask();
    bool has_short_missile = (mask & 0x1) != 0;
    bool has_ir_missile = (mask & 0x3) != 0;
    bool has_long_missile = (mask & 0x700) != 0;
    int ahead = (int) owner->plane->forward.AngleBetween(delta);
    int aims = (int) candidate->plane->forward.AngleBetween(-delta);
    int heading = (int) owner->plane->forward.AngleBetween(candidate->plane->forward);

    if (ahead > 135) {
        if (!fire_control_active) {
            score.aptitude -= 4;
        }
        score.a -= 4;
        score.b += (aims < 45) ? 8 : 2;
    } else if (ahead > 90) {
        if (!fire_control_active) {
            score.aptitude -= 1;
        }
        score.a -= 2;
        if (aims < 60) {
            score.b += 5;
        }
    } else if (ahead > 30) {
        if (aims < 60) {
            score.b += 4;
        }
        score.aptitude += 2;
        score.a += 2;
    } else {
        if (aims < 60) {
            score.b += 4;
        }
        score.aptitude += 4;
        score.a += 4;
        if (heading < 30) {
            score.a += 4;
        }
    }

    if (heading > 80 && heading < 100) {
        score.a -= 5;
    } else if (heading > 60 && heading < 120) {
        score.a -= 3;
    }

    RSIntel &intel = owner->mission->intel;
    if (distance > intel.range_far) {
        score.aptitude -= 3;
        score.a -= 5;
    } else if (distance > intel.range_medium && has_long_missile) {
        score.aptitude -= 1;
        score.a += 3;
    } else if (has_ir_missile) {
        if (distance > intel.range_long) {
            score.a -= 2;
            score.aptitude -= 1;
        } else if (distance < intel.range_close) {
            score.a -= 1;
            score.b += 4;
        }
        if (!has_short_missile) {
            if (heading > 150) {
                score.a -= 3;
            } else if (heading > 60) {
                score.a -= 1;
            }
        }
    }

    if (candidate == this->air_target) {
        score.a += 3;
        score.aptitude += 5;
    }
    if (candidate == owner->target) {
        score.a += 2;
        score.aptitude += 4;
    }
    if (candidate->target == owner) {
        score.aptitude += 2;
        score.b += 1;
    }
    if (fire_control_active) {
        score.aptitude += 4;
    }
    return score;
}
/**
 * SCAIBrain::executeGoalAction
 *
 * Implementation du selecteur GOAL_EXECUTE_ACTION (Goal_ExecuteAction,
 * cf. analysis/AI_SYSTEM.md §4.3) : retraduit l'etat persistant
 * current_command (pose par le script PROG dans onMissionUpdate, ou par un
 * ordre radio via override_progs) en appel de la methode de comportement
 * correspondante. Le script ne fait que POSER current_command ; c'est ici,
 * uniquement, qu'il est EXECUTE — voir onAIRefresh()/runGoalSelectors().
 * Extrait tel quel de l'ancien onMissionUpdate, seule sa cadence d'appel
 * change (25Hz via AIRefreshEvent au lieu de chaque frame).
 *
 * @return true si un objectif de navigation pure etait actif et a ete
 * execute ce tick (le selecteur "prend la main" — cf. Goal_ExecuteAction,
 * AI_SYSTEM.md §4.3) ; false si current_command est vide (OP_NOOP) OU si
 * c'est un objectif de combat (DESTROY_TARGET/DEFEND_TARGET/DEFEND_AREA).
 *
 * Le cas combat est deliberement traite a part : ces objectifs peuvent
 * rester actifs tres longtemps (tant que la cible n'est pas detruite), et
 * comme 2 precede 4 dans tous les fichiers PROF echantillons, un simple
 * "true tant que current_command != OP_NOOP" empecherait le tournoi MVRS
 * (selecteur 4) de jamais tourner en combat — exactement le moment ou il
 * doit prendre la main pour la maneuvre. On execute quand meme l'objectif
 * ici (pour garder l'avion en route vers/apres la cible), mais on rend la
 * main a runGoalSelectors() pour que 4 (une fois cable) ait sa chance dans
 * le meme passage. A revoir/confirmer une fois l'articulation reelle
 * Goal_ExecuteAction/AI_BehaviorStateMachine tranchee en ASM (AI_SYSTEM.md
 * §4.4 suggere que certains selecteurs delegue a MVRS en interne plutot
 * qu'une simple exclusion mutuelle au niveau de la boucle GOAL).
 */
bool SCAIBrain::executeGoalAction() {
    owner->protectSelf();
    if (brain_orders_enabled && fleeing) {
        if (!this->navigateToPoint(brain_destination, 2000.0f)) {
            this->wander();
        }
        return true;
    }
    switch (owner->current_command) {
        case OP_SET_WAIT_FOR_SECONDS:
            owner->current_command_executed = owner->wait(owner->current_command_arg);
        break;
        case OP_SET_OBJ_TAKE_OFF:
            if (brain_orders_enabled) {
                owner->current_command_executed = this->takeoffOrder();
                break;
            }
            owner->current_command_executed = owner->takeOff(owner->current_command_arg);
        break;
        case OP_SET_OBJ_LAND:
            if (brain_orders_enabled) {
                owner->current_command_executed = this->landingOrder(owner->current_command_arg, owner->current_command_arg2);
                break;
            }
            owner->current_command_executed = owner->land(owner->current_command_arg);
        break;
        case OP_SET_OBJ_FLY_TO_WP:
            owner->current_command_executed = owner->flyToWaypoint(owner->current_command_arg);
            this->navigateToPilotWaypoint();
        break;
        case OP_SET_OBJ_FLY_TO_AREA:
            owner->current_command_executed = owner->flyToArea(owner->current_command_arg);
            this->navigateToPilotWaypoint();
        break;
        case OP_SET_OBJ_FOLLOW_ALLY:
            if (brain_orders_enabled) {
                return this->followAllyOrder(owner->current_command_arg);
            }
            owner->current_command_executed = owner->followAlly(owner->current_command_arg);
        break;
        case OP_SET_OBJ_DESTROY_TARGET:
            if (brain_orders_enabled) {
                return this->destroyTargetOrder(owner->current_command_arg);
            }
            owner->current_command_executed = owner->destroyTarget(owner->current_command_arg);
            return false;
        case OP_SET_OBJ_DEFEND_TARGET:
            if (brain_orders_enabled) {
                return this->defendTargetOrder(owner->current_command_arg);
            }
            owner->current_command_executed = owner->defendTarget(owner->current_command_arg);
            return false;
        case OP_SET_OBJ_DEFEND_AREA:
            owner->current_command_executed = owner->defendArea(owner->current_command_arg);
            return false;
        default:
            return false;
    }
    return true;
}
/**
 * SCAIBrain::tryWanderRandom
 *
 * Implementation du selecteur GOAL_WANDER_RANDOM (Goal_WanderRandom,
 * cf. analysis/AI_SYSTEM.md §4, analysis/AI_TACTICAL_GLOSSARY.md §2) :
 * fait naviguer l'acteur vers un SPOT de la mission tire au hasard.
 * Autonome comme tout selecteur GOAL — verifie lui-meme si l'objectif
 * courant est atteint (via le booleen de retour de flyToWaypoint) avant
 * d'en tirer un nouveau, sans dependre du sort de GOAL_EXECUTE_ACTION.
 *
 * @return true si le selecteur a pris la main ce tick, false s'il ne
 * s'applique pas (une cible est deja engagee).
 */
bool SCAIBrain::tryWanderRandom() {
    if (owner->current_target != SCMissionActors::NO_TARGET) {
        return false;
    }
    size_t spot_count = owner->mission->mission->mission_data.spots.size();
    if (spot_count == 0) {
        return false;
    }
    owner->current_command = prog_op::OP_SET_OBJ_FLY_TO_WP;
    bool arrived = owner->flyToWaypoint(owner->current_command_arg);
    this->navigateToPilotWaypoint();
    if (arrived) {
        owner->current_command_arg = (uint8_t)(std::rand() % spot_count);
    }
    owner->current_command_executed = arrived;
    return true;
}
/**
 * SCAIBrain::tryActiveWingman
 *
 * Implementation du selecteur GOAL_ACTIVE_WINGMAN (Goal_ActiveWingmanEngagement,
 * cf. analysis/AI_SYSTEM.md §4.4) : execute le script d'ordre radio accepte
 * (override_progs) s'il y en a un en cours. Ne "gagne" jamais le tick — le
 * script ne fait que POSER current_command (setObjective, cf. SCProg.cpp),
 * c'est toujours GOAL_EXECUTE_ACTION (executeGoalAction) qui l'execute
 * reellement ; runGoalSelectors() doit donc toujours continuer vers le
 * selecteur suivant du fichier apres cet appel.
 */
void SCAIBrain::tryActiveWingman() {
    if (owner->override_progs.empty() || owner->is_destroyed) {
        return;
    }
    SCProg *p = new SCProg(owner, owner->override_progs, owner->mission, 255);
    p->execute();
    delete p;
    if (owner->current_command_executed) {
        owner->override_progs.clear();
        owner->override_progs.shrink_to_fit();
    }
}
/**
 * SCAIBrain::runGoalSelectors
 *
 * Parcourt profile->ai.goal dans l'ordre du fichier et s'arrete au premier
 * selecteur qui "prend la main" ce tick (cf. AI_TopLevelThink,
 * analysis/AI_SYSTEM.md §4.2). Chaque selecteur est autonome : il gere son
 * propre etat et signale lui-meme s'il a agi ou non ce tick, ce qui permet
 * de passer au suivant du fichier quand il n'a rien a faire (ex. GOAL_EXECUTE_ACTION
 * sans commande active laisse la main a GOAL_WANDER_RANDOM s'il suit dans
 * le fichier). Sélecteurs cables : 2 (GOAL_EXECUTE_ACTION, executeGoalAction),
 * 3 (GOAL_WANDER_RANDOM, tryWanderRandom) et 5 (GOAL_ACTIVE_WINGMAN,
 * tryActiveWingman — ne gagne jamais le tick, voir sa doc). 4 reste un point
 * d'extension explicite pour une prochaine session (tournoi MVRS — voir
 * analysis/AI_IMPLEMENTATION_GUIDE.md §3) et ne "prend" jamais la main pour
 * l'instant.
 *
 * @return true si un selecteur a agi ce tick, false sinon.
 */
bool SCAIBrain::runGoalSelectors() {
    if (owner->plane->on_ground) {
        return this->executeGoalAction();
    }
    for (uint8_t rawSelector : owner->profile->ai.goal) {
        switch ((GoalSelector) rawSelector) {
            case GOAL_EMPTY:
                continue;
            case GOAL_EXECUTE_ACTION:
                if (this->executeGoalAction()) {
                    return true;
                }
                continue;
            case GOAL_WANDER_RANDOM:
                if (this->tryWanderRandom()) {
                    return true;
                }
                continue;
            case GOAL_BEHAVIOR_STATE_MACHINE:
                if (brain_orders_enabled && this->combatStep(false)) {
                    return true;
                }
                continue;
            case GOAL_ACTIVE_WINGMAN:
                if (brain_orders_enabled) {
                    if (this->moraleReaction()) {
                        return true;
                    }
                    continue;
                }
                // Ne gagne jamais le tick : pose seulement current_command
                // depuis l'ordre radio en cours, si il y en a un — c'est
                // GOAL_EXECUTE_ACTION, plus loin dans le fichier, qui
                // l'execute reellement (voir tryActiveWingman()).
                this->tryActiveWingman();
                continue;
            default:
                continue;
        }
    }
    return false;
}

ThreatScore SCAIBrain::scoreGroundCandidate(SCMissionActors *candidate, Vector3D delta) {
    ThreatScore score;
    float distance = delta.Length();
    int ahead = (int) owner->plane->forward.AngleBetween(delta);
    RSIntel &intel = owner->mission->intel;
    RSEntity *entity = candidate->object->entity;

    if (candidate == owner->target) {
        score.aptitude += 3;
        score.a += 6;
    }
    if (entity->entity_type == EntityType::swpn && entity->swpn_data != nullptr && entity->swpn_data->weapons_round > 0) {
        float range = (float) entity->swpn_data->effective_range;
        if (range > 0.0f && distance <= range) {
            score.b = (int) (10.0f * (1.0f - distance / range)) + 5;
        }
    }
    if (distance > intel.range_ground) {
        score.aptitude -= 4;
    } else if (ahead < 45) {
        score.aptitude += 5;
        score.a += 6;
    } else if (ahead < 90) {
        score.aptitude += 3;
        score.a += 3;
    }
    if (fire_control_active) {
        score.aptitude += 4;
    }
    return score;
}

ThreatScore SCAIBrain::scoreMissile(SCSimulatedObject *missile) {
    ThreatScore score;
    Vector3D missile_position = {missile->x, missile->y, missile->z};
    Vector3D missile_forward = {missile->vx, missile->vy, missile->vz};
    Vector3D delta = missile_position - owner->plane->position;
    float distance = delta.Length();
    int ahead = (int) owner->plane->forward.AngleBetween(delta);
    int aims = (int) missile_forward.AngleBetween(-delta);
    RSIntel &intel = owner->mission->intel;
    int trigger_happy = owner->profile->ai.atrb.FL;

    if (aims <= 90 && distance < intel.range_far) {
        bool long_range = (missile->obj->wdat->weapon_id >= 1) && ((1 << (missile->obj->wdat->weapon_id - 1)) & 0x700) != 0;
        if (long_range || distance <= intel.range_long) {
            float range = long_range ? (float) intel.range_far : (float) intel.range_long;
            score.b += (int) (16.0f * (1.0f - distance / range)) + 24;
            score.aptitude += (trigger_happy * trigger_happy) / 16 - 8;
            if (ahead > 135 && !fire_control_active) {
                score.aptitude -= 4;
            }
        }
    }
    if (fire_control_active) {
        score.aptitude += 4;
    }
    return score;
}

uint16_t SCAIBrain::selectWeaponMask() {
    if (air_target == nullptr || air_target->plane == nullptr) {
        return 0;
    }
    Vector3D delta = air_target->plane->position - owner->plane->position;
    float distance = delta.Length();
    int ahead = (int) owner->plane->forward.AngleBetween(delta);
    int tail_aspect = (int) air_target->plane->forward.AngleBetween(delta);
    bool crossing = tail_aspect > 40 && tail_aspect < 140;
    RSIntel &intel = owner->mission->intel;
    uint16_t loaded = this->loadedWeaponMask();
    bool has_gun = (loaded & 0x800) != 0;
    bool has_short_missile = false;
    bool has_ir_missile = false;
    bool has_long_missile = false;
    if (intel.status_flag) {
        has_short_missile = (loaded & 0x1) != 0;
        has_ir_missile = (loaded & 0x3) != 0;
        has_long_missile = (loaded & 0x700) != 0;
    }

    if (ahead >= 90) {
        return 0;
    }
    if (distance < intel.range_gun && has_gun) {
        return 0x800;
    }
    if (distance > intel.range_medium && has_long_missile) {
        return 0x700;
    }
    if (has_ir_missile && distance < intel.range_long) {
        return (crossing && has_short_missile) ? 0x1 : 0x3;
    }
    return 0;
}

int SCAIBrain::computeFireSolutionQuality() {
    if (air_target == nullptr || air_target->plane == nullptr) {
        return 0;
    }
    Vector3D delta = air_target->plane->position - owner->plane->position;
    float distance = delta.Length();
    int ahead = (int) owner->plane->forward.AngleBetween(delta);
    int tail_aspect = (int) air_target->plane->forward.AngleBetween(delta);
    bool crossing = tail_aspect > 50 && tail_aspect < 130;
    RSIntel &intel = owner->mission->intel;
    if (ahead >= 90) {
        return 0;
    }
    int quality = 10 - (ahead * 10) / 35;
    if (weapon_mask == 0x800) {
        if (distance >= intel.range_gun) {
            return 0;
        }
        float error = owner->plane->forward.AngleBetween(delta);
        float target_speed_per_tick = std::fabs(air_target->plane->forwardSpeedPerTick()) * air_target->plane->tps * TICK_DURATION;
        float tolerance = radToDegree(atanf(target_speed_per_tick / distance));
        if (tolerance <= 0.0f) {
            return 0;
        }
        float difference = error - tolerance;
        if (difference < 0.0f) {
            quality = (int) floorf(8.0f - 2.0f * difference / tolerance);
        } else {
            quality = (int) floorf(8.0f - 4.0f * difference / tolerance);
        }
        if (crossing) {
            quality -= 4;
        }
    } else {
        if (crossing) {
            quality -= 4;
        }
        switch (weapon_mask) {
            case 0x1:
            case 0x2:
            case 0x3:
                if (distance >= intel.range_long) {
                    quality -= 10;
                } else if (distance < intel.range_close) {
                    quality -= crossing ? 10 : 3;
                }
                break;
            case 0x100:
            case 0x700:
                if (distance >= intel.range_far) {
                    quality -= 10;
                } else if (distance < intel.range_medium) {
                    quality -= crossing ? 10 : 3;
                }
                break;
            default:
                quality = 0;
                break;
        }
    }
    if (quality > 10) {
        return 10;
    }
    if (quality < 0) {
        return 0;
    }
    return quality;
}

bool SCAIBrain::reactionThreshold(int quality) {
    return 2 * quality >= owner->profile->ai.atrb.AA;
}

RSEntity *SCAIBrain::loadedMissile(uint16_t mask) {
    for (auto weap : owner->plane->weaps_load) {
        if (weap == nullptr || weap->nb_weap <= 0) {
            continue;
        }
        int weapon_id = weap->objct->wdat->weapon_id;
        if (weapon_id >= 1 && (mask & (1 << (weapon_id - 1))) != 0) {
            return weap->objct;
        }
    }
    return nullptr;
}

int SCAIBrain::seekerSignature(RSEntity *weapon, SCMissionActors *candidate, Vector3D reference_velocity) {
    RSEntity *entity = candidate->object->entity;
    int aspec = weapon->wdat->weapon_aspec;
    if (aspec == 3 || aspec == 4) {
        if (entity->radar_signature == nullptr) {
            return 0;
        }
        return aspec == 3 ? entity->radar_signature->unknown3 : entity->radar_signature->unknown2;
    }
    if (candidate->plane == nullptr) {
        return entity->radar_signature != nullptr ? entity->radar_signature->unknown1 : 0;
    }
    Vector3D target_velocity = this->actorVelocity(candidate);
    bool behind = reference_velocity.x * target_velocity.x + reference_velocity.y * target_velocity.y + reference_velocity.z * target_velocity.z > 0.0f;
    float factor = behind ? 100.0f : 50.0f;
    float signature = 10.0f + target_velocity.Length() / 602.0f * factor;
    if (candidate->plane->GetThrottle() > 50) {
        signature += factor;
    }
    return ((int) signature) & 0xFF;
}

bool SCAIBrain::seekerSees(RSEntity *weapon, SCMissionActors *candidate) {
    Vector3D position = candidate->plane != nullptr ? candidate->plane->position : candidate->object->position;
    Vector3D delta = position - owner->plane->position;
    float distance = delta.Length();
    if (distance <= 0.0f || distance > (float) weapon->wdat->target_range) {
        return false;
    }
    return owner->plane->forward.AngleBetween(delta) <= (float) weapon->wdat->tracking_cone;
}

SCMissionActors *SCAIBrain::seekerSelect(RSEntity *weapon, SCMissionActors *desired) {
    int aspec = weapon->wdat->weapon_aspec;
    Vector3D reference_velocity = this->actorVelocity(owner);
    if (aspec == 5 || aspec == 6) {
        return (desired != nullptr && this->seekerSees(weapon, desired)) ? desired : nullptr;
    }
    SCMissionActors *best = nullptr;
    int best_signature = 0;
    for (auto actor : owner->mission->actors) {
        if (actor->is_destroyed || actor->object == nullptr || actor->object->entity == nullptr) {
            continue;
        }
        if (actor->plane != nullptr && !actor->plane->object->alive) {
            continue;
        }
        uint8_t type = actor->object->entity->target_type;
        if (type != 1 && type != 4) {
            continue;
        }
        int signature = this->seekerSignature(weapon, actor, reference_velocity);
        if (signature > best_signature && this->seekerSees(weapon, actor)) {
            best_signature = signature;
            best = actor;
        }
    }
    if (best == nullptr) {
        return nullptr;
    }
    SCMissionActors *result = desired;
    if (best != desired) {
        SCMissionActors *player = this->playerActor();
        int weight = best == player ? 5 : 3;
        int signature = this->seekerSignature(weapon, best, reference_velocity);
        bool steal = false;
        if (aspec == 1) {
            steal = (signature > 210 && (std::rand() % 10) < weight) || signature == 210;
        } else if (aspec == 2 || aspec == 4) {
            steal = signature >= 245 && (std::rand() % 10) < weight;
        }
        if (steal) {
            return best;
        }
    }
    if (result == nullptr) {
        return nullptr;
    }
    if (aspec == 1) {
        Vector3D target_velocity = this->actorVelocity(result);
        if (reference_velocity.x * target_velocity.x + reference_velocity.y * target_velocity.y + reference_velocity.z * target_velocity.z < 0.0f) {
            return nullptr;
        }
    }
    return result;
}

bool SCAIBrain::testMissileLock(RSEntity *missile) {
    SCMissionActors *locked = this->seekerSelect(missile, air_target);
    if (locked != air_target && debug_ticks % 25 == 0) {
        printf("AI %s#%d seeker aspec=%d no lock on %s (seeker holds %s)\n", owner->actor_name.c_str(), owner->actor_id, missile->wdat->weapon_aspec, air_target->actor_name.c_str(), locked != nullptr ? locked->actor_name.c_str() : "nothing");
    }
    return locked == air_target;
}

void SCAIBrain::updateFireControl() {
    fire_request = false;
    if (air_target == nullptr || threat_state > 1 || evasion_hold > 0) {
        burst_remaining = 0;
        burst_weapon = 0;
        lock_target = nullptr;
        return;
    }
    uint16_t requested_weapon = 0;
    if (burst_remaining > 0 && burst_weapon == 0x800) {
        burst_remaining--;
        fire_request = true;
        requested_weapon = 0x800;
    } else if (weapon_mask == 0x800) {
        if (fire_solution_quality >= 2 && this->reactionThreshold(fire_solution_quality)) {
            burst_remaining = ((std::rand() & 3) + 4) * fire_solution_quality / 10;
            if (burst_remaining > 1) {
                burst_weapon = 0x800;
                fire_request = true;
                requested_weapon = 0x800;
            }
        }
    } else if (weapon_mask != 0) {
        RSEntity *missile = this->loadedMissile(weapon_mask);
        if (missile != nullptr) {
            if (lock_target != air_target) {
                lock_target = air_target;
                printf("AI %s#%d seeker tracking target=%s weapon_id=%d aspec=%d cone=%d range=%u\n", owner->actor_name.c_str(), owner->actor_id, air_target->actor_name.c_str(), missile->wdat->weapon_id, missile->wdat->weapon_aspec, missile->wdat->tracking_cone, missile->wdat->target_range);
            } else if (this->testMissileLock(missile)) {
                fire_request = true;
                requested_weapon = weapon_mask;
                lock_target = nullptr;
            }
        }
    }
    if (fire_request) {
        owner->pilot->Fire(requested_weapon, air_target);
        printf("AI %s#%d fire weapon=0x%X quality=%d burst=%d\n", owner->actor_name.c_str(), owner->actor_id, requested_weapon, fire_solution_quality, burst_remaining);
    }
}

void SCAIBrain::resetGroundAttack() {
    if (ground_phase == 3) {
        owner->pilot->disengageAutopilot();
    }
    ground_phase = 0;
    ground_weapon = nullptr;
    ground_released = nullptr;
    ground_attack_target = nullptr;
    ground_attack_active = false;
}

float SCAIBrain::headingDelta(Vector3D direction) {
    float direction_heading = radToDegree(atan2f(direction.z, direction.x));
    float nose_heading = radToDegree(atan2f(owner->plane->forward.z, owner->plane->forward.x));
    float delta = direction_heading - nose_heading;
    while (delta > 180.0f) {
        delta -= 360.0f;
    }
    while (delta < -180.0f) {
        delta += 360.0f;
    }
    return std::fabs(delta);
}

RSEntity *SCAIBrain::selectGroundWeapon() {
    static const int order[] = {ID_AGM65D, ID_GBU15, ID_MK20, ID_MK82, 7, ID_LAU3};
    for (int weapon_id : order) {
        for (auto weap : owner->plane->weaps_load) {
            if (weap != nullptr && weap->nb_weap > 0 && weap->objct->wdat->weapon_id == weapon_id) {
                return weap->objct;
            }
        }
    }
    return nullptr;
}

Vector3D SCAIBrain::predictBombImpact(RSEntity *bomb) {
    GunSimulatedObject *simulated = new GunSimulatedObject();
    Vector3D initial_thrust = owner->plane->getWeaponIntialVector(1.0f);
    simulated->obj = bomb;
    simulated->x = owner->plane->x;
    simulated->y = owner->plane->y;
    simulated->z = owner->plane->z;
    simulated->vx = initial_thrust.x;
    simulated->vy = initial_thrust.y;
    simulated->vz = initial_thrust.z;
    simulated->weight = bomb->weight_in_kg;
    simulated->azimuthf = owner->plane->yaw;
    simulated->elevationf = owner->plane->pitch;
    simulated->target = nullptr;
    simulated->mission = owner->mission;
    Vector3D impact{0.0f, 0.0f, 0.0f};
    Vector3D velocity{0.0f, 0.0f, 0.0f};
    std::tie(impact, velocity) = simulated->ComputeTrajectoryUntilGround(owner->plane->tps);
    delete simulated;
    return impact;
}

void SCAIBrain::updateGroundAttack(SCMissionActors *target) {
    ground_attack_seen = true;
    bool ground = target != nullptr && !target->is_destroyed && target->object != nullptr && target->object->entity != nullptr && target->object->entity->target_type == 2;
    if (!ground || threat_state > 1 || owner->plane->on_ground) {
        this->resetGroundAttack();
        return;
    }
    if (ground_attack_target != target) {
        this->resetGroundAttack();
        ground_attack_target = target;
    }
    if (ground_phase == 6) {
        return;
    }
    Vector3D own_position = owner->plane->position;
    if (debug_ticks != ground_last_tick + 1) {
        ground_last_position = own_position;
    }
    ground_last_tick = debug_ticks;
    Vector3D own_velocity = (own_position - ground_last_position) * (1.0f / TICK_DURATION);
    float own_speed = own_velocity.Length();
    ground_last_position = own_position;
    Vector3D target_position = target->object->position;
    owner->current_target = target->actor_id;
    owner->pilot->target_waypoint = target_position;
    Vector3D aim_point = target_position;
    aim_point.y += 1000.0f;
    Vector3D away = own_position - aim_point;
    float horizontal_distance = sqrtf(away.x * away.x + away.z * away.z);
    float angle = this->headingDelta(away);
    int previous_phase = ground_phase;
    float heading_error = 0.0f;
    float pitch_error = 0.0f;
    float ground_y = owner->plane->area->getY(own_position.x, own_position.z);
    float nose_elevation = radToDegree(asinf(std::max(-1.0f, std::min(1.0f, owner->plane->forward.y))));

    if (ground_phase != 3) {
        owner->pilot->disengageAutopilot();
    }
    if (ground_phase == 4) {
        if (ground_released == nullptr) {
            for (auto weapon : owner->plane->weaps_object) {
                if ((weapon->obj == ground_weapon || ground_weapon->wdat->weapon_id == ID_LAU3) && std::find(ground_known_objects.begin(), ground_known_objects.end(), weapon) == ground_known_objects.end()) {
                    ground_released = weapon;
                    printf("AI %s#%d bomb released weapon=%d\n", owner->actor_name.c_str(), owner->actor_id, ground_weapon->wdat->weapon_id);
                    break;
                }
            }
            if (ground_released == nullptr && --ground_release_wait <= 0) {
                printf("AI %s#%d bomb release not seen, attack restarts\n", owner->actor_name.c_str(), owner->actor_id);
                this->resetGroundAttack();
                return;
            }
        } else if (std::find(owner->plane->weaps_object.begin(), owner->plane->weaps_object.end(), ground_released) == owner->plane->weaps_object.end()) {
            this->resetGroundAttack();
            return;
        }
        owner->pilot->SetPitchCommand(5.0f, 5.0f);
        owner->pilot->target_speed_ms = (float) owner->object->entity->jdyn->ai_speed_cruise;
        ground_attack_active = true;
        return;
    }

    bool wings_level = false;
    if (ground_phase <= 1) {
        bool aligned = angle > 170.0f;
        bool close = horizontal_distance < 5000.0f;
        if (!aligned && close) {
            ground_phase = 0;
        } else if (aligned && close) {
            wings_level = true;
            owner->pilot->BeginManual();
            owner->pilot->CmdThrottle(5);
            if (owner->pilot->CmdRollTo(0.0f, 5.0f)) {
                owner->pilot->CmdThrottle(3);
                ground_phase = 2;
            }
        } else if (horizontal_distance > 8000.0f || aligned) {
            ground_phase = 1;
        }
    }

    if (ground_phase == 2) {
        Vector3D autopilot_velocity = aim_point - own_position;
        autopilot_velocity.Normalize();
        owner->pilot->engageAutopilot(aim_point, autopilot_velocity * 100.0f);
        ground_autopilot_target = target_position;
        ground_phase3_time = 0.0f;
        ground_phase = 3;
    } else if (ground_phase == 3) {
        if (target_position.x != ground_autopilot_target.x || target_position.y != ground_autopilot_target.y || target_position.z != ground_autopilot_target.z) {
            Vector3D autopilot_velocity = aim_point - own_position;
            autopilot_velocity.Normalize();
            owner->pilot->setAutopilotTarget(aim_point, autopilot_velocity * 100.0f);
            ground_autopilot_target = target_position;
        }
    } else if (!wings_level) {
        Vector3D direction = ground_phase == 0 ? away : -away;
        if (ground_phase == 0) {
            direction.y = 0.0f;
        }
        this->computeAttitudeError(direction, heading_error, pitch_error);
        if (angle > 169.0f && horizontal_distance > 9000.0f) {
            owner->pilot->SetPitchCommand(0.0f, 5.0f);
        } else if (ground_phase == 1 && angle > 169.0f && own_position.y - aim_point.y > 2000.0f) {
            owner->pilot->SetPitchCommand(-40.0f, 5.0f);
        } else {
            owner->pilot->SetGuidanceDirection(direction);
        }
    }
    owner->pilot->target_speed_ms = (float) owner->object->entity->jdyn->ai_speed_cruise;
    ground_attack_active = true;

    if (ground_phase == 3) {
        if (ground_weapon == nullptr) {
            ground_weapon = this->selectGroundWeapon();
            if (ground_weapon == nullptr || (ground_weapon->wdat->weapon_id != ID_MK20 && ground_weapon->wdat->weapon_id != ID_MK82 && ground_weapon->wdat->weapon_id != ID_LAU3)) {
                printf("AI %s#%d ground attack weapon=%d not handled, old code takes over\n", owner->actor_name.c_str(), owner->actor_id, ground_weapon != nullptr ? ground_weapon->wdat->weapon_id : 0);
                owner->pilot->disengageAutopilot();
                ground_phase = 6;
                ground_attack_active = false;
                return;
            }
        }
        if (ground_weapon->wdat->weapon_id == ID_LAU3) {
            ground_phase3_time += TICK_DURATION;
            if (ground_phase3_time >= 3.0f) {
                ground_known_objects = owner->plane->weaps_object;
                owner->pilot->Fire(1 << (ground_weapon->wdat->weapon_id - 1), target);
                owner->pilot->disengageAutopilot();
                ground_released = nullptr;
                ground_release_wait = 25;
                ground_phase = 4;
                printf("AI %s#%d rockets fired d=%.0f own_y=%.0f\n", owner->actor_name.c_str(), owner->actor_id, horizontal_distance, own_position.y);
            } else if (owner->pilot->autopilotReached()) {
                owner->pilot->disengageAutopilot();
                ground_phase = 0;
            }
            return;
        }
        Vector3D impact = this->predictBombImpact(ground_weapon);
        float drop_height = own_position.y - target_position.y;
        float descent_speed = -own_velocity.y;
        float ballistic_time = drop_height > 0.0f ? (-descent_speed + sqrtf(descent_speed * descent_speed + 2.0f * 9.8f * drop_height)) / 9.8f : 0.0f;
        float ballistic_range = sqrtf(own_velocity.x * own_velocity.x + own_velocity.z * own_velocity.z) * ballistic_time;
        float miss = sqrtf((impact.x - target_position.x) * (impact.x - target_position.x) + (impact.z - target_position.z) * (impact.z - target_position.z));
        float tolerance = 20.0f + own_speed * TICK_DURATION;
        if ((std::rand() & 0xF) > owner->profile->ai.atrb.AG) {
            tolerance += 150.0f;
        }
        if (miss <= tolerance) {
            ground_known_objects = owner->plane->weaps_object;
            owner->pilot->Fire(1 << (ground_weapon->wdat->weapon_id - 1), target);
            owner->pilot->disengageAutopilot();
            ground_released = nullptr;
            ground_release_wait = 25;
            ground_phase = 4;
            printf("AI %s#%d bomb release requested weapon=%d miss=%.0f tolerance=%.0f d=%.0f own_y=%.0f speed=%.0f\n", owner->actor_name.c_str(), owner->actor_id, ground_weapon->wdat->weapon_id, miss, tolerance, horizontal_distance, own_position.y, own_speed);
        } else if (owner->pilot->autopilotReached()) {
            owner->pilot->disengageAutopilot();
            ground_phase = 0;
        }
        if (debug_ticks % 25 == 0) {
            printf("AI %s#%d ground attack phase=%d target=%s d=%.0f angle=%.0f miss=%.0f tolerance=%.0f impact=(%.0f,%.0f) impact_y=%.0f target_y=%.0f range=%.0f ballistic_range=%.0f own_y=%.0f speed=%.0f autopilot=%d reached=%d\n", owner->actor_name.c_str(), owner->actor_id, ground_phase, target->actor_name.c_str(), horizontal_distance, angle, miss, tolerance, impact.x, impact.z, impact.y, target_position.y, sqrtf((impact.x - own_position.x) * (impact.x - own_position.x) + (impact.z - own_position.z) * (impact.z - own_position.z)), ballistic_range, own_position.y, own_speed, owner->pilot->autopilotActive() ? 1 : 0, owner->pilot->autopilotReached() ? 1 : 0);
        }
    } else if (debug_ticks % 25 == 0) {
        printf("AI %s#%d ground attack phase=%d target=%s d=%.0f angle=%.0f heading_err=%.1f pitch_err=%.1f own_y=%.0f speed=%.0f\n", owner->actor_name.c_str(), owner->actor_id, ground_phase, target->actor_name.c_str(), horizontal_distance, angle, heading_error, pitch_error, own_position.y, own_speed);
    }
    if (ground_phase != previous_phase) {
        printf("AI %s#%d ground attack phase %d -> %d\n", owner->actor_name.c_str(), owner->actor_id, previous_phase, ground_phase);
    }
}

void SCAIBrain::updatePursuit() {
    pursuit_active = false;
    Vector3D own_position = owner->plane->position;
    Vector3D own_velocity = (own_position - own_last_position) * (1.0f / TICK_DURATION);
    own_last_position = own_position;
    if (air_target == nullptr || air_target->plane == nullptr || threat_state > 1 || owner->plane->on_ground) {
        pursuit_last_target = nullptr;
        aim_trim = 0.0f;
        owner->pilot->attitude_mode = false;
        return;
    }
    if (pursuit_last_target != air_target) {
        aim_trim = 0.0f;
    }
    Vector3D target_position = air_target->plane->position;
    Vector3D target_velocity = {0.0f, 0.0f, 0.0f};
    if (pursuit_last_target == air_target) {
        target_velocity = (target_position - target_last_position) * (1.0f / TICK_DURATION);
    }
    pursuit_last_target = air_target;
    target_last_position = target_position;

    RSIntel &intel = owner->mission->intel;
    Vector3D delta = target_position - own_position;
    float distance = delta.Length();
    float own_speed = own_velocity.Length();
    float time_to_go = own_speed > 1.0f ? distance / own_speed : 0.0f;
    time_to_go = std::min(time_to_go, 3.0f);
    Vector3D lead = target_position + target_velocity * time_to_go;
    Vector3D direction = lead - own_position;
    float horizontal = sqrtf(direction.x * direction.x + direction.z * direction.z);
    direction.y = std::max(-horizontal, std::min(horizontal, direction.y));
    float delta_horizontal = sqrtf(delta.x * delta.x + delta.z * delta.z);
    float nose_elevation = radToDegree(asinf(std::max(-1.0f, std::min(1.0f, owner->plane->forward.y))));
    float los_elevation = radToDegree(atan2f(delta.y, delta_horizontal));
    float aim_error = los_elevation - nose_elevation;
    if (distance < intel.range_medium && std::fabs(aim_error) < 10.0f) {
        aim_trim += tanf(degreeToRad(aim_error)) * delta_horizontal * 0.05f;
        aim_trim = std::max(-300.0f, std::min(300.0f, aim_trim));
    } else if (distance >= intel.range_medium) {
        aim_trim = 0.0f;
    }
    Vector3D waypoint = own_position + direction;
    waypoint.y = own_position.y + std::max(-400.0f, std::min(400.0f, direction.y + aim_trim));

    float heading_error = 0.0f;
    float pitch_error = 0.0f;
    if (attitude_control_enabled) {
        this->computeAttitudeError(lead - own_position, heading_error, pitch_error);
        owner->pilot->SetGuidanceDirection(lead - own_position);
        owner->pilot->target_waypoint = lead;
    } else {
        owner->pilot->SetTargetWaypoint(waypoint);
        if (waypoint.y < owner->plane->y) {
            float ground_y = owner->plane->area->getY(waypoint.x, waypoint.z);
            owner->pilot->target_climb = (int) std::max(waypoint.y, ground_y + 1000.0f);
        }
    }
    float target_forward_speed = air_target->plane->forwardSpeedPerTick();
    if (distance > intel.range_medium) {
        owner->pilot->target_speed = -60;
    } else {
        float faster = distance > intel.range_gun * 0.6f ? 10.0f : 0.0f;
        owner->pilot->target_speed = (int) std::max(-60.0f, target_forward_speed - faster);
    }
    pursuit_active = true;
    if (debug_ticks % 25 == 0) {
        printf("AI %s#%d pursuit target=%s mission_target=%s d=%.0f own_speed=%.0f time_to_go=%.1f lead_dy=%.0f own_y=%.0f target_y=%.0f dy=%.0f waypoint_y=%.0f climb_cmd=%d speed_cmd=%d vz=%.0f target_vz=%.0f nose_elev=%.1f los_elev=%.1f aim_trim=%.0f heading_err=%.1f pitch_err=%.1f\n", owner->actor_name.c_str(), owner->actor_id, air_target->actor_name.c_str(), owner->target != nullptr ? owner->target->actor_name.c_str() : "none", distance, own_speed, time_to_go, lead.y - target_position.y, own_position.y, target_position.y, target_position.y - own_position.y, waypoint.y, owner->pilot->target_climb, owner->pilot->target_speed, owner->plane->vz, air_target->plane->vz, nose_elevation, los_elevation, aim_trim, heading_error, pitch_error);
    }
}

int SCAIBrain::missileDistanceBand() {
    Vector3D missile_position = {missile_threat->x, missile_threat->y, missile_threat->z};
    float distance = (missile_position - owner->plane->position).Length();
    int weapon_id = missile_threat->obj->wdat->weapon_id;
    int missile_mask = weapon_id >= 1 ? (1 << (weapon_id - 1)) : 0;
    RSIntel &intel = owner->mission->intel;
    if ((missile_mask & 0x700) != 0) {
        if (distance < intel.range_medium) {
            return 1;
        }
        return distance < intel.range_far ? 2 : 3;
    }
    if ((missile_mask & 0x3) != 0) {
        if (distance < intel.range_close) {
            return 1;
        }
        return distance < intel.range_long ? 2 : 3;
    }
    return 0;
}

void SCAIBrain::reactToMissile() {
    if (missile_threat != nullptr && (!missile_threat->alive || missile_threat->target != owner)) {
        missile_threat = nullptr;
        threat_state = 0;
        evasion_hold = 0;
        return;
    }
    if (threat_state == 2) {
        evasion_hold = 25;
    }
    if (missile_threat == nullptr || evasion_hold <= 0) {
        return;
    }
    evasion_hold--;
    int band = this->missileDistanceBand();
    if (band == 0) {
        return;
    }
    Vector3D own_position = owner->plane->position;
    Vector3D missile_position = {missile_threat->x, missile_threat->y, missile_threat->z};
    Vector3D to_missile = missile_position - own_position;
    float floor = this->floorAltitude();
    to_missile.y = own_position.y < floor ? 2.0f * floor - own_position.y : floor - own_position.y;
    Vector3D escape = -to_missile;
    if (band != 3) {
        escape = {to_missile.z, 0.0f, -to_missile.x};
        Vector3D forward_horizontal = {owner->plane->forward.x, 0.0f, owner->plane->forward.z};
        if (escape.DotProduct(&forward_horizontal) < 0.0f) {
            escape = -escape;
        }
        escape.y = band == 1 ? 0.0f : to_missile.y;
    }
    owner->pilot->SetGuidanceDirection(escape);
    owner->pilot->target_speed_ms = (float) owner->object->entity->jdyn->ai_speed_max;
    SCMissionActors *player = this->playerActor();
    if (missile_threat != complained_missile && missile_threat->shooter != nullptr && missile_threat->shooter == player && player->team_id == owner->team_id) {
        owner->setMessage(0x0E);
        complained_missile = missile_threat;
    }
    evasion_active = true;
    if (debug_ticks % 25 == 0) {
        printf("AI %s#%d evade band=%d missile_d=%.0f hold=%d\n", owner->actor_name.c_str(), owner->actor_id, band, to_missile.Length(), evasion_hold);
    }
}

void SCAIBrain::computeAttitudeError(Vector3D direction, float &heading_error, float &pitch_error) {
    float azimuth = atan2f(direction.z, direction.x) * 180.0f / (float) M_PI;
    azimuth -= 360.0f;
    azimuth += 90.0f;
    while (azimuth < 0.0f) {
        azimuth += 360.0f;
    }
    if (azimuth > 360.0f) {
        azimuth -= 360.0f;
    }
    float target_yaw = norm3600(3600.0f - azimuth * 10.0f);
    heading_error = -signed1800(target_yaw - owner->plane->yaw) / 10.0f;

    float horizontal = sqrtf(direction.x * direction.x + direction.z * direction.z);
    float desired_elevation = radToDegree(atan2f(direction.y, horizontal));
    float nose_elevation = radToDegree(asinf(std::max(-1.0f, std::min(1.0f, owner->plane->forward.y))));
    pitch_error = desired_elevation - nose_elevation;
}

Vector3D SCAIBrain::runwayAxis(Vector3D direction) {
    const float zero = 1.0f / 256.0f;
    if (fabsf(direction.x) < zero && fabsf(direction.z) >= zero) {
        return Vector3D(0.0f, 0.0f, direction.z > 0.0f ? 1.0f : -1.0f);
    }
    if (fabsf(direction.z) < zero && fabsf(direction.x) >= zero) {
        return Vector3D(direction.x > 0.0f ? 1.0f : -1.0f, 0.0f, 0.0f);
    }
    printf("AI %s#%d runway not axial (%.3f, %.3f), using the nose direction\n", owner->actor_name.c_str(), owner->actor_id, direction.x, direction.z);
    Vector3D horizontal(direction.x, 0.0f, direction.z);
    horizontal.Normalize();
    return horizontal;
}

void SCAIBrain::endGroundOp() {
    owner->pilot->EndGroundOps();
    ground_op = GROUND_OP_NONE;
}

bool SCAIBrain::takeoffOrder() {
    SCPlane *plane = owner->plane;
    SCPilot *pilot = owner->pilot;
    RSEntity *entity = plane->object->entity;
    if (ground_op != GROUND_OP_TAKEOFF) {
        if (owner->taken_off || plane->velocity.Length() > 10.0f) {
            owner->taken_off = true;
            return true;
        }
        ground_op = GROUND_OP_TAKEOFF;
        ground_op_phase = 0;
        ground_op_time = 0.0f;
        ground_op_stick = 0.0f;
        landing_done = false;
        ground_op_axis = this->runwayAxis(plane->forward);
        pilot->BeginGroundOps();
        printf("AI %s#%d takeoff accel=%d rotate=%d pitch=%d gain=%d\n", owner->actor_name.c_str(), owner->actor_id, entity->takeoff_roll_accel, entity->takeoff_rotate_speed, entity->takeoff_climb_pitch, entity->takeoff_pitch_gain);
    }
    switch (ground_op_phase) {
        case 0: {
            pilot->CmdGroundControls(0.0f, 10, plane->GetFlaps(), 1, 0);
            ground_op_time += TICK_DURATION;
            float speed = entity->takeoff_roll_accel * ground_op_time;
            if (speed > entity->takeoff_rotate_speed) {
                pilot->CmdKinematic(false, ground_op_axis * speed, ground_op_axis, 0.0f);
                ground_op_phase = 1;
            } else {
                pilot->CmdKinematic(true, ground_op_axis * speed, ground_op_axis, 0.0f);
            }
            break;
        }
        case 1: {
            if (plane->y - plane->groundlevel > 300.0f) {
                pilot->CmdGroundControls(ground_op_stick, 5, 1, 1, 0);
                ground_op_phase = 2;
                break;
            }
            // AI_PitchAttitudeHold_126CC
            float gain = (float) entity->takeoff_pitch_gain;
            float error = (float) entity->takeoff_climb_pitch - pilot->NosePitch();
            ground_op_stick = floorf(std::clamp(error * gain / 8.0f, -gain, gain));
            pilot->CmdGroundControls(ground_op_stick, 10, 1, 1, 0);
            break;
        }
        case 2:
            pilot->CmdGroundControls(ground_op_stick, 5, 0, 0, 0);
            ground_op_phase = 3;
            break;
        case 3:
            if (pilot->NosePitch() > 17.0f) {
                ground_op_stick = -16.0f;
            } else {
                ground_op_stick = 8.0f;
                ground_op_phase = 4;
            }
            pilot->CmdGroundControls(ground_op_stick, 5, 0, 0, 0);
            break;
        default:
            this->endGroundOp();
            owner->taken_off = true;
            printf("AI %s#%d takeoff done\n", owner->actor_name.c_str(), owner->actor_id);
            return true;
    }
    return false;
}

bool SCAIBrain::landingOrder(uint8_t approach_spot, uint8_t touchdown_spot) {
    if (landing_done) {
        return true;
    }
    SCPlane *plane = owner->plane;
    SCPilot *pilot = owner->pilot;
    RSEntity *entity = plane->object->entity;
    std::vector<SPOT *> &spots = owner->mission->mission->mission_data.spots;
    if (ground_op != GROUND_OP_LANDING) {
        // Player_ResolveAttachPointN_5305A : index hors table -> (0, 0, 0)
        Vector3D approach(0.0f, 0.0f, 0.0f);
        Vector3D touchdown(0.0f, 0.0f, 0.0f);
        if (approach_spot < spots.size()) {
            approach = spots[approach_spot]->position;
        }
        if (touchdown_spot < spots.size()) {
            touchdown = spots[touchdown_spot]->position;
        }
        auto it = std::find(owner->mission->friendlies.begin(), owner->mission->friendlies.end(), owner);
        if (it != owner->mission->friendlies.end()) {
            owner->mission->friendlies.erase(it);
        }
        // LandingBehavior_Start_75746 : l'IA est teleportee au point d'approche
        ground_op_origin = approach;
        // Landing_Phase1_SetupApproach_75D51
        landing_target = touchdown + Vector3D(0.0f, (float) entity->landing_aim_height, 0.0f);
        Vector3D delta = landing_target - ground_op_origin;
        ground_op_axis = this->runwayAxis(delta);
        landing_speed = (float) entity->landing_speed;
        landing_duration = delta.Length() / landing_speed;
        landing_dir = delta;
        landing_dir.Normalize();
        landing_counter = 0;
        landing_pitch = 0.0f;
        landing_leveled = false;
        ground_op_time = 0.0f;
        ground_op = GROUND_OP_LANDING;
        ground_op_phase = 2;
        pilot->BeginGroundOps();
        pilot->CmdGroundControls(0.0f, plane->GetThrottle() / 10, plane->GetFlaps(), 1, 0);
        pilot->CmdPlaceAt(ground_op_origin, ground_op_axis, 0.0f);
        printf("AI %s#%d landing approach=%d touchdown=%d speed=%d aim=%d steps=%d\n", owner->actor_name.c_str(), owner->actor_id, approach_spot, touchdown_spot, entity->landing_speed, entity->landing_aim_height, entity->landing_pitch_steps);
        return false;
    }
    switch (ground_op_phase) {
        case 2: {
            // Landing_Phase2_Approach_76325
            ground_op_time += TICK_DURATION;
            landing_counter -= 1;
            if (landing_counter < entity->landing_pitch_steps) {
                landing_counter = entity->landing_pitch_steps;
            } else {
                landing_pitch += 1.0f;
            }
            pilot->CmdKinematic(true, landing_dir * landing_speed, ground_op_axis, landing_pitch);
            if (ground_op_time >= landing_duration) {
                ground_op_time = 0.0f;
                ground_op_phase = 3;
            }
            break;
        }
        case 3: {
            // Landing_Phase3_TouchdownRoll_765B2
            if (ground_op_time == 0.0f) {
                pilot->CmdPlaceAt(landing_target, ground_op_axis, landing_pitch);
            }
            landing_counter += 1;
            if (landing_counter >= entity->landing_pitch_steps / 6) {
                landing_counter = 0;
            } else {
                landing_pitch -= 1.0f;
            }
            ground_op_time += TICK_DURATION;
            if (landing_counter == 0 && !landing_leveled) {
                landing_pitch = 0.0f;
                landing_leveled = true;
                ground_op_time = 0.0f;
                ground_op_phase = 4;
            }
            pilot->CmdKinematic(true, ground_op_axis * landing_speed, ground_op_axis, landing_pitch);
            break;
        }
        case 4: {
            // Landing_Phase4_Braking_76C09 : la vitesse baisse par secondes entieres
            ground_op_time += TICK_DURATION;
            int speed = (int) landing_speed - 2 * (int) ground_op_time;
            pilot->CmdKinematic(true, ground_op_axis * (float) speed, ground_op_axis, 0.0f);
            if (speed <= 0) {
                ground_op_phase = 5;
            }
            break;
        }
        default: {
            // Landing_Phase5_Stop_76E67
            Vector3D parked(plane->x, plane->groundlevel, plane->z);
            pilot->CmdGroundControls(0.0f, -1, 0, 1, 0);
            pilot->CmdPlaceAt(parked, ground_op_axis, 0.0f);
            landing_done = true;
            this->endGroundOp();
            printf("AI %s#%d landed\n", owner->actor_name.c_str(), owner->actor_id);
            return true;
        }
    }
    return false;
}
