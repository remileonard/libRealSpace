#include "precomp.h"

/**
 * @brief Executes the script program for the actor.
 * 
 * This function iterates through the program instructions and executes them based on the opcode.
 * It handles various operations such as setting labels, calling labels, conditional jumps, 
 * checking if the target is in the same area, and performing specific actions like take off, 
 * land, fly to waypoint, destroy target, etc.
 * 
 * The function maintains several state variables to control the flow of execution:
 * - `jump_to`: The label to jump to.
 * - `call_to`: The label to call.
 * - `flag_number`: The current flag number.
 * - `exec`: A flag indicating whether the current instruction should be executed.
 * - `objective_flag`: A flag indicating the result of the last objective-related operation.
 * - `true_flag`: A flag indicating the result of the last flag check operation.
 * 
 * The function supports the following opcodes:
 * - `OP_EXIT_PROG`: Exits the program.
 * - `OP_SET_LABEL`: Sets the execution point to a label.
 * - `OP_CALL_LABEL`: Calls a label.
 * - `OP_GOTO_LABEL_IF_TRUE`: Jumps to a label if the objective flag is true.
 * - `OP_GOTO_LABEL_IF_FALSE`: Jumps to a label if the objective flag is false.
 * - `OP_IF_TARGET_IN_AREA`: Checks if the target is in the same area.
 * - `OP_85_UNKNOWN`: Checks a mission flag.
 * - `OP_INSTANT_DESTROY_TARGET`: Placeholder for instant destroy target operation.
 * - `OP_SET_OBJ_TAKE_OFF`: Sets the objective to take off.
 * - `OP_SET_OBJ_LAND`: Sets the objective to land.
 * - `OP_SET_OBJ_FLY_TO_WP`: Sets the objective to fly to a waypoint.
 * - `OP_SET_OBJ_FLY_TO_AREA`: Sets the objective to fly to an area.
 * - `OP_SET_OBJ_DESTROY_TARGET`: Sets the objective to destroy a target.
 * - `OP_SET_OBJ_DEFEND_TARGET`: Sets the objective to defend a target.
 * - `OP_SET_MESSAGE`: Sets a message.
 * - `OP_SET_OBJ_FOLLOW_ALLY`: Sets the objective to follow an ally.
 * - `OP_DEACTIVATE_OBJ`: Deactivates an objective.
 * - `OP_ACTIVATE_OBJ`: Activates an objective.
 * - `OP_SELECT_FLAG`: Selects a flag.
 * - `OP_SAVE_VALUE_TO_FLAG`: Saves a value to a flag.
 * - `OP_STORE_CALL_TO_VALUE`: Stores the call-to value in a flag.
 * - `OP_EXECUTE_CALL`: Executes a call to a label.
 */
