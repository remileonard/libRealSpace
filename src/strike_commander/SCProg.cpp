#include "precomp.h"
#include "../engine/gametimer.h"

// Table des instructions des scripts de mission : numéro → fonction. Une autre version du jeu redéfinit cette méthode
// (appel de celle-ci, puis ajout, remplacement ou retrait).
void SCProg::initInstructions() {
    this->instructions.clear();
    this->instructions[OP_EXIT_PROG] = std::bind(&SCProg::opExitProg, this, std::placeholders::_1);
    this->instructions[OP_SPOT_DATA] = std::bind(&SCProg::opSpotData, this, std::placeholders::_1);
    this->instructions[OP_SET_LABEL] = std::bind(&SCProg::opSetLabel, this, std::placeholders::_1);
    this->instructions[OP_MOVE_VALUE_TO_WORK_REGISTER] = std::bind(&SCProg::opMoveValueToWorkRegister, this, std::placeholders::_1);
    this->instructions[OP_GOTO_IF_CURRENT_COMMAND_IN_PROGRESS] = std::bind(&SCProg::opGotoIfCurrentCommandInProgress, this, std::placeholders::_1);
    this->instructions[OP_IF_TARGET_IN_AREA] = std::bind(&SCProg::opIfTargetInArea, this, std::placeholders::_1);
    this->instructions[OP_ADD_1_TO_FLAG] = std::bind(&SCProg::opAdd1ToFlag, this, std::placeholders::_1);
    this->instructions[OP_REMOVE_1_TO_FLAG] = std::bind(&SCProg::opRemove1ToFlag, this, std::placeholders::_1);
    this->instructions[OP_ADD_WORK_REGISTER_TO_FLAG] = std::bind(&SCProg::opAddWorkRegisterToFlag, this, std::placeholders::_1);
    this->instructions[OP_INSTANT_DESTROY_TARGET] = std::bind(&SCProg::opInstantDestroyTarget, this, std::placeholders::_1);
    this->instructions[OP_SET_WAIT_FOR_SECONDS] = std::bind(&SCProg::opSetWaitForSeconds, this, std::placeholders::_1);
    this->instructions[OP_SET_OBJ_TAKE_OFF] = std::bind(&SCProg::opSetObjTakeOff, this, std::placeholders::_1);
    this->instructions[OP_SET_OBJ_LAND] = std::bind(&SCProg::opSetObjLand, this, std::placeholders::_1);
    this->instructions[OP_SET_OBJ_FLY_TO_WP] = std::bind(&SCProg::opSetObjFlyToWp, this, std::placeholders::_1);
    this->instructions[OP_SET_OBJ_FLY_TO_AREA] = std::bind(&SCProg::opSetObjFlyToArea, this, std::placeholders::_1);
    this->instructions[OP_SET_OBJ_DESTROY_TARGET] = std::bind(&SCProg::opSetObjDestroyTarget, this, std::placeholders::_1);
    this->instructions[OP_SET_OBJ_DEFEND_TARGET] = std::bind(&SCProg::opSetObjDefendTarget, this, std::placeholders::_1);
    this->instructions[OP_SET_OBJ_DEFEND_AREA] = std::bind(&SCProg::opSetObjDefendArea, this, std::placeholders::_1);
    this->instructions[OP_SET_MESSAGE] = std::bind(&SCProg::opSetMessage, this, std::placeholders::_1);
    this->instructions[OP_SET_OBJ_FOLLOW_ALLY] = std::bind(&SCProg::opSetObjFollowAlly, this, std::placeholders::_1);
    this->instructions[OP_DEACTIVATE_OBJ] = std::bind(&SCProg::opDeactivateObj, this, std::placeholders::_1);
    this->instructions[OP_SHOW_ON_NAV_MAP] = std::bind(&SCProg::opShowHideOnNavMap, this, std::placeholders::_1);
    this->instructions[OP_HIDE_FROM_NAV_MAP] = std::bind(&SCProg::opShowHideOnNavMap, this, std::placeholders::_1);
    this->instructions[OP_JOIN_PLAYER_TEAM] = std::bind(&SCProg::opJoinLeavePlayerTeam, this, std::placeholders::_1);
    this->instructions[OP_LEAVE_PLAYER_TEAM] = std::bind(&SCProg::opJoinLeavePlayerTeam, this, std::placeholders::_1);
    this->instructions[OP_PACIFY_TARGET] = std::bind(&SCProg::opPacifyTarget, this, std::placeholders::_1);
    this->instructions[OP_SET_OBJ_PATROL_POSITION] = std::bind(&SCProg::opSetObjPatrolPosition, this, std::placeholders::_1);
    this->instructions[OP_ACTIVATE_OBJ] = std::bind(&SCProg::opActivateObj, this, std::placeholders::_1);
    this->instructions[OP_MOVE_FLAG_TO_WORK_REGISTER] = std::bind(&SCProg::opMoveFlagToWorkRegister, this, std::placeholders::_1);
    this->instructions[OP_SAVE_VALUE_TO_GAMFLOW_REGISTER] = std::bind(&SCProg::opSaveValueToGameflowRegister, this, std::placeholders::_1);
    this->instructions[OP_SET_EXIT_CODE] = std::bind(&SCProg::opSetExitCode, this, std::placeholders::_1);
    this->instructions[OP_MOVE_WORK_REGISTER_TO_FLAG] = std::bind(&SCProg::opMoveWorkRegisterToFlag, this, std::placeholders::_1);
    this->instructions[OP_EXECUTE_CALL] = std::bind(&SCProg::opExecuteCall, this, std::placeholders::_1);
    this->instructions[OP_EXEC_SUB_PROG] = std::bind(&SCProg::opExecSubProg, this, std::placeholders::_1);
    this->instructions[OP_CMP_WORK_WITH_VALUE] = std::bind(&SCProg::opCmpWorkWithValue, this, std::placeholders::_1);
    this->instructions[OP_CMP_VALUE_WITH_WORK] = std::bind(&SCProg::opCmpValueWithWork, this, std::placeholders::_1);
    this->instructions[OP_BRANCH_IF_EQUAL] = std::bind(&SCProg::opBranchIfEqual, this, std::placeholders::_1);
    this->instructions[OP_BRANCH_IF_NOT_EQUAL] = std::bind(&SCProg::opBranchIfNotEqual, this, std::placeholders::_1);
    this->instructions[OP_BRANCH_IF_LESS] = std::bind(&SCProg::opBranchIfLess, this, std::placeholders::_1);
    this->instructions[OP_BRANCH_IF_GREATER] = std::bind(&SCProg::opBranchIfGreater, this, std::placeholders::_1);
    this->instructions[OP_SELECT_FLAG_208] = std::bind(&SCProg::opSelectFlag208, this, std::placeholders::_1);
    this->instructions[OP_DIST_TO_TARGET] = std::bind(&SCProg::opDistToTarget, this, std::placeholders::_1);
    this->instructions[OP_DIST_TO_SPOT] = std::bind(&SCProg::opDistToSpot, this, std::placeholders::_1);
    this->instructions[OP_IS_TARGET_ALIVE] = std::bind(&SCProg::opIsTargetAlive, this, std::placeholders::_1);
    this->instructions[OP_IS_TARGET_ACTIVE] = std::bind(&SCProg::opIsTargetActive, this, std::placeholders::_1);
    this->instructions[OP_SET_FLAG_TO_TRUE] = std::bind(&SCProg::opSetFlagToTrue, this, std::placeholders::_1);
    this->instructions[OP_SET_FLAG_TO_FALSE] = std::bind(&SCProg::opSetFlagToFalse, this, std::placeholders::_1);
    this->instructions[OP_TEST_FLAG] = std::bind(&SCProg::opTestFlag, this, std::placeholders::_1);
    this->instructions[OP_MUL_VALUE_WITH_WORK] = std::bind(&SCProg::opMulValueWithWork, this, std::placeholders::_1);
    this->instructions[OP_ACTIVATE_SCENE] = std::bind(&SCProg::opActivateScene, this, std::placeholders::_1);
    this->instructions[OP_DEACTIVATE_SCENE] = std::bind(&SCProg::opDeactivateScene, this, std::placeholders::_1);
}

