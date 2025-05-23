#include "RSMission.h"
#include "block_def.h"

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
    std::map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["MISN"] = std::bind(&RSMission::parseMISN, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}

void RSMission::parseMISN(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;

    std::map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
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

    lexer.InitFromRAM(data, size, handlers);
}

void RSMission::parseMISN_VERS(uint8_t *data, size_t size) {
    ByteStream stream(data);
    this->mission_data.version = stream.ReadUShort();
}
void RSMission::parseMISN_INFO(uint8_t *data, size_t size) {
    ByteStream stream(data);
    if (size > 0) {
        this->mission_data.info = stream.ReadString(size);
    }
}
void RSMission::parseMISN_TUNE(uint8_t *data, size_t size) {
    ByteStream stream(data);
    this->mission_data.tune = stream.ReadByte();
}
void RSMission::parseMISN_NAME(uint8_t *data, size_t size) {
    ByteStream stream(data);
    for (int i = 0; i < size; i++) {
        this->mission_data.name.push_back(stream.ReadByte());
    }
    std::transform(this->mission_data.name.begin(), this->mission_data.name.end(), this->mission_data.name.begin(), ::toupper);
}
void RSMission::parseMISN_WRLD(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;

    std::map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["FILE"] = std::bind(&RSMission::parseMISN_WRLD_FILE, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}
void RSMission::parseMISN_WRLD_FILE(uint8_t *data, size_t size) {
    ByteStream stream(data);
    this->mission_data.world_filename = stream.ReadStringNoSize(size+1);
}
void RSMission::parseMISN_AREA(uint8_t *data, size_t size) {
    ByteStream stream(data);

    size_t read = 0;
    uint8_t buffer;
    int cpt = 0;
    while (read < size) {
        buffer = stream.ReadByte();
        AREA *tmparea = new AREA();

        tmparea->id = ++cpt;
        tmparea->AreaType = '\0';
        switch (buffer) {
        case 'S':
            tmparea->AreaType = 'S';
            for (int k = 0; k < 33; k++) {
                tmparea->AreaName[k] = stream.ReadByte();
            }
            missionstrtoupper(tmparea->AreaName);
            tmparea->position.x = stream.ReadInt24LE() * BLOCK_COORD_SCALE;
            tmparea->position.z = -stream.ReadInt24LE() * BLOCK_COORD_SCALE;
            tmparea->position.y = stream.ReadInt24LE() * HEIGH_MAP_SCALE;

            tmparea->AreaWidth = stream.ReadUShort();
            tmparea->unknown_bytes.push_back(stream.ReadByte());
            read += 49;
            break;
        case 'C':
            tmparea->AreaType = 'C';
            for (int k = 0; k < 33; k++) {
                tmparea->AreaName[k] = stream.ReadByte();
            }
            missionstrtoupper(tmparea->AreaName);
            tmparea->position.x = stream.ReadInt24LE() * BLOCK_COORD_SCALE;
            tmparea->position.z = -stream.ReadInt24LE() * BLOCK_COORD_SCALE;
            tmparea->position.y = stream.ReadInt24LE() * HEIGH_MAP_SCALE;

            
            tmparea->AreaWidth = stream.ReadUShort() * (uint16_t)(BLOCK_COORD_SCALE);

            // unsigned int Blank0; // off 48-49
            tmparea->unknown_bytes.push_back(stream.ReadByte());
            tmparea->unknown_bytes.push_back(stream.ReadByte());
            tmparea->AreaHeight = stream.ReadUShort() * (int) HEIGH_MAP_SCALE;
            // unsigned char Blank1; // off 52
            tmparea->unknown_bytes.push_back(stream.ReadByte());
            read += 52;
            break;
        case 'B':
            tmparea->AreaType = 'B';
            for (int k = 0; k < 33; k++) {
                tmparea->AreaName[k] = stream.ReadByte();
            }
            missionstrtoupper(tmparea->AreaName);
            tmparea->position.x = stream.ReadInt24LE() * BLOCK_COORD_SCALE;
            tmparea->position.z = -stream.ReadInt24LE() * BLOCK_COORD_SCALE;
            tmparea->position.y = stream.ReadInt24LE() * HEIGH_MAP_SCALE;

            tmparea->AreaWidth = stream.ReadUShort() * (uint16_t)(BLOCK_COORD_SCALE);

            // unsigned char Blank0[10]; // off 48-59
            for (int k = 0; k < 10; k++) {
                tmparea->unknown_bytes.push_back(stream.ReadByte());
            }
            // unsigned int AreaHeight; // off 60-61
            tmparea->AreaHeight = stream.ReadUShort()  * (int) HEIGH_MAP_SCALE;

            // unsigned char Unknown[5]; // off 62-67
            for (int k = 0; k < 5; k++) {
                tmparea->unknown_bytes.push_back(stream.ReadByte());
            }

            read += 67;
            break;
        default:
            read++;
            printf("ERROR IN PARSING AREA\n");
            break;
        }
        if (tmparea->AreaType != '\0') {
            this->mission_data.areas.push_back(tmparea);
        }
    }
}
int32_t ReadInt24LE_fromVec(std::vector<uint8_t>data, int offset) {
    int32_t i = 0;
    uint8_t buffer[4];
    buffer[0] = data[offset];
    buffer[1] = data[offset+1];
    buffer[2] = data[offset+2];
    buffer[3] = data[offset+3];
    i = (buffer[2] << 16) | (buffer[1] << 8) | (buffer[0] << 0);
    if (buffer[2] & 0x80) {
        i = (0xff << 24) | i;
    }
    return i;
}
void RSMission::parseMISN_SPOT(uint8_t *data, size_t size) {
    size_t numParts = size / 14;
    ByteStream stream(data);
    for (int i = 0; i < numParts; i++) {
        SPOT *spt = new SPOT();
        if (spt != NULL) {
            spt->id = i;
            spt->area_id = 0;
            spt->area_id |= stream.ReadByte() << 0;
            spt->area_id |= stream.ReadByte() << 8;

            spt->unknown1 = stream.ReadByte();
            int32_t x, z;
            int16_t y;
            x = stream.ReadInt24LE();
            z = stream.ReadInt24LE();
            y = 0;
            y |= stream.ReadByte() << 0;
            y |= stream.ReadByte() << 8;
            spt->position = Vector3D(x * BLOCK_COORD_SCALE, y * HEIGH_MAP_SCALE, -z * BLOCK_COORD_SCALE);
            
            spt->unknown2 = stream.ReadByte();
            this->mission_data.spots.push_back(spt);
        }
    }
}
void RSMission::parseMISN_NUMS(uint8_t *data, size_t size) {
    ByteStream stream(data);
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
    for (int i = 0; i < size; i++) {
        this->mission_data.flags.push_back(data[i]);
    }
}
void RSMission::parseMISN_CAST(uint8_t *data, size_t size) {
    size_t nbactor = size / 9;
    ByteStream stream(data);
    
    for (int i = 0; i < nbactor; i++) {
        CAST *tmpcast = new CAST();
        std::string actor = stream.ReadString(8);
        stream.ReadByte();
        tmpcast->actor = actor;
        tmpcast->profile = nullptr;
        this->mission_data.casting.push_back(tmpcast);
    }
}
void RSMission::parseMISN_PROG(uint8_t *data, size_t size) {
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
void RSMission::parseMISN_PART(uint8_t *data, size_t size) {
    printf("PARSING PART\n");
    printf("Number of entries %llu\n", size / 62);
    ByteStream stream(data);
    size_t numParts = size / 62;
    for (int i = 0; i < numParts; i++) {
        MISN_PART *prt = new MISN_PART();
        prt->id = 0;
        prt->id |= stream.ReadByte() << 0;
        prt->id |= stream.ReadByte() << 8;
        prt->member_name = stream.ReadString(8);
        std::transform(prt->member_name.begin(), prt->member_name.end(), prt->member_name.begin(), ::toupper);
        prt->member_name_destroyed = stream.ReadString(8);
        std::transform(prt->member_name_destroyed.begin(), prt->member_name_destroyed.end(), prt->member_name_destroyed.begin(), ::toupper);
        prt->weapon_load = stream.ReadString(8);
        std::transform(prt->weapon_load.begin(), prt->weapon_load.end(), prt->weapon_load.begin(), ::toupper);
        
        prt->area_id = stream.ReadByte();
        prt->unknown1 = stream.ReadByte();
        prt->unknown2 = 0;
        prt->unknown2 |= stream.ReadByte() << 0;
        prt->unknown2 |= stream.ReadByte() << 8;
        int32_t x, z;
        int16_t y;
        x = stream.ReadInt24LE();
        z = stream.ReadInt24LE();
        y = 0;
        y |= stream.ReadByte() << 0;
        y |= stream.ReadByte() << 8;
        prt->position = Vector3D(x * BLOCK_COORD_SCALE, y * HEIGH_MAP_SCALE, -z * BLOCK_COORD_SCALE);

        prt->unknown3 = stream.ReadByte();
        prt->azymuth = 0;
        prt->azymuth |= stream.ReadByte() << 0;
        prt->azymuth |= stream.ReadByte() << 8;
        for (int k = 0; k < 11; k++) {
            prt->unknown_bytes.push_back(stream.ReadByte());
        }
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
    ByteStream stream(data);
    size_t read = 0;
    while (read < size) {
        uint16_t buffer = stream.ReadShort();
        read+=2;
        this->mission_data.team.push_back(buffer);
    }
}
void RSMission::parseMISN_PLAY(uint8_t *data, size_t size) {
    IFFSaxLexer lexer;

    std::map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["SCNE"] = std::bind(&RSMission::parseMISN_PLAY_SCEN, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}
void RSMission::parseMISN_PLAY_SCEN(uint8_t *data, size_t size) {
    MISN_SCEN *scen = new MISN_SCEN();
    ByteStream stream(data);
    scen->is_active = stream.ReadByte();
    scen->area_id = stream.ReadShort();
    int16_t prog_id = 0;
    for (int i = 0 ; i<3; i++) {
        prog_id = stream.ReadShort();
        scen->progs_id.push_back(prog_id);
    }
    scen->on_is_activated = scen->progs_id[0];
    scen->on_leaving = scen->progs_id[1];
    scen->on_mission_update = scen->progs_id[2];
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
    for (int i = 0; i < size; i++) {
        this->mission_data.load.push_back(data[i]);
    }
}