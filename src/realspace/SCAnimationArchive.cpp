//
//  SCAnimationArchive.cpp
//  libRealSpace
//
//  Created on 2/10/2025.
//

#include "precomp.h"
#include "SCAnimationArchive.h"
#include "../commons/ByteStream.h"
#include "AssetManager.h"

SCAnimationArchive::SCAnimationArchive() {
}

SCAnimationArchive::~SCAnimationArchive() {
}

bool SCAnimationArchive::SaveToFile(const char* filename, const std::vector<MIDGAME_SHOT*>& shots) {
    IFFWriter writer;
    writer.StartIFF("ANIM");
    WriteAnimation(writer, shots);
    return writer.SaveToFile(filename);
}

bool SCAnimationArchive::SaveToMemory(uint8_t** data, size_t* size, const std::vector<MIDGAME_SHOT*>& shots) {
    IFFWriter writer;
    writer.StartIFF("ANIM");
    WriteAnimation(writer, shots);
    return writer.SaveToMemory(data, size);
}

bool SCAnimationArchive::LoadFromFile(const char* filename, std::vector<MIDGAME_SHOT*>& shots) {
    IFFSaxLexer lexer;
    this->shots = &shots;
    std::unordered_map<std::string, std::function<void(uint8_t*, size_t)>> events;
    events["ANIM"] = std::bind(&SCAnimationArchive::HandleANIM, this, std::placeholders::_1, std::placeholders::_2);
    return lexer.InitFromFile(filename, events);
}

bool SCAnimationArchive::LoadFromRAM(uint8_t* data, size_t size, std::vector<MIDGAME_SHOT*>& shots) {
    IFFSaxLexer lexer;
    
    std::unordered_map<std::string, std::function<void(uint8_t*, size_t)>> events;
    events["ANIM"] = std::bind(&SCAnimationArchive::HandleANIM, this, std::placeholders::_1, std::placeholders::_2);
    
    return lexer.InitFromRAM(data, size, events);
}

void SCAnimationArchive::WriteAnimation(IFFWriter& writer, const std::vector<MIDGAME_SHOT*>& shots) {
    writer.StartChunk("HEAD");
    // En-tête - version du format, nombre de shots, etc.
    writer.WriteUint32(shots.size()); // Nombre de shots
    writer.EndChunk();
    
    for (const MIDGAME_SHOT* shot : shots) {
        WriteShot(writer, shot);
    }
}

void SCAnimationArchive::WriteShot(IFFWriter& writer, const MIDGAME_SHOT* shot) {
    writer.StartChunk("SHOT");
    // Propriétés du shot
    writer.StartChunk("SHTP");
    writer.WriteInt32(shot->nbframe);
    writer.WriteUint8(shot->music);
    writer.WriteUint16(shot->sound_time_code);
    writer.EndChunk();
    
    // Arrière-plans
    if (!shot->background.empty()) {
        WriteBackgrounds(writer, shot->background);
    }
    
    // Sprites
    if (!shot->sprites.empty()) {
        WriteSprites(writer, shot->sprites);
    }
    
    // Premier-plans
    if (!shot->foreground.empty()) {
        WriteForegrounds(writer, shot->foreground);
    }
    
    // Personnages
    if (!shot->characters.empty()) {
        WriteCharacters(writer, shot->characters);
    }
    
    if (shot->sound_pak != nullptr && shot->sound_pak_entry_id >= 0) {
        writer.StartChunk("SOND");
        std::string archiveName = shot->sound_pak->GetName();
        if (archiveName.length() < 4 || archiveName.compare(archiveName.length() - 4, 4, ".PAK") != 0) {
            archiveName += ".PAK";
        }
        AssetManager& Assets = AssetManager::getInstance();
        for (auto treRef: Assets.treEntries) {
            if (treRef.first.length() >= archiveName.length() &&
                treRef.first.compare(treRef.first.length() - archiveName.length(), archiveName.length(), archiveName) == 0) {
                archiveName = treRef.first;
                break;
            }
        }
        writer.WriteUint16(archiveName.length());
        writer.WriteString(archiveName.c_str(), archiveName.length());
        writer.WriteUint16(shot->sound_pak_entry_id);
        writer.EndChunk();
    }
    writer.EndChunk();
}

