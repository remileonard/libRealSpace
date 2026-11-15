#include "PSMission.h"
#include "PSMissionParser.h"
#include "PSArea.h"
#include "PSBrain.h"
#include "PSMissionActors.h"
#include "PSEntity.h"
#include "PSPlane.h"
#include "PSProg.h"
#include "PSState.h"
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
    for (auto turret : this->turrets) {
        delete turret;
    }
    this->turrets.clear();
}

// Un enfant SWPN d'un avion ou d'un navire devient une tourelle qui suit son porteur. Montage dans le repère du modèle
// (nez = z, haut = y, droite = x du chunk CHLD), angles du chunk (axe X = tangage, axe Y = roulis, axe vertical = lacet).
void PSMission::createTurrets(SCMissionActors *actor) {
    if (actor->object->entity == nullptr) {
        return;
    }
    EntityType type = actor->object->entity->entity_type;
    if (type != EntityType::jet && type != EntityType::ship && type != EntityType::carr) {
        return;
    }
    for (auto child : actor->object->entity->chld) {
        if (child->objct == nullptr || child->objct->entity_type != EntityType::swpn || child->objct->swpn_data == nullptr) {
            continue;
        }
        MISN_PART *part = new MISN_PART();
        part->id = actor->object->id;
        part->member_name = child->objct->name;
        part->entity = child->objct;
        part->position = actor->object->position;
        PSMissionActorsTurret *turret = new PSMissionActorsTurret();
        turret->parent = actor;
        turret->actor_name = child->name;
        turret->actor_id = actor->actor_id;
        turret->object = part;
        turret->creation_part = *part;
        turret->health = child->objct->health;
        turret->team_id = actor->team_id;
        turret->mission = this;
        turret->is_active = false;
        turret->mount = child;
        PSEntity *entity = static_cast<PSEntity *>(child->objct);
        printf("PSMission: tourelle %s sur %s#%d, decalage nez %.3f haut %.3f droite %.3f m, tangage %.1f roulis %.1f lacet %.1f, secteur %s elevation [%.0f, %.0f] gisement [%.0f, %.0f], munitions %d, portee %d\n", child->name.c_str(), actor->actor_name.c_str(), actor->actor_id, child->z, child->y, child->x, child->pitch, child->roll, child->yaw, entity->has_gun_arc ? "GARC" : "absent", entity->arc_elevation_min, entity->arc_elevation_max, entity->arc_azimuth_min, entity->arc_azimuth_max, child->objct->swpn_data->weapons_round, child->objct->swpn_data->effective_range);
        if (child->roll != 0.0f) {
            printf("PSMission: tourelle %s, roulis de montage %.1f non applique\n", child->name.c_str(), child->roll);
        }
        this->turrets.push_back(turret);
    }
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
    // Un objet placé dans une zone n'a ici que son décalage : le centre de la zone est ajouté à l'activation. Au sol
    // ou en vol, manette et vitesse se décident à la position réelle (le jeu ne crée l'objet qu'à l'activation, à sa
    // position finale : PartEntry_ResolveSpawnPositionAndActivate_5313D).
    Vector3D spawn = part->position;
    bool in_area = part->placement == 0 && part->area_id < this->mission->mission_data.areas.size();
    if (in_area) {
        spawn += this->mission->mission_data.areas[part->area_id]->position;
    }
    float spawn_ground = this->area->getY(spawn.x, spawn.z);
    if (!in_area && abs(spawn_ground - part->position.y) <= 10) {
        part->position.y = spawn_ground;
    }
    actor->plane->on_ground = part->speed == 0;
    if (spawn.y < spawn_ground) {
        actor->plane->on_ground = true;
    }
    if (actor->plane->on_ground) {
        if (actor->plane->GetWheel() == 0) {
            actor->plane->SetWheel();
        }
    }
    if (!actor->plane->on_ground) {
        actor->plane->SetThrottle(100);
        actor->pilot->target_climb = (int)(spawn.y);
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
    printf("PSMission: avion de %s#%d, decalage (%.1f, %.1f, %.1f), position reelle (%.1f, %.1f, %.1f), sol %.1f, cap %d, vitesse %d, %s\n", actor->actor_name.c_str(), actor->actor_id, part->position.x, part->position.y, part->position.z, spawn.x, spawn.y, spawn.z, spawn_ground, part->azymuth, part->speed, actor->plane->on_ground ? "au sol" : "en vol");
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
            printf("PSMission: piste %d %s a (%.1f, %.1f, %.1f), dans la region X %d a %d, Z %d a %d\n", part->id, part->member_name.c_str(), position.x, position.y, position.z, runway.lx, runway.hx, runway.ly, runway.hy);
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
    // Drapeaux de la mission : un par numéro de 2 octets, pour qu'aucune lecture ni écriture d'un script ne tombe hors
    // de la table ; ils partent des drapeaux de la partie (PSState), posés par le déroulement et les missions précédentes.
    this->mission->mission_data.flags.assign(FLAG_COUNT, 0);
    for (auto &flag : PSState::getInstance().flags) {
        this->mission->mission_data.flags[flag.first] = flag.second;
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
            // Entité vide : l'objet existe pour les scripts et les scènes, sans modèle ni avion.
            part->entity = this->createEntity();
            part->entity->name = part->member_name;
            part->entity->entity_type = EntityType::ornt;
            part->entity->health = 50;
            printf("PSMission: objet %d %s sans fichier objet, entite vide\n", part->id, part->member_name.c_str());
        }
        for (auto cast : mission->mission_data.casting) {
            if (part->id == search_id) {
                SCMissionActors *actor = new PSMissionActors();
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
                    actor->brain = new PSBrain(actor);
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
    for (auto actor : this->actors) {
        this->createTurrets(actor);
    }
    // Un script désigne un objet par le numéro de son PART, pas par son rang : les objets du terrain prennent des
    // numéros au-dessus du plus grand numéro de PART.
    int terrain_id = 0;
    for (auto part : mission->mission_data.parts) {
        if (part->id >= terrain_id) {
            terrain_id = part->id + 1;
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
        part->id = terrain_id++;
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
    PSArea *ps_area = static_cast<PSArea *>(this->area);
    ps_area->carriers.clear();
    for (auto actor : this->actors) {
        if (actor->object == nullptr || actor->object->entity == nullptr || actor->object->entity->entity_type != EntityType::carr) {
            continue;
        }
        if (static_cast<PSEntity *>(actor->object->entity)->has_carrier_deck) {
            ps_area->carriers.push_back(actor);
            printf("PSMission: porte-avions %s#%d donne a la carte, position (%.1f, %.1f, %.1f), cap %d\n", actor->actor_name.c_str(), actor->actor_id, actor->object->position.x, actor->object->position.y, actor->object->position.z, actor->object->azymuth);
        }
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

// Fin de mission, ordre de PACIFIC.EXE (sub_BE97E de ovr344, pas encore nommée) : script de sortie de la scène courante,
// puis celui de la scène globale.
void PSMission::finishScenes() {
    if (this->current_scene != nullptr && this->current_scene != this->global_scene) {
        this->current_scene->runLeaving();
    }
    if (this->global_scene != nullptr) {
        this->global_scene->runLeaving();
    }
}

// Music_ScanNearbyEnemies_5B962 : objets du camp 255 à moins de 18 520 m du joueur ; bit 0 = avion dont le pilote est à
// bord, bit 1 = autre objet (catégories 1, 19, 20, 21, 23, 24 : décor ORNT, SWPN, sol, XMIT, navires).
uint8_t PSMission::musicScanEnemies(SCMissionActors *excluded) {
    uint8_t flags = 0;
    Vector3D player_position = this->player->plane->position;
    for (auto actor : this->actors) {
        if (actor == excluded || actor == this->player || !actor->is_active || actor->is_destroyed) {
            continue;
        }
        if (actor->object == nullptr || actor->object->entity == nullptr || !actor->object->alive || actor->team_id != 255) {
            continue;
        }
        Vector3D position = actor->plane != nullptr ? actor->plane->position : actor->object->position;
        if ((position - player_position).Length() >= 18520.0f) {
            continue;
        }
        EntityType type = actor->object->entity->entity_type;
        if (actor->plane != nullptr && type == EntityType::jet && !actor->plane->ejected) {
            flags |= 1;
        }
        if (type == EntityType::ornt || type == EntityType::swpn || type == EntityType::ground || type == EntityType::xmit || type == EntityType::ship || type == EntityType::carr) {
            flags |= 2;
        }
    }
    return flags;
}

// Music_CombatIntensitySelector_5BBF8 (table off_5BD55) : pendant ces pistes, le combat ne passe pas à 9 ou 1 et ne
// revient pas à la piste de la mission.
bool PSMission::musicTuneLocked() {
    int current = (int)this->Mixer.getMusicID();
    return current == 4 || current == 11 || current == 12 || current == 15 || current == 16 || current == 19;
}

// Music_TargetWithinRange_5B85C : la cible du joueur est un avion, pilote à bord, à moins de 1 500 m.
bool PSMission::musicTargetWithinRange() {
    SCMissionActors *target = this->player->target;
    if (target == nullptr || target->plane == nullptr || target->plane->ejected) {
        return false;
    }
    return (target->plane->position - this->player->plane->position).Length() < 1500.0f;
}

// Music_CombatIntensitySelector_5BBF8, pistes de Pacific Strike.
void PSMission::onMusicUpdate() {
    SCPlane *player_plane = this->player->plane;
    if (player_plane->ejected || player_plane->crached || !this->player->object->alive) {
        return;
    }
    if (this->Mixer.getMusicID() == 20 || this->Mixer.getRequestedMusicID() != this->Mixer.getMusicID()) {
        return;
    }
    uint8_t flags = this->musicScanEnemies(nullptr);
    this->music_enemy_flags = flags;
    bool locked = this->musicTuneLocked();
    if (flags == 0) {
        if (this->musicFallback() || locked) {
            return;
        }
        int mission_tune = (int)this->mission->mission_data.tune;
        if ((uint32_t)mission_tune != this->Mixer.getMusicID()) {
            this->musicRequest(mission_tune, "aucun ennemi, piste de la mission");
        }
        return;
    }
    this->music_combat_happened = true;
    int tune = -1;
    const char *reason = "";
    if (this->player_tail_threat != nullptr) {
        tune = 13;
        reason = "un ennemi attaque le joueur";
    } else if (this->musicTargetWithinRange()) {
        tune = 18;
        reason = "cible du joueur a moins de 1500 m";
    } else if (static_cast<PSPlane *>(player_plane)->criticallyDamaged()) {
        tune = 2;
        reason = "moteur, ailerons ou profondeur > 90 %";
    } else if (static_cast<PSPlane *>(player_plane)->seriouslyDamaged()) {
        tune = 10;
        reason = "gravement endommage";
    } else if (!locked) {
        tune = (flags & 1) ? 9 : 1;
        reason = (flags & 1) ? "avion ennemi proche" : "menaces au sol seules";
    }
    if (tune >= 0 && (uint32_t)tune != this->Mixer.getMusicID()) {
        this->musicRequest(tune, reason);
    }
}

// Music_SelectTuneCandidate_5BB21, étape de fin de combat : 17 si la condition de mission est remplie, sinon 3 après
// un combat (le combat est alors oublié).
bool PSMission::musicFallback() {
    if (this->mission_over && this->mission_won) {
        this->musicRequest(17, "fin de combat, condition de mission remplie");
        return true;
    }
    if (this->music_combat_happened) {
        this->music_combat_happened = false;
        this->musicRequest(3, "fin de combat");
        return true;
    }
    return false;
}

// Ponctuations de victoire du joueur : 16 (avion), 12 (décor, défense fixe, objet au sol), 15 (autres).
bool PSMission::musicIsPunctuation(int tune) {
    return tune == 12 || tune == 15 || tune == 16;
}

// World_OnObjectDestroyed_553B6, partie musique.
void PSMission::onActorDestroyed(SCMissionActors *actor) {
    SCPlane *player_plane = this->player->plane;
    if (actor == this->player || player_plane->crached || !this->player->object->alive) {
        return;
    }
    if (actor->plane != nullptr && actor->plane->ejected) {
        return;
    }
    if (actor->object == nullptr || actor->object->entity == nullptr) {
        return;
    }
    EntityType type = actor->object->entity->entity_type;
    if (type != EntityType::ornt && type != EntityType::jet && type != EntityType::swpn && type != EntityType::ground && type != EntityType::xmit && type != EntityType::ship && type != EntityType::carr) {
        return;
    }
    printf("MUSIC destroyed %s#%d team=%d by=%s\n", actor->actor_name.c_str(), actor->actor_id, actor->team_id, actor->last_hit_by != nullptr ? actor->last_hit_by->actor_name.c_str() : "none");
    if (actor->team_id == 255) {
        uint8_t before = this->music_enemy_flags;
        uint8_t after = this->musicScanEnemies(actor);
        this->music_enemy_flags = after;
        bool changed = false;
        if (after != 0 && (before & 1) && !(after & 1)) {
            this->musicRequest(1, "dernier avion ennemi abattu");
            changed = true;
        } else if (after == 0 && before != 0) {
            changed = this->musicFallback();
        }
        if (changed || actor->last_hit_by != this->player) {
            return;
        }
        int punctuation = 15;
        if (type == EntityType::jet) {
            punctuation = 16;
        } else if (type == EntityType::ornt || type == EntityType::swpn || type == EntityType::ground) {
            punctuation = 12;
        }
        if (std::rand() % 100 < 11) {
            punctuation = 15;
        }
        this->music_resume_tune = (int)this->Mixer.getMusicID();
        this->musicRequest(punctuation, "victoire du joueur");
        return;
    }
    if (actor->team_id == 1) {
        int tune = (std::rand() % 2 == 1) ? 19 : 4;
        this->musicRequest(tune, "objet allie detruit");
    }
}
