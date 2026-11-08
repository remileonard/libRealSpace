//
//  SCState.h
//  libRealSpace
//
//  Created by Rémi LEONARD on 09/09/2024.
//  Copyright (c) 2013 Fabien Sanglard. All rights reserved.
//

#pragma once
#include <array>

// Compte rendu de vol transmis de SCStrike à SCGameFlow : contenu de misn2op.iff (GAMEFLOW_SYSTEM.md § 6).
// Réglages des écrans d'entraînement (Training_InitSettings_598FC, 42 octets dans OPTEST).
struct MISN_TRAINING {
    // +40 : 1 = DOGFIGHT, 2 = SEARCH AND DESTROY.
    uint8_t type{ 0 };
    // +12, +14, +16 : nombre d'avions des groupes 0, 1, 2.
    std::array<uint8_t, 3> group_count{};
    // +6, +8, +10 : type d'avion des groupes (rang dans SCTrainingMenu::training_planes).
    std::array<uint8_t, 3> group_plane{};
    // +0 : altitude choisie, en pieds (5000 par défaut).
    int32_t altitude_feet{ 5000 };
    // +18 : 0 = rookie, 1 = veteran, 2 = ace.
    uint8_t pilot_level{ 0 };
};

struct MissionReport {
    uint8_t exit_code{0};                                                    // EXIT (2 = joueur mort)
    std::string mission_name;                                                // INFO
    std::unordered_map<uint8_t, int16_t> weapons_left;                       // WPNS : nouveau stock, par weapon_ids
    bool weapons_reported{false};                                            // WPNS ecrit (stock pris au depart)
    int16_t planes_lost{0};                                                  // JETS
    std::array<int16_t, 8> results{};                                        // STAT, par numéro de valeur
    std::unordered_map<uint8_t, std::unordered_map<uint8_t, int16_t>> kills; // KILL : PilotsId, KillBoardType
    std::vector<std::string> died;                                           // DIED
    int32_t score{0};                                                        // SCOR
};

class SCState {
private:
    inline static std::unique_ptr<SCState> s_instance{};    
public:
    std::unordered_map<uint8_t, bool> requierd_flags;
    std::unordered_map<uint8_t, bool> mission_flyed_success;
    std::vector<int64_t> missions_flags;
    std::unordered_map<uint8_t, std::uint8_t> weapon_load_out;
    std::unordered_map<uint8_t, bool> aircraftHooks;
    std::unordered_map<uint8_t, std::unordered_map<uint8_t, int16_t>> kill_board;
    std::unordered_map<uint8_t, uint8_t> pilot_roaster;
    std::unordered_map<uint8_t, int16_t> weapon_inventory;
    bool mission_accepted{false};
    uint8_t current_mission{0};
    uint8_t current_scene{0};
    uint8_t mission_id{0};
    uint8_t next_mission{0};
    uint8_t mission_flyed{0};
    uint8_t tune_modifier{1};
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
    bool player_dead{false};
    bool player_ejected{false};
    int32_t lost_aircraft{0};
    MissionReport mission_report;
    // Mission d'entraînement : posée par SCTrainingMenu avant le vol, lue par SCMission::loadMission
    // (OPTEST : [partie+249] = 254, mission « instant »).
    bool training_active{false};
    MISN_TRAINING training;

    static SCState& getInstance() {
        if (!SCState::hasInstance()) {
            SCState::setInstance(std::make_unique<SCState>());
        }
        SCState& instance = SCState::instance();
        return instance;
    };
    static SCState& instance() {
        return *s_instance;
    }
    static void setInstance(std::unique_ptr<SCState> inst) {
        s_instance = std::move(inst);
    }
    static bool hasInstance() { return (bool)s_instance; }


    SCState();
    void Load(std::string filename);
    void Save(std::string filename);
    // Fichier de partie de Tactical Operations (598 octets).
    void LoadTO(std::string filename);
    void SaveTO(std::string filename);
    void Reset();
    // Pacific Strike : variables et drapeaux d'une seule table, partagée par le déroulement, les conversations et les
    // missions (GameVar_Get_56414, GameVar_SetFlag_5646D, GameVar_ClearFlag_56520, GameVar_Set_564BB de PACIFIC.EXE).
    // Numéro < 256 : variable (mot) ; numéro >= 256 : drapeau (bit numéro - 256).
    std::unordered_map<uint16_t, int16_t> game_vars;
    int16_t getGameVar(uint16_t number);
    void setGameVar(uint16_t number, int16_t value);
    void setGameFlag(uint16_t number);
    void clearGameFlag(uint16_t number);
};