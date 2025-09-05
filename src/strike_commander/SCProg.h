#pragma once

#include "precomp.h"


class SCProg {
public:
    SCMissionActors *actor;
    std::vector<PROG> prog;
    SCMission *mission;
    uint8_t prog_id{0};
    std::unordered_map<uint8_t, size_t> labels;

    SCProg(SCMissionActors *profile, std::vector<PROG> prog, SCMission *mission, uint8_t prog_id = 0) {
        this->actor = profile;
        this->prog = prog;
        this->mission = mission;
        this->prog_id = prog_id;
        this->labels.clear();
    };
    ~SCProg() {
        this->prog.clear();
        this->prog.shrink_to_fit();
        this->labels.clear();
        this->actor = nullptr;
        this->mission = nullptr;
    };
    void execute();
};