#include "PSProg.h"
#include "PSMission.h"
#include "PSEntity.h"
#include <set>

PSProg::PSProg(SCMissionActors *profile, std::vector<PROG> prog, SCMission *mission, uint8_t prog_id) : SCProg(profile, prog, mission, prog_id) {
}

// Table de Strike Commander, plus les instructions propres à Pacific Strike.
std::map<uint8_t, SCProg::Instruction> PSProg::buildInstructionTable() {
    std::map<uint8_t, Instruction> table = SCProg::buildInstructionTable();
    table[PS_OP_NOOP_5] = static_cast<Instruction>(&PSProg::opNoop5);
    table[PS_OP_SWAP_WORK_AND_FLAG] = static_cast<Instruction>(&PSProg::opSwapWorkAndFlag);
    table[PS_OP_ADD_FLAG_TO_WORK] = static_cast<Instruction>(&PSProg::opAddFlagToWork);
    table[PS_OP_ADD_VALUE_TO_FLAG] = static_cast<Instruction>(&PSProg::opAddValueToFlag);
    table[PS_OP_SUB_FLAG_FROM_WORK] = static_cast<Instruction>(&PSProg::opSubFlagFromWork);
    table[PS_OP_SUB_VALUE_FROM_FLAG] = static_cast<Instruction>(&PSProg::opSubValueFromFlag);
    table[PS_OP_SUB_WORK_FROM_FLAG] = static_cast<Instruction>(&PSProg::opSubWorkFromFlag);
    table[PS_OP_OR_FLAG_WITH_WORK] = static_cast<Instruction>(&PSProg::opOrFlagWithWork);
    table[PS_OP_AND_FLAG_WITH_WORK] = static_cast<Instruction>(&PSProg::opAndFlagWithWork);
    table[PS_OP_XOR_FLAG_WITH_WORK] = static_cast<Instruction>(&PSProg::opXorFlagWithWork);
    table[PS_OP_MUL_FLAG_WITH_WORK] = static_cast<Instruction>(&PSProg::opMulFlagWithWork);
    table[PS_OP_DIV_WORK_BY_FLAG] = static_cast<Instruction>(&PSProg::opDivWorkByFlag);
    table[PS_OP_CMP_WORK_WITH_FLAG] = static_cast<Instruction>(&PSProg::opCmpWorkWithFlag);
    table[PS_OP_CMP_FLAG_WITH_WORK] = static_cast<Instruction>(&PSProg::opCmpFlagWithWork);
    table[PS_OP_MOVE_RESULT_TO_FLAG] = static_cast<Instruction>(&PSProg::opMoveResultToFlag);
    table[PS_OP_ORDER_FRIENDLY_OF_TYPE] = static_cast<Instruction>(&PSProg::opOrderFriendlyOfType);
    table[PS_OP_SPAWN_EFFECT_ON_OBJ] = static_cast<Instruction>(&PSProg::opSpawnEffectOnObj);
    table[PS_OP_FIRE_AT_SPOT] = static_cast<Instruction>(&PSProg::opFireAtSpot);
    table[PS_OP_SPAWN_AT_SPOT] = static_cast<Instruction>(&PSProg::opSpawnAtSpot);
    table[PS_OP_DETACH_CARRIED] = static_cast<Instruction>(&PSProg::opDetachCarried);
    table[PS_OP_LAUNCH_FROM_OBJ] = static_cast<Instruction>(&PSProg::opLaunchFromObj);
    table[PS_OP_SET_OBJ_DESTROY_TARGET_B] = static_cast<Instruction>(&PSProg::opSetObjDestroyTargetB);
    table[PS_OP_ATTACK_RANDOM_ENEMY_OF_TYPE] = static_cast<Instruction>(&PSProg::opAttackRandomEnemyOfType);
    table[PS_OP_DIVE_ON_RANDOM_ENEMY_OF_TYPE] = static_cast<Instruction>(&PSProg::opDiveOnRandomEnemyOfType);
    table[PS_OP_ORDER_OBJ_AND_SPOT] = static_cast<Instruction>(&PSProg::opOrderObjAndSpot);
    return table;
}

const std::map<uint8_t, SCProg::Instruction> &PSProg::instructionTable() {
    static const std::map<uint8_t, Instruction> table = PSProg::buildInstructionTable();
    return table;
}

