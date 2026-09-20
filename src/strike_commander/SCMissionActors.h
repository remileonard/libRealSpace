#pragma once
#include "precomp.h"

class SCMission;
class SCAIBrain;


class SCMissionActors {
public:
    bool talkative{true};
    int score{0};
    int plane_down{0};
    int ground_down{0};
    std::string actor_name;
    uint8_t actor_id{0};
    RSProf *profile{nullptr};
    std::vector<PROG> prog;
    std::vector<PROG> on_mission_start;
    std::vector<PROG> on_is_activated;
    std::vector<PROG> on_is_destroyed;
    std::vector<PROG> on_update;
    std::vector<PROG> override_progs;
    std::vector<SCSimulatedObject *> weapons_shooted;
    MISN_PART *object{nullptr};
    SCPlane *plane{nullptr};
    SCPilot *pilot{nullptr};
    SCAIBrain *brain{nullptr};
    SCMission *mission{nullptr};
    SCMissionActors *target{nullptr};
    SCMissionActors *attacker{nullptr};
    SCSimulatedObject *weapon_shooted_at_me{nullptr};
    prog_op current_objective;
    Vector3D formation_pos_offset{150.0f, 0.0f, 0.0f};
    Vector3D attack_pos_offset{0.0f, 0.0f, -1000.0f};
    bool is_active{false};
    bool is_hidden{true};
    bool taken_off{false};
    bool is_destroyed{false};
    bool prog_executed{false};
    int health{0};
    int team_id{0};
    static constexpr int NO_TARGET = -1;
    int current_target{NO_TARGET};
    bool current_command_executed{false};
    prog_op current_command{prog_op::OP_NOOP};
    prog_op override_command{prog_op::OP_NOOP};
    uint8_t current_command_arg;
    Vector3D aiming_vector{0.0f, 0.0f, 0.0f};
    std::vector<uint8_t> executed_opcodes;
    int retarget_cooldown{0};
    int timer{0};
    int wait_timer{0};
    virtual bool wait(int seconds);
    virtual bool execute();
    virtual bool takeOff(uint8_t arg); 
    virtual bool land(uint8_t arg);
    virtual bool flyToWaypoint(uint8_t arg);
    virtual bool flyToArea(uint8_t arg);
    virtual bool destroyTarget(uint8_t arg);
    virtual bool defendTarget(uint8_t arg);
    virtual bool defendArea(uint8_t arg);
    virtual bool deactivate(uint8_t arg);
    virtual bool setMessage(uint8_t arg);
    virtual bool followAlly(uint8_t arg);
    virtual bool protectSelf();
    virtual bool ifTargetInSameArea(uint8_t arg);
    virtual bool respondToRadioMessage(int message_id, SCMission *mission, SCMissionActors *sender=nullptr);
    virtual bool activateTarget(uint8_t arg);
    // Pose l'objectif courant (current_command/current_command_arg) sans
    // l'executer : le script PROG (SCProg) appelle uniquement ceci, c'est
    // SCAIBrain::executeGoalAction() qui execute reellement la commande persistee, a
    // la cadence GOAL/AIRefresh. Ne remet current_command_executed a false
    // que lors d'une VRAIE transition (commande ou argument different) —
    // sinon, comme le script repasse par cet appel a chaque frame tant que
    // l'objectif est en cours, on ecraserait en permanence le resultat
    // calcule par le dernier passage du GOAL loop.
    // Virtuel : le script de mission du joueur (SCMissionActorsPlayer) est
    // une liste continue d'objectifs executee UNE SEULE FOIS au chargement
    // (script d'initialisation, pas un etat re-evalue en continu comme pour
    // l'IA) — sa surcharge execute donc directement la commande ici, au
    // moment ou elle est posee.
    virtual void setObjective(prog_op command, uint8_t arg);
    virtual int getDistanceToTarget(uint8_t arg);
    virtual int getDistanceToSpot(uint8_t arg);
    virtual void shootWeapon(SCMissionActors *target);
    virtual void hasBeenHit(SCSimulatedObject *weapon, SCMissionActors *attacker);
    SCMissionActors();
    ~SCMissionActors();
private:
    Vector3D target_position{0.0f, 0.0f, 0.0f};
    int target_position_update{0};
    int current_weapon_index{-1};
    
    AssetManager &Assets = AssetManager::getInstance();
    MessageBus &messageBus = MessageBus::getInstance();
    void onEvent(const EventMessage &event);
    void onGettingHit(const MissionEventActorHit &event);
    void onMissionUpdate(const MissionUpdateEvent &event);
    // Point d'entree de la decision IA (AI_TopLevelThink), a la cadence fixe
    // ~25fps d'AIRefreshEvent — voir analysis/AI_SYSTEM.md §4 et
    // AI_IMPLEMENTATION_GUIDE.md §2. Ne concerne que les acteurs porteurs
    // d'un profil GOAL (ai.isAI && !ai.goal.empty()). L'execution du script
    // de mission (on_update) reste dans onMissionUpdate, a chaque frame,
    // pour tous les acteurs : elle POSE current_command, elle ne l'execute
    // pas — c'est le role de SCAIBrain::executeGoalAction(). override_progs
    // (les ordres radio acceptes, expression du selecteur GOAL_ACTIVE_WINGMAN)
    // est en revanche traite par SCAIBrain::tryActiveWingman() — uniquement pour
    // les acteurs qui ont 5 dans leur GOAL.
    void onAIRefresh(const AIRefreshEvent &event);
    MessageBus::SubscriptionId subscription_id{-1};
};

class SCMissionActorsPlayer : public SCMissionActors {
public:
    bool takeOff(uint8_t arg) override;
    bool land(uint8_t arg) override;
    bool flyToWaypoint(uint8_t arg) override;
    bool flyToArea(uint8_t arg) override;
    bool destroyTarget(uint8_t arg) override;
    bool defendTarget(uint8_t arg) override;
    bool setMessage(uint8_t arg) override;
    void hasBeenHit(SCSimulatedObject *weapon, SCMissionActors *attacker) override;
    void setObjective(prog_op command, uint8_t arg) override;
};

class SCMissionActorsStrikeBase : public SCMissionActors {
public:
    bool setMessage(uint8_t arg) override;
};