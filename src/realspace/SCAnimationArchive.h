//
//  SCAnimationArchive.h
//  libRealSpace
//
//  Created on 2/10/2025.
//

#pragma once
#include <stdint.h>
#include <vector>
#include <string>
#include <unordered_map>

#include "../commons/IFFSaxLexer.h"
#include "../commons/IFFWriter.h"
#include "PakArchive.h"

typedef struct MIDGAME_SHOT_BG {
    PakArchive *pak;
    RSImageSet *image;
    PakArchive *pak_palette{nullptr};
    uint8_t palette;
    RSPalette *pal;
    Point2D position_start;
    Point2D position_end;
    Point2D velocity;
    int shapeid;
    int pak_entry_id;
    bool use_external_palette{false};
} MIDGAME_SHOT_BG;

typedef struct MIDGAME_SHOT_CHARACTER {
    std::string character_name;
    std::string background_name;
    Point2D position_start;
    Point2D position_end;
    Point2D velocity;
    RSImageSet *image;
    RSPalette *pal;
    uint8_t palette;
    uint8_t cloth_id;
    uint8_t head_id;
    uint8_t expression_id;
    bool talking{false};
} MIDGAME_SHOT_CHARACTER;

typedef struct MIDGAME_SHAPE_DATA {
    PakArchive *pak;
    uint8_t shape_id;
    uint8_t sub_shape_id;
    uint8_t pal_id;
    Point2D start;
    Point2D end;
    Point2D velocity;
    uint8_t keep_first_frame;
} MIDGAME_SHAPE_DATA;

typedef struct MIDGAME_SHOT_SPRITE {
    PakArchive *pak;
    int pak_entry_id;
    RSImageSet *image;
    RSPalette *pal;
    PakArchive *pak_palette{nullptr};
    uint8_t palette{0};
    Point2D position_start;
    Point2D position_end;
    Point2D velocity;
    int shapeid;
    uint8_t keep_first_frame;
    bool use_external_palette{false};
} MIDGAME_SHOT_SPRITE;

typedef struct MIDGAME_SOUND {
    uint8_t *data;
    size_t size;
    
} MIDGAME_SOUND;

typedef struct MIDGAME_SHOT {
    std::vector<MIDGAME_SHOT_BG *>background;
    std::vector<MIDGAME_SHOT_SPRITE *>sprites;
    std::vector<MIDGAME_SHOT_BG *>foreground;
    std::vector<MIDGAME_SHOT_CHARACTER *>characters;
    int nbframe{0};
    int music{255};
    MIDGAME_SOUND *sound{nullptr};
    int sound_time_code{0};
} MIDGAME_SHOT;

typedef struct MIDGAME_DATA_SHOT {
    std::vector<MIDGAME_SHAPE_DATA> background;
    std::vector<MIDGAME_SHAPE_DATA> forground;
    std::vector<MIDGAME_SHAPE_DATA> sprites;
    int nbframe;
    uint8_t music{255};
    PakEntry *sound{nullptr};
    int sound_time_code{0};
} MIDGAME_DATA_SHOT;

typedef struct MIDGAME_DATA {
    std::vector<MIDGAME_DATA_SHOT> shots;
} MIDGAME_DATA;

class SCAnimationArchive {
public:
    SCAnimationArchive();
    ~SCAnimationArchive();

    // Enregistrer une animation dans un fichier IFF
    bool SaveToFile(const char* filename, const std::vector<MIDGAME_SHOT*>& shots);
    
    // Enregistrer une animation en mémoire
    bool SaveToMemory(uint8_t** data, size_t* size, const std::vector<MIDGAME_SHOT*>& shots);

    // Charger une animation depuis un fichier IFF
    bool LoadFromFile(const char* filename, std::vector<MIDGAME_SHOT*>& shots);
    
    // Charger une animation depuis la mémoire
    bool LoadFromRAM(uint8_t* data, size_t size, std::vector<MIDGAME_SHOT*>& shots);
    std::vector<MIDGAME_SHOT*>* shots{nullptr};
private:
    // Structure pour stocker les références aux PakArchive
    struct PakArchiveRef {
        std::string name;
        PakArchive* archive;
    };

    // Liste des archives utilisées lors du chargement
    std::vector<PakArchiveRef> pakArchives;

    // Variables temporaires utilisées pendant le chargement
    MIDGAME_SHOT* currentShot;
    MIDGAME_SHOT_BG* currentBg;
    MIDGAME_SHOT_SPRITE* currentSprite;

    // Méthodes pour l'écriture
    void WriteAnimation(IFFWriter& writer, const std::vector<MIDGAME_SHOT*>& shots);
    void WriteShot(IFFWriter& writer, const MIDGAME_SHOT* shot);
    void WriteBackgrounds(IFFWriter& writer, const std::vector<MIDGAME_SHOT_BG*>& backgrounds);
    void WriteBackground(IFFWriter& writer, const MIDGAME_SHOT_BG* bg);
    void WriteSprites(IFFWriter& writer, const std::vector<MIDGAME_SHOT_SPRITE*>& sprites);
    void WriteSprite(IFFWriter& writer, const MIDGAME_SHOT_SPRITE* sprite);
    void WriteForegrounds(IFFWriter& writer, const std::vector<MIDGAME_SHOT_BG*>& foregrounds);
    void WriteSound(IFFWriter& writer, const MIDGAME_SOUND* sound);

    // Handler pour l'IFFSaxLexer
    void HandleChunks(uint8_t* data, size_t size);
    void HandleANIM(uint8_t* data, size_t size);
    void HandleSHOT(uint8_t* data, size_t size);
    void HandleSHTP(uint8_t* data, size_t size);
    void HandleBGND(uint8_t* data, size_t size);
    void HandleBKGD(uint8_t* data, size_t size);
    void HandleSPRT(uint8_t* data, size_t size);
    void HandleSPR(uint8_t* data, size_t size);
    void HandleFGND(uint8_t* data, size_t size);
    void HandleFRGD(uint8_t* data, size_t size);
    void HandleSOND(uint8_t* data, size_t size);

    // Utilitaires
    PakArchive* FindOrLoadPakArchive(const std::string& name);
};