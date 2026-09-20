#include "precomp.h"
#include "SCAIBrain.h"

SCAIBrain::SCAIBrain(SCMissionActors *owner) {
    this->owner = owner;
}

void SCAIBrain::tick() {
    this->acquireBestThreat(true);
    weapon_mask = this->selectWeaponMask();
    if (weapon_mask != last_weapon_mask) {
        printf("AI %s#%d weapon_mask=0x%X\n", owner->actor_name.c_str(), owner->actor_id, weapon_mask);
        last_weapon_mask = weapon_mask;
    }
    fire_solution_quality = this->computeFireSolutionQuality();
    if (fire_control_enabled) {
        this->updateFireControl();
    }
    if (weapon_mask != 0 && debug_ticks % 25 == 0) {
        printf("AI %s#%d weapon_mask=0x%X quality=%d\n", owner->actor_name.c_str(), owner->actor_id, weapon_mask, fire_solution_quality);
    }
    this->runGoalSelectors();
    evasion_active = false;
    if (evasion_enabled) {
        this->reactToMissile();
    }
    if (pursuit_enabled && !evasion_active) {
        this->updatePursuit();
    }
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
    int weight_a = owner->profile->ai.atrb.AR - owner->profile->ai.atrb.TH + 16;
    int weight_b = owner->profile->ai.atrb.TH - owner->profile->ai.atrb.AR + 16;
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
                bool accepted = this->skillCheck(owner->profile->ai.atrb.TH, score.aptitude);
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
            bool accepted = this->skillCheck(owner->profile->ai.atrb.TH, score.aptitude);
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
        bool accepted = this->skillCheck(owner->profile->ai.atrb.TH, score.aptitude);
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
    debug_ticks++;
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
    switch (owner->current_command) {
        case OP_SET_WAIT_FOR_SECONDS:
            owner->current_command_executed = owner->wait(owner->current_command_arg);
        break;
        case OP_SET_OBJ_TAKE_OFF:
            owner->current_command_executed = owner->takeOff(owner->current_command_arg);
        break;
        case OP_SET_OBJ_LAND:
            owner->current_command_executed = owner->land(owner->current_command_arg);
        break;
        case OP_SET_OBJ_FLY_TO_WP:
            owner->current_command_executed = owner->flyToWaypoint(owner->current_command_arg);
        break;
        case OP_SET_OBJ_FLY_TO_AREA:
            owner->current_command_executed = owner->flyToArea(owner->current_command_arg);
        break;
        case OP_SET_OBJ_FOLLOW_ALLY:
            owner->current_command_executed = owner->followAlly(owner->current_command_arg);
        break;
        case OP_SET_OBJ_DESTROY_TARGET:
            owner->current_command_executed = owner->destroyTarget(owner->current_command_arg);
            return false;
        case OP_SET_OBJ_DEFEND_TARGET:
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
                // TODO tournoi MVRS — AI_IMPLEMENTATION_GUIDE.md §3
                continue;
            case GOAL_ACTIVE_WINGMAN:
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
    int trigger_happy = owner->profile->ai.atrb.TH;

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
        float target_speed_per_tick = std::fabs(air_target->plane->vz) * air_target->plane->tps * TICK_DURATION;
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

bool SCAIBrain::testMissileLock(RSEntity *missile) {
    Vector3D delta = air_target->plane->position - owner->plane->position;
    float distance = delta.Length();
    if (distance <= 0.0f || distance > missile->wdat->target_range) {
        return false;
    }
    return owner->plane->forward.AngleBetween(delta) <= 90.0f - missile->wdat->tracking_cone;
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

void SCAIBrain::updatePursuit() {
    pursuit_active = false;
    Vector3D own_position = owner->plane->position;
    Vector3D own_velocity = (own_position - own_last_position) * (1.0f / TICK_DURATION);
    own_last_position = own_position;
    bool combat_order = owner->current_command == OP_SET_OBJ_DESTROY_TARGET || owner->current_command == OP_SET_OBJ_DEFEND_TARGET;
    if (air_target == nullptr || air_target->plane == nullptr || threat_state > 1 || !combat_order || owner->plane->on_ground) {
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
        this->computeAttitudeError(direction, heading_error, pitch_error);
        if (distance < intel.range_medium) {
            heading_error = std::max(-10.0f, std::min(10.0f, heading_error));
            pitch_error = std::max(-10.0f, std::min(10.0f, pitch_error));
        }
        float ground_y = owner->plane->area->getY(own_position.x, own_position.z);
        if (own_position.y - ground_y < 1000.0f && pitch_error < 0.0f) {
            pitch_error = own_position.y - ground_y < 500.0f ? 10.0f : 0.0f;
        }
        owner->pilot->SetAttitudeError(heading_error, pitch_error, distance < intel.range_medium ? 0.5f : 2.0f);
        owner->pilot->target_waypoint = lead;
    } else {
        owner->pilot->SetTargetWaypoint(waypoint);
        if (waypoint.y < owner->plane->y) {
            float ground_y = owner->plane->area->getY(waypoint.x, waypoint.z);
            owner->pilot->target_climb = (int) std::max(waypoint.y, ground_y + 1000.0f);
        }
    }
    float target_forward_speed = air_target->plane->vz;
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
    to_missile.y = 0.0f;
    Vector3D escape = -to_missile;
    if (band != 3) {
        escape = {to_missile.z, 0.0f, -to_missile.x};
        Vector3D forward_horizontal = {owner->plane->forward.x, 0.0f, owner->plane->forward.z};
        if (escape.DotProduct(&forward_horizontal) < 0.0f) {
            escape = -escape;
        }
    }
    escape.Normalize();
    Vector3D waypoint = own_position + escape * 5000.0f;
    waypoint.y = own_position.y;
    owner->pilot->SetTargetWaypoint(waypoint);
    owner->pilot->target_speed = -60;
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
