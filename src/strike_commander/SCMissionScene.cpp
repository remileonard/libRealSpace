#include "precomp.h"
#include "SCMissionScene.h"

void SCMissionScene::onEvent(const EventMessage &event) {
    if (auto activated = dynamic_cast<const MissionEventSceneActivated *>(&event)) {
        if (activated->scene == this->scene) {
            this->activate();
        }
        return;
    }
    if (auto update = dynamic_cast<const MissionEventSceneUpdate *>(&event)) {
        if (update->scene == this->scene) {
            this->runMissionUpdate();
        }
        return;
    }
    if (auto left = dynamic_cast<const MissionEventSceneLeft *>(&event)) {
        if (left->scene == this->scene) {
            this->runLeaving();
        }
        return;
    }
}

void SCMissionScene::runProg(int16_t prog_id) {
    if (prog_id < 0 || prog_id >= (int16_t) this->mission->mission->mission_data.prog.size()) {
        return;
    }
    std::vector<PROG> prog;
    for (auto prg: *this->mission->mission->mission_data.prog[prog_id]) {
        prog.push_back(prg);
    }
    SCProg *p = new SCProg(this->mission->player, prog, this->mission, prog_id);
    p->scene_script = true;
    p->execute();
    delete p;
}

void SCMissionScene::runMissionUpdate() {
    this->runProg(this->scene->on_mission_update);
}

void SCMissionScene::runLeaving() {
    this->runProg(this->scene->on_leaving);
}

void SCMissionScene::activate() {
    MISN_SCEN *scene = this->scene;
    if (scene->has_been_activated) {
        return;
    }
    printf("SCENE: activation, zone %d, %zu objets, script %d\n", scene->area_id, scene->cast.size(), scene->on_is_activated);
    RSMission *mission = this->mission->mission;
    SCMission *sc_mission = this->mission;
    for (auto cast: scene->cast) {
        int i=0;
        for (auto part: mission->mission_data.parts) {
            if (i == cast) {
                for (auto actor: sc_mission->actors) {
                    if (actor->actor_name == "PLAYER") {
                        continue;
                    }
                    if (actor->actor_id == part->id && (actor->is_active == false)) {
                        if (actor->is_destroyed) {
                            sc_mission->respawnActor(actor);
                        }
                        actor->is_active = true;
                        actor->is_hidden = false;
                        if (scene->area_id != -1 || actor->object->placement != 0) {
                            if (actor->object->placement != 0) {
                                sc_mission->placeRelativeToPlayer(actor);
                            } else if (actor->object->area_id != 255) {
                                actor->object->position += sc_mission->mission->mission_data.areas[scene->area_id]->position;
                            }
                            float ground_y = sc_mission->area->getY(actor->object->position.x, actor->object->position.z);
                            
                            if (actor->plane != nullptr) {
                                actor->plane->on_ground = false;
                                actor->plane->x = actor->object->position.x;
                                actor->plane->y = actor->object->position.y;
                                actor->plane->z = actor->object->position.z;
                                if (abs(ground_y - actor->object->position.y) <= 10 ) {
                                    actor->object->position.y = ground_y;
                                }
                                actor->plane->on_ground = actor->object->speed == 0;
                                if (actor->plane->on_ground) {
                                    if (actor->plane->GetWheel() == 0) {
                                        actor->plane->SetWheel();
                                    }
                                }
                                if (actor->object->position.y < ground_y) {
                                    actor->plane->position.y += ground_y;
                                }
                            } else if (actor->object->position.y < ground_y) {
                                actor->object->position.y = ground_y;
                            }
                        }
                        actor->wait_timer = 0.0f;
                        if (actor->on_is_activated.size() > 0) {
                            SCProg *p = new SCProg(actor, actor->on_is_activated, sc_mission, actor->object->on_is_activated);
                            p->execute();
                            delete p;
                        }
                        if (actor->object->entity->entity_type == EntityType::rnwy) {
                            for (auto runway: sc_mission->area->objectOverlay) {
                                Vector3D pos = actor->object->position;
                                if (pos.x >= runway.lx && pos.x <= runway.hx && pos.z >= runway.ly && pos.z <= runway.hy) {

                                    float width = (float)std::abs(runway.lx - runway.hx); 
                                    float length = (float)std::abs(runway.ly - runway.hy);
                                    
                                    float angle = (float)std::atan2(runway.ly - runway.hy, 
                                                            runway.lx - runway.hx);
                                    
                                    actor->object->entity->bb.min.x = -width / 2.0f;
                                    actor->object->entity->bb.max.x = width / 2.0f;
                                    actor->object->entity->bb.min.y = -5;
                                    actor->object->entity->bb.max.y = 5;
                                    actor->object->entity->bb.min.z = -length / 2.0f;
                                    actor->object->entity->bb.max.z = length / 2.0f;
                                    actor->object->azymuth = angle * 180.0f / M_PI;
                                    
                                    break;
                                }
                            }
                        }
                        break;
                    }
                }
                break;
            }
            i++;
        }
    }
    this->runProg(scene->on_is_activated);
    scene->has_been_activated = 1;
}

SCMissionScene::SCMissionScene(SCMission *mission, MISN_SCEN *scene) {
    this->mission = mission;
    this->scene = scene;
    this->subscription_id =
        MessageBus::getInstance().subscribeEvent(std::bind(&SCMissionScene::onEvent, this, std::placeholders::_1));
}

SCMissionScene::~SCMissionScene() {
    if (this->subscription_id != -1) {
        MessageBus::getInstance().unsubscribe(this->subscription_id);
    }
}
