#include "precomp.h"
#include "SCAIBrain.h"

static float mvWrap180(float angle) {
    while (angle > 180.0f) {
        angle -= 360.0f;
    }
    while (angle < -180.0f) {
        angle += 360.0f;
    }
    return angle;
}

static float mvCompass(Vector3D v) {
    return radToDegree(atan2f(-v.x, v.z));
}

static float mvElevation(Vector3D v) {
    return radToDegree(atan2f(v.y, sqrtf(v.x * v.x + v.z * v.z)));
}

static Vector3D mvRotateHorizontal(Vector3D v, float degrees) {
    float length = sqrtf(v.x * v.x + v.z * v.z);
    float angle = atan2f(v.x, v.z) - degreeToRad(degrees);
    return Vector3D(sinf(angle) * length, 0.0f, cosf(angle) * length);
}

Vector3D SCAIBrain::actorVelocity(SCMissionActors *actor) {
    SCPlane *plane = actor->plane;
    float tps = plane->tps > 0 ? (float) plane->tps : 25.0f;
    return Vector3D(plane->x - plane->last_px, plane->y - plane->last_py, plane->z - plane->last_pz) * tps;
}

float SCAIBrain::floorAltitude() {
    return owner->plane->area->getY(owner->plane->x, owner->plane->z) + 200.0f;
}

float SCAIBrain::indicatedAirspeed() {
    float speed = this->actorVelocity(owner).Length();
    float altitude = std::max(0.0f, owner->plane->y);
    float sigma = powf(std::max(0.0f, 1.0f - 2.2558e-5f * altitude), 4.2559f);
    return speed * sqrtf(sigma);
}

bool SCAIBrain::tooSlow() {
    return owner->plane->wing_stall > 0 || this->indicatedAirspeed() <= (float) owner->object->entity->jdyn->ai_speed_min;
}

bool SCAIBrain::tooLow() {
    return owner->plane->y < owner->plane->area->getY(owner->plane->x, owner->plane->z) + 4.0f * 200.0f;
}

int SCAIBrain::decisionWeight() {
    int value = owner->object->entity->jdyn->ai_decision_weight;
    if (value >= 2) {
        return (value - 2) * 3 / 2 + 3;
    }
    return -((2 - value) * 3 / 2 + 3);
}

void SCAIBrain::buildCombatContext(SCMissionActors *target) {
    JDYN *jdyn = owner->object->entity->jdyn;
    ctx.aircraft = owner->plane != nullptr;
    Vector3D own_velocity = this->actorVelocity(owner);
    ctx.my_speed = own_velocity.Length();
    ctx.cruise = (float) jdyn->ai_speed_cruise;
    ctx.min_speed = (float) jdyn->ai_speed_min;
    ctx.ias = this->indicatedAirspeed();
    ctx.target = target;
    ctx.behind = false;
    ctx.head_on = false;
    if (target == nullptr || target->plane == nullptr) {
        ctx.dist = 0.0f;
        return;
    }
    Vector3D forward = owner->plane->forward;
    Vector3D target_velocity = this->actorVelocity(target);
    ctx.D = target->plane->position - owner->plane->position;
    ctx.dist = ctx.D.Length();
    ctx.future_rel = (target->plane->position + target_velocity * 3.0f) - (owner->plane->position + own_velocity * 3.0f);
    ctx.nose_angle = forward.AngleBetween(ctx.D);
    ctx.future_angle = forward.AngleBetween(ctx.future_rel);
    ctx.target_vel_angle = ctx.D.AngleBetween(target_velocity);
    ctx.aspect = 180.0f - ctx.target_vel_angle;
    ctx.target_speed = target_velocity.Length();
    ctx.side = mvWrap180(mvCompass(ctx.D) - mvCompass(forward));
    Vector3D direction = ctx.D;
    direction.Normalize();
    float los_speed = std::fabs(target_velocity.x * direction.x + target_velocity.y * direction.y + target_velocity.z * direction.z);
    bool close = ctx.dist < 3.0f * los_speed;
    if (ctx.nose_angle >= 100.0f && (ctx.future_angle <= 80.0f || (ctx.aspect < 50.0f && close))) {
        ctx.behind = true;
    } else if (ctx.nose_angle <= 60.0f && ctx.aspect < 60.0f && close) {
        ctx.head_on = true;
    }
}

