#include "precomp.h"

bool SCMissionActors::execute() {
    return true;
}
bool SCMissionActors::takeOff(uint8_t arg) {
    return true;
}
bool SCMissionActors::land(uint8_t arg) {
    return true;
}
bool SCMissionActors::flyToWaypoint(uint8_t arg) {
    return true;
}
bool SCMissionActors::flyToArea(uint8_t arg) {
    return true;
}
bool SCMissionActors::destroyTarget(uint8_t arg) {
    return true;
}
bool SCMissionActors::defendTarget(uint8_t arg) {
    return true;
}
bool SCMissionActors::deactivate(uint8_t arg) {
    return true;
}
bool SCMissionActors::setMessage(uint8_t arg) {
    return true;
}


bool SCMissionActorsPlayer::takeOff(uint8_t arg) {
    SCMissionWaypoint *waypoint = new SCMissionWaypoint();
    waypoint->spot = this->mission->mission->mission_data.spots[arg];
    waypoint->objective = new std::string("take off");
    this->mission->waypoints.push_back(waypoint);
    return true;
}
bool SCMissionActorsPlayer::land(uint8_t arg) {
    SCMissionWaypoint *waypoint = new SCMissionWaypoint();
    waypoint->spot = this->mission->mission->mission_data.spots[arg];
    waypoint->objective = new std::string("landing");
    this->mission->waypoints.push_back(waypoint);
    return true;
}
bool SCMissionActorsPlayer::flyToWaypoint(uint8_t arg) {
    SCMissionWaypoint *waypoint = new SCMissionWaypoint();
    waypoint->spot = this->mission->mission->mission_data.spots[arg];
    waypoint->objective = new std::string("Fly to\nWay Point");
    this->mission->waypoints.push_back(waypoint);
    return true;
}
bool SCMissionActorsPlayer::flyToArea(uint8_t arg) {
    SCMissionWaypoint *waypoint = new SCMissionWaypoint();
    waypoint->spot = this->mission->mission->mission_data.spots[arg];
    waypoint->objective = new std::string("Fly to\nWay Area");
    this->mission->waypoints.push_back(waypoint);
    return true;
}
bool SCMissionActorsPlayer::destroyTarget(uint8_t arg) {
    SCMissionWaypoint *waypoint = new SCMissionWaypoint();
    waypoint->spot = this->mission->mission->mission_data.spots[arg];
    waypoint->objective = new std::string("Destroy\nTarget");
    this->mission->waypoints.push_back(waypoint);
    return true;
}
bool SCMissionActorsPlayer::defendTarget(uint8_t arg) {
    SCMissionWaypoint *waypoint = new SCMissionWaypoint();
    waypoint->spot = this->mission->mission->mission_data.spots[arg];
    waypoint->objective = new std::string("Defend\nAlly");
    this->mission->waypoints.push_back(waypoint);
    return true;
}
/*
Fly to#Precise Way
Defend#Point
Follow#Leader
*/
bool SCMissionActorsPlayer::setMessage(uint8_t arg) {
    std::transform(this->mission->mission->mission_data.messages[arg]->begin(), this->mission->mission->mission_data.messages[arg]->end(), this->mission->mission->mission_data.messages[arg]->begin(), ::tolower);
    if (this->mission->waypoints.size() > 0) {
        this->mission->waypoints.back()->message = this->mission->mission->mission_data.messages[arg];
    }
    return true;
}