#include "PSStrike.h"


void PSStrike::setMission(char const *missionName) {
    if (Mixer.isSoundPlaying(5)) {
        Mixer.stopSound(5);
    }
    Mixer.stopMusic();
    
    if (this->current_mission != nullptr) {
        this->current_mission->cleanup();
        delete this->current_mission;
        this->current_mission = nullptr;
    } 
    this->miss_file_name = missionName;
    this->current_mission = new PSMission(missionName, &object_cache);
    ai_planes.clear();
    ai_planes.shrink_to_fit();
    
    MISN_PART *playerCoord = this->current_mission->player->object;
    this->area = this->current_mission->area;
    new_position.x = playerCoord->position.x;
    new_position.z = playerCoord->position.z;
    new_position.y = playerCoord->position.y;

    camera = Renderer.getCamera();
    camera->SetPosition(&new_position);
    this->current_target = 0;
    this->target = this->current_mission->enemies[this->current_target];
    this->nav_point_id = 0;
    this->player_plane = this->current_mission->player->plane;
    this->player_plane->yaw = (360 - playerCoord->azymuth) * 10.0f;
    this->player_plane->object = playerCoord;
    float ground = this->area->getY(new_position.x, new_position.z);
    if (ground < new_position.y) {
        this->player_plane->SetThrottle(100);
        this->player_plane->SetWheel();
        this->player_plane->vz = -20;
        this->player_plane->Simulate();
    }
    if (GameState.weapon_load_out.size()>1) {
        this->player_plane->object->entity->weaps.clear();
        std::unordered_map<weapon_type_shp_id, std::string> weapon_names = {
            {AIM9J, "SWINDERJ"},
            {AIM9M, "SWINDERM"},
            {AIM120, "AMRAAM"},
            {AGM65D, "AGM-65D"},
            {DURANDAL, "DURANDAL"},
            {MK20, "MK20"},
            {MK82, "MK82"},
            {GBU15, "GBU-15G"},
            {LAU3, "POD"}
        };
        RSEntity::WEAPS *weap = new RSEntity::WEAPS();
        weap->name = "20MM";
        weap->nb_weap = 1000;
        weap->objct = loadWeapon(weap->name);
        this->player_plane->object->entity->weaps.push_back(weap);
        std::unordered_map<weapon_type_shp_id, bool> loaded;
        for (int i=1; i<5; i++) {
            if (loaded.find(weapon_type_shp_id(GameState.weapon_load_out[i])) != loaded.end()) {
                continue;
            }
            RSEntity::WEAPS *weap = new RSEntity::WEAPS();
            weap->name = weapon_names[weapon_type_shp_id(GameState.weapon_load_out[i])];
            weap->nb_weap = GameState.weapon_load_out[GameState.weapon_load_out[i]];
            weap->objct = loadWeapon(weap->name);
            this->player_plane->object->entity->weaps.push_back(weap);
            loaded[weapon_type_shp_id(GameState.weapon_load_out[i])] = true;
        }
    }
    
    this->player_plane->InitLoadout();
    this->player_prof = this->current_mission->player->profile;
    this->cockpit = new SCCockpit();
    this->cockpit->player_plane = this->player_plane;
    this->cockpit->init();
    
    this->cockpit->current_mission = this->current_mission;
    this->cockpit->nav_point_id = &this->nav_point_id;
    Mixer.stopMusic();
    Mixer.switchBank(2);
    Mixer.playMusic(this->current_mission->mission->mission_data.tune+1);
    verticalOffset = 0.0f;
    SCStrike::verticalOffset = 0.0f;
    SCRenderer::instance().verticalOffset = verticalOffset;
    SCRenderer::instance().initRenderCameraView();
}
PSStrike::PSStrike() {}

PSStrike::~PSStrike() {}
