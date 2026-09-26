#pragma once
#include "precomp.h"

class SCMissionActors;
class SCSimulatedObject;

enum ReactionLevel : uint8_t {
    REACT_NONE = 0,
    REACT_ENGAGED = 1,
    REACT_MISSILE = 2,
    REACT_NEW_TARGET = 3,
    REACT_GROUND_AVOID = 4,
    REACT_STALL_RECOVERY = 5
};

struct CombatContext {
    SCMissionActors *target{nullptr};
    bool aircraft{true};
    float my_speed{0.0f};
    float cruise{0.0f};
    float min_speed{0.0f};
    float ias{0.0f};
    Vector3D D{0.0f, 0.0f, 0.0f};
    Vector3D future_rel{0.0f, 0.0f, 0.0f};
    float dist{0.0f};
    float nose_angle{0.0f};
    float future_angle{0.0f};
    float target_vel_angle{0.0f};
    float aspect{0.0f};
    float target_speed{0.0f};
    float side{0.0f};
    bool behind{false};
    bool head_on{false};
};

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
    bool brain_orders_enabled{true};
    uint8_t reaction_level{REACT_NONE};
    bool just_hit{false};
    SCMissionActors *last_attacker{nullptr};
    bool objective_locked{false};
    int morale{2};
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
    std::vector<SCSimulatedObject *> ground_known_objects;
    int ground_release_wait{0};
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
    int seekerSignature(RSEntity *weapon, SCMissionActors *candidate, Vector3D reference_velocity);
    bool seekerSees(RSEntity *weapon, SCMissionActors *candidate);
    SCMissionActors *seekerSelect(RSEntity *weapon, SCMissionActors *desired);
    bool reactionThreshold(int quality);
    void updateFireControl();
    void updatePursuit();
    void updateGroundAttack(SCMissionActors *target);
    bool combatStep(bool ground_allowed);
    bool destroyTargetOrder(uint8_t arg);
    bool takeoffOrder();
    bool landingOrder(uint8_t approach_spot, uint8_t touchdown_spot);
    bool groundOpRunning() const { return ground_op != GROUND_OP_NONE; }
    bool defendTargetOrder(uint8_t arg);
    bool followAllyOrder(uint8_t arg);
    CombatContext ctx;
    int maneuver_id{0};
    SCMissionActors *maneuver_target{nullptr};
    uint8_t maneuver_level{0};
    float maneuver_timer{0.0f};
    int maneuver_phase{0};
    int maneuver_phase_hint{0};
    int maneuver_legs{0};
    int maneuver_side{0};
    uint8_t maneuver_bits{0};
    Vector3D maneuver_point{0.0f, 0.0f, 0.0f};
    Vector3D maneuver_leg{0.0f, 0.0f, 0.0f};
    float maneuver_leg_timer{0.0f};
    float maneuver_start_heading{0.0f};
    int maneuver_uses[32]{};
    bool threat_alerted{false};
    bool combat_step_called{false};
    float retarget_clock{0.0f};
    bool retarget_fired{false};
    SCSimulatedObject *complained_missile{nullptr};
    float ground_phase3_time{0.0f};
    Vector3D actorVelocity(SCMissionActors *actor);
    float floorAltitude();
    float indicatedAirspeed();
    bool tooSlow();
    bool tooLow();
    int decisionWeight();
    void buildCombatContext(SCMissionActors *target);
    int scoreManeuver(int id, SCMissionActors *target);
    bool runTournament();
    void applyManeuver(int id, SCMissionActors *target, uint8_t level);
    void endManeuver();
    bool tickManeuver();
    bool tickLeg();
    void maneuverSpeed(float wanted);
    void speedThrottle(float wanted);
    bool ejectDecision(int mode);
    void runReflexes();
    void incomingThreatWarning();
    int computeMorale();
    bool canHoldOrder();
    bool moraleReaction();
    void leaveFight();
    SCMissionActors *playerActor();
    SCMissionActors *leaderActor();
    bool disciplined{true};
    float discipline_timer{0.0f};
    float morale_timer{0.0f};
    bool mutiny{false};
    bool fleeing{false};
    Vector3D brain_destination{0.0f, 0.0f, 0.0f};
    Vector3D home_position{0.0f, 0.0f, 0.0f};
    bool home_set{false};
    int enemies_alive{0};
    int own_losses{0};
    bool enemies_active{false};
    bool engageAttackerReaction();
    uint8_t leader_state{0};
    bool navigateToPoint(Vector3D point, float radius);
    void stopNavigation();
    void navigateToPilotWaypoint();
    void wander();
    bool nav_active{false};
    bool nav_requested{false};
    Vector3D wander_point{0.0f, 0.0f, 0.0f};
    bool wander_point_set{false};
    int defend_state{-1};
    bool ground_attack_seen{false};
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
    enum GroundOp { GROUND_OP_NONE, GROUND_OP_TAKEOFF, GROUND_OP_LANDING };
    GroundOp ground_op{GROUND_OP_NONE};
    int ground_op_phase{0};
    float ground_op_time{0.0f};
    float ground_op_stick{0.0f};
    Vector3D ground_op_axis{0.0f, 0.0f, 1.0f};
    Vector3D ground_op_origin{0.0f, 0.0f, 0.0f};
    Vector3D landing_target{0.0f, 0.0f, 0.0f};
    Vector3D landing_dir{0.0f, 0.0f, 0.0f};
    float landing_speed{0.0f};
    float landing_duration{0.0f};
    float landing_pitch{0.0f};
    int landing_counter{0};
    bool landing_leveled{false};
    bool landing_done{false};
    Vector3D runwayAxis(Vector3D direction);
    void endGroundOp();
};
