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

void RSSound::addEffect(SoundEffectIds id, size_t rank) {
    if (rank < this->sounds.size()) {
        this->effects[id].push_back(this->sounds[rank]);
    }
}

void RSSound::addFarEffect(SoundEffectIds id, size_t rank) {
    if (rank < this->sounds.size()) {
        this->far_effects[id].push_back(this->sounds[rank]);
    }
}

// Strike Commander, version CD-ROM : rangs de DSOUNDFX.PAK écrits par DigitalSFX_Construct_58656 (SOUND_EFFECTS.md § 2.4) ;
// les sons voisins d'un même effet sont tirés au hasard. Séquences XMIDI : numéros des effets de SOUNDFX.ADL (§ 2.2).
void RSSound::initEffects() {
    this->addEffect(SoundEffectIds::MK82_DROP, 1);
    this->addFarEffect(SoundEffectIds::MK82_DROP, 0);
    this->addEffect(SoundEffectIds::MK20_DROP, 1);
    this->addFarEffect(SoundEffectIds::MK20_DROP, 0);
    this->addEffect(SoundEffectIds::EXPLOSION_1, 3);
    this->addEffect(SoundEffectIds::EXPLOSION_1, 4);
    this->addFarEffect(SoundEffectIds::EXPLOSION_1, 2);
    this->addEffect(SoundEffectIds::EXPLOSION_2, 3);
    this->addEffect(SoundEffectIds::EXPLOSION_2, 4);
    this->addFarEffect(SoundEffectIds::EXPLOSION_2, 2);
    this->addEffect(SoundEffectIds::EXPLOSION_3, 3);
    this->addEffect(SoundEffectIds::EXPLOSION_3, 4);
    this->addFarEffect(SoundEffectIds::EXPLOSION_3, 2);
    this->addEffect(SoundEffectIds::EXPLOSION_4, 3);
    this->addEffect(SoundEffectIds::EXPLOSION_4, 4);
    this->addFarEffect(SoundEffectIds::EXPLOSION_4, 2);
    this->addEffect(SoundEffectIds::WATER_SPLASH, 3);
    this->addEffect(SoundEffectIds::WATER_SPLASH, 4);
    this->addFarEffect(SoundEffectIds::WATER_SPLASH, 2);
    this->addEffect(SoundEffectIds::AIM9_SHOOT, 6);
    this->addEffect(SoundEffectIds::AIM120_SHOOT, 7);
    this->addEffect(SoundEffectIds::POD_SHOOT, 8);
    this->addEffect(SoundEffectIds::MAVERICK_SHOOT, 9);
    this->addEffect(SoundEffectIds::GUN_IMPACT_1, 10);
    this->addEffect(SoundEffectIds::GUN_IMPACT_1, 11);
    this->addEffect(SoundEffectIds::GUN_IMPACT_1, 12);
    this->addEffect(SoundEffectIds::PLAYER_HIT, 13);
    this->addEffect(SoundEffectIds::PLAYER_HIT, 14);
    this->addEffect(SoundEffectIds::PLAYER_HIT, 15);
    this->addEffect(SoundEffectIds::GEARS_UP, 19);
    this->addEffect(SoundEffectIds::GEARS_DOWN, 20);
    this->addEffect(SoundEffectIds::TOUCH_DOWN, 21);
    this->addEffect(SoundEffectIds::ENGINE_MIL, 23);
    this->addEffect(SoundEffectIds::ENGINE_AFB, 24);
    this->addEffect(SoundEffectIds::ENGINE_START_MIL, 25);
    this->addEffect(SoundEffectIds::ENGINE_START_AFB, 26);
    this->addEffect(SoundEffectIds::ENGINE_SHUT_DOWN, 27);
    this->addEffect(SoundEffectIds::ENGINE_MIL_SHUT_DOWN, 28);
    this->addEffect(SoundEffectIds::ENGINE_AFB_SHUT_DOWN, 29);
    this->addEffect(SoundEffectIds::ENGINE_AFB_TO_MIL, 30);
    this->addEffect(SoundEffectIds::GUN_HIT, 31);
    this->fx_effects[SoundEffectIds::AIM9_SHOOT] = 1;
    this->fx_effects[SoundEffectIds::AIM120_SHOOT] = 1;
    this->fx_effects[SoundEffectIds::POD_SHOOT] = 1;
    this->fx_effects[SoundEffectIds::MAVERICK_SHOOT] = 1;
    this->fx_effects[SoundEffectIds::GUN_FIRE] = 2;
    this->fx_effects[SoundEffectIds::EXPLOSION_1] = 3;
    this->fx_effects[SoundEffectIds::EXPLOSION_2] = 3;
    this->fx_effects[SoundEffectIds::EXPLOSION_3] = 3;
    this->fx_effects[SoundEffectIds::EXPLOSION_4] = 3;
    this->fx_effects[SoundEffectIds::WATER_SPLASH] = 3;
    this->fx_effects[SoundEffectIds::GEARS_UP] = 4;
    this->fx_effects[SoundEffectIds::GEARS_DOWN] = 5;
    this->fx_effects[SoundEffectIds::MK82_DROP] = 8;
    this->fx_effects[SoundEffectIds::MK20_DROP] = 8;
    this->fx_effects[SoundEffectIds::PLAYER_HIT] = 9;
    this->fx_effects[SoundEffectIds::GUN_IMPACT_1] = 10;
    this->fx_effects[SoundEffectIds::GUN_HIT] = 10;
    this->fx_effects[SoundEffectIds::TOUCH_DOWN] = 12;
    printf("RSSound: %zu sons, %zu effets proches, %zu effets lointains\n", this->sounds.size(), this->effects.size(), this->far_effects.size());
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

// Son de l'effet pour une distance à la caméra (DigitalSFX_PlayEffect_58789) : un son lointain au-delà de 1000 m, un son
// proche en deçà ; nul si l'effet n'a pas de son de ce côté (la synthèse prend alors le relais).
MemSound *RSSound::effect(SoundEffectIds id, float distance) {
    std::unordered_map<int, std::vector<MemSound *>> &table = distance > 1000.0f ? this->far_effects : this->effects;
    auto found = table.find(id);
    if (found == table.end() || found->second.empty()) {
        return nullptr;
    }
    return found->second[std::rand() % found->second.size()];
}