int SCAIBrain::scoreManeuver(int id, SCMissionActors *target) {
    this->buildCombatContext(target);
    RSIntel &intel = owner->mission->intel;
    float range = intel.range_gun;
    int flying = owner->profile->ai.atrb.FL;
    int uses = maneuver_uses[id];
    bool has_target = target != nullptr && target->plane != nullptr;
    int score = 0;
    switch (id) {
        case 1:
        case 2: {
            if (!has_target || ctx.aspect > 20.0f || this->tooSlow() || this->tooLow()) {
                return 0;
            }
            score = 5 - uses / 2;
            if (id == 1) {
                if (ctx.aircraft && ctx.behind) {
                    return 0;
                }
                if (ctx.aspect < 30.0f && ctx.nose_angle > 90.0f) {
                    score += 2;
                    if (ctx.nose_angle > 135.0f && ctx.dist < range) {
                        score += 2;
                    }
                } else {
                    score -= ctx.aspect > 90.0f ? 10 : ctx.aspect > 45.0f ? 4 : 0;
                    if (ctx.nose_angle < 90.0f) {
                        score -= 3;
                    }
                    if (ctx.dist > range) {
                        score -= 3;
                    }
                }
                if (ctx.aircraft) {
                    score -= flying / 2;
                }
            } else {
                if (ctx.aircraft) {
                    return 0;
                }
                if (ctx.nose_angle > 135.0f && ctx.aspect < 30.0f && ctx.dist < range) {
                    score += 7;
                } else {
                    score -= ctx.aspect > 90.0f ? 10 : ctx.aspect > 45.0f ? 4 : 0;
                    if (ctx.nose_angle < 90.0f) {
                        score -= 3;
                    }
                    if (ctx.dist > range) {
                        score -= 3;
                    }
                }
                if (ctx.ias < ctx.min_speed) {
                    score -= 6;
                }
                if (this->skillCheck(owner->profile->ai.atrb.SM, 5)) {
                    score += 3;
                }
            }
            break;
        }
        case 3: {
            maneuver_phase_hint = 0;
            if (!has_target || ctx.aspect > 20.0f) {
                return 0;
            }
            score = 5;
            if (ctx.nose_angle < 90.0f) {
                score -= 3;
            }
            if (ctx.nose_angle > 135.0f && ctx.dist <= range) {
                score += 3;
            }
            if (ctx.dist > range) {
                score -= 3;
            }
            if (ctx.aircraft) {
                score -= flying / 4;
            }
            break;
        }
        case 4: {
            if (!has_target || ctx.aspect > 20.0f || this->tooSlow() || this->tooLow() || (ctx.aircraft && ctx.behind)) {
                return 0;
            }
            score = 5;
            if (ctx.nose_angle < 35.0f) {
                score += (ctx.future_angle > 135.0f && ctx.aspect < 45.0f) ? 4 : -10;
            } else if (ctx.aspect < 30.0f && ctx.nose_angle > 90.0f) {
                score += 2;
                if (ctx.nose_angle > 135.0f && ctx.dist < range) {
                    score += 2;
                }
            } else {
                if (ctx.aspect > 45.0f) {
                    score -= 2;
                }
                if (ctx.nose_angle < 90.0f) {
                    score -= 2;
                }
                if (ctx.dist > 2.0f * range) {
                    score -= 3;
                }
            }
            if (ctx.ias < ctx.min_speed) {
                score -= 8;
            } else if (ctx.ias < ctx.cruise) {
                score -= 2;
            }
            break;
        }
        case 5:
        case 6: {
            maneuver_phase_hint = 3;
            if (!has_target || !ctx.aircraft) {
                return 0;
            }
            if (id == 5) {
                if (this->tooSlow() || ctx.behind || ctx.ias < ctx.cruise) {
                    return 0;
                }
            } else {
                bool blocked = owner->plane->y < this->floorAltitude() + 4000.0f || ctx.behind;
                if (blocked) {
                    return 0;
                }
            }
            score = 3 - uses * 2;
            float dz = ctx.future_rel.y;
            if (id == 5) {
                if (dz < -2500.0f) {
                    score -= 7;
                } else if (dz >= 1250.0f) {
                    score += 2;
                }
            } else {
                if (dz > -2000.0f) {
                    score -= 7;
                } else {
                    score += 2;
                }
            }
            if (ctx.nose_angle < 35.0f) {
                score += (ctx.future_angle > 145.0f && ctx.aspect < 45.0f) ? 4 : -10;
            } else if (ctx.nose_angle > 90.0f && ctx.aspect < 45.0f) {
                if (ctx.dist < 2.0f * range) {
                    score += 7;
                } else {
                    score -= 2;
                }
            }
            if (id == 5) {
                if (ctx.my_speed < ctx.target_speed / 2.0f) {
                    score -= 4;
                } else if (ctx.my_speed < ctx.target_speed) {
                    score -= 2;
                } else if (ctx.my_speed > 2.0f * ctx.target_speed) {
                    score += 2;
                }
                score += this->decisionWeight();
                if (this->tooLow()) {
                    score += 4;
                } else if (owner->plane->y > (float) intel.unknown_range) {
                    score -= 6;
                    maneuver_phase_hint = 1;
                }
            } else {
                if (ctx.ias > ctx.cruise) {
                    score -= 4;
                    maneuver_phase_hint = 2;
                } else if (this->tooSlow()) {
                    score += 6;
                }
                if (ctx.my_speed > 2.0f * ctx.target_speed) {
                    score -= 6;
                } else if (ctx.my_speed > ctx.target_speed) {
                    score -= 3;
                } else if (ctx.my_speed < ctx.target_speed) {
                    score += 2;
                }
            }
            break;
        }
        case 7: {
            maneuver_bits = 0;
            if (owner->plane->wing_stall > 0 || !has_target) {
                return 0;
            }
            score = 5;
            if (flying > 13 || this->skillCheck(flying, -7)) {
                if (ctx.aspect < 90.0f) {
                    maneuver_bits |= 1;
                }
                if (ctx.nose_angle > 110.0f) {
                    maneuver_bits |= 1;
                    score -= 3;
                } else if (ctx.nose_angle < 50.0f) {
                    score += 3;
                }
                if (ctx.target_vel_angle < 90.0f) {
                    score += 4;
                }
                float elevation = mvElevation(ctx.D);
                if (elevation > 45.0f) {
                    score -= (int) ((elevation - 20.0f) * 6.0f / 25.0f) + 4;
                    score -= this->tooSlow() ? 4 : 2;
                } else if (elevation < -30.0f) {
                    score -= 4;
                } else {
                    score += 4;
                }
                if (ctx.aircraft && ctx.behind) {
                    maneuver_point = ctx.future_rel;
                    maneuver_bits |= 2;
                    score += 10;
                } else if (ctx.aircraft && ctx.head_on) {
                    maneuver_point = ctx.future_rel;
                    maneuver_bits |= 8;
                    score += 10;
                }
            } else if ((ctx.nose_angle < 60.0f && ctx.target_vel_angle < 60.0f) || ctx.dist > range) {
                score += 2;
            } else {
                score -= 2;
            }
            return std::max(1, std::min(9, score));
        }
        case 13: {
            if (!has_target || this->tooSlow() || owner->plane->y >= owner->object->entity->jdyn->ai_engage_range) {
                return 0;
            }
            score = 1 - 2 * uses;
            bool not_far_below = ctx.D.y > -400.0f;
            if (ctx.target_speed < ctx.my_speed && not_far_below && ctx.my_speed >= (ctx.cruise + ctx.min_speed) / 2.0f && ctx.aircraft && (ctx.behind || ctx.head_on)) {
                if (ctx.target_speed <= ctx.min_speed) {
                    score += 5;
                } else {
                    score += (int) (5.0f * (ctx.my_speed - ctx.target_speed) / ctx.target_speed);
                }
            }
            if (ctx.dist > (float) intel.range_long && ctx.aspect < 30.0f) {
                score += 5;
            } else if (ctx.dist < range) {
                score -= 5;
                if (ctx.aspect < 20.0f) {
                    score -= 5;
                }
            }
            score += this->decisionWeight();
            if (this->tooLow()) {
                score += 4;
            }
            break;
        }
        case 14: {
            if (has_target || owner->pilot->autopilotActive()) {
                return 0;
            }
            float clear = std::min(9.0f * (float) std::max(flying, 8) / 32.0f, 5.0f);
            float height = owner->plane->y - owner->plane->area->getY(owner->plane->x, owner->plane->z);
            float roll = owner->pilot->BankAngle();
            float vertical_speed = this->actorVelocity(owner).y;
            if (height < clear * (1.0f + sinf(degreeToRad(roll / 2.0f)) / 2.0f) || (owner->plane->y < this->floorAltitude() && vertical_speed < 0.0f)) {
                return 10;
            }
            return 0;
        }
        case 15: {
            if (has_target || owner->pilot->autopilotActive()) {
                return 0;
            }
            if (owner->plane->wing_stall > 0 || (ctx.ias <= ctx.min_speed && owner->pilot->NosePitch() > 30.0f)) {
                return 10;
            }
            return 0;
        }
        case 16:
            return flying < 12 ? 1 : 0;
        case 19: {
            SCMissionActors *ground = ground_target != nullptr ? ground_target : owner->target;
            bool ground_ok = ground != nullptr && !ground->is_destroyed && ground->object != nullptr && ground->object->entity != nullptr && ground->object->entity->target_type == 2;
            return (ground_ok && (this->loadedWeaponMask() & 0xFC) != 0) ? 5 : 0;
        }
        default:
            return 0;
    }
    return std::max(0, std::min(9, score));
}

