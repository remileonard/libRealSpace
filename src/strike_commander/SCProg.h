#pragma once

#include "precomp.h"

class SCProg {
public:
    SCMissionActors *actor{ nullptr };
    std::vector<PROG> prog;
    SCMission *mission{ nullptr };
    uint8_t prog_id{ 0 };
    std::unordered_map<uint8_t, size_t> labels;
    int task_state{ 0 };
    bool scene_script{ false };

    SCProg(SCMissionActors *profile, std::vector<PROG> prog, SCMission *mission, uint8_t prog_id = 0) {
        this->actor = profile;
        this->prog = prog;
        this->mission = mission;
        this->prog_id = prog_id;
        this->labels.clear();
    };
    virtual ~SCProg() {
        this->prog.clear();
        this->prog.shrink_to_fit();
        this->labels.clear();
        this->actor = nullptr;
        this->mission = nullptr;
    };
    void execute();

protected:
    // Instructions du script : numéro → fonction membre, table construite une seule fois et partagée. Chaque fonction
    // lit et modifie l'état du processeur ci-dessous ; pendant un saut (exec faux), seul le label cherché agit. Une
    // autre version du jeu redéfinit instructionTable pour rendre sa propre table.
    typedef void (SCProg::*Instruction)(const PROG &);
    virtual const std::map<uint8_t, Instruction> &instructionTable();
    static std::map<uint8_t, Instruction> buildInstructionTable();

    // État du processeur, remis à zéro par execute.
    size_t pc{ 0 };
    size_t jump_to{ 0 };
    bool exec{ true };
    bool stopped{ false };
    int work_register{ 0 };
    int compare_flag{ 0 };
    SPOT *spot{ nullptr };

    void jumpTo(size_t label);
    SCMissionActors *findActor(uint16_t actor_id);

    virtual void opExitProg(const PROG &instruction);
    virtual void opSpotData(const PROG &instruction);
    virtual void opSetLabel(const PROG &instruction);
    virtual void opMoveValueToWorkRegister(const PROG &instruction);
    virtual void opGotoIfCurrentCommandInProgress(const PROG &instruction);
    virtual void opIfTargetInArea(const PROG &instruction);
    virtual void opAdd1ToFlag(const PROG &instruction);
    virtual void opRemove1ToFlag(const PROG &instruction);
    virtual void opAddWorkRegisterToFlag(const PROG &instruction);
    virtual void opInstantDestroyTarget(const PROG &instruction);
    virtual void opSetWaitForSeconds(const PROG &instruction);
    virtual void opSetObjTakeOff(const PROG &instruction);
    virtual void opSetObjLand(const PROG &instruction);
    virtual void opSetObjFlyToWp(const PROG &instruction);
    virtual void opSetObjFlyToArea(const PROG &instruction);
    virtual void opSetObjDestroyTarget(const PROG &instruction);
    virtual void opSetObjDefendTarget(const PROG &instruction);
    virtual void opSetObjDefendArea(const PROG &instruction);
    virtual void opSetMessage(const PROG &instruction);
    virtual void opSetObjFollowAlly(const PROG &instruction);
    virtual void opDeactivateObj(const PROG &instruction);
    virtual void opShowHideOnNavMap(const PROG &instruction);
    virtual void opJoinLeavePlayerTeam(const PROG &instruction);
    virtual void opPacifyTarget(const PROG &instruction);
    virtual void opSetObjPatrolPosition(const PROG &instruction);
    virtual void opActivateObj(const PROG &instruction);
    virtual void opMoveFlagToWorkRegister(const PROG &instruction);
    virtual void opSaveValueToGameflowRegister(const PROG &instruction);
    virtual void opSetExitCode(const PROG &instruction);
    virtual void opMoveWorkRegisterToFlag(const PROG &instruction);
    virtual void opExecuteCall(const PROG &instruction);
    virtual void opExecSubProg(const PROG &instruction);
    virtual void opCmpWorkWithValue(const PROG &instruction);
    virtual void opCmpValueWithWork(const PROG &instruction);
    virtual void opBranchIfEqual(const PROG &instruction);
    virtual void opBranchIfNotEqual(const PROG &instruction);
    virtual void opBranchIfLess(const PROG &instruction);
    virtual void opBranchIfGreater(const PROG &instruction);
    virtual void opSelectFlag208(const PROG &instruction);
    virtual void opDistToTarget(const PROG &instruction);
    virtual void opDistToSpot(const PROG &instruction);
    virtual void opIsTargetAlive(const PROG &instruction);
    virtual void opIsTargetActive(const PROG &instruction);
    virtual void opSetFlagToTrue(const PROG &instruction);
    virtual void opSetFlagToFalse(const PROG &instruction);
    virtual void opTestFlag(const PROG &instruction);
    virtual void opMulValueWithWork(const PROG &instruction);
    virtual void opActivateScene(const PROG &instruction);
    virtual void opDeactivateScene(const PROG &instruction);
};
