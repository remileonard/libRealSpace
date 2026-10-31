#include "precomp.h"
#include "SCMissionScene.h"

// Lance un script PROG de la scène (-1 = aucun).
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

// Scene_RunUpdateScript_531F2 : à chaque image, pour la scène globale et la scène courante (MAIN_GAME_TICK).
void SCMissionScene::runMissionUpdate() {
    this->runProg(this->scene->on_mission_update);
}

// Scene_RunLeavingScript_53211 : quand la scène cesse d'être courante, et en fin de mission.
void SCMissionScene::runLeaving() {
    this->runProg(this->scene->on_leaving);
}

// Scene_TriggerActivation_531CD : quand la scène devient la scène courante (ou au chargement pour la scène
// globale) : apparition des objets de la scène qui ne sont pas déjà là (Scene_EnsureUnitsSpawned_530FF), puis
// script d'activation. Le jeu relance ce script à chaque fois que la scène redevient courante.
void SCMissionScene::activate() {
    MISN_SCEN *scene = this->scene;
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
                    if (actor->actor_id == part->id && actor->is_active == false) {
                        actor->is_active = true;
                        actor->is_hidden = false;
                        // Le mode de placement ne dépend ni de la zone de l'objet ni de celle de la scène.
                        if (scene->area_id != -1 || actor->object->placement != 0) {
                            if (actor->object->placement != 0) {
                                // Repère de l'avion du joueur : décalage tourné, cap = cap du joueur + cap du fichier.
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
                                // Vitesse initiale nulle : au sol, train sorti (PartEntry_ResolveSpawnPositionAndActivate_51EDC).
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
                        
                        // Shared_TriggerExprInstruction_5247D : minuteur WAIT (+0x3A) remis a zero a l'activation
                        actor->wait_timer = 0.0f;
                        if (actor->on_is_activated.size() > 0) {
                            SCProg *p = new SCProg(actor, actor->on_is_activated, sc_mission, actor->object->on_is_activated);
                            p->execute();
                            delete p;
                        }
                        if (actor->object->entity->entity_type == EntityType::rnwy) {
                            for (auto runway: sc_mission->area->objectOverlay) {
                                Vector3D pos = actor->object->position;
                                
                                // Vérifier si la position de l'objet est à l'intérieur de la piste
                                if (pos.x >= runway.lx && pos.x <= runway.hx && 
                                    pos.z >= runway.ly && pos.z <= runway.hy) {
                                    
                                    // Calculer les dimensions de la piste
                                    float width = (float)std::abs(runway.lx - runway.hx); 
                                    float length = (float)std::abs(runway.ly - runway.hy);
                                    
                                    // Calculer l'orientation (angle) de la piste
                                    float angle = (float)std::atan2(runway.ly - runway.hy, 
                                                            runway.lx - runway.hx);
                                    
                                    // Recalculer la bounding box
                                    actor->object->entity->bb.min.x = -width / 2.0f;
                                    actor->object->entity->bb.max.x = width / 2.0f;
                                    actor->object->entity->bb.min.y = -5;
                                    actor->object->entity->bb.max.y = 5;
                                    actor->object->entity->bb.min.z = -length / 2.0f;
                                    actor->object->entity->bb.max.z = length / 2.0f;
                                    
                                    // Appliquer l'orientation à l'objet
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
}

SCMissionScene::~SCMissionScene() {
}