// Drapeau de la mission (mission_data.flags, la table que le débogueur affiche) ; la table couvre tous les numéros
// qu'un script peut écrire (2 octets).
int16_t PSProg::flag(uint16_t number) {
    return this->mission->mission->mission_data.flags[number];
}

// Écrit le drapeau de la mission et le recopie dans PSState, que lisent le déroulement et les conversations. À partir
// de 256, un drapeau vaut 0 ou 1. La trace ne sort que si la valeur change (un script est rejoué à chaque image).
void PSProg::setVar(const char *name, uint16_t number, int value) {
    std::vector<int16_t> &flags = this->mission->mission->mission_data.flags;
    int16_t before = flags[number];
    int16_t after = (int16_t)value;
    if (number >= 256) {
        after = value != 0 ? 1 : 0;
    }
    flags[number] = after;
    PSGameState.setFlag(number, after);
    if (before != after) {
        printf("PSProg %d: %s %d, %d -> %d\n", this->prog_id, name, number, before, after);
    }
}

// Argument supplémentaire de l'instruction en cours (1 à 3), rangé par PSMissionParser dans les OP_SPOT_DATA qui
// la suivent ; 65 535 s'il est absent.
uint16_t PSProg::extraArg(size_t rank) {
    for (size_t k = 1; k <= rank; k++) {
        if (this->pc + k >= this->prog.size() || this->prog[this->pc + k].opcode != OP_SPOT_DATA) {
            return 65535;
        }
    }
    return this->prog[this->pc + rank].arg;
}

// 255 = l'objet qui joue le script (Expr_VM_ReadVarOrOverride_51EEA).
SCMissionActors *PSProg::actorOrSelf(uint16_t actor_id) {
    if (actor_id == 255) {
        return this->actor;
    }
    return this->findActor(actor_id);
}

// Chunk KIND de l'objet : une mission de Pacific Strike ne crée que des PSEntity (PSMission::createEntity).
PSKind PSProg::actorKind(SCMissionActors *candidate) {
    return static_cast<PSEntity *>(candidate->object->entity)->kind;
}

// MissionScript_PickRandomObjectOfType_545A4 : 6 objets au plus dont le chunk KIND vaut kind et, pour un avion,
// dont le camp vaut team ; un tiré au hasard.
SCMissionActors *PSProg::pickRandomOfKind(PSKind kind, int team) {
    std::vector<SCMissionActors *> found;
    for (auto candidate : this->mission->actors) {
        if (found.size() >= 6) {
            break;
        }
        if (candidate->is_destroyed || candidate->object == nullptr || candidate->object->entity == nullptr) {
            continue;
        }
        if (!candidate->is_active && candidate != this->mission->player) {
            continue;
        }
        if (actorKind(candidate) != kind) {
            continue;
        }
        if (candidate->plane != nullptr && candidate->team_id != team) {
            continue;
        }
        found.push_back(candidate);
    }
    if (found.empty()) {
        return nullptr;
    }
    return found[rand() % found.size()];
}

// Ordres 174 et 175 : la cible est tirée une fois dans le camp adverse, puis gardée.
SCMissionActors *PSProg::randomEnemyOfKind(PSKind kind) {
    PSMission *ps_mission = static_cast<PSMission *>(this->mission);
    SCMissionActors *enemy = nullptr;
    auto kept = ps_mission->random_targets.find(this->actor);
    if (kept != ps_mission->random_targets.end()) {
        enemy = kept->second;
    }
    if (enemy == nullptr || enemy->is_destroyed) {
        int team = this->actor->team_id == 1 ? 0xFF : 1;
        enemy = this->pickRandomOfKind(kind, team);
        ps_mission->random_targets[this->actor] = enemy;
        if (enemy != nullptr) {
            printf("PSProg %d: %s#%d tire au hasard la cible %s#%d (type %d, camp %d)\n", this->prog_id, this->actor->actor_name.c_str(), this->actor->actor_id, enemy->actor_name.c_str(), enemy->actor_id, kind, team);
        }
    }
    return enemy;
}

// Une seule fois par texte : un script est rejoué à chaque image.
void PSProg::traceOnce(const char *text) {
    static std::set<std::string> done;
    std::string line = "PSProg " + std::to_string(this->prog_id) + ", " + this->actor->actor_name + "#" + std::to_string(this->actor->actor_id) + ": " + text;
    if (done.insert(line).second) {
        printf("%s\n", line.c_str());
    }
}

