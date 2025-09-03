//
//  SCFontManager.h
//  libRealSpace
//
//  Created by Fabien Sanglard on 2/5/2014.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//

#pragma once
#include <unordered_map>
#include <string.h>

#include "RSFont.h"
#include "AssetManager.h"
#include "TreArchive.h"

class RSFontManager{
private:
    std::unordered_map<std::string, RSFont*> fonts;
    inline static std::unique_ptr<RSFontManager> s_instance{};
public:
    static RSFontManager& getInstance() {
        if (!RSFontManager::hasInstance()) {
            RSFontManager::setInstance(std::make_unique<RSFontManager>());
        }
        RSFontManager& instance = RSFontManager::instance();
        return instance;
    };
    static RSFontManager& instance() {
        return *s_instance;
    }
    static void setInstance(std::unique_ptr<RSFontManager> inst) {
        s_instance = std::move(inst);
    }

    static bool hasInstance() { return (bool)s_instance; }

    RSFontManager();
    ~RSFontManager();
    
    void init();
    RSFont* GetFont(const char* name);
};

