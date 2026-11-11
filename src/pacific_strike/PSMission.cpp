#include "PSMission.h"
#include "PSMissionParser.h"
#include "PSArea.h"
#include "PSEntity.h"
#include "PSPlane.h"
#include "PSProg.h"
#include "../strike_commander/SCAIBrain.h"

PSMission::PSMission() {
    this->last_time = SDL_GetTicks();
    this->last_tick = 0;
    this->tick_counter = 0;
    this->tps = 0;
}
PSMission::PSMission(std::string mission_name, std::unordered_map<std::string, RSEntity *> *objCache) {
    this->mission_name = mission_name;
    this->obj_cache = objCache;
    this->last_time = SDL_GetTicks();
    this->last_tick = 0;
    this->tick_counter = 0;
    this->tps = 0;
    this->loadMission();
    subscription_id = this->messageBus.subscribeEvent(std::bind(&PSMission::onEvent, this, std::placeholders::_1));
}

PSMission::~PSMission() {
}

RSEntity *PSMission::createEntity() {
    return new PSEntity();
}

// Avion d'un objet de mission : copie de SCMission::createAiPlane, avec un PSPlane.
void PSMission::createAiPlane(SCMissionActors *actor) {
    MISN_PART *part = actor->object;
    actor->pilot = new SCPilot();
    actor->pilot->actor = actor;
    actor->plane = new PSPlane(actor->object->entity->jdyn->max_g, -(float)actor->object->entity->jdyn->max_g / 2.0f, 40.0f, 60.0f, (float)actor->object->entity->jdyn->pitch_rate_limit_dps / 2.0f, (float)actor->object->entity->jdyn->rate_limit_dps / 2.0f, actor->object->entity->wing_area, (float)actor->object->entity->weight_in_kg, (float)actor->object->entity->jdyn->fuel_capacity, (float)actor->object->entity->thrust_in_newton, sqrtf(actor->object->entity->wing_area / ((float)M_PI * 0.83f * actor->object->entity->jdyn->induced_drag_k)), actor->object->entity->jdyn->induced_drag_k, (int)actor->object->entity->jdyn->stall_speed_ms, this->area, part->position.x, part->position.y, part->position.z);
    actor->plane->yaw = (360 - part->azymuth) * 10.0f;
    for (auto &sys : actor->object->entity->sysm) {
        for (auto &subsys : sys.second) {
            actor->plane->system_health[sys.first][subsys.first] = subsys.second;
        }
    }
    part->weapon_load.shrink_to_fit();
    if (part->weapon_load.size() > 0) {
        std::string weapon_path_name = Assets.object_root_path + part->weapon_load + ".IFF";
        std::transform(weapon_path_name.begin(), weapon_path_name.end(), weapon_path_name.begin(), ::toupper);
        TreEntry *weapon_entry = Assets.GetEntryByName(weapon_path_name.c_str());
        part->entity->weaps.clear();
        part->entity->weaps.shrink_to_fit();
        static_cast<PSEntity *>(part->entity)->loadWeaponLoad(weapon_entry->data, weapon_entry->size);
    }
    actor->plane->object = part;
    actor->plane->InitLoadout();
    actor->plane->pilot = actor;
    if (abs(this->area->getY(part->position.x, part->position.z) - part->position.y) <= 10) {
        part->position.y = this->area->getY(part->position.x, part->position.z);
    }
    actor->plane->on_ground = part->speed == 0;
    if (part->position.y < this->area->getY(part->position.x, part->position.z)) {
        actor->plane->on_ground = true;
    }
    if (actor->plane->on_ground) {
        if (actor->plane->GetWheel() == 0) {
            actor->plane->SetWheel();
        }
    }
    if (!actor->plane->on_ground) {
        actor->plane->SetThrottle(100);
        actor->pilot->target_climb = (int)(part->position.y);
        actor->plane->vz = -20;
        float spawn_yaw = tenthOfDegreeToRad(actor->plane->yaw);
        float spawn_speed = (float)part->speed;
        actor->plane->velocity = Vector3D(-sinf(spawn_yaw) * spawn_speed, 0.0f, -cosf(spawn_yaw) * spawn_speed);
        actor->pilot->target_azimut = actor->plane->azimuthf / 10.0f;
        actor->pilot->target_speed = -20;
    } else {
        actor->plane->SetThrottle(0);
        actor->pilot->target_climb = 0;
        actor->plane->vz = 0;
        actor->pilot->target_azimut = actor->plane->azimuthf / 10.0f;
        actor->pilot->target_speed = 0;
    }
    actor->pilot->plane = actor->plane;
    printf("PSMission: avion de %s#%d a (%.1f, %.1f, %.1f), cap %d, vitesse %d, %s\n", actor->actor_name.c_str(), actor->actor_id, part->position.x, part->position.y, part->position.z, part->azymuth, part->speed, actor->plane->on_ground ? "au sol" : "en vol");
}