bool SCAIBrain::runTournament() {
    static const int fixed_ids[8] = {20, 14, 15, 16, 21, 7, 19, 4};
    int ids[25];
    int weights[25];
    int count = 8;
    for (int i = 0; i < 8; i++) {
        ids[i] = fixed_ids[i];
        weights[i] = 0;
    }
    for (auto &entry : owner->profile->ai.mvrs) {
        bool fixed = false;
        for (int i = 0; i < 8; i++) {
            if (ids[i] == entry.node_id) {
                weights[i] = entry.value;
                fixed = true;
            }
        }
        if (!fixed && count < 25) {
            ids[count] = entry.node_id;
            weights[count] = entry.value;
            count++;
        }
    }
    int best = -1000;
    int best_id = 0;
    int best_hint = 0;
    uint8_t best_bits = 0;
    Vector3D best_point = {0.0f, 0.0f, 0.0f};
    for (int i = 0; i < count; i++) {
        maneuver_phase_hint = 0;
        maneuver_bits = 0;
        int score = this->scoreManeuver(ids[i], air_target);
        if (score == 0) {
            continue;
        }
        int total = score + weights[i] + ((std::rand() & 1) ? 1 : -1);
        if (total > best) {
            best = total;
            best_id = ids[i];
            best_hint = maneuver_phase_hint;
            best_bits = maneuver_bits;
            best_point = maneuver_point;
        }
    }
    if (best_id == 0) {
        return false;
    }
    maneuver_phase_hint = best_hint;
    maneuver_bits = best_bits;
    maneuver_point = best_point;
    this->applyManeuver(best_id, air_target, REACT_NONE);
    printf("AI %s#%d maneuver %d wins (score %d)\n", owner->actor_name.c_str(), owner->actor_id, best_id, best);
    return this->tickManeuver();
}

