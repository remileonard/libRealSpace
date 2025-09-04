#include "precomp.h"


/**
 * @brief Interpret and execute the scripted program (bytecode) attached to this SCProg instance.
 *
 * This method walks sequentially through the vector of PROG instructions (this->prog) and
 * interprets each opcode, mutating actor state, mission state, flags, command queues, and
 * various runtime control variables used for mission logic (e.g. branching, sub‑program
 * invocation, conditional evaluation).
 *
 * Control Flow Mechanism:
 * - A simple linear interpreter with manual flow control using:
 *   - jump_to + OP_SET_LABEL + setting exec=false to defer execution until a matching label.
 *   - Conditional gotos (OP_GOTO_LABEL_IF_TRUE / OP_GOTO_LABEL_IF_FALSE /
 *     OP_IF_LESS_THAN_GOTO / OP_IF_GREATER_THAN_GOTO).
 *   - Sub‑program execution via OP_EXEC_SUB_PROG (recursively creates and runs a new SCProg).
 *   - Indirect jump via OP_EXECUTE_CALL (jump target taken from work_register).
 * - The variable 'exec' gates whether the current instruction is active. When a jump is
 *   scheduled, exec is set to false until a matching OP_SET_LABEL with the desired label id
 *   (jump_to) is encountered, at which point execution resumes.
 *
 * State / Working Registers:
 * - work_register: General purpose integer scratch register used by arithmetic, comparisons,
 *   indirect jumps, and data movement (e.g. flags <-> work register).
 * - true_flag / compare_flag: Results of condition evaluation (boolean or tri-state compare)
 *   drive conditional jumps and branching logic.
 * - jump_to: Holds the label id to resume execution at after a jump is requested.
 * - call_to (currently unused) and flag_number (currently unused) appear reserved for future
 *   extensions.
 *
 * Side Effects on Actor:
 * - Clears and repopulates actor->executed_opcodes with indices of instructions actually
 *   executed (some conditional instructions only push if taken).
 * - Issues or updates current command (take off, land, move, destroy target, defend, follow,
 *   etc.) and tracks its completion state (current_command_executed).
 * - Queries spatial / combat related conditions (distance to target / spot, target alive,
 *   target in area) to set true_flag.
 * - Can instantly destroy a target actor (OP_INSTANT_DESTROY_TARGET), optionally spawning
 *   explosion effects.
 * - Can activate/deactivate actors or scenes.
 * - Sets in‑game messages via OP_SET_MESSAGE.
 *
 * Side Effects on Mission:
 * - Reads / writes mission->mission->mission_data.flags (increment, decrement, set boolean,
 *   arithmetic with work_register).
 * - Stores to mission->gameflow_registers via OP_SAVE_VALUE_TO_GAMFLOW_REGISTER.
 * - Activates scenes matching an area id.
 *
 * Comparison & Branching:
 * - OP_CMP_GREATER_EQUAL_THAN sets compare_flag to -1 / 0 / 1 for less / equal / greater,
 *   and sets true_flag to true if >=, false otherwise.
 * - Subsequent conditional opcodes test compare_flag or true_flag to decide jumps.
 *
 * Sub‑program Execution:
 * - OP_EXEC_SUB_PROG looks up a referenced PROG vector in mission data, constructs a temporary
 *   SCProg, and executes it immediately (recursive interpretation).
 *
 * Notable Implementation Details / Caveats:
 * - Some fields (call_to, flag_number) are declared but never used.
 * - true_flag is sometimes reset implicitly (e.g., only changed by certain opcodes); logic that
 *   depends on stale values must ensure an opcode establishing it has run.
 * - No bounds checking for many flag / actor indices (assumes valid script).
 * - No cycle detection; malicious or ill‑formed scripts with self‑referential jumps could stall
 *   progression (although loop constructs rely on label scanning rather than rewinding i).
 *
 * Logging / Tracing:
 * - The order and subset of executed instructions can be reconstructed from actor->executed_opcodes,
 *   which stores the numeric index (i) of each opcode actually processed (including some that
 *   only trigger under conditional execution).
 *
 * Thread Safety:
 * - Not thread‑safe; mutates shared mission and actor state without synchronization.
 *
 * Performance:
 * - Single pass over instructions; label resolution is linear (no precomputed label index), so
 *   large scripts with many jumps could incur extra scanning cost.
 *
 * Lifecycle:
 * - Returns immediately upon first OP_EXIT_PROG unless true_flag was set (in which case it clears
 *   true_flag and continues), enabling a "conditional early exit" pattern.
 *
 * Preconditions:
 * - this->actor, this->mission, and mission->mission->mission_data structures must be valid.
 * - Flags / actors / scenes referenced by indices must exist.
 *
 * Postconditions:
 * - Actor command state, flags, and mission side effects reflect all executed opcodes up to
 *   normal termination or early exit.
 *
 *
 * @return void (effects are applied through side effects).
 */
