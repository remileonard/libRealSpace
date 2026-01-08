#include "precomp.h"
#include "SCMissionActors.h"
#include <cstdlib>
#include <ctime>

bool SCMissionActors::wait(int seconds) {
    if (this->current_command != prog_op::OP_SET_WAIT_FOR_SECONDS) {
        if (this->wait_timer == 0) {
            if (this->plane != nullptr && this->plane->tps > 0) {
                this->wait_timer = seconds * this->plane->tps;
            } else {
                this->wait_timer = seconds * 60;
            }
                
        }
    }
    if (this->wait_timer > 0) {
        this->wait_timer--;
    }
    if (this->wait_timer <= 0) {
        this->wait_timer = -1;
        return true;
    }
    return false;
}

bool SCMissionActors::execute() { return true; }
/**
 * SCMissionActors::takeOff
 *
 * Called when the "take off" mission objective is triggered.
 *
 * If the actor has already taken off, this function does nothing and
 * returns true.
 *
 * Otherwise, this function sets the actor's current objective to
 * OP_SET_OBJ_TAKE_OFF and sets the pilot's target climb to 300 units above
 * the current height.  If the actor is close enough to the target height,
 * the actor is marked as having taken off and the function returns true.
 *
 * @param arg Unused argument.
 *
 * @return True if the actor has taken off, false otherwise.
 */
bool SCMissionActors::takeOff(uint8_t arg) {
    if (taken_off) {
        return true;
    }
    this->current_objective = OP_SET_OBJ_TAKE_OFF;
    if (this->pilot->target_climb == 0) {
        this->pilot->target_speed = -15;
        this->pilot->target_climb = (int) (this->plane->y + 300.0f);
        this->pilot->target_azimut = this->plane->yaw;
    }
    if (std::abs(this->plane->y-this->pilot->target_climb) < 10.0f) {
        this->taken_off = true;
    }
    return this->taken_off;
}
bool SCMissionActors::land(uint8_t arg) {
    this->current_objective = OP_SET_OBJ_LAND;
    auto it = std::find(this->mission->friendlies.begin(), this->mission->friendlies.end(), this);
    if (it != this->mission->friendlies.end()) {
        this->mission->friendlies.erase(it);
    }
    if (arg < this->mission->mission->mission_data.spots.size()) {
        SPOT *wp = this->mission->mission->mission_data.spots[arg];
        this->pilot->SetTargetWaypoint(wp->position);
        this->pilot->target_speed = -10;
        Vector3D position = {this->plane->x, this->plane->y, this->plane->z};
        Vector3D diff = wp->position - position;
        float dist = diff.Length();
        const float landing_dist = 3000.0f;
        if (dist < landing_dist) {
            this->pilot->turning = false;
            this->pilot->land = true;
            this->pilot->target_speed = (int) (-10.0f * (dist/landing_dist));
        }
        if (dist < 2000.0f) {
            return true;
        }
    }
    return false;
}
bool SCMissionActors::flyToWaypoint(uint8_t arg) {
    this->current_objective = OP_SET_OBJ_FLY_TO_WP;
    if (arg < this->mission->mission->mission_data.spots.size()) {
        SPOT *wp = this->mission->mission->mission_data.spots[arg];
        if (this->pilot != nullptr) {
            this->pilot->SetTargetWaypoint(wp->position);
            this->pilot->target_speed = -10;
            Vector3D position = {this->plane->x, this->plane->y, this->plane->z};
            Vector3D diff = wp->position - position;
            float dist = diff.Length();
            if (dist < 5000.0f) {
                return true;
            }
        }
    }
    
    return false;
}
bool SCMissionActors::flyToArea(uint8_t arg) {
    this->current_objective = OP_SET_OBJ_FLY_TO_AREA;
    SPOT *wp = nullptr;
    for (auto spot: this->mission->mission->mission_data.spots) {
        if (spot->id == arg) {
            wp = spot;
            break;
        }
    }
    
    if (wp != nullptr) {
        int area_id = this->mission->getAreaID({wp->position.x, wp->position.y, wp->position.z});

        AREA *area = nullptr;
        for (auto area_test: this->mission->mission->mission_data.areas) {
            if (area_test->id == area_id) {
                area = area_test;
                break;
            }
        }
        this->pilot->SetTargetWaypoint(area->position);
        this->pilot->target_speed = -10;
        Vector3D position = {this->plane->x, this->plane->y, this->plane->z};
        Vector3D diff = area->position - position;
        float dist = diff.Length();
        if (dist < 3000.0f) {
            return true;
        }
    }
    return false;
}
/**
 * SCMissionActors::destroyTarget
 *
 * Called when the "destroy target" mission objective is triggered.
 *
 * This function sets the actor's current objective to
 * OP_SET_OBJ_DESTROY_TARGET and sets the pilot's target waypoint to the
 * position of the target actor.  If the target actor is not on the ground,
 * the pilot's target climb is set to the target actor's current height and
 * the target speed is set to either -60 or the target actor's current
 * vertical speed, depending on how far away the actor is.
 *
 * If the target actor is already destroyed, this function returns true.
 *
 * @param arg The ID of the target actor to destroy.
 *
 * @return True if the target actor is already destroyed, false otherwise.
 */
