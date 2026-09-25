#pragma once
#include "precomp.h"

class SCMissionActors;
class SCSimulatedObject;

struct ThreatScore {
    int a{0};
    int b{0};
    int aptitude{0};
};

class SCAIBrain {
public:
    static constexpr float TICK_DURATION = 1.0f / 25.0f;
    SCAIBrain(SCMissionActors *owner);
    void tick();
    bool runGoalSelectors();

    SCMissionActors *air_target{nullptr};
    SCMissionActors *ground_target{nullptr};
    SCSimulatedObject *missile_threat{nullptr};
    uint8_t threat_state{0};
    uint16_t weapon_mask{0};
    bool fire_control_enabled{true};
    bool fire_request{false};
    bool pursuit_enabled{true};
    bool pursuit_active{false};
    bool attitude_control_enabled{true};
    bool evasion_enabled{true};
    bool evasion_active{false};
    bool ground_attack_enabled{true};
    bool ground_attack_active{false};
    int fire_solution_quality{0};

    SCMissionActors *acquireBestThreat(bool allow_new_target);

private:
    SCMissionActors *owner{nullptr};
    int last_air_candidates{-1};
    int last_ground_candidates{-1};
    int last_missile_candidates{-1};
    int debug_ticks{0};
    int last_weapon_mask{-1};
    int burst_remaining{0};
    uint16_t burst_weapon{0};
    SCMissionActors *lock_target{nullptr};
    SCMissionActors *ground_attack_target{nullptr};
    int ground_phase{0};
    RSEntity *ground_weapon{nullptr};
    SCSimulatedObject *ground_released{nullptr};
    Vector3D ground_last_position{0.0f, 0.0f, 0.0f};
    int ground_last_tick{-2};
    Vector3D ground_autopilot_target{0.0f, 0.0f, 0.0f};
    SCMissionActors *pursuit_last_target{nullptr};
    float aim_trim{0.0f};
    int evasion_hold{0};
    Vector3D target_last_position{0.0f, 0.0f, 0.0f};
    Vector3D own_last_position{0.0f, 0.0f, 0.0f};
    bool fire_control_active{false};
    bool skillCheck(uint8_t stat, int modifier);
    uint16_t loadedWeaponMask();
    uint16_t selectWeaponMask();
    int computeFireSolutionQuality();
    RSEntity *loadedMissile(uint16_t mask);
    bool testMissileLock(RSEntity *missile);
    bool reactionThreshold(int quality);
    void updateFireControl();
    void updatePursuit();
    void updateGroundAttack();
    void resetGroundAttack();
    RSEntity *selectGroundWeapon();
    Vector3D predictBombImpact(RSEntity *bomb);
    float headingDelta(Vector3D direction);
    void computeAttitudeError(Vector3D direction, float &heading_error, float &pitch_error);
    int missileDistanceBand();
    void reactToMissile();
    ThreatScore scoreAirCandidate(SCMissionActors *candidate, Vector3D delta);
    ThreatScore scoreGroundCandidate(SCMissionActors *candidate, Vector3D delta);
    ThreatScore scoreMissile(SCSimulatedObject *missile);
    bool executeGoalAction();
    bool tryWanderRandom();
    void tryActiveWingman();
};
