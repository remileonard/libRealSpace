#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include "../commons/ByteStream.h"
// ============================================================================
//  Script COMP  (chunk FORM CAMR / WRLD)  —  bytecode exécuté par le solveur de
//  manoeuvre AI_ManeuverSolutionMain_781D0. Instructions de taille variable.
//  Ce header ne contient QUE le décodage ; l'exécution est ailleurs.
// ============================================================================

// L'opcode EST la valeur de l'octet. Noms d'après le rôle décodé côté moteur.
enum COMPOp : uint8_t {
    OP_IA_ARM_HOLD             = 0x01, // i32 a             -> A0 ; mode 1 (maintien temporisé)
    OP_IA_SET_POS_ABS          = 0x02, // i32 x,y,z         -> pos 14/18/1C (<<8)
    OP_IA_REBUILD_MATRIX       = 0x03, // -
    OP_IA_ARM_TURN             = 0x04, // i32 a ; i16 x,y,z -> A0 / cible E4/E8/EC ; mode 4
    OP_IA_SET_ROT_RATE         = 0x05, // i16 x,y,z         -> D8/DC/E0 (<<8)
    OP_IA_SET_VELOCITY         = 0x06, // i32 a,b,c         -> C0/C4/C8 (raw)
    OP_IA_BIND_ENTITY          = 0x07, // char[8]           -> entité liée obj+92
    OP_IA_SET_A0_MODE8         = 0x08, // i32 a             -> A0 ; mode 8
    OP_IA_SET_DIST             = 0x09, // i32 a             -> BC
    OP_IA_SET_REL_POS          = 0x0A, // i32 x,y,z         -> B0/B4/B8 (<<8)
    OP_IA_SET_A0_MODE_B        = 0x0B, // i32 a             -> A0 ; mode 0xB
    OP_IA_SET_DIST_C           = 0x0C, // i32 a             -> BC ; flags FF=0,100=1
    OP_IA_DIV_SETUP            = 0x0D, // i32 p,q,r ; u8 flag -> A4/A0/A8 (division 24.8)
    OP_IA_SET_POS_REL_ENTITY   = 0x0E, // i32 x,y,z         -> pos, tourné+offset par entité (<<8)
    OP_IA_ARM_APPROACH_ANGLE   = 0x0F, // i16 h,k           -> F0 (raw) / F2 (<<8)
    OP_IA_COMPUTE_GEOM         = 0x10, // -
    OP_IA_SET_ORIENT_AXES      = 0x11, // i16 a,b,c         -> axes matrice (<<8)
    OP_IA_SET_ANCHOR_OFS       = 0x12, // i32 x,y,z         -> 94/98/9C (raw)
    OP_IA_FLAG_FF              = 0x13, // -                 -> FF=1,100=0
    OP_IA_VEL_FROM_ORIENT_ROW  = 0x14, // i32 s             -> C0/C4/C8 = ligne_orient(2C) * s
    OP_IA_SET_VEC_CC           = 0x15, // i32 x,y,z         -> CC/D0/D4 (raw)
    OP_IA_ACCEL_FROM_ORIENT_ROW= 0x16, // i32 s             -> CC/D0/D4 = ligne_orient(2C) * s
    OP_IA_VEL_DIV              = 0x17, // i32 d             -> C0/C4/C8 = (<<8) / d
    OP_IA_VEL_FROM_ENTITY      = 0x18, // -                 -> C0/C4/C8 = vitesse(entité)
    OP_IA_ROT_VEL_BY_ENTITY    = 0x19, // -                 -> tourne C0 par orient(entité)
    OP_IA_ROT_VEC_CC_BY_ENTITY = 0x1A, // -                 -> tourne CC par orient(entité)
    OP_IA_START_FLAG           = 0x1B, // -                 -> byte_70471=0 ; obj+101=1
    OP_IA_DIST_TO_ENTITY       = 0x1C, // -                 -> BC = |pos_entité - pos|
    OP_IA_ARM_MOVE_SEGMENT     = 0x1D, // i32 a,x,y,z       -> A0 / cible E4/E8/EC ; mode 0x1D
    OP_IA_END                  = 0xFE, // char[8]           -> FIN ; caméra suivante = ce nom
};

// Forme binaire d'un opcode. Ordre des opérandes : nI32 i32, puis nI16 i16.
struct COMPShape {
    uint8_t size;     // taille totale de l'instruction (opcode inclus) ; 0 = opcode non défini
    uint8_t nI32;     // nombre d'opérandes i32   (a, b, c, d dans l'ordre)
    uint8_t nI16;     // nombre d'opérandes i16   (sign-étendues, à la suite des i32)
    bool    hasName;  // char[8] juste après l'opcode  (BIND_ENTITY / END)
    bool    hasFlag;  // octet de sélection de formule à la fin  (DIV_SETUP)
};

// Indexé par la valeur de l'octet d'opcode ; défini dans le .cpp.
extern const COMPShape COMP_SHAPE[256];


// Noms lisibles des opcodes (logs / désassembleur).
extern const std::unordered_map<COMPOp, std::string> comp_op_names;
std::string compOpName(COMPOp op);   // pour les logs / le désassembleur (défini dans le .cpp)

struct COMPInstr {
    COMPOp      op   = OP_IA_END;
    uint8_t     size = 0;                 // taille dans le fichier (re-dump / debug)
    uint8_t     argc = 0;                 // nombre d'opérandes numériques
    int32_t     args[4] = { 0, 0, 0, 0 }; // i32 puis i16 (sign-étendues), sens selon op
    std::string name;                     // OP_IA_BIND_ENTITY / OP_IA_END
    uint8_t     divFlag = 0;              // OP_IA_DIV_SETUP
};

// Une séquence caméra COMP : en-tête 0x1C octets + programme décodé.
struct RSCameraSequence {
    std::string           name;          // en-tête +0x00  ("STARTCAM"/"TAKEOFF"/...)
    uint16_t              flags   = 0;   // en-tête +0x08
    uint32_t              farClip = 0;   // en-tête +0x0A   (50000)
    int16_t              param0  = 0;   // en-tête +0x0E   (le moteur fait movsx<<8)
    int32_t              leadIn  = 0;   // en-tête +0x10   (le moteur traite en 24.8)
    uint16_t              viewW   = 0;   // en-tête +0x18   (319)
    uint16_t              viewH   = 0;   // en-tête +0x1A   (199)
    std::vector<COMPInstr> program;      // décodé depuis +0x1C, se termine par OP_IA_END
    std::string           handoffView;  // 8 c après OP_IA_END  ("COCKPIT")
};

COMPInstr        decodeCOMPInstr(ByteStream &s);
RSCameraSequence decodeCOMPSequence(uint8_t *data, size_t size);