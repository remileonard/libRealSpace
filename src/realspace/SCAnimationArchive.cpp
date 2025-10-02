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
    
    // Nom de l'archive pak
    std::string archiveName = bg->pak->GetName();
    AssetManager& Assets = AssetManager::getInstance();
    for (auto treRef: Assets.treEntries) {
        if (treRef.first.length() >= archiveName.length() &&
            treRef.first.compare(treRef.first.length() - archiveName.length(), archiveName.length(), archiveName) == 0) {
            // archiveName is a suffix of treRef.first
            archiveName = treRef.first;
            break;
        }
    }
    writer.WriteUint16(archiveName.length());
    writer.WriteString(archiveName.c_str(), archiveName.length());
    
    writer.WriteUint16(bg->pak_entry_id);
    writer.WriteUint16(bg->palette);
    writer.WriteUint16(bg->shapeid);
    
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
    writer.StartChunk("SPRI");
    
    // Pour les sprites, nous devons déterminer l'archive d'origine
    // Ce sera probablement stocké dans une propriété liée à l'image
    std::string archiveName = "UNKNOWN"; // À remplacer par la vraie valeur
    writer.WriteUint16(archiveName.length());
    writer.WriteString(archiveName.c_str(), archiveName.length());
    
    // ID d'entrée à déterminer
    writer.WriteUint16(0);
    writer.WriteUint16(sprite->palette);
    
    // Positions et vélocité
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
        
        // Nom de l'archive pak
        std::string archiveName = fg->pak->GetName();
        writer.WriteUint16(archiveName.length());
        writer.WriteString(archiveName.c_str(), archiveName.length());
        
        writer.WriteUint16(fg->pak_entry_id);
        writer.WriteUint16(fg->palette);
        writer.WriteUint16(fg->shapeid);
        
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

void SCAnimationArchive::WriteSound(IFFWriter& writer, const MIDGAME_SOUND* sound) {
    writer.StartChunk("SOND");
    if (sound != nullptr && sound->data != nullptr) {
        writer.WriteData(sound->data, sound->size);
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

    lexer.InitFromRAM(data, size, events);
    
    // Ajouter le shot à la liste
    shots->push_back(currentShot);
    currentShot = nullptr;
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
        
        // Trouver ou charger l'archive
        bg->pak = FindOrLoadPakArchive(archiveName);
        
        bg->pak_entry_id = stream.ReadUShort();
        bg->palette = stream.ReadUShort();
        bg->shapeid = stream.ReadUShort();
        
        // Lire les positions et la vélocité
        bg->position_start.x = stream.ReadShort();
        bg->position_start.y = stream.ReadShort();
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
    events["SPRI"] = std::bind(&SCAnimationArchive::HandleSPR, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, events);
}

void SCAnimationArchive::HandleSPR(uint8_t* data, size_t size) {
    if (currentShot) {
        ByteStream stream(data);
        
        MIDGAME_SHOT_SPRITE* sprite = new MIDGAME_SHOT_SPRITE();
        
        // Lire le nom de l'archive
        uint16_t nameLength = stream.ReadUShort();
        std::string archiveName = stream.ReadString(nameLength);
        
        // Trouver ou charger l'archive
        PakArchive* pak = FindOrLoadPakArchive(archiveName);
        
        uint16_t pak_entry_id = stream.ReadUShort();
        sprite->palette = stream.ReadUShort();
        
        // Lire les positions et la vélocité
        sprite->position_start.x = stream.ReadShort();
        sprite->position_start.y = stream.ReadShort();
        sprite->position_end.x = stream.ReadShort();
        sprite->position_end.y = stream.ReadShort();
        sprite->velocity.x = stream.ReadShort();
        sprite->velocity.y = stream.ReadShort();
        sprite->keep_first_frame = stream.ReadByte();
        
        // Charger l'image
        if (pak != nullptr) {
            PakEntry* entry = pak->GetEntry(pak_entry_id);
            if (entry != nullptr) {
                RSImageSet* tmp_img = new RSImageSet();
                tmp_img->InitFromPakEntry(entry);
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
    events["VERS"] = std::bind(&SCAnimationArchive::HandleFRGD, this, std::placeholders::_1, std::placeholders::_2);
    
    
    lexer.InitFromRAM(data, size, events);
}

void SCAnimationArchive::HandleFRGD(uint8_t* data, size_t size) {
    if (currentShot) {
        ByteStream stream(data);
        
        MIDGAME_SHOT_BG* fg = new MIDGAME_SHOT_BG();
        
        // Lire le nom de l'archive
        uint16_t nameLength = stream.ReadUShort();
        std::string archiveName = stream.ReadString(nameLength);
        
        // Trouver ou charger l'archive
        fg->pak = FindOrLoadPakArchive(archiveName);
        
        fg->pak_entry_id = stream.ReadUShort();
        fg->palette = stream.ReadUShort();
        fg->shapeid = stream.ReadUShort();
        
        // Lire les positions et la vélocité
        fg->position_start.x = stream.ReadShort();
        fg->position_start.y = stream.ReadShort();
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
    if (currentShot && size > 0) {
        MIDGAME_SOUND* sound = new MIDGAME_SOUND();
        sound->size = size;
        sound->data = new uint8_t[size];
        memcpy(sound->data, data, size);
        
        currentShot->sound = sound;
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