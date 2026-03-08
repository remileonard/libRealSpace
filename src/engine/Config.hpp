#pragma once

#include <SimpleIni.h>
#include <memory>
class Config {
private:
    CSimpleIniA _ini;
    inline static std::unique_ptr<Config> s_instance{};
public:
    Config() = default;
    virtual ~Config() = default;
    
    static Config& getInstance() {
        if (!Config::hasInstance()) {
            Config::setInstance(std::make_unique<Config>());
        }
        Config& instance = Config::instance();
        return instance;
    };
    static Config& instance() {
        return *s_instance;
    };
    static void setInstance(std::unique_ptr<Config> inst) {
        s_instance = std::move(inst);
    };
    static bool hasInstance() { return (bool)s_instance; };
    bool load(const std::string& filePath);
    std::string getString(const std::string& section, const std::string& key, const std::string& defaultValue = "") const;
    int getInt(const std::string& section, const std::string& key, int defaultValue = 0) const;
    bool getBool(const std::string& section, const std::string& key, bool defaultValue = false) const;

};