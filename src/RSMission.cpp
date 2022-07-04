//
//  RSMission.cpp
//  libRealSpace
//
//  Created by Rémi LEONARD on 04/07/2022.
//  Copyright © 2022 Fabien Sanglard. All rights reserved.
//

#include "precomp.h"
#include "RSMission.h"

RSMission::RSMission() {
    printf("CREATE MISSION\n");
}

RSMission::~RSMission() {
    printf("DELETE MISSION\n");
}

void RSMission::PrintMissionInfos() {
    printf("MISSION : %s\n", missionName);
    printf("IN AREA : %s\n", missionAreaFile);
    printf("INFOS : %s\n", missionInfo);
    printf("MSGS : \n");
    for (int i = 0; i < missionMessages.size(); i++) {
        printf("\tM[%d]: %s\n",missionMessages[i]->id, missionMessages[i]->message);
    }
    printf("CASTING : \n");
    for (int i = 0; i < missionCasting.size(); i++) {
        printf("\tCAST[%d]: %s\n", missionCasting[i]->id, missionCasting[i]->actor);
    }
    
    printf("MISSION SPOTS : \n");
    for (int i = 0; i < missionSpots.size(); i++) {
        printf("SPOT[%d] at {%d,%d,%d}, UN(%d)\n",
            missionSpots[i]->id,
            missionSpots[i]->XAxis,
            missionSpots[i]->YAxis,
            missionSpots[i]->ZAxis,
            missionSpots[i]->unknown
        );
    }
}
void RSMission::InitFromName(char *name) {
    TreArchive treArchive;
    treArchive.InitFromFile("MISSIONS.TRE");
    char *path = "..\\..\\DATA\\MISSIONS\\";
    char finalPath[255];
    sprintf(finalPath, "%s%s", path, name);
    TreEntry* mission = treArchive.GetEntryByName(finalPath);
    //TreEntry* mission = tres[TRE_MISSIONS]->GetEntryByName("..\\..\\DATA\\MISSIONS\\MISN-1A.IFF");
    
    IffLexer missionIFF;
    missionIFF.InitFromRAM(mission->data, mission->size);
    InitFromIFF(&missionIFF);
}
void RSMission::InitFromIFF(IffLexer* lexer) {
    IffChunk* chunk;
    printf("PARSING MISSION DATA\n");

    chunk = lexer->GetChunkByID('WRLD');
    parseWORLD(chunk);

    chunk = lexer->GetChunkByID('INFO');
    parseINFO(chunk);

    chunk = lexer->GetChunkByID('PALT');
    parsePALT(chunk);

    chunk = lexer->GetChunkByID('TERA');
    parseTERA(chunk);

    chunk = lexer->GetChunkByID('SKYS');
    parseSKYS(chunk);

    chunk = lexer->GetChunkByID('GLNT');
    parseGLNT(chunk);

    chunk = lexer->GetChunkByID('SMOK');
    parseSMOK(chunk);

    chunk = lexer->GetChunkByID('LGHT');
    parseLGHT(chunk);

    chunk = lexer->GetChunkByID('VERS');
    

    chunk = lexer->GetChunkByID('NAME');
    parseNAME(chunk);

    chunk = lexer->GetChunkByID('FILE');
    parseFILE(chunk);

    chunk = lexer->GetChunkByID('AREA');
    parseAREA(chunk);

    chunk = lexer->GetChunkByID('SPOT');
    parseSPOT(chunk);

    chunk = lexer->GetChunkByID('NUMS');
    

    chunk = lexer->GetChunkByID('MSGS');
    parseMSGS(chunk);
    
    chunk = lexer->GetChunkByID('FLAG');


    chunk = lexer->GetChunkByID('CAST');
    parseCAST(chunk);

    chunk = lexer->GetChunkByID('PROG');
    parsePROG(chunk);
    
    chunk = lexer->GetChunkByID('PART');
    parsePART(chunk);

    chunk = lexer->GetChunkByID('TEAM');


    chunk = lexer->GetChunkByID('LOAD');


    chunk = lexer->GetChunkByID('SCNE');

    
    
}
void RSMission::parseLOAD(IffChunk* chunk) {

}
void RSMission::parseCAST(IffChunk* chunk) {
    if (chunk != NULL) {
        CAST* cst;
        size_t fsize = chunk->size;
        size_t nbactor = fsize / 9;
        ByteStream stream(chunk->data);
        for (int i = 0; i < nbactor; i++) {
            cst = (CAST*)malloc(sizeof(CAST));
            if (cst != NULL) {
                cst->id = i;
                for (int k = 0; k < 8; k++) {
                    cst->actor[k] = stream.ReadByte();
                }
                cst->actor[8] = '\0';
                stream.ReadByte();
                missionCasting.push_back(cst);
            }
        }
    }
}
void RSMission::parseMSGS(IffChunk* chunk) {
    if (chunk != NULL) {
        size_t fsize = chunk->size;
        size_t read = 0;
        size_t strc = 0;
        size_t msgc = 0;
        ByteStream stream(chunk->data);
        MSGS* scmsg = NULL;
        size_t r;
        while (read < fsize) {
            r = stream.ReadByte();
            read++;
            if (r != '\0') {
                if (scmsg == NULL) {
                    scmsg = (MSGS*)malloc(sizeof(MSGS));
                    if (scmsg == NULL) {
                        read = fsize + 1;
                    }
                    else {
                        strc = 0;
                        scmsg->id = msgc++;
                    }
                }
                if (scmsg != NULL) {
                    scmsg->message[strc++] = r;
                }
            }
            else {
                if (scmsg != NULL) {
                    scmsg->message[strc++] = '\0';
                    missionMessages.push_back(scmsg);
                }
                scmsg = NULL;
            }
        }
    }
}

