#include "Config.hpp"

bool Config::load(const std::string& filePath) {
    _ini.SetUnicode();
    SI_Error result = _ini.LoadFile(filePath.c_str());
    return result == SI_OK;
}

std::string Config::getString(const std::string& section, const std::string& key, const std::string& defaultValue) const {
    return _ini.GetValue(section.c_str(), key.c_str(), defaultValue.c_str());
}

int Config::getInt(const std::string& section, const std::string& key, int defaultValue) const {
    return static_cast<int>(_ini.GetLongValue(section.c_str(), key.c_str(), defaultValue));
}

bool Config::getBool(const std::string& section, const std::string& key, bool defaultValue) const {
    return _ini.GetBoolValue(section.c_str(), key.c_str(), defaultValue);
}