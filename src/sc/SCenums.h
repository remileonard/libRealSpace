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
    OP_GOTO_IF_FALSE_73 = 73,
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
    CAT_PACK1 = 85,
    CAT_PACK2 = 86,
    CAT_PACK3 = 87,
    CAT_PACK4 = 88
};