void SCProg::execute() {
    uint8_t i = 0;
    size_t jump_to = 0;
    size_t call_to = 0;
    int flag_number = 0;
    int work_register = 0;
    int compare_flag = prog_compare_return_values::PROG_CMP_UNSET;
    bool exec = true;
    bool objective_flag = false;
    bool true_flag = false;
    SPOT *spot = nullptr;
    this->actor->executed_opcodes.clear();
    this->actor->executed_opcodes.shrink_to_fit();
    for (auto prog : this->prog) {
        switch (prog.opcode) {
            case OP_EXIT_PROG:
                if (exec) {
                    this->actor->executed_opcodes.push_back(i);
                    if (true_flag) {
                        true_flag = false;
                    } else {
                        return;
                    }
                }
            break;
            case OP_SPOT_DATA:
                if (exec) {
                    this->actor->executed_opcodes.push_back(i);
                    if (prog.arg < this->mission->mission->mission_data.spots.size()) {
                        spot = this->mission->mission->mission_data.spots[prog.arg];
                    }
                }
            break;
            case OP_SET_LABEL:
                if (jump_to == prog.arg) {
                    exec = true;
                    this->actor->executed_opcodes.push_back(i);
                }
            break;
            case OP_MOVE_VALUE_TO_WORK_REGISTER:
                if (exec) {
                    this->actor->executed_opcodes.push_back(i);
                    work_register = prog.arg;
                }
            break;
            case OP_GOTO_IF_CURRENT_COMMAND_IN_PROGRESS:
                if (exec) {
                    switch (this->actor->current_command) {
                        case OP_SET_OBJ_TAKE_OFF:
                            this->actor->current_command_executed = this->actor->takeOff(this->actor->current_command_arg);
                            break;
                        case OP_SET_OBJ_LAND:
                            this->actor->current_command_executed = this->actor->land(this->actor->current_command_arg);
                            break;
                        case OP_SET_OBJ_FLY_TO_WP:
                            this->actor->current_command_executed = this->actor->flyToWaypoint(this->actor->current_command_arg);
                            break;
                        case OP_SET_OBJ_FLY_TO_AREA:
                            this->actor->current_command_executed = this->actor->flyToArea(this->actor->current_command_arg);
                            break;
                        case OP_SET_OBJ_DESTROY_TARGET:
                            this->actor->current_command_executed = this->actor->destroyTarget(this->actor->current_command_arg);
                            break;
                        case OP_SET_OBJ_DEFEND_TARGET:
                            this->actor->current_command_executed = this->actor->defendTarget(this->actor->current_command_arg);
                            break;
                        case OP_SET_OBJ_FOLLOW_ALLY:
                            this->actor->current_command_executed = this->actor->followAlly(this->actor->current_command_arg);
                            break;
                        default:
                            break;
                    }
                    if (!this->actor->current_command_executed) {
                        jump_to = prog.arg;
                        exec = false;
                    }
                    this->actor->executed_opcodes.push_back(i);
                }
            break;
            case OP_IF_TARGET_IN_AREA:
                if (exec) {
                    this->actor->executed_opcodes.push_back(i);
                    true_flag = this->actor->ifTargetInSameArea(prog.arg);
                }
            break;
            case OP_ADD_1_TO_FLAG:
                if (exec) {
                    this->actor->executed_opcodes.push_back(i);
                    this->mission->mission->mission_data.flags[prog.arg]++;
                }
            break;
            case OP_REMOVE_1_TO_FLAG:
                if (exec) {
                    this->actor->executed_opcodes.push_back(i);
                    this->mission->mission->mission_data.flags[prog.arg]--;
                }
            break;
            case OP_ADD_WORK_REGISTER_TO_FLAG:
                if (exec) {
                    this->actor->executed_opcodes.push_back(i);
                    this->mission->mission->mission_data.flags[prog.arg] += work_register;
                }
            break;
            case OP_INSTANT_DESTROY_TARGET:
                if (exec) {
                    this->actor->executed_opcodes.push_back(i);
                    for (auto actor: this->mission->actors) {
                        if (actor->actor_id == prog.arg) {
                            actor->object->alive = false;
                            actor->is_destroyed = false;
                            if (actor->object->entity->explos->objct != nullptr) {
                                actor->mission->explosions.push_back(new SCExplosion(actor->object->entity->explos->objct, actor->object->position));
                            }
                            break;
                        }
                    }
                }
            break;
            case OP_SET_OBJ_UNKNOWN:
                if (exec) {
                    this->actor->executed_opcodes.push_back(i);
                    this->actor->flyToWaypoint(prog.arg);
                    this->actor->current_command_executed = true;
                }
            break;
            case OP_SET_OBJ_TAKE_OFF:
                if (exec) {
                    this->actor->executed_opcodes.push_back(i);
                    this->actor->current_command_executed = this->actor->takeOff(prog.arg);
                    this->actor->current_command = OP_SET_OBJ_TAKE_OFF;
                }
            break;
            case OP_SET_OBJ_LAND:
                if (exec) {
                    this->actor->executed_opcodes.push_back(i);
                    this->actor->current_command_executed = this->actor->land(prog.arg);
                    this->actor->current_command = OP_SET_OBJ_LAND;
                    this->actor->current_command_arg = prog.arg;
                }
            break;
            case OP_SET_OBJ_FLY_TO_WP:
                if (exec) {
                    this->actor->executed_opcodes.push_back(i);
                    this->actor->current_command_executed = this->actor->flyToWaypoint(prog.arg);
                    this->actor->current_command = OP_SET_OBJ_FLY_TO_WP;
                    this->actor->current_command_arg = prog.arg;
                }
            break;
            case OP_SET_OBJ_FLY_TO_AREA:
                if (exec) {
                    this->actor->executed_opcodes.push_back(i);
                    this->actor->current_command_executed = this->actor->flyToArea(prog.arg);
                    this->actor->current_command = OP_SET_OBJ_FLY_TO_AREA;
                    this->actor->current_command_arg = prog.arg;
                }
            break;
            case OP_SET_OBJ_DESTROY_TARGET:
                if (exec) {
                    this->actor->executed_opcodes.push_back(i);
                    this->actor->current_command_executed = this->actor->destroyTarget(prog.arg);
                    this->actor->current_command = OP_SET_OBJ_DESTROY_TARGET;
                    this->actor->current_command_arg = prog.arg;
                }
            break;
            case OP_SET_OBJ_DEFEND_TARGET:
                if (exec) {
                    this->actor->executed_opcodes.push_back(i);
                    this->actor->current_command_executed = this->actor->defendTarget(prog.arg);
                    this->actor->current_command = OP_SET_OBJ_DEFEND_TARGET;
                    this->actor->current_command_arg = prog.arg;
                }
            break;
            case OP_SET_MESSAGE:
                if (exec) {
                    this->actor->executed_opcodes.push_back(i);
                    this->actor->setMessage(prog.arg);
                }
            break;
            case OP_SET_OBJ_FOLLOW_ALLY:
                if (exec) {
                    this->actor->executed_opcodes.push_back(i);
                    this->actor->current_command_executed = this->actor->followAlly(prog.arg);
                    this->actor->current_command = OP_SET_OBJ_DEFEND_TARGET;
                    this->actor->current_command_arg = prog.arg;
                }
            break;
            case OP_DEACTIVATE_OBJ:
                if (exec) {
                    this->actor->executed_opcodes.push_back(i);
                    this->actor->deactivate(prog.arg);
                }
            break;
            case OP_ACTIVATE_OBJ:
                if (exec) {
                    this->actor->executed_opcodes.push_back(i);
                    this->actor->activateTarget(prog.arg);
                }
            break;
            case OP_MOVE_FLAG_TO_WORK_REGISTER:
                if (exec) {
                    this->actor->executed_opcodes.push_back(i);
                    work_register = this->mission->mission->mission_data.flags[prog.arg];
                }
            break;
            case OP_SAVE_VALUE_TO_GAMFLOW_REGISTER:
                if (exec) {
                    this->actor->executed_opcodes.push_back(i);
                    this->mission->gameflow_registers[prog.arg] = work_register;
                }
            break;
            case OP_MOVE_WORK_REGISTER_TO_FLAG:
                if (exec) {
                    this->actor->executed_opcodes.push_back(i);
                    this->mission->mission->mission_data.flags[prog.arg] = (uint8_t) work_register;
                }
            break;
            case OP_EXECUTE_CALL:
                if (exec) {
                    this->actor->executed_opcodes.push_back(i);
                    jump_to = work_register;
                    exec = false;
                }
            break;
            case OP_EXEC_SUB_PROG:
                if (exec) {
                    this->actor->executed_opcodes.push_back(i);
                    std::vector<PROG> *sub_prog = this->mission->mission->mission_data.prog[prog.arg];
                    SCProg *sub_prog_obj = new SCProg(this->actor, *sub_prog, this->mission);
                    sub_prog_obj->execute();
                    delete sub_prog_obj;
                }
            break;
            case OP_CMP_GREATER_EQUAL_THAN:
                if (exec) {
                    this->actor->executed_opcodes.push_back(i);
        
                    
                    bool is_equal = (work_register == prog.arg);
                    bool is_less = (work_register < prog.arg);
                    bool is_greater = (work_register > prog.arg);

                    if (is_equal) {
                        compare_flag |= PROG_CMP_EQUAL;
                    }
                    if (is_less) {
                        compare_flag |= PROG_CMP_LESS;
                    }
                    if (is_greater) {
                        compare_flag |= PROG_CMP_GREATER;
                    }
                    if (is_less || is_equal) {
                        compare_flag |= PROG_CMP_LESS_EQUAL;
                    }
                    if (is_greater || is_equal) {
                        compare_flag |= PROG_CMP_GREATER_EQUAL;
                    }
                    true_flag = is_equal;
                }
            break;
            case OP_BRANCH_IF_EQUAL_OR_TRUE:
                if (exec) {
                    if (true_flag || compare_flag == prog_compare_return_values::PROG_CMP_EQUAL) {
                        jump_to = prog.arg;
                        exec = false;
                        this->actor->executed_opcodes.push_back(i);
                    }
                }
            break;
            case OP_BRANCH_IF_NOT_EQUAL_OR_FALSE:
                if (exec) {
                    if (compare_flag != 0 || !true_flag) {
                        jump_to = prog.arg;
                        exec = false;
                        this->actor->executed_opcodes.push_back(i);
                    }
                }
            break;
            case OP_IF_LESS_THAN_GOTO:
                if (exec) {
                    
                    if (compare_flag == -1) {
                        this->actor->executed_opcodes.push_back(i);
                        jump_to = prog.arg;
                        exec = false;
                    }
                }
            break;
            case OP_IF_GREATER_THAN_GOTO:
                if (exec) {
                    
                    if (compare_flag == 1) {
                        this->actor->executed_opcodes.push_back(i);
                        jump_to = prog.arg;
                        exec = false;
                    }
                }
            break;
            case OP_SELECT_FLAG_208:
                if (exec) {
                    this->actor->executed_opcodes.push_back(i);
                    // je sais pas
                }
            break;
            case OP_DIST_TO_TARGET:
                if (exec) {
                    this->actor->executed_opcodes.push_back(i);
                    work_register = this->actor->getDistanceToTarget(prog.arg);
                }
            break;
            case OP_DIST_TO_SPOT:
                if (exec) {
                    this->actor->executed_opcodes.push_back(i);
                    work_register = this->actor->getDistanceToSpot(prog.arg);
                }
            break;
            case OP_IS_TARGET_ALIVE:
                if (exec) {
                    this->actor->executed_opcodes.push_back(i);
                    true_flag = this->mission->actors[prog.arg]->object->alive == true;
                }
            break;
            case OP_IS_TARGET_ACTIVE:
                if (exec) {
                    this->actor->executed_opcodes.push_back(i);
                    true_flag = this->mission->actors[prog.arg]->is_active == true;
                }
            break;
            case OP_SET_FLAG_TO_TRUE:
                if (exec) {
                    this->actor->executed_opcodes.push_back(i);
                    this->mission->mission->mission_data.flags[prog.arg] = 1;
                }
            break;
            case OP_SET_FLAG_TO_FALSE:
                if (exec) {
                    this->actor->executed_opcodes.push_back(i);
                    this->mission->mission->mission_data.flags[prog.arg] = 0;
                }
            break;
            case OP_TEST_FLAG:
                if (exec) {
                    this->actor->executed_opcodes.push_back(i);
                    true_flag = this->mission->mission->mission_data.flags[prog.arg] != 0;
                    if (true_flag) {
                        compare_flag = prog_compare_return_values::PROG_CMP_EQUAL;
                    } else {
                        compare_flag = prog_compare_return_values::PROG_CMP_NOT_EQUAL;
                    }
                }
            break;
            case OP_MUL_VALUE_WITH_WORK:
                if (exec) {
                    this->actor->executed_opcodes.push_back(i);
                    work_register *= prog.arg;
                }
            break;
            case OP_ACTIVATE_SCENE:
                if (exec) {
                    this->actor->executed_opcodes.push_back(i);
                    for (auto scen: this->mission->mission->mission_data.scenes) {
                        if (scen->area_id == prog.arg) {
                            scen->is_active = 1;
                            break;
                        }
                    }
                }
            break;
            default:
            break;
        }
        i++;
    }
}