void SCAIBrain::applyManeuver(int id, SCMissionActors *target, uint8_t level) {
    maneuver_id = id;
    maneuver_target = target;
    maneuver_level = level;
    maneuver_timer = 2.0f;
    maneuver_phase = 0;
    maneuver_legs = 0;
    maneuver_start_heading = mvCompass(owner->plane->forward);
    maneuver_uses[id]++;
    owner->pilot->disengageAutopilot();
    this->buildCombatContext(target);
    switch (id) {
        case 1: {
            float side = ctx.side >= 0.0f ? 1.0f : -1.0f;
            maneuver_leg = mvRotateHorizontal(owner->plane->forward, 30.0f * side);
            maneuver_leg_timer = 1.0f;
            maneuver_legs = 4;
            break;
        }
        case 2: {
            float stick = owner->pilot->RollStickValue();
            maneuver_side = stick > 3.0f ? 1 : stick < -3.0f ? 0 : (std::rand() & 1);
            break;
        }
        case 3:
            maneuver_timer = 4.0f;
            break;
        case 4:
            maneuver_side = ctx.side >= 0.0f ? 1 : 0;
            break;
        case 5:
            maneuver_timer = 5.0f;
            maneuver_phase = maneuver_phase_hint;
            break;
        case 6:
            maneuver_timer = 5.0f;
            maneuver_phase = maneuver_phase_hint;
            break;
        case 7:
            maneuver_timer = (maneuver_bits & 1) ? 1.0f : 2.0f;
            if ((maneuver_bits & 0x0A) == 0 && !this->tooLow()) {
                maneuver_timer = 4.0f;
            }
            maneuver_bits &= ~0x14;
            break;
        case 13:
            maneuver_timer = 4.0f;
            maneuver_phase = 1;
            break;
        case 16:
            maneuver_timer = 1.5f;
            break;
        default:
            break;
    }
}

void SCAIBrain::endManeuver() {
    if (maneuver_id == 0) {
        return;
    }
    printf("AI %s#%d maneuver %d ends\n", owner->actor_name.c_str(), owner->actor_id, maneuver_id);
    if (maneuver_level != REACT_NONE && reaction_level == maneuver_level) {
        reaction_level = REACT_NONE;
    }
    if (maneuver_id == 19) {
        this->resetGroundAttack();
    }
    maneuver_id = 0;
    maneuver_target = nullptr;
    maneuver_level = REACT_NONE;
}