bool SCMissionActors::destroyTarget(uint8_t arg) {
    Vector3D wp;
    bool should_talk = false;
    bool is_new_target = false;
    bool is_ground_target = false;
    if (this->pilot == nullptr) {
        return false;
    }
    if (this->target == nullptr) {
        should_talk = true;
    }
    if (this->current_target == 0) {
        this->current_target = arg;
        is_new_target = true;
    }
    Vector3D position;
    if (this->plane == nullptr) {
        position = {this->object->position.x, this->object->position.y, this->object->position.z};
    } else {
        position = {this->plane->x, this->plane->y, this->plane->z};
    }
    if (this->target == nullptr) {
        for (auto actor: this->mission->actors) {
            if (actor->actor_id == this->current_target) {
                this->target = actor;
                actor->attacker = this;
                break;
            }
        }
    }
    if (this->current_target != 0 && this->target != nullptr && this->target->plane != nullptr && this->target->plane->object->alive == 0) {
        this->current_target = 0;
        this->target->attacker = nullptr;
        this->target = nullptr;
        this->target_position.Clear();
        if (!is_new_target && this->current_objective == OP_SET_OBJ_DESTROY_TARGET) {
            std::srand(static_cast<unsigned int>(std::time(nullptr)));
            int talking = std::rand() % 16;
            if (talking >= this->profile->ai.atrb.VB) {
                this->setMessage(13);
            }
        }
        return true;
    }
    this->current_objective = OP_SET_OBJ_DESTROY_TARGET;
    uint8_t area_id = this->mission->getAreaID(position);
    if (this->target != nullptr) {
        SCMissionActors *actor = this->target;
        if (actor->plane != nullptr) {
            wp.x = actor->plane->x;
            wp.y = actor->plane->y;
            wp.z = actor->plane->z;
            wp = wp + actor->attack_pos_offset;
            if (target_position.Length() == 0.0f) {
                target_position = wp;
                target_position_update = 2000;
            }
            Vector3D target_position_diff = target_position - wp;
            if (target_position_diff.Length() > 0.0f && target_position_update == 0) {
                target_position = wp;
                target_position_update = 2000;
            } else if (target_position_diff.Length() > 0.0f) {
                target_position_update -= 1;
            }
            this->pilot->SetTargetWaypoint(wp);
            Vector3D diff = wp - position;
            float dist = diff.Length();
            if (!actor->plane->on_ground) {
                Vector3D real_pos = {actor->plane->x, actor->plane->y, actor->plane->z};
                Vector3D real_dist = real_pos - position;
                float real_distance = real_dist.Length();
                int hpt_id = 0;
                int attack_range = 0;
                int max_weap = 1;
                for (auto weap : this->plane->weaps_load) {
                    if (weap == nullptr) {
                        hpt_id++;
                        continue;
                    }
                    int effective_range = weap->objct->wdat->effective_range + 500 * (this->profile->ai.atrb.AA / 16);
                    if (effective_range >= real_distance) {
                        if (weap->nb_weap > 0) {
                            attack_range = weap->objct->wdat->target_range;
                            if (current_weapon_index != weap->objct->wdat->weapon_category) {
                                current_weapon_index = weap->objct->wdat->weapon_category;
                                this->plane->wp_cooldown = 0;
                            }
                            if (weap->objct->wdat->weapon_category==0) {
                                max_weap = 40;
                            }
                            break;
                        }
                    }
                    hpt_id++;
                }
                this->pilot->target_climb = (int) wp.y;
                if (dist > attack_range - 1000.0f) {
                    this->pilot->target_speed = -60;
                } else if (dist < attack_range - 300.0f) {
                    this->pilot->target_speed = (int) actor->plane->vz;
                    // Calculate azimuth between plane and target
                    float target_azimuth = 0.0f;
                    if (actor->plane != nullptr) {
                        Vector3D target_dir = {actor->plane->x - this->plane->x,
                                              0.0f,  // Ignore Y for horizontal azimuth
                                              actor->plane->z - this->plane->z};
                        // Calculate the angle in radians and convert to degrees
                        target_azimuth = atan2f(target_dir.z, target_dir.x) * 180.0f / M_PI;
                        target_azimuth -= 360.0f;
                        target_azimuth += 90.0f;
                        if (target_azimuth > 360.0f) {
                            target_azimuth -= 360.0f;
                        }
                        while (target_azimuth < 0.0f) {
                            target_azimuth += 360.0f;
                        }
                        target_azimuth = 360.0f - target_azimuth;;
                        float target_diff = target_azimuth - (this->plane->yaw/10.0f);
                        while (target_diff > 180.0f) target_diff -= 360.0f;
                        while (target_diff < -180.0f) target_diff += 360.0f;
     
                        // Only shoot if target is within firing arc
                        if (std::abs(target_diff) < 30.0f) {
                            if (this->plane->weaps_object.size() < max_weap) {
                                int should_shoot = std::rand() % 16;
                                if (should_shoot <= this->profile->ai.atrb.TH) {
                                    this->plane->Shoot(hpt_id, actor, this->mission);
                                }
                                
                            }
                        }    
                    }
                }
            }
        } else {
            wp.x = actor->object->position.x;
            wp.y = this->plane->y;
            wp.z = actor->object->position.z;
            this->pilot->SetTargetWaypoint(wp);
            this->pilot->target_speed = -10;
            is_ground_target = true;
        }
        if (should_talk) {
            std::srand(static_cast<unsigned int>(std::time(nullptr)));
            int talking = std::rand() % 16;
            if (talking >= this->profile->ai.atrb.VB) {
                if (is_ground_target) {
                    this->setMessage(11);
                } else {
                    this->setMessage(12);
                }
            }
        }
    }
    return false;
}
/**
 * SCMissionActors::defendTarget
 *
 * Called when the "defend target" mission objective is triggered.
 *
 * This function sets the actor's current objective to OP_SET_OBJ_DEFEND_TARGET.
 * It first checks if there is an existing goal to destroy a target. If so,
 * it attempts to destroy that target and resets the goal if successful.
 *
 * If no current goal exists, the function determines the area in which the actor is located
 * and checks for any enemies within the same area. If an enemy is found, it sets the goal
 * to destroy that enemy and attempts to do so.
 *
 * @param arg Unused argument.
 *
 * @return True if no action is taken or if the current goal is successfully completed,
 *         false otherwise.
 */

