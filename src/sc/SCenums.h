#pragma once

enum weapon_type_shp_id {
    AIM9J = 29,
    AIM9M = 30,
    AIM120 = 116,
    AGM65D = 31,
    DURANDAL = 32,
    MK20 = 33,
    MK82 = 34,
    GBU15 = 35,
    LAU3 = 36
};

enum weapon_ids {
    ID_AIM9J = 1,
    ID_AIM9M = 2,
    ID_AGM65D = 3,
    ID_LAU3 = 4,
    ID_MK20 = 5,
    ID_MK82 = 6,
    ID_DURANDAL = 7,
    ID_GBU15 = 8,
    ID_AIM120 = 9,
    ID_20MM = 12,
};
static std::map<weapon_ids, weapon_type_shp_id> weapon_inv_to_loadout = {
    {ID_AIM9J, AIM9J},
    {ID_AIM9M, AIM9M},
    {ID_AGM65D, AGM65D},
    {ID_LAU3, LAU3},
    {ID_MK20, MK20},
    {ID_MK82, MK82},
    {ID_DURANDAL, DURANDAL},
    {ID_GBU15, GBU15},
    {ID_AIM120, AIM120}
};

enum prog_op {
    OP_EXIT_PROG = 1,
    OP_EXEC_SUB_PROG = 2,
    OP_SET_LABEL = 8,
    OP_INVERT_FLAG = 9,
    OP_MOVE_VALUE_TO_WORK_REGISTER = 16,
    OP_MOVE_FLAG_TO_WORK_REGISTER = 17,
    OP_SAVE_VALUE_TO_GAMFLOW_REGISTER = 20,
    OP_ADD_WORK_REGISTER_TO_FLAG = 35,
    OP_MUL_VALUE_WITH_WORK = 46,
    OP_CMP_GREATER_EQUAL_THAN = 64,
    OP_GET_FLAG_ID = 69,
    OP_GOTO_IF_CURRENT_COMMAND_IN_PROGRESS = 70,
    OP_GOTO_LABEL_IF_TRUE = 72,
    OP_GOTO_IF_GREATER_OR_EQUAL = 73,
    OP_IF_LESS_THAN_GOTO = 74,
    OP_IF_GREATER_THAN_GOTO = 75,
    OP_EXECUTE_CALL = 79,
    OP_MOVE_WORK_REGISTER_TO_FLAG = 80,
    OP_SET_FLAG_TO_TRUE = 82,
    OP_ADD_1_TO_FLAG = 85,
    OP_ACTIVATE_SCENE = 128,
    OP_ACTIVATE_OBJ = 144,
    OP_IF_TARGET_IN_AREA = 146,
    OP_INSTANT_DESTROY_TARGET = 148,
    OP_DIST_TO_TARGET = 149,
    OP_DIST_TO_SPOT = 151,
    OP_IS_TARGET_ALIVE = 152,
    OP_SET_OBJ_UNKNOWN = 160,
    OP_SET_OBJ_TAKE_OFF = 161,
    OP_SET_OBJ_LAND = 162,
    OP_SET_OBJ_FLY_TO_WP = 165,
    OP_SET_OBJ_FLY_TO_AREA = 166,
    OP_SET_OBJ_DESTROY_TARGET = 167,
    OP_SET_OBJ_DEFEND_TARGET = 168,
    OP_SET_OBJ_FOLLOW_ALLY = 170,
    OP_SET_MESSAGE = 171,
    OP_DEACTIVATE_OBJ = 190,
    OP_SELECT_FLAG_208 = 208,
};

