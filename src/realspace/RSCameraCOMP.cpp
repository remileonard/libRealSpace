#include "RSCameraCOMP.h"
#include "../commons/ByteStream.h"
#include <algorithm>

// ---- table des formes, indexée par l'octet d'opcode -------------------------
// (designated array initializers : C, et extension GCC/Clang en C++ — comme le
//  reste de libRealSpace. Les entrées non listées valent {0,0,0,false,false}.)
// { size, nI32, nI16, hasName, hasFlag, {fix par slot} }
const COMPShape COMP_SHAPE[256] = {
    [OP_IA_ARM_HOLD]             = {  5, 1, 0, false, false, {false, false, false, false} },
    [OP_IA_SET_POS_ABS]          = { 13, 3, 0, false, false, {false, false, false, false} }, // <<8
    [OP_IA_REBUILD_MATRIX]       = {  1, 0, 0, false, false, {false, false, false, false} },
    [OP_IA_ARM_TURN]             = { 11, 1, 3, false, false, {false, false, false, false} }, // arg0 compteur, arg1-3 deg
    [OP_IA_SET_ROT_RATE]         = {  7, 0, 3, false, false, {false, false, false, false} }, // deg
    [OP_IA_SET_VELOCITY]         = { 13, 3, 0, false, false, {true,  true,  true,  false} }, // 24.8
    [OP_IA_BIND_ENTITY]          = {  9, 0, 0, true,  false, {false, false, false, false} },
    [OP_IA_SET_A0_MODE8]         = {  5, 1, 0, false, false, {false, false, false, false} }, // compteur
    [OP_IA_SET_DIST]             = {  5, 1, 0, false, false, {true,  false, false, false} }, // 24.8
    [OP_IA_SET_REL_POS]          = { 13, 3, 0, false, false, {false, false, false, false} }, // <<8
    [OP_IA_SET_A0_MODE_B]        = {  5, 1, 0, false, false, {false, false, false, false} }, // compteur
    [OP_IA_SET_DIST_C]           = {  5, 1, 0, false, false, {true,  false, false, false} }, // 24.8
    [OP_IA_DIV_SETUP]            = { 14, 3, 0, false, true,  {true,  false, true,  false} }, // arg1 compteur
    [OP_IA_SET_POS_REL_ENTITY]   = { 13, 3, 0, false, false, {false, false, false, false} }, // <<8
    [OP_IA_ARM_APPROACH_ANGLE]   = {  5, 0, 2, false, false, {false, false, false, false} }, // brut + deg
    [OP_IA_COMPUTE_GEOM]         = {  1, 0, 0, false, false, {false, false, false, false} },
    [OP_IA_SET_ORIENT_AXES]      = {  7, 0, 3, false, false, {false, false, false, false} }, // deg
    [OP_IA_SET_ANCHOR_OFS]       = { 13, 3, 0, false, false, {true,  true,  true,  false} }, // 24.8
    [OP_IA_FLAG_FF]              = {  1, 0, 0, false, false, {false, false, false, false} },
    [OP_IA_VEL_FROM_ORIENT_ROW]  = {  5, 1, 0, false, false, {true,  false, false, false} }, // 24.8
    [OP_IA_SET_VEC_CC]           = { 13, 3, 0, false, false, {true,  true,  true,  false} }, // 24.8
    [OP_IA_ACCEL_FROM_ORIENT_ROW]= {  5, 1, 0, false, false, {true,  false, false, false} }, // 24.8
    [OP_IA_VEL_DIV]              = {  5, 1, 0, false, false, {false, false, false, false} }, // diviseur brut
    [OP_IA_VEL_FROM_ENTITY]      = {  1, 0, 0, false, false, {false, false, false, false} },
    [OP_IA_ROT_VEL_BY_ENTITY]    = {  1, 0, 0, false, false, {false, false, false, false} },
    [OP_IA_ROT_VEC_CC_BY_ENTITY] = {  1, 0, 0, false, false, {false, false, false, false} },
    [OP_IA_START_FLAG]           = {  1, 0, 0, false, false, {false, false, false, false} },
    [OP_IA_DIST_TO_ENTITY]       = {  1, 0, 0, false, false, {false, false, false, false} },
    [OP_IA_ARM_MOVE_SEGMENT]     = { 17, 4, 0, false, false, {false, true,  true,  true } }, // arg0 compteur
    [OP_IA_END]                  = {  9, 0, 0, true,  false, {false, false, false, false} },
};
const std::unordered_map<COMPOp, std::string> comp_op_names = {
    {OP_IA_ARM_HOLD,             "OP_IA_ARM_HOLD"},
    {OP_IA_SET_POS_ABS,          "OP_IA_SET_POS_ABS"},
    {OP_IA_REBUILD_MATRIX,       "OP_IA_REBUILD_MATRIX"},
    {OP_IA_ARM_TURN,             "OP_IA_ARM_TURN"},
    {OP_IA_SET_ROT_RATE,         "OP_IA_SET_ROT_RATE"},
    {OP_IA_SET_VELOCITY,         "OP_IA_SET_VELOCITY"},
    {OP_IA_BIND_ENTITY,          "OP_IA_BIND_ENTITY"},
    {OP_IA_SET_A0_MODE8,         "OP_IA_SET_A0_MODE8"},
    {OP_IA_SET_DIST,             "OP_IA_SET_DIST"},
    {OP_IA_SET_REL_POS,          "OP_IA_SET_REL_POS"},
    {OP_IA_SET_A0_MODE_B,        "OP_IA_SET_A0_MODE_B"},
    {OP_IA_SET_DIST_C,           "OP_IA_SET_DIST_C"},
    {OP_IA_DIV_SETUP,            "OP_IA_DIV_SETUP"},
    {OP_IA_SET_POS_REL_ENTITY,   "OP_IA_SET_POS_REL_ENTITY"},
    {OP_IA_ARM_APPROACH_ANGLE,   "OP_IA_ARM_APPROACH_ANGLE"},
    {OP_IA_COMPUTE_GEOM,         "OP_IA_COMPUTE_GEOM"},
    {OP_IA_SET_ORIENT_AXES,      "OP_IA_SET_ORIENT_AXES"},
    {OP_IA_SET_ANCHOR_OFS,       "OP_IA_SET_ANCHOR_OFS"},
    {OP_IA_FLAG_FF,              "OP_IA_FLAG_FF"},
    {OP_IA_VEL_FROM_ORIENT_ROW,  "OP_IA_VEL_FROM_ORIENT_ROW"},
    {OP_IA_SET_VEC_CC,           "OP_IA_SET_VEC_CC"},
    {OP_IA_ACCEL_FROM_ORIENT_ROW,"OP_IA_ACCEL_FROM_ORIENT_ROW"},
    {OP_IA_VEL_DIV,              "OP_IA_VEL_DIV"},
    {OP_IA_VEL_FROM_ENTITY,      "OP_IA_VEL_FROM_ENTITY"},
    {OP_IA_ROT_VEL_BY_ENTITY,    "OP_IA_ROT_VEL_BY_ENTITY"},
    {OP_IA_ROT_VEC_CC_BY_ENTITY, "OP_IA_ROT_VEC_CC_BY_ENTITY"},
    {OP_IA_START_FLAG,           "OP_IA_START_FLAG"},
    {OP_IA_DIST_TO_ENTITY,       "OP_IA_DIST_TO_ENTITY"},
    {OP_IA_ARM_MOVE_SEGMENT,     "OP_IA_ARM_MOVE_SEGMENT"},
    {OP_IA_END,                  "OP_IA_END"},
};