// Ajouter cette nouvelle fonction pour écrire les personnages
void SCAnimationArchive::WriteCharacters(IFFWriter& writer, const std::vector<MIDGAME_SHOT_CHARACTER*>& characters) {
    writer.StartChunk("CHAR");
    for (const MIDGAME_SHOT_CHARACTER* character : characters) {
        writer.StartChunk("CHRC");
        
        // Écriture du nom du personnage
        writer.WriteUint16(character->character_name.length());
        writer.WriteString(character->character_name.c_str(), character->character_name.length());
        
        // Écriture du nom du fond
        writer.WriteUint16(character->background_name.length());
        writer.WriteString(character->background_name.c_str(), character->background_name.length());
        
        // Positions et vélocité
        writer.WriteInt16(character->position_start.x);
        writer.WriteInt16(character->position_start.y);
        writer.WriteInt16(character->position_end.x);
        writer.WriteInt16(character->position_end.y);
        writer.WriteInt16(character->velocity.x);
        writer.WriteInt16(character->velocity.y);
        
        // IDs des éléments du personnage
        writer.WriteUint8(character->palette);
        writer.WriteUint8(character->cloth_id);
        writer.WriteUint8(character->head_id);
        writer.WriteUint8(character->expression_id);
        writer.WriteUint8(character->talking ? 1 : 0);
        
        writer.EndChunk();
    }
    writer.EndChunk();
}

void SCAnimationArchive::WriteBackgrounds(IFFWriter& writer, const std::vector<MIDGAME_SHOT_BG*>& backgrounds) {
    writer.StartChunk("BGND");
    for (const MIDGAME_SHOT_BG* bg : backgrounds) {
        WriteBackground(writer, bg);
    }
    writer.EndChunk();
}

void SCAnimationArchive::WriteBackground(IFFWriter& writer, const MIDGAME_SHOT_BG* bg) {
    writer.StartChunk("BKGD");
    
    AssetManager& Assets = AssetManager::getInstance();

    std::string archiveName = bg->pak->GetName();
    if (archiveName.length() > 0) {
        if (archiveName.length() < 4 || archiveName.compare(archiveName.length() - 4, 4, ".PAK") != 0) {
            archiveName += ".PAK";
        }
        for (auto treRef: Assets.treEntries) {
            if (treRef.first.length() >= archiveName.length() &&
                treRef.first.compare(treRef.first.length() - archiveName.length(), archiveName.length(), archiveName) == 0) {
                archiveName = treRef.first;
                break;
            }
        }
    }

    std::string paletteArchiveName = bg->pak_palette ? bg->pak_palette->GetName() : "";
    if (paletteArchiveName.length() > 0) {    
        if (paletteArchiveName.length() < 4 || paletteArchiveName.compare(paletteArchiveName.length() - 4, 4, ".PAK") != 0) {
            paletteArchiveName += ".PAK";
        }
        for (auto treRef: Assets.treEntries) {
            if (treRef.first.length() >= paletteArchiveName.length() &&
                treRef.first.compare(treRef.first.length() - paletteArchiveName.length(), paletteArchiveName.length(), paletteArchiveName) == 0) {
                paletteArchiveName = treRef.first;
                break;
            }
        }
    }
    
    writer.WriteUint16(archiveName.length());
    writer.WriteString(archiveName.c_str(), archiveName.length());
    writer.WriteUint16(paletteArchiveName.length());
    writer.WriteString(paletteArchiveName.c_str(), paletteArchiveName.length());
    writer.WriteUint16(bg->pak_entry_id);
    writer.WriteUint16(bg->palette);
    writer.WriteUint16(bg->shapeid);
    writer.WriteInt8(bg->use_external_palette ? 1 : 0);
    // Positions et vélocité
    writer.WriteInt16(bg->position_start.x);
    writer.WriteInt16(bg->position_start.y);
    writer.WriteInt16(bg->position_end.x);
    writer.WriteInt16(bg->position_end.y);
    writer.WriteInt16(bg->velocity.x);
    writer.WriteInt16(bg->velocity.y);
    
    writer.EndChunk();
}