void SCProg::execute() {
    size_t i = 0;
    size_t jump_to = 0;
    size_t call_to = 0;
    int flag_number = 0;
    int work_register = 0;
    int compare_flag = 0;
    bool exec = true;
    bool objective_flag = false;
    bool true_flag = false;
    for (auto prog : this->prog) {
        switch (prog.opcode) {
            case OP_EXIT_PROG:
                if (exec) {
                    if (true_flag) {
                        true_flag = false;
                    } else {
                        return;
                    }
                }
            break;
            case OP_INVERT_FLAG:
                if (exec) {
                    //this->mission->mission->mission_data.flags[prog.arg] = !this->mission->mission->mission_data.flags[prog.arg];
                }
            break;
            case OP_SET_LABEL:
                if (jump_to == prog.arg) {
                    exec = true;
                }
            break;
            case OP_CALL_LABEL:
                if (exec) {
                    call_to = prog.arg;
                }
            break;
            case OP_GOTO_LABEL_IF_TRUE:
                if (exec) {
                    if (true_flag) {
                        jump_to = prog.arg;
                        exec = false;
                    }
                }
            break;
            case OP_GOTO_IF_CURRENT_COMMAND_IN_PROGRESS:
                if (exec) {
                    if (!this->actor->current_command_executed) {
                        jump_to = prog.arg;
                        exec = false;
                    }
                }
            break;
            case OP_IF_TARGET_IN_AREA:
                if (exec) {
                    this->actor->current_command_executed = this->actor->ifTargetInSameArea(prog.arg);
                }
            break;
            case OP_ADD_1_TO_FLAG:
                if (exec) {
                    this->mission->mission->mission_data.flags[prog.arg]++;
                }
            break;
            case OP_ADD_3_TO_FLAG:
                if (exec) {
                    this->mission->mission->mission_data.flags[prog.arg] += 3;
                }
            case OP_INSTANT_DESTROY_TARGET:
                if (exec) {
                    this->mission->actors[prog.arg]->object->alive = false;
                }
            break;
            case OP_SET_OBJ_TAKE_OFF:
                if (exec) {
                    this->actor->current_command_executed = this->actor->takeOff(prog.arg);
                }
            break;
            case OP_SET_OBJ_LAND:
                if (exec) {
                    this->actor->current_command_executed = this->actor->land(prog.arg);
                }
            break;
            case OP_SET_OBJ_FLY_TO_WP:
                if (exec) {
                    this->actor->current_command_executed = this->actor->flyToWaypoint(prog.arg);
                }
            break;
            case OP_SET_OBJ_FLY_TO_AREA:
                if (exec) {
                    this->actor->current_command_executed = this->actor->flyToArea(prog.arg);
                }
            break;
            case OP_SET_OBJ_DESTROY_TARGET:
                if (exec) {
                    this->actor->current_command_executed = this->actor->destroyTarget(prog.arg);
                }
            break;
            case OP_SET_OBJ_DEFEND_TARGET:
                if (exec) {
                    this->actor->current_command_executed = this->actor->defendTarget(prog.arg);
                }
            break;
            case OP_SET_MESSAGE:
                if (exec) {
                    this->actor->current_command_executed = this->actor->setMessage(prog.arg);
                }
            break;
            case OP_SET_OBJ_FOLLOW_ALLY:
                if (exec) {
                    this->actor->current_command_executed = this->actor->followAlly(prog.arg);
                }
            break;
            case OP_DEACTIVATE_OBJ:
                if (exec) {
                    this->actor->current_command_executed = this->actor->deactivate(prog.arg);
                }
            break;
            case OP_ACTIVATE_OBJ:
                if (exec) {
                    this->actor->activateTarget(prog.arg);
                }
            case OP_SELECT_FLAG:
                if (exec) {
                    work_register = this->mission->mission->mission_data.flags[prog.arg];
                }
            break;
            case OP_SAVE_VALUE_TO_FLAG:
                if (exec) {
                    this->mission->mission->mission_data.flags[prog.arg] = work_register;
                }
            break;
            case OP_STORE_CALL_TO_VALUE:
                if (exec) {
                    this->mission->mission->mission_data.flags[prog.arg] = (uint8_t) call_to;
                }
            break;
            case OP_EXECUTE_CALL:
                if (exec) {
                    jump_to = work_register;
                    exec = false;
                }
            break;
            case OP_EXEC_SUB_PROG:
                if (exec) {
                    std::vector<PROG> *sub_prog = this->mission->mission->mission_data.prog[prog.arg];
                    SCProg *sub_prog_obj = new SCProg(this->actor, *sub_prog, this->mission);
                    sub_prog_obj->execute();
                }
            break;
            case OP_CMP_GREATER_EQUAL_THAN:
                if (exec) {
                    if (work_register == prog.arg) {
                        compare_flag = 0;
                    } else if (work_register > prog.arg) {
                        compare_flag = 1;
                    } else if (work_register < prog.arg) {
                        compare_flag = -1;
                    }
                }
            break;
            case OP_GOTO_IF_TRUE_74:
                if (exec) {
                    if (compare_flag == 1) {
                        jump_to = prog.arg;
                        exec = false;
                    }
                }
            break;
            case OP_GOTO_IF_FALSE_73:
                if (exec) {
                    if (!true_flag) {
                        jump_to = prog.arg;
                        exec = false;
                    }
                }
            case OP_SELECT_FLAG_208:
                if (exec) {
                    // je sais pas
                }
            break;
            case OP_DIST_TO_TARGET:
                if (exec) {
                    work_register = this->actor->getDistanceToTarget(prog.arg);
                }
            break;
            case OP_SET_FLAG_TO_TRUE:
                if (exec) {
                    this->mission->mission->mission_data.flags[prog.arg] = 1;
                }
            break;
            case OP_GET_FLAG_ID:
                if (exec) {
                    true_flag = this->mission->mission->mission_data.flags[prog.arg] > 0;
                }
            break;
            case OP_ADD_VALUE_TO_WORK:
                if (exec) {
                    work_register += prog.arg;
                }
            break;
            default:
            break;
        }
    }
}