// Pistes des missions de Pacific Strike : RWAYS, RWAYM1, RWAYM2, RWAYN et RUNWAYZ n'ont pas de fichier objet.
bool PSMission::isRunwayName(const std::string &name) {
    return name.rfind("RWAY", 0) == 0 || name.rfind("RUNWAY", 0) == 0;
}

// Entité de type piste sans modèle ; sa boîte est calculée à l'activation depuis la région de piste du terrain qui
// contient l'objet (SCMissionActors::activateTarget).
RSEntity *PSMission::createRunwayEntity(MISN_PART *part) {
    RSEntity *runway_entity = this->createEntity();
    runway_entity->name = part->member_name;
    runway_entity->entity_type = EntityType::rnwy;
    runway_entity->health = 50;
    runway_entity->bb.min.x = -1.0f;
    runway_entity->bb.min.y = -1.0f;
    runway_entity->bb.min.z = -1.0f;
    runway_entity->bb.max.x = 1.0f;
    runway_entity->bb.max.y = 1.0f;
    runway_entity->bb.max.z = 1.0f;
    Vector3D position = part->position;
    if (part->placement == 0 && part->area_id < this->mission->mission_data.areas.size()) {
        position += this->mission->mission_data.areas[part->area_id]->position;
    }
    bool inside = false;
    for (auto &runway : this->area->objectOverlay) {
        if (position.x >= runway.lx && position.x <= runway.hx && position.z >= runway.ly && position.z <= runway.hy) {
            inside = true;
            printf("PSMission: piste %d %s a (%.1f, %.1f, %.1f), dans la region X %.1f a %.1f, Z %.1f a %.1f\n", part->id, part->member_name.c_str(), position.x, position.y, position.z, runway.lx, runway.hx, runway.ly, runway.hy);
            break;
        }
    }
    if (!inside) {
        printf("PSMission: piste %d %s a (%.1f, %.1f, %.1f), dans aucune des %zu regions de piste du terrain\n", part->id, part->member_name.c_str(), position.x, position.y, position.z, this->area->objectOverlay.size());
    }
    return runway_entity;
}

