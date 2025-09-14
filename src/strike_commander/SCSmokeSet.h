#pragma once
#include "precomp.h"

class SCSmokeSet {
private:
    RSSmokeSet *smoke_set;
    VGAPalette palette;
    AssetManager &Assets = AssetManager::getInstance();
    void generateMissileSmokeTextures(int frames, int size);
    inline static std::unique_ptr<SCSmokeSet> s_instance{};
    bool init_done{false};
public: 
    std::vector<Texture *> textures;
    std::vector<std::vector<Texture *>> smoke_textures;
    std::vector<Texture *> missile_smoke_textures;
    static SCSmokeSet& getInstance() {
        if (!SCSmokeSet::hasInstance()) {
            SCSmokeSet::setInstance(std::make_unique<SCSmokeSet>());
        }
        SCSmokeSet& instance = SCSmokeSet::instance();
        if (!instance.init_done) {
            instance.init();
            instance.init_done = true;
        }
        return instance;
    };
    static SCSmokeSet& instance() {
        return *s_instance; 
    }
    static void setInstance(std::unique_ptr<SCSmokeSet> inst) {
        s_instance = std::move(inst);
    }
    static bool hasInstance() { return (bool)s_instance; }
    SCSmokeSet();
    ~SCSmokeSet();
    void init();
};