bool SCMissionActors::defendTarget(uint8_t arg) {
    if (this->current_target != 0) {
        bool ret = this->destroyTarget(this->current_target);
        if (ret) {
            this->current_target = 0;
        }
        return ret;
    }
    this->current_objective = OP_SET_OBJ_DEFEND_TARGET;
    Vector3D position = {this->plane->x, this->plane->y, this->plane->z};
    for (auto actor: this->mission->enemies) {
        if (actor->plane != nullptr) {
            if (actor->is_active && actor->target != nullptr && actor->target->actor_id == arg) {
                if (actor->plane != nullptr && actor->plane->object->alive == 0) {
                    continue;
                }
                this->current_target = actor->actor_id;
                this->target = actor;
                bool ret = this->destroyTarget(actor->actor_id);
                if (ret) {
                    this->current_target = 0;
                }
                return ret;
            }
        }
    }
    for (auto actor: this->mission->enemies) {
        if (actor->plane != nullptr) {
            if (actor->is_active && actor->target != nullptr && actor->target->actor_id == this->actor_id) {
                if (actor->plane != nullptr && actor->plane->object->alive == 0) {
                    continue;
                }
                this->current_target = actor->actor_id;
                bool ret = this->destroyTarget(actor->actor_id);
                if (ret) {
                    this->current_target = 0;
                }
                return ret;
            }
        }
    }
    return this->followAlly(arg);
}
bool SCMissionActors::defendArea(uint8_t arg) { 
    this->current_objective = OP_SET_OBJ_DEFEND_AREA;
    if (this->current_target != 0) {
        bool ret = this->destroyTarget(this->current_target);
        if (ret) {
            this->current_target = 0;
        }
        return ret;
    }
    Vector3D position = {this->plane->x, this->plane->y, this->plane->z};
    SPOT *wp = nullptr;
    for (auto spot: this->mission->mission->mission_data.spots) {
        if (spot->id == arg) {
            wp = spot;
            break;
        }
    }
    
    if (wp != nullptr) {
        int area_id = this->mission->getAreaID({wp->position.x, wp->position.y, wp->position.z});
        Vector3D position = {this->plane->x, this->plane->y, this->plane->z};
        int test_area_id = this->mission->getAreaID(position);
        if (area_id != test_area_id) {
            this->current_target = 0;
            return this->flyToArea(arg);
        }
        if (this->current_target == 0) {
            bool is_enemy = false;
            for (auto actor: this->mission->enemies) {
                if (actor->plane != nullptr) {
                    if (actor->actor_id == this->actor_id) {
                        is_enemy = true;
                        break;
                    }
                }
            }
            if (is_enemy) {
                for (auto actor: this->mission->friendlies) {
                    if (actor->plane != nullptr) {
                        uint8_t actor_area_id = this->mission->getAreaID({actor->plane->x, actor->plane->y, actor->plane->z});
                        if (actor_area_id == area_id) {
                            this->current_target = actor->actor_id;
                            break;
                        }
                    }
                }
            } else {
                for (auto actor: this->mission->enemies) {
                    if (actor->plane != nullptr) {
                        uint8_t actor_area_id = this->mission->getAreaID({actor->plane->x, actor->plane->y, actor->plane->z});
                        if (actor_area_id == area_id) {
                            this->current_target = actor->actor_id;
                            break;
                        }
                    }
                }
            }
        }
    }
    return true;
}
/**
 * @brief Deactivates an actor in the mission based on the provided actor ID.
 *
 * This function iterates through the list of actors in the mission and sets the
 * `is_active` flag to `false` for the actor whose `actor_id` matches the provided argument.
 *
 * @param arg The ID of the actor to deactivate.
 * @return true if an actor with the specified ID was found and deactivated, false otherwise.
 */