// Parcourt le script : chaque instruction est passée à sa fonction (table instructions), qui tient compte de l'état
// du processeur (exec faux pendant un saut vers jump_to). Une instruction jouée, ou le label qui termine un saut, est
// notée dans les traces. OP_EXIT_PROG arrête le parcours (stopped).
void SCProg::execute() {
    this->pc = 0;
    this->jump_to = 0;
    this->exec = true;
    this->stopped = false;
    this->work_register = 0;
    this->compare_flag = prog_compare_return_values::PROG_CMP_UNSET;
    this->spot = nullptr;
    this->actor->executed_opcodes.clear();
    this->actor->executed_opcodes.shrink_to_fit();
    this->mission->progs_traces[this->prog_id].clear();
    this->mission->progs_traces[this->prog_id].shrink_to_fit();
    for (this->pc = 0; this->pc < this->prog.size(); this->pc++) {
        const PROG &instruction = this->prog[this->pc];
        bool was_executing = this->exec;
        auto found = this->instructions.find(instruction.opcode);
        if (found != this->instructions.end()) {
            found->second(instruction);
        }
        if (was_executing || this->exec) {
            this->actor->executed_opcodes.push_back((uint8_t)this->pc);
            this->mission->progs_traces[this->prog_id].push_back((uint8_t)this->pc);
        }
        if (this->stopped) {
            return;
        }
    }
}