void SCAnimationArchive::WriteSprites(IFFWriter& writer, const std::vector<MIDGAME_SHOT_SPRITE*>& sprites) {
    writer.StartChunk("SPRT");
    for (const MIDGAME_SHOT_SPRITE* sprite : sprites) {
        WriteSprite(writer, sprite);
    }
    writer.EndChunk();
}

void SCAnimationArchive::WriteSprite(IFFWriter& writer, const MIDGAME_SHOT_SPRITE* sprite) {
    if (sprite->pak == nullptr) {
        return;
    }
    writer.StartChunk("SPRI");
    std::string archiveName = sprite->pak->GetName();
    if (archiveName.length() < 4 || archiveName.compare(archiveName.length() - 4, 4, ".PAK") != 0) {
        archiveName += ".PAK";
    }
    
    std::string paletteArchiveName = sprite->pak_palette ? sprite->pak_palette->GetName() : "";
    if (paletteArchiveName.length() < 4 || paletteArchiveName.compare(paletteArchiveName.length() - 4, 4, ".PAK") != 0) {
        paletteArchiveName += ".PAK";
    }
    AssetManager& Assets = AssetManager::getInstance();
    for (auto treRef: Assets.treEntries) {
        if (treRef.first.length() >= archiveName.length() &&
            treRef.first.compare(treRef.first.length() - archiveName.length(), archiveName.length(), archiveName) == 0) {
            // archiveName is a suffix of treRef.first
            archiveName = treRef.first;
            break;
        }
    }
    if (paletteArchiveName.length() > 0) {    
        for (auto treRef: Assets.treEntries) {
            if (treRef.first.length() >= paletteArchiveName.length() &&
                treRef.first.compare(treRef.first.length() - paletteArchiveName.length(), paletteArchiveName.length(), paletteArchiveName) == 0) {
                paletteArchiveName = treRef.first;
                break;
            }
        }
    }
    writer.WriteUint16(archiveName.length());
    writer.WriteString(archiveName.c_str(), archiveName.length());
    
    writer.WriteUint16(paletteArchiveName.length());
    writer.WriteString(paletteArchiveName.c_str(), paletteArchiveName.length());

    writer.WriteUint16(sprite->pak_entry_id);
    writer.WriteUint16(sprite->shapeid);
    writer.WriteUint16(sprite->palette);
    writer.WriteUint8(sprite->use_external_palette ? 1 : 0);

    writer.WriteInt16(sprite->position_start.x);
    writer.WriteInt16(sprite->position_start.y);
    writer.WriteInt16(sprite->position_end.x);
    writer.WriteInt16(sprite->position_end.y);
    writer.WriteInt16(sprite->velocity.x);
    writer.WriteInt16(sprite->velocity.y);
    writer.WriteUint8(sprite->keep_first_frame);
    
    writer.EndChunk();
}