void RSMission::parseWORLD(IffChunk* chunk) {
    
}

void RSMission::parseINFO(IffChunk* chunk) {
    size_t fsize = chunk->size;
    ByteStream stream(chunk->data);
    for (int i = 0; i < fsize; i++) {
        missionInfo[i] = stream.ReadByte();
    }
    missionInfo[fsize] = '\0';
}
void RSMission::parseNAME(IffChunk* chunk) {
    if (chunk != NULL) {
        size_t fsize = chunk->size;
        ByteStream stream(chunk->data);
        for (int i = 0; i < fsize; i++) {
            missionName[i] = stream.ReadByte();
        }
        missionName[fsize] = '\0';
    }
}
void RSMission::parseFILE(IffChunk* chunk) {
    char areaName[9 + 4];
    
    if (chunk != NULL) {
        size_t fsize = chunk->size;
        ByteStream stream(chunk->data);
        for (int i = 0; i < fsize; i++) {
            missionAreaFile[i] = stream.ReadByte();
        }
        missionAreaFile[fsize] = '\0';
        for (int i = 0; i < strlen(missionAreaFile); i++) {
            areaName[i] = toupper(missionAreaFile[i]);
        }
        sprintf(areaName, "%s.PAK", areaName);
        area.InitFromPAKFileName(areaName);
    }
}
void RSMission::parsePALT(IffChunk* chunk) {
    
}

void RSMission::parseTERA(IffChunk* chunk) {
    
}

void RSMission::parseSKYS(IffChunk* chunk) {
    
}

void RSMission::parseAREA(IffChunk* chunk) {
    if (chunk != NULL) {
        printf("PARSING AREA\n");

        ByteStream stream(chunk->data);
        size_t fsize = chunk->size;
        size_t read = 0;
        uint8_t buffer;
        int cpt = 0;
        while (read < fsize) {
            buffer = stream.ReadByte();
            MissionArea* marea;
            marea = (MissionArea*)malloc(sizeof(MissionArea));
            if (marea != NULL) {
                marea->id = ++cpt;
                switch (buffer) {
                case 'S':
                    marea->AreaType = 'S';
                    for (int k = 0; k < 33; k++) {
                        marea->AreaName[k] = stream.ReadByte();
                    }
                    marea->XAxis = stream.ReadInt24LE();
                    marea->YAxis = stream.ReadInt24LE();
                    marea->ZAxis = stream.ReadInt24LE();
                    marea->AreaWidth = stream.ReadUShort();
                    stream.ReadByte();
                    read += 49;
                    break;
                case 'C':
                    marea->AreaType = 'C';
                    for (int k = 0; k < 33; k++) {
                        marea->AreaName[k] = stream.ReadByte();
                    }
                    marea->XAxis = stream.ReadInt24LE();
                    marea->YAxis = stream.ReadInt24LE();
                    marea->ZAxis = stream.ReadInt24LE();
                    marea->AreaWidth = stream.ReadUShort();

                    //unsigned int Blank0; // off 48-49
                    stream.ReadByte();
                    stream.ReadByte();
                    marea->AreaHeight = stream.ReadUShort();
                    //unsigned char Blank1; // off 52
                    stream.ReadByte();
                    read += 52;
                    break;
                case 'B':
                    marea->AreaType = 'B';
                    for (int k = 0; k < 33; k++) {
                        marea->AreaName[k] = stream.ReadByte();
                    }
                    marea->XAxis = stream.ReadInt24LE();
                    marea->YAxis = stream.ReadInt24LE();
                    marea->ZAxis = stream.ReadInt24LE();
                    marea->AreaWidth = stream.ReadUShort();

                    //unsigned char Blank0[10]; // off 48-59
                    for (int k = 0; k < 10; k++) {
                        stream.ReadByte();
                    }
                    //unsigned int AreaHeight; // off 60-61
                    marea->AreaHeight = stream.ReadUShort();

                    //unsigned char Unknown[5]; // off 62-67
                    for (int k = 0; k < 5; k++) {
                        stream.ReadByte();
                    }

                    read += 67;
                    break;
                }
                //missionAreas.push_back(area);
                area.missionAreas.push_back(marea);
            }
            else {
                read = fsize + 1;
                // exit the loop, memory error
            }
        }
    }
}