bool SCMissionActors::deactivate(uint8_t arg) {
    for (auto actor: this->mission->actors) {
        if (actor->actor_id == arg) {
            actor->is_active = false;
            return true;
        }
    }
    return false;
}
/**
 * @brief Sets a message for the mission actor.
 * 
 * This function sets a message for the mission actor based on the provided argument.
 * 
 * @param arg The index of the message to be set.
 * @return true Always returns true.
 */
bool SCMissionActors::setMessage(uint8_t arg) {
    if (!this->talkative) {
        return false;
    }
    for (auto message: this->profile->radi.msgs) {
        if (message.first == arg) {
            std::string *message = new std::string();
            *message = this->profile->radi.info.callsign + ": " + this->profile->radi.msgs[arg];
            RadioMessages *msg = new RadioMessages();
            msg->message = *message;
            
            if (this->mission->sound.inGameVoices.size() > 0) {
                
                if (this->mission->sound.inGameVoices.find(this->profile->radi.spch) != this->mission->sound.inGameVoices.end()) {
                    if (this->mission->sound.inGameVoices[this->profile->radi.spch]->messages.find(arg) != this->mission->sound.inGameVoices[this->profile->radi.spch]->messages.end()) {
                        MemSound *message_sound = this->mission->sound.inGameVoices[this->profile->radi.spch]->messages[arg];
                        msg->sound = message_sound;
                    }
                }
                
                
            }
            this->mission->radio_messages.push_back(msg);
            return true;
        }
    }
    return false;    
}
/**
 * @brief Sets the current objective to follow an ally and adjusts the pilot's target waypoint, speed, and climb.
 * 
 * This function iterates through the mission's actors to find the actor with the specified ID.
 * If the actor is found, it sets the waypoint to the actor's position plus the formation position offset,
 * and adjusts the pilot's target waypoint, speed, and climb based on the distance to the waypoint and whether
 * the actor's plane is on the ground.
 * 
 * @param arg The ID of the ally actor to follow.
 * @return true if the ally actor with the specified ID is found and the objective is set, false otherwise.
 */
bool SCMissionActors::followAlly(uint8_t arg) {
    Vector3D wp;
    this->current_objective = OP_SET_OBJ_FOLLOW_ALLY;
    if (this->attacker != nullptr) {
        this->destroyTarget(this->attacker->actor_id);
    }
    for (auto actor: this->mission->actors) {
        if (actor->actor_id == arg) {
            if (actor->is_destroyed || (!actor->is_active && (actor->actor_name != "PLAYER"))) {
                return false; // Actor not found or plane not alive
            }
            if (actor->plane == nullptr) {
                return false;
            }
            wp.x = actor->plane->x;
            wp.y = actor->plane->y;
            wp.z = actor->plane->z;
            wp = wp + actor->formation_pos_offset;
            this->pilot->SetTargetWaypoint(wp);
            Vector3D position = {this->plane->x, this->plane->y, this->plane->z};
            Vector3D diff = wp - position;
            float dist = diff.Length();
            if (!actor->plane->on_ground) {
                this->pilot->target_climb = (int) wp.y;
                if (dist > 1000.0f) {
                    this->pilot->target_speed = -60;
                } else if (dist < 400.0f) {
                    this->pilot->target_speed = (int) actor->plane->vz;
                    this->pilot->turning = false;
                }
            }
            
            return true;
        }
    }
    return false;
}
/**
 * SCMissionActors::ifTargetInSameArea
 *
 * Returns true if the actor with the given ID is in the same area as
 * the current actor, false otherwise.
 *
 * @param arg The ID of the target actor to check.
 *
 * @return True if the target actor is in the same area as the current
 * actor, false otherwise.
 */
