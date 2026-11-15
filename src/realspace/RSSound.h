#pragma once
#include "precomp.h"
#include <cstdint>
#include <vector>
#include <unordered_map>

// Effets que le jeu veut jouer ; chaque jeu associe à chacun ses sons (RSSound::initEffects).
enum SoundEffectIds {
    MK82_DROP = 0,
    MK20_DROP = 1,
    EXPLOSION_1 = 2,
    EXPLOSION_2 = 3,
    EXPLOSION_3 = 4,
    EXPLOSION_4 = 5,
    AIM9_SHOOT = 6,
    AIM120_SHOOT = 7,
    POD_SHOOT = 8,
    MAVERICK_SHOOT = 9,
    GUN_IMPACT_1 = 10,
    GUN_IMPACT_2 = 11,
    GUN_IMPACT_3 = 12,
    GUN_IMPACT_4 = 13,
    GUN_IMPACT_5 = 14,
    GUN_IMPACT_6 = 15,
    PLANE_PASS_BY_1 = 16,
    PLANE_PASS_BY_2 = 17,
    PLANE_PASS_BY_3 = 18,
    GEARS_UP = 19,
    GEARS_DOWN = 20,
    TOUCH_DOWN = 21,
    ENGINE_MIL = 23,
    ENGINE_AFB = 24,
    ENGINE_START_MIL = 25,
    ENGINE_START_AFB = 26,
    ENGINE_SHUT_DOWN = 27,
    ENGINE_MIL_SHUT_DOWN = 28,
    ENGINE_AFB_SHUT_DOWN = 29,
    ENGINE_AFB_TO_MIL = 30,
    GUN_HIT = 31,
    PLAYER_HIT = 32,
    WATER_SPLASH = 33,
    GUN_FIRE = 34,
    TURRET_FIRE = 35
};

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
    bool initialized{false};
public:
    std::unordered_map<uint8_t, InGameVoice *> inGameVoices;
    std::vector<MemSound *> sounds;
    std::unordered_map<int, std::vector<MemSound *>> effects;
    // Effets joués en séquence XMIDI (SOUNDFX.ADL) : numéro de la séquence de chaque effet.
    std::unordered_map<int, int> fx_effects;
    virtual ~RSSound() = default;
    void init();
    MemSound *effect(SoundEffectIds id);
    int fxEffect(SoundEffectIds id);
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

protected:
    virtual void initEffects();

};