void SCAnimationArchive::WriteForegrounds(IFFWriter& writer, const std::vector<MIDGAME_SHOT_BG*>& foregrounds) {
    writer.StartChunk("FGND");
    for (const MIDGAME_SHOT_BG* fg : foregrounds) {
        writer.StartChunk("FRGD");
        
        std::string archiveName = fg->pak->GetName();
        if (archiveName.length() < 4 || archiveName.compare(archiveName.length() - 4, 4, ".PAK") != 0) {
            archiveName += ".PAK";
        }
        
        std::string paletteArchiveName = fg->pak_palette ? fg->pak_palette->GetName() : "";
        if (paletteArchiveName.length() < 4 || paletteArchiveName.compare(paletteArchiveName.length() - 4, 4, ".PAK") != 0) {
            paletteArchiveName += ".PAK";
        }
        AssetManager& Assets = AssetManager::getInstance();
        for (auto treRef: Assets.treEntries) {
            if (treRef.first.length() >= archiveName.length() &&
                treRef.first.compare(treRef.first.length() - archiveName.length(), archiveName.length(), archiveName) == 0) {
                archiveName = treRef.first;
                break;
            }
        }
        if (paletteArchiveName.length() > 0) {    
            for (auto treRef: Assets.treEntries) {
                if (treRef.first.length() >= paletteArchiveName.length() &&
                    treRef.first.compare(treRef.first.length() - paletteArchiveName.length(), paletteArchiveName.length(), paletteArchiveName) == 0) {
                    paletteArchiveName = treRef.first;
                    break;
                }
            }
        }
        writer.WriteUint16(archiveName.length());
        writer.WriteString(archiveName.c_str(), archiveName.length());
        writer.WriteUint16(paletteArchiveName.length());
        writer.WriteString(paletteArchiveName.c_str(), paletteArchiveName.length());
        writer.WriteUint16(fg->pak_entry_id);
        writer.WriteUint16(fg->palette);
        writer.WriteUint16(fg->shapeid);
        writer.WriteUint8(fg->use_external_palette ? 1 : 0);
        
        // Positions et vélocité
        writer.WriteInt16(fg->position_start.x);
        writer.WriteInt16(fg->position_start.y);
        writer.WriteInt16(fg->position_end.x);
        writer.WriteInt16(fg->position_end.y);
        writer.WriteInt16(fg->velocity.x);
        writer.WriteInt16(fg->velocity.y);
        
        writer.EndChunk();
    }
    writer.EndChunk();
}

void SCAnimationArchive::HandleANIM(uint8_t* data, size_t size) {
    IFFSaxLexer lexer;
    
    std::unordered_map<std::string, std::function<void(uint8_t*, size_t)>> events;
    // events["HEAD"] = std::bind(&SCAnimationArchive::HandleANIM, this, std::placeholders::_1, std::placeholders::_2);
    events["SHOT"] = std::bind(&SCAnimationArchive::HandleSHOT, this, std::placeholders::_1, std::placeholders::_2);
    
    lexer.InitFromRAM(data, size, events);
}

// Mettre à jour HandleSHOT pour inclure les gestionnaires de personnages
void SCAnimationArchive::HandleSHOT(uint8_t* data, size_t size) {
    // Créer un nouveau shot
    currentShot = new MIDGAME_SHOT();
    
    IFFSaxLexer lexer;
    
    std::unordered_map<std::string, std::function<void(uint8_t*, size_t)>> events;
    events["SHTP"] = std::bind(&SCAnimationArchive::HandleSHTP, this, std::placeholders::_1, std::placeholders::_2);
    events["BGND"] = std::bind(&SCAnimationArchive::HandleBGND, this, std::placeholders::_1, std::placeholders::_2);
    events["SPRT"] = std::bind(&SCAnimationArchive::HandleSPRT, this, std::placeholders::_1, std::placeholders::_2);
    events["FGND"] = std::bind(&SCAnimationArchive::HandleFGND, this, std::placeholders::_1, std::placeholders::_2);
    events["SOND"] = std::bind(&SCAnimationArchive::HandleSOND, this, std::placeholders::_1, std::placeholders::_2);
    events["CHAR"] = std::bind(&SCAnimationArchive::HandleCHAR, this, std::placeholders::_1, std::placeholders::_2);

    lexer.InitFromRAM(data, size, events);
    
    // Ajouter le shot à la liste
    shots->push_back(currentShot);
    currentShot = nullptr;
}