bool SCMissionActors::ifTargetInSameArea(uint8_t arg) {
    Vector3D position;
    if (this->plane == nullptr) {
        position = {this->object->position.x, this->object->position.y, this->object->position.z};
    } else {
        position = {this->plane->x, this->plane->y, this->plane->z};
    }
    Uint8 area_id = this->mission->getAreaID(position);
    for (auto actor: this->mission->actors) {
        if (actor->actor_id == arg) {
            if (actor->is_active == true && !actor->is_destroyed) {
                return true;
            }
            /*if (actor->plane == nullptr) {
                return (area_id == actor->object->area_id);
            }
            Uint8 target_area_id = this->mission->getAreaID({actor->plane->x, actor->plane->y, actor->plane->z});
            if (area_id == target_area_id) {
                return true;
            }*/
        }
    }
    return false;
}
bool SCMissionActors::respondToRadioMessage(int message_id, SCMission *mission, SCMissionActors *sender) {
    int cpt = 1;
    float health_remaing = this->health / (float) this->object->entity->health * 100.0f;
    bool obeying = true;
    for (auto ask: this->profile->radi.opts) {
        if (cpt == message_id) {
            switch (ask) {
                case 'd':
                {
                    // request status 2 - 5 
                    
                    if (health_remaing > 75.0f) {
                        this->setMessage(2); // All is well
                    } else if (health_remaing > 50.0f) {
                        this->setMessage(3); // Minor damage
                    } else if (health_remaing > 25.0f) {
                        this->setMessage(4); // Major damage
                    } else {
                        this->setMessage(5); // Critical damage
                    }
                }
                break;
                case 'e':
                    // request take off
                break;
                case 'f':
                    // request land 
                break;
                case 'g':
                {
                    uint8_t area_to_defend = this->mission->getAreaID({this->plane->x, this->plane->y, this->plane->z});
                    this->override_progs.clear();
                    this->override_progs.push_back({prog_op::OP_SET_OBJ_DEFEND_AREA, area_to_defend});
                    this->setMessage(1);
                }
                break;
                case 'h':
                    // build formation
                    this->override_progs.clear();
                    this->setMessage(1);
                break;
                case 'i':
                    // attack my target
                    if (sender != nullptr && sender->target != nullptr) {
                        if (this->current_command == prog_op::OP_SET_OBJ_DEFEND_TARGET || this->current_command == prog_op::OP_SET_OBJ_DESTROY_TARGET) {
                            obeying = std::rand() % 16 < this->profile->ai.atrb.LY;
                        }
                        if (obeying) {
                            this->override_progs.clear();
                            this->override_progs.push_back({prog_op::OP_SET_OBJ_DESTROY_TARGET, sender->target->actor_id});
                            this->setMessage(1);
                        } else {
                            this->setMessage(0);
                        }
                    }
                break;
                case 'j':
                    if (this->current_command == prog_op::OP_SET_OBJ_DEFEND_TARGET || this->current_command == prog_op::OP_SET_OBJ_DESTROY_TARGET || health_remaing > 75.0f) {
                        obeying = std::rand() % 16 < this->profile->ai.atrb.LY;
                    }
                    if (health_remaing < 30.0f) {
                        obeying = true;
                    }
                    if (obeying) {
                        this->override_progs.clear();
                        this->override_progs.push_back({prog_op::OP_SET_OBJ_LAND, 0});
                        this->setMessage(1);
                    } else {
                        this->setMessage(0);
                    }
                    
                break;
                case 'k':
                    if (sender != nullptr) {
                        this->override_progs.clear();
                        this->override_progs.push_back({prog_op::OP_SET_OBJ_DEFEND_TARGET, sender->actor_id});
                        this->setMessage(1);
                    }
                break;
                case 'l':
                    // maintain radio silence
                    this->talkative = false;
                break;
                case 'm':
                    // break radio silence
                    this->talkative = true;
                    this->setMessage(1);
                break;
                default:
                break;
            }
        }
        cpt++;
    }
    return false;
}
bool SCMissionActors::protectSelf() {
    if (this->attacker != nullptr) {
        if (this->attacker->actor_name == "PLAYER") {
            return false;
        }
        if (this->attacker->plane != nullptr) {
            for (auto weap: this->attacker->plane->weaps_object) {
                if (weap->target == this) {
                    // Check if this actor is in the friendlies list
                    bool is_friendly = false;
                    for (auto friendly : this->mission->friendlies) {
                        if (friendly == this) {
                            is_friendly = true;
                            break;
                        }
                    }

                    // If this is a friendly actor, attempt to evade the incoming weapon
                    if (is_friendly) {
                        weap->target = nullptr; // Disengage the weapon from this actor
                        return true;
                    }
                }
            }
        }
    }
    return false;
}
/**
 * @brief Activates the target actor with the specified ID.
 *
 * This function iterates through the list of actors in the mission and sets the
 * `is_active` flag to true for the actor whose `actor_id` matches the provided argument.
 *
 * @param arg The ID of the actor to be activated.
 * @return true if the actor with the specified ID was found and activated, false otherwise.
 */