void SCProg::jumpTo(size_t label) {
    this->jump_to = label;
    this->exec = false;
}

SCMissionActors *SCProg::findActor(uint16_t actor_id) {
    for (auto candidate : this->mission->actors) {
        if (candidate->actor_id == actor_id) {
            return candidate;
        }
    }
    return nullptr;
}

void SCProg::opExitProg(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->stopped = true;
}

void SCProg::opSpotData(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    if (instruction.arg < this->mission->mission->mission_data.spots.size()) {
        this->spot = this->mission->mission->mission_data.spots[instruction.arg];
    }
}

// Seule instruction qui agit pendant un saut : le label cherché rétablit l'exécution.
void SCProg::opSetLabel(const PROG &instruction) {
    if (this->jump_to == instruction.arg) {
        this->exec = true;
    }
}

void SCProg::opMoveValueToWorkRegister(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->work_register = instruction.arg;
}

void SCProg::opGotoIfCurrentCommandInProgress(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    if (this->task_state != 0) {
        this->jumpTo(instruction.arg);
    }
}

void SCProg::opIfTargetInArea(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    if (this->actor->ifTargetInSameArea(instruction.arg)) {
        this->compare_flag = prog_compare_return_values::PROG_CMP_EQUAL;
    } else {
        this->compare_flag = prog_compare_return_values::PROG_CMP_NOT_EQUAL;
    }
}

void SCProg::opAdd1ToFlag(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->mission->mission->mission_data.flags[instruction.arg]++;
}

void SCProg::opRemove1ToFlag(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->mission->mission->mission_data.flags[instruction.arg]--;
}

void SCProg::opAddWorkRegisterToFlag(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->mission->mission->mission_data.flags[instruction.arg] += this->work_register;
}

void SCProg::opInstantDestroyTarget(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    for (auto actor : this->mission->actors) {
        if (actor->actor_id == instruction.arg) {
            actor->object->alive = false;
            actor->is_destroyed = false;
            if (actor->object->entity->explos != nullptr && actor->object->entity->explos->objct != nullptr) {
                actor->mission->explosions.push_back(new SCExplosion(actor->object->entity->explos->objct, actor->object->position));
            }
            break;
        }
    }
}

// Expr_VM_Interpreter_51106 cas 0xA0 : minuteur de l'objet de mission (+0x3A), global pour une scene
void SCProg::opSetWaitForSeconds(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    float &timer = this->scene_script ? this->mission->scene_wait_timer : this->actor->wait_timer;
    this->task_state = 1;
    if (timer == 0.0f) {
        timer = (float)instruction.arg;
    } else {
        timer -= GameTimer::getInstance().getDeltaTime();
        if (timer <= 0.0f) {
            timer = 0.0f;
            this->task_state = 0;
        }
    }
}