// Copie de SCMission::loadMission, avec ce qui change pour Pacific Strike : lecture des scripts (PSMissionParser),
// terrain compressé et régions de pistes (PSArea), avions (createAiPlane).
void PSMission::loadMission() {
    this->difficulty = Config::getInstance().getInt("Gameplay", "difficulty", 0);
    std::string miss_file_name = Assets.mission_root_path + this->mission_name;
    std::transform(miss_file_name.begin(), miss_file_name.end(), miss_file_name.begin(), ::toupper);
    TreEntry *mission_tre = Assets.GetEntryByName(miss_file_name.c_str());
    this->mission = new PSMissionParser();
    this->mission->InitFromRAM(mission_tre->data, mission_tre->size);

    TreEntry *intel_tre = Assets.GetEntryByName(Assets.intel_root_path + "INTEL.IFF");
    if (intel_tre != nullptr) {
        this->intel.InitFromRAM(intel_tre->data, intel_tre->size);
    }

    std::string area_filename = Assets.mission_root_path + this->mission->mission_data.world_filename + ".IFF";
    std::transform(area_filename.begin(), area_filename.end(), area_filename.begin(), ::toupper);
    this->world = new RSWorld();
    TreEntry *treEntry = Assets.GetEntryByName(area_filename.c_str());
    if (treEntry != NULL) {
        this->world->InitFromRAM(treEntry->data, treEntry->size);
    }
    if (this->world->has_light) {
        Renderer.setObjectLight(this->world->light_ambient_weight, this->world->light_weight, this->world->light_direction[0], this->world->light_direction[1], this->world->light_direction[2]);
    }
    std::string area_fn = "..\\..\\DATA\\TERRAIN\\" + this->world->tera + ".ZIP";
    std::transform(area_fn.begin(), area_fn.end(), area_fn.begin(), ::toupper);
    this->area = new PSArea();
    this->area->InitFromZipFileName(area_fn);

    Renderer.InvalidateAABBCache();
    Renderer.PrecomputeAABBs(this->area, 0, 1);
    for (int i = 0; i < 256; i++) {
        this->mission->mission_data.flags.push_back(0);
    }
    for (auto &area_entity : this->area->objects) {
        area_entity.entity = LoadEntity(area_entity.name);
    }
    for (auto scene : this->mission->mission_data.scenes) {
        this->scenes.push_back(new SCMissionScene(this, scene));
    }
    printf("PSMission: %s, monde %s, terrain %s, %zu zones, %zu scenes, %zu objets, %zu programmes, %zu SPOT\n", this->mission_name.c_str(), this->mission->mission_data.world_filename.c_str(), area_fn.c_str(), this->mission->mission_data.areas.size(), this->scenes.size(), this->mission->mission_data.parts.size(), this->mission->mission_data.prog.size(), this->mission->mission_data.spots.size());
    size_t prog_count = this->mission->mission_data.prog.size();
    int cpt_actor = 0;
    for (auto part : mission->mission_data.parts) {
        int search_id = 0;
        if (part->entity == nullptr) {
            part->entity = LoadEntity(part->member_name);
        }
        if (part->entity == nullptr && isRunwayName(part->member_name)) {
            part->entity = this->createRunwayEntity(part);
        }
        if (part->entity == nullptr) {
            printf("PSMission: objet %d %s sans fichier objet\n", part->id, part->member_name.c_str());
        }
        for (auto cast : mission->mission_data.casting) {
            if (part->id == search_id) {
                SCMissionActors *actor = new SCMissionActors();
                if (cast->actor == "PLAYER") {
                    actor = new SCMissionActorsPlayer();
                    if (part->area_id != 255 && part->placement == 0) {
                        Vector3D correction = this->mission->mission_data.areas[part->area_id]->position;
                        part->position.x += correction.x;
                        part->position.y += correction.y;
                        part->position.z += correction.z;
                    }
                } else if (cast->actor == "STRIBASE") {
                    actor = new SCMissionActorsStrikeBase();
                } else if (part->entity != nullptr && part->entity->entity_type == EntityType::swpn) {
                    actor = new SCMissionActorsSwpn();
                }
                if (cast->actor == "TEAM0") {
                    cast->actor = GameState.wingman;
                }
                actor->team_id = part->team;
                actor->actor_name = cast->actor;
                actor->actor_id = part->id;
                actor->object = part;
                actor->creation_part = *part;
                if (part->entity == nullptr) {
                    actor->health = 50;
                } else {
                    actor->health = part->entity->health;
                }
                actor->profile = this->LoadProfile(cast->actor);
                actor->mission = this;
                if (actor->object->on_is_activated != 255 && actor->object->on_is_activated < prog_count) {
                    for (auto op : *this->mission->mission_data.prog[actor->object->on_is_activated]) {
                        actor->on_is_activated.push_back(op);
                    }
                }
                if (actor->object->on_is_destroyed != 255 && actor->object->on_is_destroyed < prog_count) {
                    for (auto op : *this->mission->mission_data.prog[actor->object->on_is_destroyed]) {
                        actor->on_is_destroyed.push_back(op);
                    }
                }
                if (actor->object->on_missions_init != 255 && actor->object->on_missions_init < prog_count) {
                    for (auto op : *this->mission->mission_data.prog[actor->object->on_missions_init]) {
                        actor->on_mission_start.push_back(op);
                    }
                }
                if (actor->object->on_mission_update != 255 && actor->object->on_mission_update < prog_count) {
                    for (auto op : *this->mission->mission_data.prog[actor->object->on_mission_update]) {
                        actor->on_update.push_back(op);
                    }
                }
                printf("PSMission: objet %d %s (%s), zone %d, placement %d, position (%.1f, %.1f, %.1f), cap %d, camp %d, scripts %d %d %d %d\n", part->id, cast->actor.c_str(), part->member_name.c_str(), part->area_id, part->placement, part->position.x, part->position.y, part->position.z, part->azymuth, part->team, part->on_is_activated, part->on_mission_update, part->on_is_destroyed, part->on_missions_init);
                bool has_plane_data = actor->object->entity != nullptr && actor->object->entity->jdyn != nullptr;
                if (actor->profile != nullptr && actor->profile->ai.isAI && cast->actor != "PLAYER") {
                    actor->brain = new SCAIBrain(actor);
                    if (actor->profile->ai.goal.size() > 0 && has_plane_data) {
                        this->createAiPlane(actor);
                    }
                    this->actors.push_back(actor);
                } else if (actor->profile != nullptr && cast->actor == "PLAYER") {
                    actor->plane = new PSPlane(actor->object->entity->jdyn->max_g, -(float)actor->object->entity->jdyn->max_g / 2.0f, 40.0f, 60.0f, (float)actor->object->entity->jdyn->pitch_rate_limit_dps / 2.0f, (float)actor->object->entity->jdyn->rate_limit_dps / 2.0f, actor->object->entity->wing_area, (float)actor->object->entity->weight_in_kg, (float)actor->object->entity->jdyn->fuel_capacity, (float)actor->object->entity->thrust_in_newton, sqrtf(actor->object->entity->wing_area / ((float)M_PI * 0.83f * actor->object->entity->jdyn->induced_drag_k)), actor->object->entity->jdyn->induced_drag_k, (int)actor->object->entity->jdyn->stall_speed_ms, this->area, part->position.x, part->position.y, part->position.z);
                    for (auto &sys : actor->object->entity->sysm) {
                        for (auto &subsys : sys.second) {
                            actor->plane->system_health[sys.first][subsys.first] = subsys.second;
                        }
                    }
                    actor->plane->yaw = (360 - part->azymuth) * 10.0f;
                    actor->plane->simple_simulation = false;
                    part->weapon_load.shrink_to_fit();
                    if (part->weapon_load.size() > 0) {
                        std::string weapon_path_name = Assets.object_root_path + part->weapon_load + ".IFF";
                        std::transform(weapon_path_name.begin(), weapon_path_name.end(), weapon_path_name.begin(), ::toupper);
                        TreEntry *weapon_entry = Assets.GetEntryByName(weapon_path_name.c_str());
                        part->entity->weaps.clear();
                        part->entity->weaps.shrink_to_fit();
                        static_cast<PSEntity *>(part->entity)->loadWeaponLoad(weapon_entry->data, weapon_entry->size);
                        printf("PSMission: emport impose du joueur %s\n", part->weapon_load.c_str());
                    }
                    actor->plane->object = part;
                    actor->plane->InitLoadout();
                    actor->plane->pilot = actor;
                    this->actors.push_back(actor);
                    this->player = actor;
                    actor->on_nav_map = true;
                } else {
                    this->actors.push_back(actor);
                }
                cpt_actor++;
                break;
            }
            search_id++;
        }
    }
    for (auto area_actor : this->area->objects) {
        if (area_actor.entity == nullptr) {
            continue;
        }
        SCMissionActors *actor = new SCMissionActors();
        if (area_actor.entity->entity_type == EntityType::swpn) {
            actor = new SCMissionActorsSwpn();
        }
        MISN_PART *part = new MISN_PART();
        part->id = cpt_actor++;
        part->member_name = area_actor.name;
        part->member_name_destroyed = area_actor.destroyedName;
        part->entity = area_actor.entity;
        part->position = area_actor.position;
        actor->actor_name = area_actor.name;
        actor->health = area_actor.entity->health;
        actor->plane = nullptr;
        actor->pilot = nullptr;
        actor->actor_id = part->id;
        actor->object = part;
        actor->profile = nullptr;
        actor->is_active = true;
        actor->is_hidden = false;
        actor->team_id = 255;
        actor->mission = this;
        if (actor->object->entity->entity_type == EntityType::rnwy) {
            for (auto runway : this->area->objectOverlay) {
                Vector3D pos = actor->object->position;
                if (pos.x >= runway.lx && pos.x <= runway.hx && pos.z >= runway.ly && pos.z <= runway.hy) {
                    float width = std::abs(runway.lx - runway.hx);
                    float length = std::abs(runway.ly - runway.hy);
                    float angle = std::atan2(runway.ly - runway.hy, runway.lx - runway.hx);
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
        this->actors.push_back(actor);
    }
    for (auto member : this->mission->mission_data.team) {
        MISN_PART *part = this->mission->mission_data.parts[member];
        int id = part->id;
        for (auto actor : this->actors) {
            if (actor->actor_id == id) {
                this->friendlies.push_back(actor);
            }
            if (actor->object == part) {
                this->team.push_back(actor);
            }
        }
    }
    for (auto enemis : this->actors) {
        if (std::find(this->friendlies.begin(), this->friendlies.end(), enemis) == this->friendlies.end()) {
            this->enemies.push_back(enemis);
        }
    }
    this->applyDifficulty();
    if (this->player->on_is_activated.size() > 0) {
        SCProg *p = this->createProg(this->player, this->player->on_is_activated, this->player->object->on_is_activated);
        p->execute();
    }
    if (this->player->on_mission_start.size() > 0 && this->player->prog_executed == false) {
        SCProg *p = this->createProg(this->player, this->player->on_mission_start, this->player->object->on_missions_init);
        p->execute();
        this->player->prog_executed = true;
    }
    for (auto spot : this->mission->mission_data.spots) {
        spot->origin = Vector3D(spot->position.x, spot->position.y, spot->position.z);
        // AreaRelPos_ToWorld_50EEB : zone absente ou hors liste -> décalage absolu.
        if (spot->area_id >= 0 && (size_t)spot->area_id < this->mission->mission_data.areas.size()) {
            AREA *ar = this->mission->mission_data.areas[spot->area_id];
            spot->position += ar->position;
        }
    }
    if (this->player != nullptr && this->player->plane != nullptr) {
        this->camera_director->init(this->world, this->player);
    }
    this->startScenes();
    printf("PSMission: %zu acteurs, scene globale %s, scene courante %s\n", this->actors.size(), this->global_scene != nullptr ? "presente" : "absente", this->current_scene != nullptr ? "presente" : "absente");
}
void PSMission::update() {
    SCMission::update();
}

SCProg *PSMission::createProg(SCMissionActors *actor, std::vector<PROG> prog, uint8_t prog_id) {
    return new PSProg(actor, prog, this, prog_id);
}