void PSProg::opAdd1ToFlag(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->setVar("OP_ADD_1_TO_FLAG", instruction.arg, this->flag(instruction.arg) + 1);
}

void PSProg::opRemove1ToFlag(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->setVar("OP_REMOVE_1_TO_FLAG", instruction.arg, this->flag(instruction.arg) - 1);
}

void PSProg::opAddWorkRegisterToFlag(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->setVar("OP_ADD_WORK_REGISTER_TO_FLAG", instruction.arg, this->flag(instruction.arg) + this->work_register);
}

void PSProg::opMoveFlagToWorkRegister(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->work_register = this->flag(instruction.arg);
}

void PSProg::opMoveWorkRegisterToFlag(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->setVar("OP_MOVE_WORK_REGISTER_TO_FLAG", instruction.arg, this->work_register);
}

void PSProg::opSetFlagToTrue(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->setVar("OP_SET_FLAG_TO_TRUE", instruction.arg, 1);
}

void PSProg::opSetFlagToFalse(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->setVar("OP_SET_FLAG_TO_FALSE", instruction.arg, 0);
}

void PSProg::opTestFlag(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    if (this->flag(instruction.arg) != 0) {
        this->compare_flag = prog_compare_return_values::PROG_CMP_EQUAL;
    } else {
        this->compare_flag = prog_compare_return_values::PROG_CMP_NOT_EQUAL;
    }
}

void PSProg::opNoop5(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    return;
}

void PSProg::opSwapWorkAndFlag(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    int flag = this->flag(instruction.arg);
    this->setVar("OP_SWAP_WORK_AND_FLAG", instruction.arg, this->work_register);
    this->work_register = flag;
}

void PSProg::opAddFlagToWork(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->work_register += this->flag(instruction.arg);
}

void PSProg::opAddValueToFlag(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->setVar("OP_ADD_VALUE_TO_FLAG", instruction.arg, this->flag(instruction.arg) + instruction.arg);
}

void PSProg::opSubFlagFromWork(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->work_register -= this->flag(instruction.arg);
}

void PSProg::opSubValueFromFlag(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->setVar("OP_SUB_VALUE_FROM_FLAG", instruction.arg, this->flag(instruction.arg) - instruction.arg);
}

void PSProg::opSubWorkFromFlag(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->setVar("OP_SUB_WORK_FROM_FLAG", instruction.arg, this->flag(instruction.arg) - this->work_register);
}

void PSProg::opOrFlagWithWork(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->work_register |= this->flag(instruction.arg);
}

void PSProg::opAndFlagWithWork(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->work_register &= this->flag(instruction.arg);
}

void PSProg::opXorFlagWithWork(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->work_register ^= this->flag(instruction.arg);
}

void PSProg::opMulFlagWithWork(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->work_register *= this->flag(instruction.arg);
}

void PSProg::opDivWorkByFlag(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->work_register /= this->flag(instruction.arg);
}

void PSProg::opCmpWorkWithFlag(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    int flag = this->flag(instruction.arg);
    this->compare_flag = prog_compare_return_values::PROG_CMP_UNSET;
    if (this->work_register == flag) {
        this->compare_flag |= PROG_CMP_EQUAL;
    }
    if (this->work_register < flag) {
        this->compare_flag |= PROG_CMP_LESS;
    }
    if (this->work_register > flag) {
        this->compare_flag |= PROG_CMP_GREATER;
    }
}

void PSProg::opCmpFlagWithWork(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    int flag = this->flag(instruction.arg);
    this->compare_flag = prog_compare_return_values::PROG_CMP_UNSET;
    if (flag == this->work_register) {
        this->compare_flag |= PROG_CMP_EQUAL;
    }
    if (flag < this->work_register) {
        this->compare_flag |= PROG_CMP_LESS;
    }
    if (flag > this->work_register) {
        this->compare_flag |= PROG_CMP_GREATER;
    }
}

void PSProg::opMoveResultToFlag(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->setVar("OP_MOVE_RESULT_TO_FLAG", instruction.arg, this->task_state);
}

void PSProg::opOrderFriendlyOfType(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    SCMissionActors *friendly = nullptr;
    if (this->actor->plane != nullptr) {
        friendly = this->pickRandomOfKind((PSKind)instruction.arg, this->actor->team_id);
    }
    if (friendly == nullptr) {
        this->task_state = 0;
        return;
    }
    this->task_state = this->actor->setObjective(OP_SET_OBJ_DEFEND_TARGET, (uint8_t)friendly->actor_id) ? 1 : 0;
}