void SCProg::opSetObjTakeOff(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->task_state = this->actor->setObjective(OP_SET_OBJ_TAKE_OFF, instruction.arg) ? 1 : 0;
}

void SCProg::opSetObjLand(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->task_state = this->actor->setObjective(OP_SET_OBJ_LAND, instruction.arg) ? 1 : 0;
    if (this->actor->current_command == OP_SET_OBJ_LAND && this->actor->current_command_arg == instruction.arg) {
        // Expr_VM_ReadNextToken_50F85 : un opcode 9 qui suit fournit le 2e operande (spot de toucher)
        bool has_second = this->pc + 1 < this->prog.size() && this->prog[this->pc + 1].opcode == OP_SPOT_DATA;
        this->actor->current_command_arg2 = has_second ? (uint8_t)this->prog[this->pc + 1].arg : 0xFF;
    }
}

void SCProg::opSetObjFlyToWp(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->task_state = this->actor->setObjective(OP_SET_OBJ_FLY_TO_WP, instruction.arg) ? 1 : 0;
    if (this->actor->current_command == OP_SET_OBJ_FLY_TO_WP && this->actor->current_command_arg == instruction.arg) {
        // 2e operande (opcode 9) : vitesse voulue a l'arrivee (Goal_SetObjective_A307 -> +0x12B)
        bool has_second = this->pc + 1 < this->prog.size() && this->prog[this->pc + 1].opcode == OP_SPOT_DATA;
        this->actor->current_command_arg2 = has_second ? (uint8_t)this->prog[this->pc + 1].arg : 0xFF;
    }
}

// Expr_VM_Interpreter_51106 : 0xA6 -> cas par defaut, pas d'appel natif ; seul le joueur l'exploite
void SCProg::opSetObjFlyToArea(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->actor->setObjective(OP_SET_OBJ_FLY_TO_AREA, instruction.arg);
}

void SCProg::opSetObjDestroyTarget(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->task_state = this->actor->setObjective(OP_SET_OBJ_DESTROY_TARGET, instruction.arg) ? 1 : 0;
}

void SCProg::opSetObjDefendTarget(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->task_state = this->actor->setObjective(OP_SET_OBJ_DEFEND_TARGET, instruction.arg) ? 1 : 0;
}

void SCProg::opSetObjDefendArea(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->task_state = this->actor->setObjective(OP_SET_OBJ_DEFEND_AREA, instruction.arg) ? 1 : 0;
}

void SCProg::opSetMessage(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->actor->setMessage(instruction.arg);
}

// MissionScript_CallNativeHandler_52513 cas 0xAA : 0xFF = allie par defaut (PART octet 46),
// poste = mots PART 48/50/52 (cote, avant, haut)
void SCProg::opSetObjFollowAlly(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    uint8_t leader_arg = (uint8_t)instruction.arg;
    MISN_PART *part = this->actor->object;
    if (leader_arg == 0xFF && part != nullptr) {
        leader_arg = part->follow_ally;
    }
    this->task_state = this->actor->setObjective(OP_SET_OBJ_FOLLOW_ALLY, leader_arg) ? 1 : 0;
    if (part != nullptr && this->actor->current_command == OP_SET_OBJ_FOLLOW_ALLY && this->actor->current_command_arg == leader_arg) {
        this->actor->follow_slot = Vector3D((float)part->formation_side, (float)part->formation_ahead, (float)part->formation_up);
        this->actor->follow_slot_set = true;
    }
}

// Expr_VM_Interpreter_51106 cas 0x91 : Expr_Node_ClearDirtyAndNotify_5242A (inverse de 0x90)
void SCProg::opDeactivateObj(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->actor->deactivate(instruction.arg);
}

