#include "RSMission.h"
#include "block_def.h"
#include <cmath>

void missionstrtoupper(char *src) {
    int i = 0;
    for (int i = 0; i < strlen(src); i++) {
        src[i] = toupper(src[i]);
    }
}

RSMission::RSMission() { printf("CREATE MISSION\n"); }

RSMission::~RSMission() {}

MISN_PART *RSMission::getPlayerCoord() { 
    int search_id = 0;
    if (this->player != nullptr) {
        return this->player;
    }
    for (auto cast : this->mission_data.casting) {
        if (cast->actor == "PLAYER") {
            for (auto part : this->mission_data.parts) {
                if (part->id == search_id) {
                    this->player = part;
                    return part;
                }
            }
        }
        search_id++;
    }
    return nullptr;
}
MISN_PART *RSMission::getObject(const char *name) { 
    for (auto obj : this->mission_data.parts) {
        if (obj->member_name == name) {
            return obj;
        }
    }
    return nullptr;
}

void RSMission::InitFromRAM(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;
    this->player = nullptr;
    std::unordered_map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["MISN"] = std::bind(&RSMission::parseMISN, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

void RSMission::parseMISN(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;

    std::unordered_map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["VERS"] = std::bind(&RSMission::parseMISN_VERS, this, std::placeholders::_1, std::placeholders::_2);
    handlers["INFO"] = std::bind(&RSMission::parseMISN_INFO, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TUNE"] = std::bind(&RSMission::parseMISN_TUNE, this, std::placeholders::_1, std::placeholders::_2);
    handlers["NAME"] = std::bind(&RSMission::parseMISN_NAME, this, std::placeholders::_1, std::placeholders::_2);
    handlers["WRLD"] = std::bind(&RSMission::parseMISN_WRLD, this, std::placeholders::_1, std::placeholders::_2);

    handlers["AREA"] = std::bind(&RSMission::parseMISN_AREA, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SPOT"] = std::bind(&RSMission::parseMISN_SPOT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["NUMS"] = std::bind(&RSMission::parseMISN_NUMS, this, std::placeholders::_1, std::placeholders::_2);
    handlers["MSGS"] = std::bind(&RSMission::parseMISN_MSGS, this, std::placeholders::_1, std::placeholders::_2);
    handlers["FLAG"] = std::bind(&RSMission::parseMISN_FLAG, this, std::placeholders::_1, std::placeholders::_2);
    handlers["CAST"] = std::bind(&RSMission::parseMISN_CAST, this, std::placeholders::_1, std::placeholders::_2);
    handlers["PROG"] = std::bind(&RSMission::parseMISN_PROG, this, std::placeholders::_1, std::placeholders::_2);
    handlers["PART"] = std::bind(&RSMission::parseMISN_PART, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TEAM"] = std::bind(&RSMission::parseMISN_TEAM, this, std::placeholders::_1, std::placeholders::_2);
    handlers["PLAY"] = std::bind(&RSMission::parseMISN_PLAY, this, std::placeholders::_1, std::placeholders::_2);
    handlers["LOAD"] = std::bind(&RSMission::parseMISN_LOAD, this, std::placeholders::_1, std::placeholders::_2);
    handlers["CACH"] = std::bind(&RSMission::parseMISN_CACH, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

void RSMission::parseMISN_VERS(uint8_t *data, size_t size) {
    ByteStream stream(data, size);
    this->mission_data.version = stream.ReadUShort();
}
void RSMission::parseMISN_INFO(uint8_t *data, size_t size) {
    ByteStream stream(data, size);
    if (size > 0) {
        this->mission_data.info = stream.ReadString(size);
    }
}
void RSMission::parseMISN_TUNE(uint8_t *data, size_t size) {
    ByteStream stream(data, size);
    this->mission_data.tune = stream.ReadByte();
}
void RSMission::parseMISN_NAME(uint8_t *data, size_t size) {
    ByteStream stream(data, size);
    for (int i = 0; i < size; i++) {
        this->mission_data.name.push_back(stream.ReadByte());
    }
    std::transform(this->mission_data.name.begin(), this->mission_data.name.end(), this->mission_data.name.begin(), ::toupper);
}
void RSMission::parseMISN_WRLD(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;

    std::unordered_map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["FILE"] = std::bind(&RSMission::parseMISN_WRLD_FILE, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}
void RSMission::parseMISN_WRLD_FILE(uint8_t *data, size_t size) {
    ByteStream stream(data, size);
    this->mission_data.world_filename = stream.ReadStringNoSize(size);
}
void RSMission::parseMISN_AREA(uint8_t *data, size_t size) {
    size_t offset = 0;
    int cpt = 0;
    while (offset < size) {
        unsigned char shape = data[offset];
        size_t length = 0;
        if (shape == 'S') {
            length = 49;
        } else if (shape == 'C') {
            length = 53;
        } else if (shape == 'B') {
            length = 65;
        }
        if (length == 0 || offset + length > size) {
            if (length == 0 && offset + 1 < size) {
                printf("ERROR IN PARSING AREA\n");
            }
            break;
        }
        ByteStream stream(data + offset, length);
        AREA *tmparea = new AREA();
        tmparea->id = ++cpt;
        tmparea->AreaType = stream.ReadByte();
        for (int k = 0; k < 32; k++) {
            tmparea->AreaName[k] = stream.ReadByte();
        }
        tmparea->AreaName[32] = '\0';
        missionstrtoupper(tmparea->AreaName);
        float c0 = stream.ReadFixedFloatLE();
        float c1 = stream.ReadFixedFloatLE();
        float c2 = stream.ReadFixedFloatLE();
        tmparea->position.x = c0 * BLOCK_COORD_SCALE;
        tmparea->position.z = -c1 * BLOCK_COORD_SCALE;
        tmparea->position.y = c2 * HEIGH_MAP_SCALE;
        if (shape == 'S') {
            tmparea->radius = stream.ReadFixedFloatLE() * BLOCK_COORD_SCALE;
            tmparea->AreaWidth = (unsigned int) tmparea->radius;
            tmparea->AreaHeight = tmparea->AreaWidth;
        } else if (shape == 'C') {
            tmparea->height = stream.ReadFixedFloatLE() * HEIGH_MAP_SCALE;
            tmparea->radius = stream.ReadFixedFloatLE() * BLOCK_COORD_SCALE;
            tmparea->AreaWidth = (unsigned int) tmparea->height;
            tmparea->AreaHeight = (unsigned int) tmparea->radius;
        } else {
            tmparea->east = stream.ReadFixedFloatLE() * BLOCK_COORD_SCALE;
            tmparea->west = stream.ReadFixedFloatLE() * BLOCK_COORD_SCALE;
            tmparea->north = stream.ReadFixedFloatLE() * BLOCK_COORD_SCALE;
            tmparea->south = stream.ReadFixedFloatLE() * BLOCK_COORD_SCALE;
            tmparea->height = stream.ReadFixedFloatLE() * HEIGH_MAP_SCALE;
            tmparea->AreaWidth = (unsigned int) tmparea->east;
            tmparea->AreaHeight = (unsigned int) tmparea->south;
        }
        this->mission_data.areas.push_back(tmparea);
        offset += length;
    }
}
bool AREA::contains(const Vector3D &point) const {
    if (this->AreaType == 'S') {
        float dx = point.x - this->position.x;
        float dy = point.y - this->position.y;
        float dz = point.z - this->position.z;
        return std::sqrt(dx * dx + dy * dy + dz * dz) <= this->radius;
    }
    if (point.y < this->position.y || point.y > this->position.y + this->height) {
        return false;
    }
    if (this->AreaType == 'C') {
        float dx = point.x - this->position.x;
        float dz = point.z - this->position.z;
        return std::sqrt(dx * dx + dz * dz) <= this->radius;
    }
    if (this->AreaType == 'B') {
        return point.x >= this->position.x - this->west && point.x <= this->position.x + this->east &&
               point.z >= this->position.z - this->north && point.z <= this->position.z + this->south;
    }
    return false;
}
void RSMission::parseMISN_SPOT(uint8_t *data, size_t size) {
    size_t numSpots = size / 14;
    for (size_t i = 0; i < numSpots; i++) {
        ByteStream stream(data + i * 14, 14);
        SPOT *spt = new SPOT();
        spt->id = (int) i;
        // Index de zone sur 16 bits (PtrList_GetAt_53034) ; 65535 donne -1.
        spt->area_id = (short) stream.ReadUShort();
        float c0 = stream.ReadFixedFloatLE();
        float c1 = stream.ReadFixedFloatLE();
        float c2 = stream.ReadFixedFloatLE();
        spt->position = Vector3D(c0 * BLOCK_COORD_SCALE, c2 * HEIGH_MAP_SCALE, -c1 * BLOCK_COORD_SCALE);
        this->mission_data.spots.push_back(spt);
    }
}
void RSMission::parseMISN_NUMS(uint8_t *data, size_t size) {
    ByteStream stream(data, size);
    for (int i = 0; i < size; i++) {
        this->mission_data.nums.push_back(stream.ReadByte());
    }
}
void RSMission::parseMISN_MSGS(uint8_t *data, size_t size) {
    size_t read = 0;
    while (read < size) {
        
        char buffer[512];
        int i = 0;
        while (data[read] != '\0') {
            buffer[i]= data[read];
            read++;
            i++;
        }
        if (data[read] == '\0') {
            buffer[i] = '\0';
            std::string *msg = new std::string(buffer);
            read++;
            this->mission_data.messages.push_back(msg);
        }
    }
}
void RSMission::parseMISN_FLAG(uint8_t *data, size_t size) {
    if (size == 0) {
        this->mission_data.flags.clear();
        this->mission_data.flags.reserve(65536);
        for (int i = 0; i < 65536; i++) {
            this->mission_data.flags.push_back(0);
        }
        return;
    }
    ByteStream stream(data, size);
    stream.ReadByte(); // skip first byte
    size_t read = 1;
    this->mission_data.flags.push_back(0);
    while (read < size-2) {
        int16_t flag = 0;
        stream.ReadByte(); // skip first byte
        flag = stream.ReadByte();
        this->mission_data.flags.push_back(flag);
        read += 2;
    }
}
void RSMission::parseMISN_CAST(uint8_t *data, size_t size) {
    size_t nbactor = size / 9;
    ByteStream stream(data, size);
    
    for (int i = 0; i < nbactor; i++) {
        CAST *tmpcast = new CAST();
        std::string actor = stream.ReadString(8);
        std::transform(actor.begin(), actor.end(), actor.begin(), ::toupper);
        
        tmpcast->actor = actor;
        tmpcast->profile = nullptr;
        this->mission_data.casting.push_back(tmpcast);
        stream.ReadByte();
    }
}
void RSMission::paseMissionScript(uint8_t *data, size_t size) {
    std::vector<PROG> *prog;
    prog = new std::vector<PROG>();
    for (int i = 0; i < size; i=i+2) {
        PROG tmp;
        tmp.opcode = data[i];
        tmp.arg = data[i+1];
        if (tmp.opcode == 0x00 && tmp.arg == 0x00) {
            //if (prog->size() > 0) {
                this->mission_data.prog.push_back(prog);
                prog = new std::vector<PROG>();
            //}
        } else {
            prog->push_back(tmp);
        }
    }
}
void RSMission::parseMISN_PROG(uint8_t *data, size_t size) {
    this->paseMissionScript(data, size);
}
void RSMission::parseMISN_PART(uint8_t *data, size_t size) {
    size_t numParts = size / 62;
    for (size_t i = 0; i < numParts; i++) {
        ByteStream stream(data + i * 62, 62);
        MISN_PART *prt = new MISN_PART();
        prt->id = (uint8_t) stream.ReadUShort();
        prt->member_name = stream.ReadString(8);
        std::transform(prt->member_name.begin(), prt->member_name.end(), prt->member_name.begin(), ::toupper);
        prt->member_name_destroyed = stream.ReadString(8);
        std::transform(prt->member_name_destroyed.begin(), prt->member_name_destroyed.end(), prt->member_name_destroyed.begin(), ::toupper);
        prt->weapon_load = stream.ReadString(8);
        std::transform(prt->weapon_load.begin(), prt->weapon_load.end(), prt->weapon_load.begin(), ::toupper);

        uint16_t area_index = stream.ReadUShort();
        prt->area_id = area_index >= 255 ? 255 : (uint8_t) area_index;
        prt->placement = stream.ReadByte() != 0 ? 1 : 0;
        float c0 = stream.ReadFixedFloatLE();
        float c1 = stream.ReadFixedFloatLE();
        float c2 = stream.ReadFixedFloatLE();
        prt->position = Vector3D(c0 * BLOCK_COORD_SCALE, c2 * HEIGH_MAP_SCALE, -c1 * BLOCK_COORD_SCALE);
        prt->azymuth = stream.ReadUShort();
        prt->relative_position = prt->position;
        prt->relative_azymuth = prt->azymuth;

        prt->speed = stream.ReadShort();
        prt->team = stream.ReadByte();
        prt->follow_ally = stream.ReadByte();
        // Octet 47 : pas recopié par le chargeur, rôle non établi.
        prt->unknown_byte_47 = stream.ReadByte();
        prt->formation_side = stream.ReadShort();
        prt->formation_ahead = stream.ReadShort();
        prt->formation_up = stream.ReadShort();

        for (int k = 0; k < 4; k++) {
            prt->progs_id.push_back(stream.ReadByte());
            stream.ReadByte();
        }
        prt->on_is_activated = prt->progs_id[0];
        prt->on_mission_update = prt->progs_id[1];
        prt->on_is_destroyed = prt->progs_id[2];
        prt->on_missions_init = prt->progs_id[3];
        prt->entity = nullptr;
        this->mission_data.parts.push_back(prt);
    }
}
void RSMission::parseMISN_TEAM(uint8_t *data, size_t size) {
    ByteStream stream(data, size);
    size_t read = 0;
    while (read < size) {
        uint16_t buffer = stream.ReadShort();
        read+=2;
        this->mission_data.team.push_back(buffer);
    }
}
void RSMission::parseMISN_PLAY(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;

    std::unordered_map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["SCNE"] = std::bind(&RSMission::parseMISN_PLAY_SCEN, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}
void RSMission::parseMISN_PLAY_SCEN(uint8_t *data, size_t size) {
    MISN_SCEN *scen = new MISN_SCEN();
    ByteStream stream(data, size);
    scen->is_active = stream.ReadByte();
    scen->area_id = stream.ReadShort();
    int16_t prog_id = 0;
    for (int i = 0 ; i<3; i++) {
        prog_id = stream.ReadShort();
        scen->progs_id.push_back(prog_id);
    }
    // Ordre du fichier : activation, à chaque image (Scene_RunUpdateScript_531F2), sortie de scène et fin de
    // mission (Scene_RunLeavingScript_53211).
    scen->on_is_activated = scen->progs_id[0];
    scen->on_mission_update = scen->progs_id[1];
    scen->on_leaving = scen->progs_id[2];
    scen->is_coord_on_area = stream.ReadByte();
    for (int i = 0; i < 14; i++) {
        scen->unknown_bytes.push_back(stream.ReadByte());
    }
    size_t read = 24;
    while (read < size) {
        scen->cast.push_back(stream.ReadByte());
        stream.ReadByte();
        read+=2;
    }
    this->mission_data.scenes.push_back(scen);
}
void RSMission::parseMISN_LOAD(uint8_t *data, size_t size) {
    size_t count = size / 9;
    for (size_t i = 0; i < count; i++) {
        ByteStream stream(data + i * 9, 9);
        MISN_LOAD *load = new MISN_LOAD();
        load->name = stream.ReadString(8);
        std::transform(load->name.begin(), load->name.end(), load->name.begin(), ::toupper);
        load->flag = stream.ReadByte();
        this->mission_data.load.push_back(load);
    }
}
void RSMission::parseMISN_CACH(uint8_t *data, size_t size) {
    size_t count = size / 10;
    for (size_t i = 0; i < count; i++) {
        ByteStream stream(data + i * 10, 10);
        MISN_CACH *cache = new MISN_CACH();
        cache->name = stream.ReadString(8);
        std::transform(cache->name.begin(), cache->name.end(), cache->name.begin(), ::toupper);
        cache->count = stream.ReadUShort();
        this->mission_data.cache.push_back(cache);
    }
}