void PSProg::opSpawnEffectOnObj(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    char text[160];
    snprintf(text, sizeof(text), "OP_SPAWN_EFFECT_ON_OBJ objet %d, SPOT %d, LOAD %d : pas encore porte", instruction.arg, this->extraArg(1), this->extraArg(2));
    this->traceOnce(text);
}

void PSProg::opFireAtSpot(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    char text[160];
    snprintf(text, sizeof(text), "OP_FIRE_AT_SPOT objet %d, SPOT de depart %d, LOAD %d, SPOT vise %d : pas encore porte", instruction.arg, this->extraArg(1), this->extraArg(2), this->extraArg(3));
    this->traceOnce(text);
}

void PSProg::opSpawnAtSpot(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    char text[160];
    snprintf(text, sizeof(text), "OP_SPAWN_AT_SPOT %d, arguments suivants %d %d : pas encore porte", instruction.arg, this->extraArg(1), this->extraArg(2));
    this->traceOnce(text);
}

void PSProg::opDetachCarried(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    char text[160];
    snprintf(text, sizeof(text), "OP_DETACH_CARRIED objet %d : pas encore porte", instruction.arg);
    this->traceOnce(text);
}

void PSProg::opLaunchFromObj(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    SCMissionActors *launched = this->actorOrSelf(instruction.arg);
    SCMissionActors *carrier = this->actorOrSelf(this->extraArg(1));
    uint16_t spot_id = this->extraArg(2);
    if (launched == nullptr || carrier == nullptr || spot_id >= this->mission->mission->mission_data.spots.size() || launched->is_active) {
        return;
    }
    this->actor->activateTarget((uint8_t)launched->actor_id);
    Vector3D offset = this->mission->mission->mission_data.spots[spot_id]->origin;
    Vector3D position = carrier->object->position;
    Matrix orientation;
    orientation.Identity();
    if (carrier->plane != nullptr) {
        position = Vector3D(carrier->plane->x, carrier->plane->y, carrier->plane->z);
        orientation.rotateM(tenthOfDegreeToRad(carrier->plane->yaw), 0, 1, 0);
        orientation.rotateM(tenthOfDegreeToRad(carrier->plane->pitch), 1, 0, 0);
        orientation.rotateM(tenthOfDegreeToRad(carrier->plane->roll), 0, 0, 1);
    } else {
        orientation.rotateM(tenthOfDegreeToRad((360.0f - (float)carrier->object->azymuth) * 10.0f), 0, 1, 0);
    }
    position.x += offset.x * orientation.v[0][0] + offset.y * orientation.v[1][0] + offset.z * orientation.v[2][0];
    position.y += offset.x * orientation.v[0][1] + offset.y * orientation.v[1][1] + offset.z * orientation.v[2][1];
    position.z += offset.x * orientation.v[0][2] + offset.y * orientation.v[1][2] + offset.z * orientation.v[2][2];
    launched->object->position = position;
    launched->object->azymuth = carrier->object->azymuth;
    if (launched->plane != nullptr) {
        launched->plane->x = position.x;
        launched->plane->y = position.y;
        launched->plane->z = position.z;
        if (carrier->plane != nullptr) {
            launched->plane->yaw = carrier->plane->yaw;
            launched->plane->pitch = carrier->plane->pitch;
            launched->plane->roll = carrier->plane->roll;
        } else {
            launched->plane->yaw = (360.0f - (float)carrier->object->azymuth) * 10.0f;
        }
        launched->plane->on_ground = false;
    }
    printf("PSProg %d: OP_LAUNCH_FROM_OBJ %s#%d part de %s#%d, SPOT %d (%.1f, %.1f, %.1f), position (%.1f, %.1f, %.1f)\n", this->prog_id, launched->actor_name.c_str(), launched->actor_id, carrier->actor_name.c_str(), carrier->actor_id, spot_id, offset.x, offset.y, offset.z, position.x, position.y, position.z);
}

void PSProg::opSetObjDestroyTargetB(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    this->task_state = this->actor->setObjective(OP_SET_OBJ_DESTROY_TARGET, (uint8_t)instruction.arg) ? 1 : 0;
}