// Expr_VM_Interpreter_51106 cas 0xB6 / 0xB7 : bit 4 de +0x39, lu par NavMap_DrawMissionObjects_7EDCA ; 255 = soi
void SCProg::opShowHideOnNavMap(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    SCMissionActors *shown = nullptr;
    if (instruction.arg == 255) {
        shown = this->actor;
    } else {
        shown = this->findActor(instruction.arg);
    }
    if (shown != nullptr) {
        shown->on_nav_map = instruction.opcode == OP_SHOW_ON_NAV_MAP;
    }
}

// Expr_VM_Interpreter_51106 cas 0xB8 / 0xB9 : bit 5 de +0x39 (suit le joueur au pilote automatique,
// lu par MissionScenario_QueryFieldWrapper_A7FF0 depuis UIScript_ParseAndEvaluate_7A054) ; 255 = soi
void SCProg::opJoinLeavePlayerTeam(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    SCMissionActors *member = nullptr;
    if (instruction.arg == 255) {
        member = this->actor;
    } else {
        member = this->findActor(instruction.arg);
    }
    if (member != nullptr) {
        std::vector<SCMissionActors *> &team = this->mission->friendlies;
        auto it = std::find(team.begin(), team.end(), member);
        if (instruction.opcode == OP_JOIN_PLAYER_TEAM && it == team.end()) {
            team.push_back(member);
        } else if (instruction.opcode == OP_LEAVE_PLAYER_TEAM && it != team.end()) {
            team.erase(it);
        }
    }
}

void SCProg::opPacifyTarget(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    for (auto actr : this->actor->mission->actors) {
        if (actr->actor_id == instruction.arg && actr->is_active == true && actr->is_destroyed == false) {
            if (actr->plane != nullptr) {
                bool task_result = actr->setObjective(OP_SET_OBJ_PATROL_POSITION, instruction.arg);
                if (task_result) {
                    this->task_state = 1;
                } else {
                    this->task_state = 0;
                }
            } else {
                if (actr->object->entity->entity_type == EntityType::swpn && actr->object->entity->swpn_data != nullptr) {
                    actr->object->entity->swpn_data->weapons_round = 0;
                }
            }
        }
    }
}

void SCProg::opSetObjPatrolPosition(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->actor->setObjective(OP_SET_OBJ_PATROL_POSITION, instruction.arg);
}

void SCProg::opActivateObj(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->actor->activateTarget(instruction.arg);
}

void SCProg::opMoveFlagToWorkRegister(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->work_register = this->mission->mission->mission_data.flags[instruction.arg];
}

// Expr_VM_Interpreter_51106 cas 0x14 : STAT[n], 8 valeurs (n < 8)
void SCProg::opSaveValueToGameflowRegister(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    if (instruction.arg < 8) {
        auto stat = this->mission->gameflow_registers.find(instruction.arg);
        if (stat == this->mission->gameflow_registers.end() || stat->second != (int16_t)this->work_register) {
            printf("PROG %d: STAT[%d] = %d\n", this->prog_id, instruction.arg, this->work_register);
        }
        this->mission->gameflow_registers[instruction.arg] = (int16_t)this->work_register;
    }
}

// Expr_VM_Interpreter_51106 cas 0x0F : byte_706AF = n
void SCProg::opSetExitCode(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->mission->setExitCode((uint8_t)instruction.arg, "script de mission");
}

void SCProg::opMoveWorkRegisterToFlag(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->mission->mission->mission_data.flags[instruction.arg] = (uint8_t)this->work_register;
}

void SCProg::opExecuteCall(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->jumpTo(this->work_register);
}

void SCProg::opExecSubProg(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    if (instruction.arg < this->mission->mission->mission_data.prog.size()) {
        std::vector<PROG> *sub_prog = this->mission->mission->mission_data.prog[instruction.arg];
        SCProg *sub_prog_obj = new SCProg(this->actor, *sub_prog, this->mission, instruction.arg);
        sub_prog_obj->task_state = this->task_state;
        sub_prog_obj->scene_script = this->scene_script;
        sub_prog_obj->execute();
        this->task_state = sub_prog_obj->task_state;
        delete sub_prog_obj;
    }
}