static std::map<prog_op, std::string> prog_op_names = {
    {OP_EXIT_PROG, "OP_EXIT_PROG"},
    {OP_EXEC_SUB_PROG, "OP_EXEC_SUB_PROG"},
    {OP_SET_LABEL, "OP_SET_LABEL"},
    {OP_INVERT_FLAG, "OP_INVERT_FLAG"},
    {OP_MOVE_VALUE_TO_WORK_REGISTER, "OP_MOVE_VALUE_TO_WORK_REGISTER"},
    {OP_MOVE_FLAG_TO_WORK_REGISTER, "OP_MOVE_FLAG_TO_WORK_REGISTER"},
    {OP_SAVE_VALUE_TO_GAMFLOW_REGISTER, "OP_SAVE_VALUE_TO_GAMFLOW_REGISTER"},
    {OP_ADD_WORK_REGISTER_TO_FLAG, "OP_ADD_WORK_REGISTER_TO_FLAG"},
    {OP_MUL_VALUE_WITH_WORK, "OP_MUL_VALUE_WITH_WORK"},
    {OP_CMP_GREATER_EQUAL_THAN, "OP_CMP_GREATER_EQUAL_THAN"},
    {OP_GET_FLAG_ID, "OP_GET_FLAG_ID"},
    {OP_GOTO_IF_CURRENT_COMMAND_IN_PROGRESS, "OP_GOTO_IF_CURRENT_COMMAND_IN_PROGRESS"},
    {OP_GOTO_LABEL_IF_TRUE, "OP_GOTO_LABEL_IF_TRUE"},
    {OP_GOTO_IF_GREATER_OR_EQUAL, "OP_GOTO_IF_GREATER_OR_EQUAL"},
    {OP_IF_LESS_THAN_GOTO, "OP_IF_LESS_THAN_GOTO"},
    {OP_IF_GREATER_THAN_GOTO, "OP_IF_GREATER_THAN_GOTO"},
    {OP_EXECUTE_CALL, "OP_EXECUTE_CALL"},
    {OP_MOVE_WORK_REGISTER_TO_FLAG, "OP_MOVE_WORK_REGISTER_TO_FLAG"},
    {OP_SET_FLAG_TO_TRUE, "OP_SET_FLAG_TO_TRUE"},
    {OP_ADD_1_TO_FLAG, "OP_ADD_1_TO_FLAG"},
    {OP_ACTIVATE_SCENE, "OP_ACTIVATE_SCENE"},
    {OP_ACTIVATE_OBJ, "OP_ACTIVATE_OBJ"},
    {OP_IF_TARGET_IN_AREA, "OP_IF_TARGET_IN_AREA"},
    {OP_INSTANT_DESTROY_TARGET, "OP_INSTANT_DESTROY_TARGET"},
    {OP_DIST_TO_TARGET, "OP_DIST_TO_TARGET"},
    {OP_DIST_TO_SPOT, "OP_DIST_TO_SPOT"},
    {OP_IS_TARGET_ALIVE, "OP_IS_TARGET_ALIVE"},
    {OP_SET_OBJ_UNKNOWN, "OP_SET_OBJ_UNKNOWN"},
    {OP_SET_OBJ_TAKE_OFF, "OP_SET_OBJ_TAKE_OFF"},
    {OP_SET_OBJ_LAND, "OP_SET_OBJ_LAND"},
    {OP_SET_OBJ_FLY_TO_WP, "OP_SET_OBJ_FLY_TO_WP"},
    {OP_SET_OBJ_FLY_TO_AREA, "OP_SET_OBJ_FLY_TO_AREA"},
    {OP_SET_OBJ_DESTROY_TARGET, "OP_SET_OBJ_DESTROY_TARGET"},
    {OP_SET_OBJ_DEFEND_TARGET, "OP_SET_OBJ_DEFEND_TARGET"},
    {OP_SET_OBJ_FOLLOW_ALLY, "OP_SET_OBJ_FOLLOW_ALLY"},
    {OP_SET_MESSAGE, "OP_SET_MESSAGE"},
    {OP_DEACTIVATE_OBJ, "OP_DEACTIVATE_OBJ"},
    {OP_SELECT_FLAG_208, "OP_SELECT_FLAG_208"}
};

enum PilotsId {
    PRIMETIME=1,
    PHOENIX=2,
    BASELINE=3,
    ZORRO=4,
    TEX=5,
    VIXEN=6,
    HAWK=7
};

static std::map<uint8_t, std::string> pilot_names = {
    {1, "PRIMETIME"},
    {2, "PHOENIX"},
    {3, "BASELINE"},
    {4, "ZORRO"},
    {5, "TEX"},
    {6, "VIXEN"},
    {7, "HAWK"}
};
enum View { FRONT = 0, FOLLOW, RIGHT, LEFT, REAR, REAL, TARGET, EYE_ON_TARGET, MISSILE_CAM, OBJECT, AUTO_PILOT };

enum CatalogItems {
    CAT_AIM9J = 73,
    CAT_AIM9M = 74,
    CAT_AIM120 = 75,
    CAT_LAU3 = 76,
    CAT_AGM65D = 77,
    CAT_GBU15 = 78,
    CAT_MK20 = 79,
    CAT_MK82 = 80,
    CAT_DURANDAL = 81,
    CAT_PACK1 = 83,
    CAT_PACK2 = 84,
    CAT_PACK3 = 85,
    CAT_PACK4 = 86
};