bool SCMissionActors::activateTarget(uint8_t arg) {
    for (auto actor: this->mission->actors) {
        if (actor->actor_id == arg && actor->is_active == false && actor->is_destroyed == false) {
            actor->is_active = true;
            actor->is_hidden = false;
            Vector3D correction = {0.0f, 0.0f, 0.0f};
            if (actor->object->area_id != 255 && actor->object->unknown2 == 0) {
                bool spot_found = false;
                for (auto spot: this->mission->mission->mission_data.spots) {
                    if (spot->area_id == actor->object->area_id) {
                        correction = spot->position;
                        spot_found = true;
                        break;
                    }
                }
                if (!spot_found) {
                    correction = this->mission->mission->mission_data.areas[actor->object->area_id]->position;
                }
            } else {
                correction = {
                    this->mission->player->plane->x,
                    this->mission->player->plane->y,
                    this->mission->player->plane->z
                };
            }
            if ((actor->object->area_id != 255) || (actor->object->area_id == 255 && actor->object->unknown2 == 1)) {
                actor->object->position += correction;
            }
            float ground_y = this->mission->area->getY(actor->object->position.x, actor->object->position.z);
            if (actor->object->position.y < ground_y) {
                actor->object->position.y = ground_y;
            }
            if (actor->plane != nullptr) {
                if (actor->object->position.y <= ground_y) {
                    actor->object->position.y = ground_y+10.0f;
                    actor->plane->on_ground = true;
                } else {
                    actor->plane->on_ground = false;
                }
                actor->plane->x = actor->object->position.x;
                actor->plane->y = actor->object->position.y;
                actor->plane->z = actor->object->position.z;
            }
            if (actor->plane == nullptr) {
                if (actor->object->position.y > ground_y) {
                    actor->object->position.y = ground_y+2.0f;
                } else if (actor->object->position.y < ground_y) {
                    actor->object->position.y = ground_y+2.0f;
                }
            }
            if (actor->team_id == this->mission->player->team_id) {
                this->mission->friendlies.push_back(actor);
            } else {
                this->mission->enemies.push_back(actor);
            }
            if (actor->on_is_activated.size() > 0) {
                SCProg *p = new SCProg(actor, actor->on_is_activated, this->mission, actor->object->on_is_activated);
                p->execute();
            }
            return true;
        }
    }
    return false;
}

int SCMissionActors::getDistanceToTarget(uint8_t arg) {
    Vector3D position;
    if (this->plane == nullptr) {
        position = {this->object->position.x, this->object->position.y, this->object->position.z};
    } else {
        position = {this->plane->x, this->plane->y, this->plane->z};
    }
    Vector3D diff;
    for (auto actor: this->mission->actors) {
        if (actor->actor_id == arg) {
            if (actor->plane == nullptr) {
                diff = actor->object->position - position;
            } else {
                Vector3D target_pos = {actor->plane->x, actor->plane->y, actor->plane->z};
                diff = target_pos - position;
            }
            break;
        }
    }
    return (int) diff.Length()/1000;
}