void SCAIBrain::maneuverSpeed(float wanted) {
    float speed = this->actorVelocity(owner).Length();
    if (maneuver_target != nullptr && maneuver_target->plane != nullptr) {
        if (ctx.dist < 1800.0f) {
            wanted = wanted * (3600.0f - ctx.dist) / 1800.0f;
        }
        if (this->skillCheck(owner->profile->ai.atrb.FL, 0) && speed <= ctx.target_speed) {
            wanted = wanted / 2.0f;
        }
    } else {
        wanted = (float) owner->object->entity->jdyn->ai_speed_cruise;
    }
    this->speedThrottle(wanted);
}

void SCAIBrain::speedThrottle(float wanted) {
    float speed = this->actorVelocity(owner).Length();
    if (speed < wanted * 0.98f) {
        owner->pilot->CmdThrottle(10);
    } else if (speed > wanted * 1.02f) {
        owner->pilot->CmdThrottle(2);
    } else {
        owner->pilot->CmdThrottle(5);
    }
}

bool SCAIBrain::ejectDecision(int mode) {
    float current = 0.0f;
    float initial = 0.0f;
    for (auto &system : owner->object->entity->sysm) {
        auto health_system = owner->plane->system_health.find(system.first);
        for (auto &sub_system : system.second) {
            initial += (float) sub_system.second;
            if (health_system != owner->plane->system_health.end() && health_system->second.find(sub_system.first) != health_system->second.end()) {
                current += (float) health_system->second[sub_system.first];
            } else {
                current += (float) sub_system.second;
            }
        }
    }
    float damage = initial > 0.0f ? 1.0f - current / initial : 0.0f;
    bool stalled = owner->plane->wing_stall > 0;
    bool eject = damage > 0.8f || (mode == 2 && stalled) || (mode == 1 && stalled && owner->plane->y < this->floorAltitude());
    if (!eject) {
        return false;
    }
    SCMissionActors *player = this->playerActor();
    if (player != nullptr && player->team_id == owner->team_id) {
        owner->setMessage(9);
    }
    printf("AI %s#%d ejects (damage %.0f%%)\n", owner->actor_name.c_str(), owner->actor_id, damage * 100.0f);
    owner->plane->ejected = true;
    owner->plane->object->alive = false;
    return true;
}

bool SCAIBrain::tickLeg() {
    owner->pilot->CmdThrottle(10);
    owner->pilot->CmdGuidance(maneuver_leg);
    maneuver_leg_timer -= TICK_DURATION;
    return maneuver_leg_timer > 0.0f;
}

