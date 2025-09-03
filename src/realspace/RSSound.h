#pragma once
#include "precomp.h"
#include <cstdint>
#include <vector>
#include <unordered_map>

struct MemSound {
    uint8_t *data;
    size_t size;
    uint8_t id{0};
};
struct InGameVoice {
    std::unordered_map<uint8_t, MemSound *> messages;
    uint8_t id{0};
};

class RSSound {
private:
    AssetManager &Assets = AssetManager::instance();
    inline static std::unique_ptr<RSSound> s_instance{};
    
public:
    std::unordered_map<uint8_t, InGameVoice *> inGameVoices;
    std::vector<MemSound *> sounds;
    void init();
    static RSSound& getInstance() {
        if (!RSSound::hasInstance()) {
            RSSound::setInstance(std::make_unique<RSSound>());
        }
        RSSound& instance = RSSound::instance();
        return instance;
    };
    static RSSound& instance() {
        return *s_instance;
    }
    static void setInstance(std::unique_ptr<RSSound> inst) {
        s_instance = std::move(inst);
    }
    static bool hasInstance() { return (bool)s_instance; }

};