#pragma once
#include "../strike_commander/SCAIBrain.h"

// Valeurs GOAL du profil dans Pacific Strike (PilotProfile_GoalHandlerFromValue_84AA0).
enum PSGoalSelector : uint8_t {
    PS_GOAL_EMPTY = 1,
    // Goal_ExecuteObjective_C9E4
    PS_GOAL_EXECUTE_OBJECTIVE = 2,
    // Goal_WanderRandom_D001
    PS_GOAL_WANDER_RANDOM = 3,
    // AI_BehaviorStateMachine_B968
    PS_GOAL_BEHAVIOR_STATE_MACHINE = 4,
    // Goal_MoraleReaction_B712
    PS_GOAL_MORALE_REACTION = 5,
    // Goal_FollowLeaderInFormation_C91C
    PS_GOAL_FOLLOW_LEADER_IN_FORMATION = 7,
    // Goal_FollowLeader_C93E
    PS_GOAL_FOLLOW_LEADER = 8,
    // pas de gestionnaire : règle de recherche de cible ([entité+359], Targeting_MayAcquireTarget_FF05)
    PS_GOAL_FORMATION_TARGETING = 9
};

// État de vitesse de sub_9C3D (écart vitesse voulue − vitesse, en m/s), gardé d'un tick à l'autre ([entité+250]).
enum PSSpeedState : uint8_t {
    PS_SPEED_NONE = 0,
    // écart <= −21
    PS_SPEED_MUCH_TOO_FAST = 1,
    // écart > 20
    PS_SPEED_MUCH_TOO_SLOW = 2,
    // −21 < écart <= −2
    PS_SPEED_TOO_FAST = 3,
    // 1 < écart <= 20
    PS_SPEED_TOO_SLOW = 4,
    // −2 < écart < 0
    PS_SPEED_SLIGHTLY_FAST = 5,
    // 0 <= écart <= 1
    PS_SPEED_ON_TARGET = 6
};

class PSBrain : public SCAIBrain {
public:
    PSBrain(SCMissionActors *owner);
    bool runGoalSelectors() override;
    bool acquireBestThreat(bool allow_new_target) override;
    bool in_formation{ true };
    bool isFleeing();
    uint8_t statusReply();
    bool lowFuel();
    bool seriouslyDamaged();
    bool enemiesActive();
    bool unarmed();
    void fleeHome();
    bool orderInProgress();
    void joinFormation();
    void leaveFormation();

protected:
    bool formation_targeting{ false };
    bool moraleReaction() override;
    bool canTargetAircraft() override;
    bool followLeaderInFormation();
    bool followLeader();
    void formationFlight(SCMissionActors *leader);
    bool isFighter();
    int systemDamagePercent(const std::string &name);
    PSSpeedState speed_state{ PS_SPEED_NONE };
    void speedThrottle(float wanted) override;
    PSSpeedState nextSpeedState(PSSpeedState measured, PSSpeedState previous);
    bool tickManeuver() override;
    void startFollowLeader(SCMissionActors *leader);
    bool tickFollowLeader();
    int pilotLevel();
    void updateGroundAttack(SCMissionActors *target) override;
    bool groundAhead();
    bool runTournament() override;
    bool behaviorStateMachine();
    bool combatStep(bool ground_allowed) override;
    int scoreGunAttack(SCMissionActors *target);
    int gunAimQuality(SCMissionActors *target, float error);
    int scoreFollowTarget(SCMissionActors *target);
    void startGunAttack(SCMissionActors *target);
    bool tickGunAttack();
    int gun_bursts{ 0 };
    const char *last_gun_refusal{ nullptr };
    float pull_up_timer{ 0.0f };
};