int SCMissionActors::getDistanceToSpot(uint8_t arg) { 
    Vector3D position = this->mission->mission->mission_data.spots[arg]->position;
    Vector3D plane_pos = {this->plane->x, this->plane->y, this->plane->z};
    Vector3D diff = plane_pos - position;
    return (int) diff.Length()/1000;
}
void SCMissionActors::shootWeapon(SCMissionActors *target) {
    static int shoot_cooldown = 0;
    if (shoot_cooldown > 0) {
        shoot_cooldown--;
        return;
    }
    if (this->object->entity->entity_type != EntityType::swpn) {
        return;
    }
    if (this->object->entity->swpn_data == nullptr) {
        return;
    }
    if (this->object->entity->swpn_data->weapon_entity == nullptr) {
        return;
    }
    
    if (this->object->entity->swpn_data->max_simultaneous_shots > 0 && this->weapons_shooted.size() >= this->object->entity->swpn_data->max_simultaneous_shots) {
        return;
    }
    if (this->object->entity->swpn_data->weapons_round <= 0) {
        return; // No ammo left
    }
    RSEntity *weapon_entity = this->object->entity->swpn_data->weapon_entity;
    Vector3D target_position = {0.0f, 0.0f, 0.0f};
    if (target->plane != nullptr) {
        target_position = {target->plane->x, target->plane->y, target->plane->z};
    } else if (target->object != nullptr) {
        target_position = target->object->position;
    }
    Vector3D direction = target_position - this->object->position;
    float distance = direction.Length();
    
    if (distance> weapon_entity->wdat->target_range) {
        return; // No direction to shoot
    }
    this->aiming_vector = direction;
    direction.Normalize();
    float p_y = this->object->position.y;
    if (this->mission->area->getY(this->object->position.x, this->object->position.z) > this->object->position.y) {
        this->object->position.y = this->mission->area->getY(this->object->position.x, this->object->position.z)+10.0f;
    }
    SCSimulatedObject *weapon = nullptr;

    switch (weapon_entity->wdat->weapon_category) {
        case 2: // Missiles
            if (this->weapons_shooted.size()> 1) {
                return; // Too many weapons already shot
            }
            if (weapon_entity->wdat->effective_range == 0) {
                weapon_entity->wdat->effective_range = weapon_entity->wdat->target_range;
            }
            weapon = new SCSimulatedObject();
            weapon->target = target;
            weapon->obj = weapon_entity;
            weapon->x = this->object->position.x;
            weapon->y = p_y;
            weapon->z = this->object->position.z;
            shoot_cooldown = 160;
            
            weapon->vx = direction.x * weapon_entity->dynn_miss->velovity_m_per_sec / 80.0f;
            weapon->vy = direction.y * weapon_entity->dynn_miss->velovity_m_per_sec / 80.0f;
            weapon->vz = direction.z * weapon_entity->dynn_miss->velovity_m_per_sec / 80.0f;
            break;
        case 0: // Guns
            if (this->weapons_shooted.size()> 30) {
                return; // Too many weapons already shot
            }
            weapon = new GunSimulatedObject();
            shoot_cooldown = 30;
            weapon->obj = weapon_entity;
            weapon->x = this->object->position.x;
            weapon->y = p_y;
            weapon->z = this->object->position.z;

            
            weapon->vx = direction.x *  1000.0f;
            weapon->vy = direction.y *  1000.0f;
            weapon->vz = direction.z *  1000.0f;
            break;
        default:
            return;
    }
    weapon->mission = this->mission;
    weapon->shooter = this;
    this->object->entity->swpn_data->weapons_round--;
    this->weapons_shooted.push_back(weapon);
}
void SCMissionActors::hasBeenHit(SCSimulatedObject *weapon, SCMissionActors *attacker) {
    int damage = 10;
    this->health = 0;
    if (this->health <= 0 && this->object->alive) {
        if (this->profile != nullptr && this->profile->radi.msgs.size() > 0) {
            std::srand(std::time(0));
            int r = std::rand() % 16;
            if (r<=this->profile->ai.atrb.VB) {
                int message = std::rand() % 4;
                this->setMessage(23 + message); // Bail out messages
            }
        }
    }
    this->object->alive = false;
    if (this->object->entity->explos != nullptr) {
        SCExplosion *explosion = new SCExplosion(this->object->entity->explos->objct, this->object->position);
        this->mission->explosions.push_back(explosion);
        if (this->mission->sound.sounds.size() > 0) {   
            MemSound *sound;
            if (weapon->obj->entity_type == EntityType::tracer) {
                sound = this->mission->sound.sounds[SoundEffectIds::GUN_IMPACT_1];
            } else {
                sound = this->mission->sound.sounds[SoundEffectIds::EXPLOSION_1];
            }
            RSMixer::getInstance().playSoundVoc(sound->data, sound->size);
        }
    }
    attacker->score += 100;
    if (this->plane != nullptr) {
        attacker->plane_down += 1;
    } else {
        attacker->ground_down += 1;
    }
    // If the actor is destroyed, process it
   
}
/**
 * SCMissionActorsPlayer::takeOff
 *
 * Sets the current objective to a take-off objective with the given
 * argument as the target spot ID.
 *
 * @param arg The ID of the target spot to take off from.
 *
 * @return True if the objective was set successfully, false otherwise.
 */
bool SCMissionActorsPlayer::takeOff(uint8_t arg) {
    SCMissionWaypoint *waypoint = new SCMissionWaypoint();
    waypoint->spot = this->mission->mission->mission_data.spots[arg];
    waypoint->objective = new std::string("take off");
    this->mission->waypoints.push_back(waypoint);
    return true;
}
/**
 * SCMissionActorsPlayer::land
 *
 * Sets the current objective to a land objective with the given
 * argument as the target spot ID.
 *
 * @param arg The ID of the target spot to land on.
 *
 * @return True if the objective was set successfully, false otherwise.
 */
bool SCMissionActorsPlayer::land(uint8_t arg) {
    SCMissionWaypoint *waypoint = new SCMissionWaypoint();
    waypoint->spot = this->mission->mission->mission_data.spots[arg];
    waypoint->objective = new std::string("landing");
    this->mission->waypoints.push_back(waypoint);
    return true;
}
/**
 * Sets the current objective to a fly-to-waypoint objective with the given
 * argument as the target waypoint ID.
 *
 * @param arg The ID of the target waypoint to fly to.
 *
 * @return True if the objective was set successfully, false otherwise.
 */