void RSMission::parsePART(IffChunk* chunk) {
    if (chunk != NULL) {
        printf("PARSING PART\n");
        printf("Number of entries %d\n", chunk->size / 62);
        ByteStream stream(chunk->data);
        size_t numParts = chunk->size / 62;
        for (int i = 0; i < numParts; i++) {
            MapObject *prt;
            prt = (MapObject *)malloc(sizeof(MapObject));
            if (prt != NULL) {
                prt->MemberNumber = 0;
                prt->MemberNumber |= stream.ReadByte() << 0;
                prt->MemberNumber |= stream.ReadByte() << 8;
                for (int k = 0; k < 16; k++) {
                    prt->MemberName[k] = stream.ReadByte();
                }
                for (int k = 0; k < 8; k++) {
                    prt->WeaponLoad[k] = stream.ReadByte();
                }
                prt->Unknown0 = 0;
                prt->Unknown0 |= stream.ReadByte() << 0;
                prt->Unknown0 |= stream.ReadByte() << 8;

                prt->Unknown1 = 0;
                prt->Unknown1 |= stream.ReadByte() << 0;
                prt->Unknown1 |= stream.ReadByte() << 8;

                prt->position[0] = stream.ReadInt24LE();
                prt->position[2] = stream.ReadInt24LE();
                prt->position[1] = 0;
                prt->position[1] |= stream.ReadByte() << 0;
                prt->position[1] |= stream.ReadByte() << 8;

                for (int k = 0; k < 22; k++) {
                    prt->Controls[k] = stream.ReadByte();
                }

                if (prt->Unknown0 != 65535) {
                    for (int k = 0; k < area.missionAreas.size(); k++) {
                        if (area.missionAreas[k]->id-1 == prt->Unknown0) {
                            prt->position[0] += area.missionAreas[k]->XAxis;
                            prt->position[2] += area.missionAreas[k]->YAxis;
                            prt->position[1] += area.missionAreas[k]->ZAxis;
                            prt->Unknown0 = 65535;
                        }
                    }
                }
                if (strlen(prt->MemberName)>0) {
                    area.AddObject(prt);
                }
            }
        }
    }
}
void RSMission::parseGLNT(IffChunk* chunk) {
    
    if (chunk != NULL) {
        PakArchive p;
        p.InitFromRAM("GLNT", chunk->data, chunk->size);
        p.List(stdout);
        for (int i = 0; i < p.GetNumEntries(); i++) {
            PakEntry* blockEntry = p.GetEntry(i);
            printf("CONTENT OF GLNT PAK ENTRY %d\n", i);
            ByteStream stream(blockEntry->data);
            size_t fsize = blockEntry->size;
            uint8_t byte;
            int cl = 0;
            for (int read = 0; read < fsize; read++) {
                byte = stream.ReadByte();
                if (byte >= 40 && byte <= 90) {
                    printf("[%c]", char(byte));
                }
                else if (byte >= 97 && byte <= 122) {
                    printf("[%c]", char(byte));
                }
                else {
                    printf("[0x%X]", byte);
                }
                if (cl > 2) {
                    printf("\n");
                    cl = 0;
                }
                else {
                    printf("\t");
                    cl++;
                }

            }
            printf("\n");
        }
    }
}

void RSMission::parseSMOK(IffChunk* chunk) {
    
}

void RSMission::parseLGHT(IffChunk* chunk) {
    
}

void RSMission::parseSPOT(IffChunk* chunk) {
    if (chunk != NULL) {
        size_t numParts = chunk->size / 14;
        ByteStream stream(chunk->data);

        for (int i = 0; i < numParts; i++) {
            SPOT* spt;
            spt = (SPOT*)malloc(sizeof(SPOT));
            if (spt != NULL) {
                spt->id = i;
                spt->unknown = 0;
                spt->unknown |= stream.ReadByte() << 0;
                spt->unknown |= stream.ReadByte() << 8;

                stream.ReadByte();
                spt->XAxis = stream.ReadInt24LE();
                spt->YAxis = stream.ReadInt24LE();
                spt->ZAxis = stream.ReadShort();
                stream.ReadByte();
                missionSpots.push_back(spt);
            }
        }
    }
}

void RSMission::parsePROG(IffChunk* chunk) {
    
}
