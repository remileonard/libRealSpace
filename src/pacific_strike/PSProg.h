#pragma once
#include "../strike_commander/precomp.h"
#include "PSEntity.h"
#include "PSState.h"

class PSProg : public SCProg {
public:
    PSProg(SCMissionActors *profile, std::vector<PROG> prog, SCMission *mission, uint8_t prog_id = 0);

protected:
    PSState &PSGameState = PSState::getInstance();
    enum PSProgOp : uint8_t {
        PS_OP_NOOP_5 = 5,
        PS_OP_SWAP_WORK_AND_FLAG = 18,
        PS_OP_ADD_FLAG_TO_WORK = 33,
        PS_OP_ADD_VALUE_TO_FLAG = 34,
        PS_OP_SUB_FLAG_FROM_WORK = 37,
        PS_OP_SUB_VALUE_FROM_FLAG = 38,
        PS_OP_SUB_WORK_FROM_FLAG = 39,
        PS_OP_OR_FLAG_WITH_WORK = 41,
        PS_OP_AND_FLAG_WITH_WORK = 43,
        PS_OP_XOR_FLAG_WITH_WORK = 45,
        PS_OP_MUL_FLAG_WITH_WORK = 47,
        PS_OP_DIV_WORK_BY_FLAG = 49,
        PS_OP_CMP_WORK_WITH_FLAG = 66,
        PS_OP_CMP_FLAG_WITH_WORK = 67,
        PS_OP_MOVE_RESULT_TO_FLAG = 81,
        PS_OP_ORDER_FRIENDLY_OF_TYPE = 112,
        PS_OP_IS_SCENE_ENABLED = 130,
        PS_OP_SPAWN_EFFECT_ON_OBJ = 153,
        PS_OP_FIRE_AT_SPOT = 154,
        PS_OP_SPAWN_AT_SPOT = 155,
        PS_OP_DETACH_CARRIED = 156,
        PS_OP_LAUNCH_FROM_OBJ = 157,
        PS_OP_SET_OBJ_DESTROY_TARGET_B = 173,
        PS_OP_ATTACK_RANDOM_ENEMY_OF_TYPE = 174,
        PS_OP_DIVE_ON_RANDOM_ENEMY_OF_TYPE = 175,
        PS_OP_ORDER_OBJ_AND_SPOT = 193
    };
    const std::map<uint8_t, Instruction> &instructionTable() override;
    static std::map<uint8_t, Instruction> buildInstructionTable();

    int16_t flag(uint16_t number);
    void setVar(const char *name, uint16_t number, int value);
    uint16_t extraArg(size_t rank);
    SCMissionActors *actorOrSelf(uint16_t actor_id);
    PSKind actorKind(SCMissionActors *candidate);
    SCMissionActors *pickRandomOfKind(PSKind kind, int team);
    SCMissionActors *randomEnemyOfKind(PSKind kind);
    void traceOnce(const char *text);

    void opAdd1ToFlag(const PROG &instruction) override;
    void opRemove1ToFlag(const PROG &instruction) override;
    void opAddWorkRegisterToFlag(const PROG &instruction) override;
    void opMoveFlagToWorkRegister(const PROG &instruction) override;
    void opMoveWorkRegisterToFlag(const PROG &instruction) override;
    void opSetFlagToTrue(const PROG &instruction) override;
    void opSetFlagToFalse(const PROG &instruction) override;
    void opTestFlag(const PROG &instruction) override;
    void opActivateObj(const PROG &instruction) override;
    void opNoop5(const PROG &instruction);
    void opSwapWorkAndFlag(const PROG &instruction);
    void opAddFlagToWork(const PROG &instruction);
    void opAddValueToFlag(const PROG &instruction);
    void opSubFlagFromWork(const PROG &instruction);
    void opSubValueFromFlag(const PROG &instruction);
    void opSubWorkFromFlag(const PROG &instruction);
    void opOrFlagWithWork(const PROG &instruction);
    void opAndFlagWithWork(const PROG &instruction);
    void opXorFlagWithWork(const PROG &instruction);
    void opMulFlagWithWork(const PROG &instruction);
    void opDivWorkByFlag(const PROG &instruction);
    void opCmpWorkWithFlag(const PROG &instruction);
    void opCmpFlagWithWork(const PROG &instruction);
    void opMoveResultToFlag(const PROG &instruction);
    void opOrderFriendlyOfType(const PROG &instruction);
    void opIsSceneEnabled(const PROG &instruction);
    void opSpawnEffectOnObj(const PROG &instruction);
    void opFireAtSpot(const PROG &instruction);
    void opSpawnAtSpot(const PROG &instruction);
    void opDetachCarried(const PROG &instruction);
    void opLaunchFromObj(const PROG &instruction);
    void opSetObjDestroyTargetB(const PROG &instruction);
    void opAttackRandomEnemyOfType(const PROG &instruction);
    void opDiveOnRandomEnemyOfType(const PROG &instruction);
    void opOrderObjAndSpot(const PROG &instruction);
};
