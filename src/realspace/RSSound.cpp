#include "precomp.h"
#include <string>

void RSSound::init() {
    if (initialized) return;
    initialized = true;
    TreEntry *tre = Assets.GetEntryByName(Assets.intel_root_path + "SPEECH.PAK");
    if (tre != nullptr) {
        PakArchive *pak = new PakArchive();
        pak->InitFromRAM("SPEECH.PAK", tre->data, tre->size);
        if (pak->IsReady()) {
            for (size_t i = 0; i < pak->GetNumEntries(); ++i) {
                PakEntry *entry = pak->GetEntry(i);
                
                if (entry != nullptr) {
                    InGameVoice *voice = new InGameVoice();
                    voice->id = i;
                    PakArchive *subPak = new PakArchive();
                    std::string subPakName = "voice"+std::to_string(i);
                    subPak->InitFromRAM(subPakName.c_str(), entry->data, entry->size);
                    for (size_t j = 0; j < subPak->GetNumEntries(); ++j) {
                        PakEntry *subEntry = subPak->GetEntry(j);
                        if (subEntry != nullptr) {
                            MemSound *mem_sound = new MemSound();
                            mem_sound->data = subEntry->data;
                            mem_sound->size = subEntry->size;
                            mem_sound->id = j; // Assign an ID based on the entry index
                            voice->messages[mem_sound->id] = mem_sound;
                        }
                    }
                    delete subPak;
                    inGameVoices[voice->id] = voice;                   
                }
            }
        }
    }
    
    tre = Assets.GetEntryByName(Assets.sound_root_path + "DSOUNDFX.PAK");
    if (tre != nullptr) {
        PakArchive *pak = new PakArchive();
        pak->InitFromRAM("DSOUNDFX.PAK", tre->data, tre->size);
        if (pak->IsReady()) {
            for (size_t i = 0; i < pak->GetNumEntries(); ++i) {
                PakEntry *entry = pak->GetEntry(i);
                PakArchive *subPak = new PakArchive();
                std::string subPakName = "soundfx"+std::to_string(i);
                subPak->InitFromRAM(subPakName.c_str(), entry->data, entry->size);
                if (subPak->IsReady()) {
                    for (size_t j = 0; j < subPak->GetNumEntries(); ++j) {
                        PakEntry *subEntry = subPak->GetEntry(j);
                        if (subEntry != nullptr) {
                            MemSound *mem_sound = new MemSound();
                            mem_sound->data = subEntry->data;
                            mem_sound->size = subEntry->size;
                            mem_sound->id = j; // Assign an ID based on the entry index
                            sounds.push_back(mem_sound);
                        }
                    }
                }
            }
        }
    }
    this->initEffects();
}

// Strike Commander : l'effet n (jusqu'à GUN_HIT) est le n-ième son de DSOUNDFX.PAK mis à plat ; le joueur touché et la chute dans l'eau
// reprennent l'impact et l'explosion.
void RSSound::initEffects() {
    for (size_t i = 0; i < this->sounds.size() && i <= SoundEffectIds::GUN_HIT; i++) {
        this->effects[(int)i].push_back(this->sounds[i]);
    }
    if (this->sounds.size() > SoundEffectIds::GUN_IMPACT_1) {
        this->effects[SoundEffectIds::PLAYER_HIT].push_back(this->sounds[SoundEffectIds::GUN_IMPACT_1]);
        this->effects[SoundEffectIds::WATER_SPLASH].push_back(this->sounds[SoundEffectIds::EXPLOSION_1]);
    }
    this->fx_effects[SoundEffectIds::GUN_FIRE] = 2;
}

// Numéro de la séquence XMIDI de l'effet ; -1 si le jeu n'en a pas.
int RSSound::fxEffect(SoundEffectIds id) {
    auto found = this->fx_effects.find(id);
    if (found == this->fx_effects.end()) {
        return -1;
    }
    return found->second;
}

// Un des sons de l'effet, tiré au hasard quand il y en a plusieurs ; nul si le jeu n'en a pas.
MemSound *RSSound::effect(SoundEffectIds id) {
    auto found = this->effects.find(id);
    if (found == this->effects.end() || found->second.empty()) {
        return nullptr;
    }
    return found->second[std::rand() % found->second.size()];
}