bool SCMissionActorsPlayer::flyToWaypoint(uint8_t arg) {
    SCMissionWaypoint *waypoint = new SCMissionWaypoint();
    if (arg >= this->mission->mission->mission_data.spots.size()) {
        return false; // Invalid waypoint ID
    }
    waypoint->spot = this->mission->mission->mission_data.spots[arg];
    waypoint->objective = new std::string("Fly to\nWay Point");
    this->mission->waypoints.push_back(waypoint);
    return true;
}
/**
 * SCMissionActorsPlayer::flyToArea
 *
 * Sets the current objective to a fly-to-waypoint objective with the given
 * argument as the target waypoint ID.
 *
 * @param arg The ID of the target waypoint to fly to.
 *
 * @return True if the objective was set successfully, false otherwise.
 */
bool SCMissionActorsPlayer::flyToArea(uint8_t arg) {
    SCMissionWaypoint *waypoint = new SCMissionWaypoint();
    waypoint->spot = this->mission->mission->mission_data.spots[arg];
    waypoint->objective = new std::string("Fly to\nWay Area");
    this->mission->waypoints.push_back(waypoint);
    return true;
}
/**
 * Sets the current objective to a destroy-target objective with the given
 * argument as the target object ID.
 *
 * @param arg The ID of the target object to destroy.
 *
 * @return True if the objective was set successfully, false otherwise.
 */
bool SCMissionActorsPlayer::destroyTarget(uint8_t arg) {
    SCMissionWaypoint *waypoint = new SCMissionWaypoint();
    waypoint->spot = this->mission->mission->mission_data.spots[arg];
    waypoint->objective = new std::string("Destroy\nTarget");
    this->mission->waypoints.push_back(waypoint);
    return true;
}
/**
 * SCMissionActorsPlayer::defendTarget
 *
 * Sets the current objective to a defend-target objective with the given
 * argument as the target object ID.
 *
 * @param arg The ID of the target object to defend.
 *
 * @return True if the objective was set successfully, false otherwise.
 */
bool SCMissionActorsPlayer::defendTarget(uint8_t arg) {
    SCMissionWaypoint *waypoint = new SCMissionWaypoint();
    waypoint->spot = this->mission->mission->mission_data.spots[arg];
    waypoint->objective = new std::string("Defend\nAlly");
    this->mission->waypoints.push_back(waypoint);
    return true;
}

/**
 * SCMissionActorsPlayer::setMessage
 *
 * Sets the current objective to display a message with the given
 * argument as the message ID.
 *
 * @param arg The ID of the message to display.
 *
 * @return True if the objective was set successfully, false otherwise.
 */
bool SCMissionActorsPlayer::setMessage(uint8_t arg) {
    if (arg >= this->mission->mission->mission_data.messages.size()) {
        return true;
    }
    std::transform(this->mission->mission->mission_data.messages[arg]->begin(), this->mission->mission->mission_data.messages[arg]->end(), this->mission->mission->mission_data.messages[arg]->begin(), ::tolower);
    if (this->mission->waypoints.size() > 0) {
        this->mission->waypoints.back()->message = this->mission->mission->mission_data.messages[arg];
    }
    return true;
}

void SCMissionActorsPlayer::hasBeenHit(SCSimulatedObject *weapon, SCMissionActors *attacker) {
    // for the moment, the player cannot be hit
    return;
}

bool SCMissionActorsStrikeBase::setMessage(uint8_t arg) {
    RadioMessages *msg = new RadioMessages();
    msg->message = this->profile->radi.msgs[arg];
    if (this->mission->sound.inGameVoices.size() > 0) {
        if (this->mission->sound.inGameVoices.find(this->profile->radi.spch) == this->mission->sound.inGameVoices.end()) {
            printf("No voice found for %d\n", this->profile->radi.spch);
        }
        if (this->mission->sound.inGameVoices[this->profile->radi.spch]->messages.find(arg) == this->mission->sound.inGameVoices[this->profile->radi.spch]->messages.end()) {
            printf("No message found for %d\n", arg);
        }
        MemSound *message_sound = this->mission->sound.inGameVoices[this->profile->radi.spch]->messages[arg];
        msg->sound = message_sound;
    }
    this->mission->radio_messages.push_back(msg);
    if (arg == 20) {
        this->mission->mission_over = true;
        this->mission->mission_won = true;
    } else if (arg == 21) {
        this->mission->mission_over = true;
        this->mission->mission_won = false;
    }
    return true;
}
/*
@TODO
Fly to#Precise Way
Defend#Point
Follow#Leader
*/