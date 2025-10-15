#include "DebugPacificStrikeMISN.h"

DebugPacificStrikeMISN::DebugPacificStrikeMISN() {

}

DebugPacificStrikeMISN::~DebugPacificStrikeMISN() {
    
}

void DebugPacificStrikeMISN::setMission(std::string mission_name) { 
    PSStrike::setMission(mission_name.c_str());
    DebugStrike::current_mission = PSStrike::current_mission;
    DebugStrike::player_plane = PSStrike::player_plane;
    DebugStrike::cockpit = PSStrike::cockpit;
    DebugStrike::area = PSStrike::area;
    DebugStrike::camera = PSStrike::camera;
    
}