bool SCAIBrain::tickManeuver() {
    if (maneuver_id == 0) {
        return false;
    }
    SCPilot *pilot = owner->pilot;
    pilot->BeginManual();
    maneuver_timer -= TICK_DURATION;
    SCMissionActors *target = maneuver_target;
    bool has_target = target != nullptr && target->plane != nullptr && !target->is_destroyed;
    this->buildCombatContext(has_target ? target : nullptr);
    float cruise = (float) owner->object->entity->jdyn->ai_speed_cruise;
    float flying = (float) owner->profile->ai.atrb.FL;
    float climb = 40.0f * flying * flying / 256.0f + 5.0f;
    bool running = true;
    switch (maneuver_id) {
        case 1:
            if (!has_target || ctx.nose_angle < 60.0f || maneuver_legs <= 0) {
                running = false;
                break;
            }
            if (!this->tickLeg()) {
                maneuver_legs--;
                maneuver_leg = mvRotateHorizontal(owner->plane->forward, -60.0f);
                maneuver_leg_timer = 1.0f;
            }
            this->maneuverSpeed(cruise);
            break;
        case 2: {
            if (maneuver_timer <= 0.0f) {
                running = false;
                break;
            }
            Vector3D reference = owner->mission->intel.unknown_vector;
            reference.Normalize();
            Vector3D delta = reference - owner->plane->forward;
            Matrix &m = owner->plane->ptw;
            float up[3] = {m.v[1][0], m.v[1][1], m.v[1][2]};
            float d[3] = {delta.x, delta.y, delta.z};
            int axis = 0;
            for (int i = 1; i < 3; i++) {
                if (std::fabs(d[i]) > std::fabs(d[axis])) {
                    axis = i;
                }
            }
            bool same = (up[axis] >= 0.0f) == (d[axis] >= 0.0f);
            pilot->CmdPitchStick(same ? 16.0f : -8.0f);
            pilot->CmdRollStick(maneuver_side == 1 ? 16.0f : -16.0f);
            this->maneuverSpeed(cruise);
            break;
        }
        case 3:
            if (maneuver_timer <= 0.0f && maneuver_phase != 6) {
                maneuver_phase = 6;
            }
            switch (maneuver_phase) {
                case 0: {
                    bool slow = this->tooSlow();
                    bool low = this->tooLow();
                    if (slow && !low) {
                        maneuver_phase = 2;
                    } else if (slow && low) {
                        maneuver_phase = 1;
                    } else if (!slow && low) {
                        maneuver_phase = 4;
                    } else {
                        int pick = std::rand() & 3;
                        maneuver_phase = pick == 1 ? 2 : pick == 2 ? 4 : 1;
                    }
                    if (maneuver_phase == 1) {
                        maneuver_legs++;
                        float magnitude = (maneuver_legs & 1) ? 32.0f : 64.0f;
                        maneuver_leg = mvRotateHorizontal(owner->plane->forward, (maneuver_legs & 2) ? -magnitude : magnitude);
                        maneuver_leg_timer = 1.0f;
                    }
                    break;
                }
                case 1:
                    if (!this->tickLeg()) {
                        maneuver_phase = 0;
                    }
                    break;
                case 2:
                    this->speedThrottle(cruise);
                    if (pilot->CmdRollTo(180.0f, 5.0f)) {
                        pilot->CmdPitchStick(16.0f);
                    }
                    if (pilot->NosePitch() >= -30.0f) {
                        maneuver_phase = 3;
                    }
                    break;
                case 3:
                    if (this->tooLow() || this->actorVelocity(owner).Length() >= cruise) {
                        if (pilot->CmdPitchTo(5.0f, 5.0f)) {
                            maneuver_phase = 0;
                        }
                    } else {
                        pilot->CmdPitchTo(-climb, 5.0f);
                    }
                    break;
                case 4:
                    if (pilot->CmdRollTo(0.0f, 5.0f)) {
                        pilot->CmdPitchStick(16.0f);
                    }
                    if (pilot->NosePitch() >= 30.0f) {
                        maneuver_phase = 5;
                    }
                    break;
                case 5:
                    if (owner->plane->y > this->floorAltitude() + 3.0f * 200.0f || this->tooSlow()) {
                        if (pilot->CmdPitchTo(-5.0f, 5.0f)) {
                            maneuver_phase = 0;
                        }
                    } else {
                        pilot->CmdPitchTo(climb, 5.0f);
                    }
                    break;
                default:
                    this->speedThrottle(cruise);
                    if (pilot->CmdPitchTo(0.0f, 5.0f)) {
                        running = false;
                    }
                    break;
            }
            break;
        case 4: {
            float turned = std::fabs(mvWrap180(mvCompass(owner->plane->forward) - maneuver_start_heading));
            if (maneuver_timer < -2.0f) {
                running = false;
                break;
            }
            if (turned >= 90.0f || maneuver_timer <= 0.0f) {
                if (pilot->CmdPitchTo(10.0f, 5.0f)) {
                    running = false;
                }
                break;
            }
            pilot->CmdThrottle(10);
            float bank = this->tooLow() ? 60.0f : 90.0f;
            if (pilot->CmdRollTo(maneuver_side == 1 ? bank : -bank, maneuver_phase == 0 ? 30.0f : 5.0f)) {
                maneuver_phase = 1;
                pilot->CmdPitchStick(16.0f);
            }
            break;
        }
        case 5:
        case 6: {
            if (maneuver_timer <= 0.0f) {
                running = false;
                break;
            }
            this->speedThrottle(cruise);
            Vector3D direction = has_target ? ctx.D : owner->plane->forward;
            switch (maneuver_phase) {
                case 1:
                    if (maneuver_id == 5) {
                        if (owner->plane->y > (float) owner->mission->intel.unknown_range) {
                            pilot->CmdPitchTo(-30.0f, 5.0f);
                        } else {
                            maneuver_phase = 2;
                        }
                    } else {
                        if (pilot->CmdPitchTo(30.0f, 5.0f) && owner->plane->y > this->floorAltitude() + 2000.0f) {
                            maneuver_phase = 2;
                        } else if (owner->plane->y > this->floorAltitude() + 2000.0f) {
                            maneuver_phase = 2;
                        }
                    }
                    break;
                case 2:
                    if (maneuver_id == 5) {
                        pilot->CmdThrottle(10);
                        pilot->CmdPitchTo(this->tooLow() ? 5.0f : -30.0f, 5.0f);
                        if (this->actorVelocity(owner).Length() >= cruise) {
                            maneuver_phase = 3;
                        }
                    } else {
                        this->speedThrottle((float) owner->object->entity->jdyn->ai_speed_min);
                        pilot->CmdPitchTo(20.0f, 5.0f);
                        if (this->actorVelocity(owner).Length() < cruise) {
                            maneuver_phase = 3;
                        }
                    }
                    break;
                case 3:
                    if (maneuver_id == 5) {
                        if (pilot->CmdPitchTo(0.0f, 5.0f)) {
                            maneuver_phase = 4;
                        }
                    } else if (pilot->CmdPitchTo(0.0f, 5.0f) && pilot->CmdRollTo(180.0f, 5.0f)) {
                        maneuver_phase = 4;
                    }
                    break;
                case 4:
                    if (pilot->CmdPitchTo(maneuver_id == 5 ? 90.0f : -90.0f, 5.0f)) {
                        maneuver_phase = 5;
                    }
                    break;
                case 5: {
                    float r = pilot->BearingToRef(direction);
                    pilot->CmdRollTo(pilot->BankAngle() + r, 5.0f);
                    if (std::fabs(r) < 5.0f || this->tooSlow()) {
                        maneuver_phase = 6;
                    }
                    break;
                }
                case 6:
                    pilot->CmdPitchStick(16.0f);
                    if (maneuver_id == 5 ? pilot->NosePitch() <= 45.0f : pilot->NosePitch() >= -45.0f) {
                        maneuver_phase = 7;
                    }
                    break;
                case 7:
                    if (pilot->CmdPitchTo(has_target ? mvElevation(direction) : 0.0f, 5.0f)) {
                        maneuver_phase = 8;
                    }
                    break;
                default:
                    if (pilot->CmdRollTo(0.0f, 5.0f)) {
                        running = false;
                    }
                    break;
            }
            break;
        }
        case 7: {
            if (!has_target || maneuver_timer <= 0.0f) {
                running = false;
                break;
            }
            if ((maneuver_bits & 2) && ctx.aspect >= 80.0f) {
                maneuver_bits |= 4;
            }
            if (maneuver_bits & 4) {
                Vector3D point = owner->plane->position + maneuver_point;
                Vector3D direction = point - owner->plane->position;
                if (!this->tooSlow()) {
                    float horizontal = sqrtf(direction.x * direction.x + direction.z * direction.z);
                    direction.y = horizontal;
                }
                pilot->CmdGuidance(direction);
                this->speedThrottle((float) owner->object->entity->jdyn->ai_speed_min);
            } else if (maneuver_bits & 0x10) {
                Vector3D direction = maneuver_point - owner->plane->position;
                if (ctx.nose_angle < 45.0f) {
                    direction = ctx.D;
                }
                pilot->CmdGuidance(direction);
                this->maneuverSpeed((float) owner->object->entity->jdyn->ai_speed_max);
            } else {
                if (maneuver_bits & 8) {
                    float threshold = 2.0f * ctx.target_speed * ctx.target_speed / 9.0f;
                    if (ctx.dist <= threshold) {
                        maneuver_timer = 4.0f;
                        maneuver_bits |= 0x10;
                        maneuver_point = target->plane->position + this->actorVelocity(target) * 4.0f;
                    }
                } else if (ctx.nose_angle <= 60.0f && ctx.aspect < 60.0f) {
                    maneuver_bits |= 8;
                }
                pilot->CmdGuidance(ctx.D);
                this->maneuverSpeed((float) owner->object->entity->jdyn->ai_speed_max);
            }
            break;
        }
        case 13: {
            if (!has_target) {
                running = false;
                break;
            }
            float horizontal = sqrtf(ctx.D.x * ctx.D.x + ctx.D.z * ctx.D.z);
            if (maneuver_phase == 1) {
                if (horizontal < (float) owner->mission->intel.range_long) {
                    running = false;
                    break;
                }
                Vector3D direction = ctx.D;
                direction.y = 0.0f;
                pilot->CmdGuidance(direction);
                pilot->CmdThrottle(10);
                if (std::fabs(ctx.side) < 5.0f) {
                    maneuver_phase = 2;
                }
            } else if (maneuver_phase == 2) {
                if (this->tooSlow()) {
                    if (pilot->CmdPitchTo(0.0f, 5.0f)) {
                        pilot->CmdRollTo(0.0f, 5.0f);
                    }
                } else {
                    maneuver_phase = 3;
                }
            } else if (maneuver_phase == 3) {
                maneuver_timer -= TICK_DURATION;
                float min_speed = (float) owner->object->entity->jdyn->ai_speed_min;
                float angle = std::min(60.0f, 30.0f + 30.0f * (ctx.ias - cruise) / min_speed);
                pilot->CmdPitchTo(angle, 5.0f);
                pilot->CmdThrottle(10);
                if (maneuver_timer < 0.0f || this->tooSlow() || horizontal < (float) owner->mission->intel.range_long) {
                    maneuver_phase = 4;
                }
            } else {
                this->speedThrottle(cruise);
                if (pilot->CmdPitchTo(0.0f, 5.0f)) {
                    running = false;
                }
            }
            break;
        }
        case 14: {
            if (this->ejectDecision(2) || maneuver_timer < 0.0f) {
                running = false;
                break;
            }
            float vertical_speed = this->actorVelocity(owner).y;
            if (vertical_speed > 0.0f && owner->plane->y > this->floorAltitude()) {
                pilot->CmdPitchTo(30.0f, 10.0f);
                pilot->CmdThrottle(10);
                running = false;
                break;
            }
            bool nose_down = pilot->NosePitch() < 0.0f;
            pilot->CmdThrottle(nose_down && ctx.ias > ctx.min_speed ? 1 : 10);
            pilot->CmdPitchTo(30.0f, 10.0f);
            break;
        }
        case 15: {
            if (maneuver_timer < 0.0f || this->ejectDecision(1)) {
                running = false;
                break;
            }
            bool stalled = owner->plane->wing_stall > 0;
            if (pilot->NosePitch() < 0.0f && !stalled) {
                running = false;
                break;
            }
            if (pilot->NosePitch() > 60.0f && stalled) {
                pilot->CmdThrottle(0);
            } else {
                pilot->CmdThrottle(10);
                if (!stalled) {
                    pilot->CmdPitchTo(-30.0f, 10.0f);
                }
            }
            break;
        }
        case 16: {
            float min_speed = (float) owner->object->entity->jdyn->ai_speed_min;
            if (maneuver_timer <= 0.0f || ctx.ias > (cruise + min_speed) / 2.0f) {
                running = false;
                break;
            }
            this->speedThrottle((float) owner->object->entity->jdyn->ai_speed_max);
            pilot->CmdPitchTo(5.0f, 5.0f);
            break;
        }
        case 19:
            if (ground_target == nullptr) {
                running = false;
                break;
            }
            pilot->ClearGuidance();
            this->updateGroundAttack(ground_target);
            running = ground_attack_active;
            break;
        default:
            running = false;
            break;
    }
    if (!running) {
        this->endManeuver();
        owner->pilot->ClearGuidance();
    } else if (debug_ticks % 25 == 0) {
        printf("AI %s#%d maneuver %d phase=%d timer=%.1f bank=%.0f pitch=%.0f\n", owner->actor_name.c_str(), owner->actor_id, maneuver_id, maneuver_phase, maneuver_timer, pilot->BankAngle(), pilot->NosePitch());
    }
    return running;
}

