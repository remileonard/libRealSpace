# PROG chunk OPCODE LIST for missions

## Introduction

This document details the opcodes used in the PROG chunk of Strike Commander mission files. These opcodes control AI actor behavior and define mission logic.

## Opcode Categories

### Flow Control

| Dec | Hex | Opcode | Description |
|-----|-----|--------|-------------|
| 1 | 0x01 | OP_EXIT_PROG | Immediately terminates program execution |
| 8 | 0x08 | OP_SET_LABEL | Defines a label point with ID specified in the argument |
| 2 | 0x02 | OP_EXEC_SUB_PROG | Executes a subprogram referenced by the ID in the argument |
| 70 | 0x46 | OP_GOTO_IF_CURRENT_COMMAND_IN_PROGRESS | If the current command is in progress, jumps to specified label |
| 72 | 0x48 | OP_BRANCH_IF_EQUAL | If the last comparison resulted in equality, jumps to specified label |
| 73 | 0x49 | OP_BRANCH_IF_NOT_EQUAL | If the last comparison did not result in equality, jumps to specified label |
| 74 | 0x4A | OP_BRANCH_IF_LESS | If the last comparison resulted in "less than", jumps to specified label |
| 75 | 0x4B | OP_BRANCH_IF_GREATER | If the last comparison resulted in "greater than", jumps to specified label |
| 79 | 0x4F | OP_EXECUTE_CALL | Jumps to the label number stored in the work register |

### Registers and Flags

| Dec | Hex | Opcode | Description |
|-----|-----|--------|-------------|
| 16 | 0x10 | OP_MOVE_VALUE_TO_WORK_REGISTER | Loads an immediate value into the work register |
| 17 | 0x11 | OP_MOVE_FLAG_TO_WORK_REGISTER | Loads a flag value into the work register |
| 20 | 0x14 | OP_SAVE_VALUE_TO_GAMFLOW_REGISTER | Saves the work register to a game flow register |
| 80 | 0x50 | OP_MOVE_WORK_REGISTER_TO_FLAG | Saves the work register to a flag |
| 82 | 0x52 | OP_SET_FLAG_TO_TRUE | Sets a flag to 1 (true) |
| 83 | 0x53 | OP_SET_FLAG_TO_FALSE | Sets a flag to 0 (false) |
| 85 | 0x55 | OP_ADD_1_TO_FLAG | Increments a flag by 1 |
| 86 | 0x56 | OP_REMOVE_1_TO_FLAG | Decrements a flag by 1 |
| 35 | 0x23 | OP_ADD_WORK_REGISTER_TO_FLAG | Adds the work register value to the specified flag |
| 46 | 0x2E | OP_MUL_VALUE_WITH_WORK | Multiplies the work register by the specified value |

### Tests and Comparisons

| Dec | Hex | Opcode | Description |
|-----|-----|--------|-------------|
| 64 | 0x40 | OP_CMP_WORK_WITH_VALUE | Compares the work register with a value |
| 65 | 0x41 | OP_CMP_VALUE_WITH_WORK | Compares a value with the work register |
| 69 | 0x45 | OP_TEST_FLAG | Tests if a flag is non-zero |
| 146 | 0x92 | OP_IF_TARGET_IN_AREA | Checks if the target is in the same area |
| 147 | 0x93 | OP_IS_TARGET_ALIVE | Checks if the target is alive |
| 152 | 0x98 | OP_IS_TARGET_ACTIVE | Checks if the target is active |
| 149 | 0x95 | OP_DIST_TO_TARGET | Calculates distance to target and stores it in the work register |
| 151 | 0x97 | OP_DIST_TO_SPOT | Calculates distance to a spot and stores it in the work register |

### Mission Objectives

| Dec | Hex | Opcode | Description |
|-----|-----|--------|-------------|
| 161 | 0xA1 | OP_SET_OBJ_TAKE_OFF | Sets objective to take off from the specified waypoint |
| 162 | 0xA2 | OP_SET_OBJ_LAND | Sets objective to land at the specified waypoint |
| 165 | 0xA5 | OP_SET_OBJ_FLY_TO_WP | Sets objective to fly to a specified waypoint |
| 166 | 0xA6 | OP_SET_OBJ_FLY_TO_AREA | Sets objective to fly to a specified area |
| 167 | 0xA7 | OP_SET_OBJ_DESTROY_TARGET | Sets objective to destroy a specified target |
| 168 | 0xA8 | OP_SET_OBJ_DEFEND_TARGET | Sets objective to defend a specified target |
| 169 | 0xA9 | OP_SET_OBJ_DEFEND_AREA | Sets objective to defend a specified area |
| 170 | 0xAA | OP_SET_OBJ_FOLLOW_ALLY | Sets objective to follow a specified ally |
| 171 | 0xAB | OP_SET_MESSAGE | Sets a message for a waypoint in the navigation map |

### Actions on Actors and Scene

| Dec | Hex | Opcode | Description |
|-----|-----|--------|-------------|
| 144 | 0x90 | OP_ACTIVATE_OBJ | Activates a target (enables AI) |
| 148 | 0x94 | OP_INSTANT_DESTROY_TARGET | Instantly destroys a target, may trigger an explosion |
| 190 | 0xBE | OP_DEACTIVATE_OBJ | Deactivates a target (disables AI) |
| 128 | 0x80 | OP_ACTIVATE_SCENE | Activates a scene matching the specified area ID |
| 129 | 0x81 | OP_DEACTIVATE_SCENE | De activates a scene matching the specified area ID |

### Miscellaneous

| Dec | Hex | Opcode | Description |
|-----|-----|--------|-------------|
| 9 | 0x09 | OP_SPOT_DATA | Selects spot data |
| 208 | 0xD0 | OP_SELECT_FLAG_208 | Unknown for the moment |

## Implementation Notes

1. Objectives (OP_SET_OBJ_*) define a task for an actor and return "true" when completed.

2. Control flow opcodes work with a system of labels and jumps. When a jump is requested, the `exec` variable is set to false until a matching label is found.

3. The work register (`work_register`) is used for temporary calculations and data transfers.

4. Flags (`flags`) are stored in mission data and can be read, modified, and tested by opcodes.

5. Comparison opcodes set a `compare_flag` that can be tested by conditional branch opcodes.

## Team Usage

It appears that teams are defined in the mission file as follows:
- The first ushort (16 bits) is the leader
- The value is the index of the part in the mission file (not the actual ID)