// Ajouter ces nouvelles fonctions pour lire les personnages
void SCAnimationArchive::HandleCHAR(uint8_t* data, size_t size) {
    IFFSaxLexer lexer;
    
    std::unordered_map<std::string, std::function<void(uint8_t*, size_t)>> events;
    events["CHRC"] = std::bind(&SCAnimationArchive::HandleCHRC, this, std::placeholders::_1, std::placeholders::_2);
    
    lexer.InitFromRAM(data, size, events);
}

void SCAnimationArchive::HandleCHRC(uint8_t* data, size_t size) {
    if (currentShot) {
        ByteStream stream(data);
        
        MIDGAME_SHOT_CHARACTER* character = new MIDGAME_SHOT_CHARACTER();
        
        // Lecture du nom du personnage
        uint16_t charNameLength = stream.ReadUShort();
        character->character_name = stream.ReadString(charNameLength);
        ConvAssetManager& ConvAssets = ConvAssetManager::getInstance();
        character->image = ConvAssets.GetCharFace(character->character_name)->appearances;
        // Lecture du nom du fond
        uint16_t bgNameLength = stream.ReadUShort();
        if (bgNameLength > 0)
            character->background_name = stream.ReadString(bgNameLength);
        
        // Lecture des positions et vélocité
        character->position_start.x = stream.ReadShort();
        character->position_start.y = stream.ReadShort();
        character->position_end.x = stream.ReadShort();
        character->position_end.y = stream.ReadShort();
        character->velocity.x = stream.ReadShort();
        character->velocity.y = stream.ReadShort();
        
        // Lecture des IDs des éléments du personnage
        character->palette = stream.ReadByte();
        character->cloth_id = stream.ReadByte();
        character->head_id = stream.ReadByte();
        character->expression_id = stream.ReadByte();
        character->talking = stream.ReadByte() != 0;
        
        // Chargement des images du personnage
        // Cette partie dépend de comment vous stockez/chargez les images des personnages
        // et peut nécessiter une logique spécifique selon votre système de ressources
        
        currentShot->characters.push_back(character);
    }
}

void SCAnimationArchive::HandleSHTP(uint8_t* data, size_t size) {
    if (currentShot) {
        ByteStream stream(data);
        currentShot->nbframe = stream.ReadInt32LE();
        currentShot->music = stream.ReadByte();
        currentShot->sound_time_code = stream.ReadUShort();
    }
}

void SCAnimationArchive::HandleBGND(uint8_t* data, size_t size) {
    IFFSaxLexer lexer;
    std::unordered_map<std::string, std::function<void(uint8_t*, size_t)>> events;
    events["BKGD"] = std::bind(&SCAnimationArchive::HandleBKGD, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, events);
}

void SCAnimationArchive::HandleBKGD(uint8_t* data, size_t size) {
    if (currentShot) {
        ByteStream stream(data);
        
        MIDGAME_SHOT_BG* bg = new MIDGAME_SHOT_BG();
        
        // Lire le nom de l'archive
        uint16_t nameLength = stream.ReadUShort();
        std::string archiveName = stream.ReadString(nameLength);
        bg->pak = FindOrLoadPakArchive(archiveName);
        
        nameLength = stream.ReadUShort();
        if (nameLength > 0) {
            std::string paletteArchiveName = stream.ReadString(nameLength);
            // Trouver ou charger l'archive
            
            if (!paletteArchiveName.empty()) {
                bg->pak_palette = FindOrLoadPakArchive(paletteArchiveName);
            } else {
                bg->pak_palette = nullptr;
            }
        } else {
            bg->pak_palette = nullptr;
        }
        
        bg->pak_entry_id = stream.ReadUShort();
        bg->palette = stream.ReadUShort();
        bg->shapeid = stream.ReadUShort();
        bg->use_external_palette = stream.ReadByte() != 0;
        // Lire les positions et la vélocité
        bg->position_start.x = stream.ReadShort();
        bg->position_start.y = stream.ReadShort();
        bg->current_position = {bg->position_start.x, bg->position_start.y};
        bg->position_end.x = stream.ReadShort();
        bg->position_end.y = stream.ReadShort();
        bg->velocity.x = stream.ReadShort();
        bg->velocity.y = stream.ReadShort();
        
        // Charger l'image
        if (bg->pak != nullptr) {
            PakEntry* entry = bg->pak->GetEntry(bg->pak_entry_id);
            if (entry != nullptr) {
                RSImageSet* tmp_img = new RSImageSet();
                PakArchive* pk = new PakArchive();
                pk->InitFromRAM("temp", entry->data, entry->size);
                tmp_img->InitFromPakArchive(pk, 0);
                
                if (tmp_img->GetNumImages() == 0) {
                    delete tmp_img;
                    tmp_img = new RSImageSet();
                    tmp_img->InitFromPakEntry(entry);
                }
                
                bg->image = tmp_img;
                if (tmp_img->palettes.size() > 0) {
                    bg->pal = tmp_img->palettes[0];
                }
            }
        }
        currentShot->background.push_back(bg);
    }
}