void SCAIBrain::runReflexes() {
    if (maneuver_id != 0 || owner->plane->on_ground || owner->pilot->autopilotActive()) {
        return;
    }
    if (reaction_level <= REACT_STALL_RECOVERY && this->scoreManeuver(15, nullptr) > 0) {
        this->applyManeuver(15, nullptr, REACT_STALL_RECOVERY);
        reaction_level = REACT_STALL_RECOVERY;
        printf("AI %s#%d stall recovery reflex\n", owner->actor_name.c_str(), owner->actor_id);
        return;
    }
    if (reaction_level <= REACT_GROUND_AVOID && this->scoreManeuver(14, nullptr) > 0) {
        this->applyManeuver(14, nullptr, REACT_GROUND_AVOID);
        reaction_level = REACT_GROUND_AVOID;
        printf("AI %s#%d ground avoid reflex\n", owner->actor_name.c_str(), owner->actor_id);
    }
}

void SCAIBrain::incomingThreatWarning() {
    if (owner->plane->on_ground || reaction_level > REACT_ENGAGED || maneuver_id == 4 || threat_alerted) {
        if (maneuver_id != 4) {
            threat_alerted = false;
        }
        return;
    }
    if (owner->profile->ai.atrb.FL < 13) {
        return;
    }
    for (auto actor : owner->mission->actors) {
        if (actor->team_id == owner->team_id || actor->plane == nullptr || actor->is_destroyed || actor->brain == nullptr || actor->brain->air_target != owner) {
            continue;
        }
        Vector3D delta = actor->plane->position - owner->plane->position;
        if (delta.Length() > owner->mission->intel.range_gun) {
            continue;
        }
        Vector3D closing = this->actorVelocity(actor) - this->actorVelocity(owner);
        if (closing.x * delta.x + closing.y * delta.y + closing.z * delta.z >= 0.0f) {
            continue;
        }
        if (!this->skillCheck(owner->profile->ai.atrb.FL, 0)) {
            return;
        }
        this->endManeuver();
        this->applyManeuver(4, actor, REACT_NONE);
        threat_alerted = true;
        printf("AI %s#%d threat warning from %s: defensive turn\n", owner->actor_name.c_str(), owner->actor_id, actor->actor_name.c_str());
        return;
    }
}