std::string compOpName(COMPOp op) {
    std::unordered_map<COMPOp, std::string>::const_iterator it = comp_op_names.find(op);
    if (it != comp_op_names.end()) {
        return it->second;
    }
    return "OP_IA_UNKNOWN";
}

COMPInstr decodeCOMPInstr(ByteStream &s) {
    COMPInstr in;
    in.op = (COMPOp)s.ReadByte();
    const COMPShape &sh = COMP_SHAPE[in.op];
    in.size = sh.size ? sh.size : 1;      // opcode inconnu -> 1 octet consommé (comme le moteur)

    for (int i = 0; i < sh.nI32; i++) {
        float v = (float)s.ReadInt32LE();
        in.args[in.argc] = sh.fix[in.argc] ? v / 256.0f : v;
        in.argc++;
    }
    for (int i = 0; i < sh.nI16; i++) {
        float v = (float)s.ReadShort();   // signé
        in.args[in.argc] = sh.fix[in.argc] ? v / 256.0f : v;
        in.argc++;
    }
    if (sh.hasName) {
        in.name    = s.ReadString(8);
    }
    if (sh.hasFlag) {
        in.divFlag = s.ReadByte();
    }
    return in;
}

RSCameraSequence decodeCOMPSequence(uint8_t *data, size_t size) {
    RSCameraSequence seq;
    ByteStream s(data, size);

    // --- en-tête : 0x1C octets (cf. AIManeuver_Helper_780A5) ---
    seq.name    = s.ReadString(8);        // +0x00
    seq.flags   = s.ReadUShort();         // +0x08
    seq.farClip = s.ReadUInt32LE();       // +0x0A
    seq.param0  = s.ReadShort();          // +0x0E
    seq.leadIn  = s.ReadInt32LE();        // +0x10
    s.MoveForward(4);                     // +0x14 / +0x16  (deux u16 à 0)
    seq.viewW   = s.ReadUShort();         // +0x18
    seq.viewH   = s.ReadUShort();         // +0x1A   -> position = 0x1C

    // --- programme : jusqu'à OP_IA_END ---
    while (s.GetPosition() < data + size) {
        COMPInstr in = decodeCOMPInstr(s);
        seq.program.push_back(in);
        if (in.op == OP_IA_END) {
            seq.handoffView = in.name;    // 8 c lus par decodeCOMPInstr (shape hasName)
            break;
        }
    }
    return seq;
}