void SCAnimationArchive::HandleSPRT(uint8_t* data, size_t size) {
    IFFSaxLexer lexer;
    
    std::unordered_map<std::string, std::function<void(uint8_t*, size_t)>> events;
    events["SPRI"] = std::bind(&SCAnimationArchive::HandleSPRI, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, events);
}

void SCAnimationArchive::HandleSPRI(uint8_t* data, size_t size) {
    if (currentShot) {
        ByteStream stream(data);
        
        MIDGAME_SHOT_SPRITE* sprite = new MIDGAME_SHOT_SPRITE();
        
        // Lire le nom de l'archive
        uint16_t nameLength = stream.ReadUShort();
        std::string archiveName = stream.ReadString(nameLength);
        
        sprite->pak = FindOrLoadPakArchive(archiveName);
        
        uint16_t palNameLength = stream.ReadUShort();
        if (palNameLength > 0) {
            std::string paletteArchiveName = stream.ReadString(palNameLength);
            if (!paletteArchiveName.empty()) {
                sprite->pak_palette = FindOrLoadPakArchive(paletteArchiveName);
            } else {
                sprite->pak_palette = nullptr;
            }
        } else {
            sprite->pak_palette = nullptr;
        }
        
        sprite->pak_entry_id = stream.ReadUShort(); 
        sprite->shapeid = stream.ReadUShort();
        sprite->palette = stream.ReadUShort();
        sprite->use_external_palette = stream.ReadByte() != 0;
        
        sprite->position_start.x = stream.ReadShort();
        sprite->position_start.y = stream.ReadShort();
        sprite->position_end.x = stream.ReadShort();
        sprite->position_end.y = stream.ReadShort();
        sprite->velocity.x = stream.ReadShort();
        sprite->velocity.y = stream.ReadShort();
        sprite->keep_first_frame = stream.ReadByte();
        if (sprite->pak != nullptr) {
            PakEntry* entry = sprite->pak->GetEntry(sprite->pak_entry_id);
            if (entry != nullptr) {
                RSImageSet* tmp_img = new RSImageSet();
                PakArchive* pk = new PakArchive();
                pk->InitFromRAM("temp", entry->data, entry->size);
                tmp_img->InitFromPakArchive(pk, 0);
                
                if (tmp_img->GetNumImages() == 0) {
                    delete tmp_img;
                    tmp_img = new RSImageSet();
                    tmp_img->InitFromPakEntry(entry);
                }
                
                sprite->image = tmp_img;
                if (tmp_img->palettes.size() > 0) {
                    sprite->pal = tmp_img->palettes[0];
                }
            }
        }
        currentShot->sprites.push_back(sprite);
    }
}

void SCAnimationArchive::HandleFGND(uint8_t* data, size_t size) {
    IFFSaxLexer lexer;
    
    std::unordered_map<std::string, std::function<void(uint8_t*, size_t)>> events;
    events["FRGD"] = std::bind(&SCAnimationArchive::HandleFRGD, this, std::placeholders::_1, std::placeholders::_2);
    
    
    lexer.InitFromRAM(data, size, events);
}