void PSProg::opAttackRandomEnemyOfType(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    SCMissionActors *enemy = this->randomEnemyOfKind((PSKind)instruction.arg);
    if (enemy == nullptr) {
        this->task_state = 0;
        return;
    }
    this->task_state = this->actor->setObjective(OP_SET_OBJ_DESTROY_TARGET, (uint8_t)enemy->actor_id) ? 1 : 0;
}

void PSProg::opDiveOnRandomEnemyOfType(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    SCMissionActors *enemy = this->randomEnemyOfKind((PSKind)instruction.arg);
    if (enemy == nullptr) {
        this->task_state = 0;
        return;
    }
    this->traceOnce("OP_DIVE_ON_RANDOM_ENEMY_OF_TYPE : attaque en pique sans arme pas encore portee, joue comme OP_SET_OBJ_DESTROY_TARGET");
    this->task_state = this->actor->setObjective(OP_SET_OBJ_DESTROY_TARGET, (uint8_t)enemy->actor_id) ? 1 : 0;
}

// MissionScript_CallNativeHandler_53B0B cas 193, puis Goal_SetObjective_C2A8 cas 193 : atterrissage sur un objet.
// Point de toucher = position de l'objet + position du SPOT ; second point 3000 m plus loin et 1000 m plus haut ;
// l'ordre devient OP_SET_OBJ_LAND. Les deux points sont rangés dans deux SPOT ajoutés à la liste, que l'ordre
// d'atterrissage lit par leur rang.
void PSProg::opOrderObjAndSpot(const PROG &instruction) {
    if (!this->exec) {
        return;
    }
    SCMissionActors *carrier = this->actorOrSelf(instruction.arg);
    uint16_t spot_id = this->extraArg(1);
    if (carrier == nullptr || spot_id == 65535) {
        char text[160];
        snprintf(text, sizeof(text), "OP_ORDER_OBJ_AND_SPOT objet %d, SPOT %d : objet introuvable ou SPOT absent", instruction.arg, spot_id);
        this->traceOnce(text);
        return;
    }
    std::vector<SPOT *> &spots = this->mission->mission->mission_data.spots;
    Vector3D touchdown = carrier->object->position;
    if (carrier->plane != nullptr) {
        touchdown = Vector3D(carrier->plane->x, carrier->plane->y, carrier->plane->z);
    }
    if (spot_id < spots.size()) {
        touchdown += spots[spot_id]->position;
    }
    Vector3D approach = touchdown + Vector3D(0.0f, 1000.0f, -3000.0f);
    PSMission *ps_mission = static_cast<PSMission *>(this->mission);
    auto kept = ps_mission->landing_spots.find(this->actor);
    if (kept == ps_mission->landing_spots.end()) {
        SPOT *approach_spot = new SPOT();
        approach_spot->id = (int)spots.size();
        approach_spot->area_id = -1;
        spots.push_back(approach_spot);
        SPOT *touchdown_spot = new SPOT();
        touchdown_spot->id = (int)spots.size();
        touchdown_spot->area_id = -1;
        spots.push_back(touchdown_spot);
        ps_mission->landing_spots[this->actor] = spots.size() - 2;
        kept = ps_mission->landing_spots.find(this->actor);
        printf("PSProg %d: OP_ORDER_OBJ_AND_SPOT %s#%d se pose sur %s#%d, SPOT %d, toucher (%.1f, %.1f, %.1f), approche (%.1f, %.1f, %.1f), SPOT ajoutes %zu et %zu\n", this->prog_id, this->actor->actor_name.c_str(), this->actor->actor_id, carrier->actor_name.c_str(), carrier->actor_id, spot_id, touchdown.x, touchdown.y, touchdown.z, approach.x, approach.y, approach.z, kept->second, kept->second + 1);
    }
    size_t approach_rank = kept->second;
    spots[approach_rank]->position = approach;
    spots[approach_rank]->origin = approach;
    spots[approach_rank + 1]->position = touchdown;
    spots[approach_rank + 1]->origin = touchdown;
    this->task_state = this->actor->setObjective(OP_SET_OBJ_LAND, (uint8_t)approach_rank) ? 1 : 0;
    if (this->actor->current_command == OP_SET_OBJ_LAND && this->actor->current_command_arg == (uint8_t)approach_rank) {
        this->actor->current_command_arg2 = (uint8_t)(approach_rank + 1);
    }
}
