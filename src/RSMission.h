//
//  RSMission.h
//  libRealSpace
//
//  Created by Rémi LEONARD on 04/07/2022.
//  Copyright © 2022 Fabien Sanglard. All rights reserved.
//

#ifndef __libRealSpace__RSMission__
#define __libRealSpace__RSMission__

typedef struct SPOT {
    int id;
    short unknown;

    long XAxis;
    long YAxis;
    long ZAxis;
} SPOT;
typedef struct MSGS {
    char message[255];
    size_t id;
} MSGS;
typedef struct CAST {
    char actor[9];
    size_t id;
} CAST;


class RSMission {
public:
    RSMission();
    ~RSMission();

    inline char* getMissionName() {
        return (missionName);
    }
    inline char* getMissionAreaFile() {
        return (missionAreaFile);
    }
    TreArchive* tre;
    RSArea area;
    void InitFromIFF(IffLexer* lexer);
    void InitFromName(char *name);
    void PrintMissionInfos();
private:

    void parseLOAD(IffChunk* chunk);
    void parseCAST(IffChunk* chunk);
    void parseMSGS(IffChunk* chunk);
    void parseFILE(IffChunk* chunk);
    void parseNAME(IffChunk* chunk);
    void parseWORLD(IffChunk* chunk);
    void parseINFO(IffChunk* chunk);
    void parsePALT(IffChunk* chunk);
    void parseTERA(IffChunk* chunk);
    void parseSKYS(IffChunk* chunk);
    void parseGLNT(IffChunk* chunk);
    void parseSMOK(IffChunk* chunk);
    void parseLGHT(IffChunk* chunk);
    void parseAREA(IffChunk* chunk);
    void parsePART(IffChunk* chunk);
    void parseSPOT(IffChunk* chunk);
    void parsePROG(IffChunk* chunk);

    char missionInfo[255];
    char missionName[255];
    char missionAreaFile[8];

    
    std::vector<MSGS*> missionMessages;
    std::vector<SPOT*> missionSpots;
    std::vector<CAST*> missionCasting;
};
#endif /* defined(__libRealSpace__RSMission__) */