void SCAnimationArchive::HandleFRGD(uint8_t* data, size_t size) {
    if (currentShot) {
        ByteStream stream(data);
        
        MIDGAME_SHOT_BG* fg = new MIDGAME_SHOT_BG();
        
        // Lire le nom de l'archive
        uint16_t nameLength = stream.ReadUShort();
        std::string archiveName = stream.ReadString(nameLength);
        fg->pak = FindOrLoadPakArchive(archiveName);
        // Lire le nom de l'archive de palette
        uint16_t palNameLength = stream.ReadUShort();
        if (palNameLength > 0) {
            std::string paletteArchiveName = stream.ReadString(palNameLength);
            if (!paletteArchiveName.empty()) {
                fg->pak_palette = FindOrLoadPakArchive(paletteArchiveName);
            } else {
                fg->pak_palette = nullptr;
            }
        } else {
            fg->pak_palette = nullptr;
        }
        
        fg->pak_entry_id = stream.ReadUShort();
        fg->palette = stream.ReadUShort();
        fg->shapeid = stream.ReadUShort();
        fg->use_external_palette = stream.ReadByte() != 0;
        
        // Lire les positions et la vélocité
        fg->position_start.x = stream.ReadShort();
        fg->position_start.y = stream.ReadShort();
        fg->current_position = {fg->position_start.x, fg->position_start.y};
        fg->position_end.x = stream.ReadShort();
        fg->position_end.y = stream.ReadShort();
        fg->velocity.x = stream.ReadShort();
        fg->velocity.y = stream.ReadShort();
        
        // Charger l'image
        if (fg->pak != nullptr) {
            PakEntry* entry = fg->pak->GetEntry(fg->pak_entry_id);
            if (entry != nullptr) {
                RSImageSet* tmp_img = new RSImageSet();
                PakArchive* pk = new PakArchive();
                pk->InitFromRAM("temp", entry->data, entry->size);
                tmp_img->InitFromPakArchive(pk, 0);
                
                if (tmp_img->GetNumImages() == 0) {
                    delete tmp_img;
                    tmp_img = new RSImageSet();
                    tmp_img->InitFromPakEntry(entry);
                }
                
                fg->image = tmp_img;
                if (tmp_img->palettes.size() > 0) {
                    fg->pal = tmp_img->palettes[0];
                }
            }
        }
        
        currentShot->foreground.push_back(fg);
    }
}

void SCAnimationArchive::HandleSOND(uint8_t* data, size_t size) {
    if (currentShot) {
        ByteStream stream(data);
        
        uint16_t nameLength = stream.ReadUShort();
        std::string archiveName = stream.ReadString(nameLength);
        currentShot->sound_pak = FindOrLoadPakArchive(archiveName);
        currentShot->sound_pak_entry_id = stream.ReadUShort();
        
        if (currentShot->sound_pak != nullptr) {
            PakEntry* soundEntry = currentShot->sound_pak->GetEntry(currentShot->sound_pak_entry_id);
            if (soundEntry != nullptr) {
                currentShot->sound = new MIDGAME_SOUND();
                currentShot->sound->data = soundEntry->data;
                currentShot->sound->size = soundEntry->size;
            }
        }
    }
}

PakArchive* SCAnimationArchive::FindOrLoadPakArchive(const std::string& name) {
    AssetManager& Assets = AssetManager::getInstance();
    // Sinon, tenter de charger l'archive depuis les assets
    TreEntry* entry = Assets.GetEntryByName(name.c_str());
    if (entry != nullptr) {
        PakArchive* archive = new PakArchive();
        archive->InitFromRAM(name.c_str(), entry->data, entry->size);
        
        // Ajouter à la liste des archives chargées
        PakArchiveRef ref;
        ref.name = name;
        ref.archive = archive;
        pakArchives.push_back(ref);
        
        return archive;
    }
    
    return nullptr;
}