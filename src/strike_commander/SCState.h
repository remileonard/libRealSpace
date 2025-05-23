//
//  SCState.h
//  libRealSpace
//
//  Created by Rémi LEONARD on 09/09/2024.
//  Copyright (c) 2013 Fabien Sanglard. All rights reserved.
//

#pragma once
#include "precomp.h"

class SCState {
public:
    std::map<uint8_t, bool> requierd_flags;
    std::map<uint8_t, bool> mission_flyed_success;
    std::vector<int64_t> missions_flags;
    std::map<uint8_t, std::uint8_t> weapon_load_out;
    std::map<uint8_t, bool> aircraftHooks;
    std::map<uint8_t, std::map<uint8_t, uint8_t>> kill_board;
    std::map<uint8_t, uint16_t> weapon_inventory;
    bool mission_accepted{false};
    uint8_t current_mission{0};
    uint8_t current_scene{0};
    uint8_t mission_id{0};
    uint8_t next_mission{0};
    uint8_t mission_flyed{0};
    uint8_t tune_modifier{0};
    int32_t cash{0};
    int32_t proj_cash{0};
    int32_t over_head{0};
    int32_t weapons_costs{0};
    int32_t f16_replacements{0};
    std::string player_name;
    std::string player_callsign;
    std::string player_firstname;
    std::string wingman;
    int32_t score{0};
    int32_t ground_kills{0};
    int32_t air_kills{0};
    SCState();
    void Load(std::string filename);
    void Save(std::string filename);
    void Reset();
};