void SCProg::opCmpWorkWithValue(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->compare_flag = prog_compare_return_values::PROG_CMP_UNSET;
    if (this->work_register == instruction.arg) {
        this->compare_flag |= PROG_CMP_EQUAL;
    }
    if (this->work_register < instruction.arg) {
        this->compare_flag |= PROG_CMP_LESS;
    }
    if (this->work_register > instruction.arg) {
        this->compare_flag |= PROG_CMP_GREATER;
    }
}

void SCProg::opCmpValueWithWork(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->compare_flag = prog_compare_return_values::PROG_CMP_UNSET;
    if (instruction.arg == this->work_register) {
        this->compare_flag |= PROG_CMP_EQUAL;
    }
    if (instruction.arg < this->work_register) {
        this->compare_flag |= PROG_CMP_LESS;
    }
    if (instruction.arg > this->work_register) {
        this->compare_flag |= PROG_CMP_GREATER;
    }
}

void SCProg::opBranchIfEqual(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    if (this->compare_flag & prog_compare_return_values::PROG_CMP_EQUAL) {
        this->jumpTo(instruction.arg);
    }
}

void SCProg::opBranchIfNotEqual(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    if (!(this->compare_flag & prog_compare_return_values::PROG_CMP_EQUAL)) {
        this->jumpTo(instruction.arg);
    }
}

void SCProg::opBranchIfLess(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    if (this->compare_flag & prog_compare_return_values::PROG_CMP_LESS) {
        this->jumpTo(instruction.arg);
    }
}

void SCProg::opBranchIfGreater(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    if (this->compare_flag & prog_compare_return_values::PROG_CMP_GREATER) {
        this->jumpTo(instruction.arg);
    }
}

// Sans effet dans le jeu d'origine.
void SCProg::opSelectFlag208(const PROG &instruction) {
}

void SCProg::opDistToTarget(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->work_register = this->actor->getDistanceToTarget(instruction.arg);
}

void SCProg::opDistToSpot(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->work_register = this->actor->getDistanceToSpot(instruction.arg);
}

void SCProg::opIsTargetAlive(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    SCMissionActors *test_actor = this->findActor(instruction.arg);
    if (test_actor == nullptr) {
        return;
    }
    if (test_actor->object->alive == true) {
        this->compare_flag = prog_compare_return_values::PROG_CMP_NOT_EQUAL;
    } else {
        this->compare_flag = prog_compare_return_values::PROG_CMP_EQUAL;
    }
}

void SCProg::opIsTargetActive(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    SCMissionActors *test_actor = this->findActor(instruction.arg);
    if (test_actor == nullptr) {
        return;
    }
    if (test_actor->actor_name == "PLAYER") {
        this->compare_flag = prog_compare_return_values::PROG_CMP_EQUAL;
    } else if ((test_actor->is_active) && (test_actor->is_destroyed == false)) {
        this->compare_flag = prog_compare_return_values::PROG_CMP_EQUAL;
    } else {
        this->compare_flag = prog_compare_return_values::PROG_CMP_NOT_EQUAL;
    }
}

void SCProg::opSetFlagToTrue(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->mission->mission->mission_data.flags[instruction.arg] = 1;
}

void SCProg::opSetFlagToFalse(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->mission->mission->mission_data.flags[instruction.arg] = 0;
}

void SCProg::opTestFlag(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    if (this->mission->mission->mission_data.flags[instruction.arg] != 0) {
        this->compare_flag = prog_compare_return_values::PROG_CMP_EQUAL;
    } else {
        this->compare_flag = prog_compare_return_values::PROG_CMP_NOT_EQUAL;
    }
}

void SCProg::opMulValueWithWork(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->work_register *= instruction.arg;
}

void SCProg::opActivateScene(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    int scene_id = 0;
    for (auto scen : this->mission->mission->mission_data.scenes) {
        if (scene_id == instruction.arg) {
            scen->is_active = 1;
            break;
        }
        scene_id++;
    }
}

void SCProg::opDeactivateScene(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    int scene_id = 0;
    for (auto scen : this->mission->mission->mission_data.scenes) {
        if (scene_id == instruction.arg) {
            scen->is_active = 0;
            break;
        }
        scene_id++;
    }
}
