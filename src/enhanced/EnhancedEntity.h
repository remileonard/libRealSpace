#pragma once
#include <string>
#include "../realspace/RSEntity.h"

// RSEntity that replaces its airframe geometry with an OBJ file of the same base name when one is found.
class EnhancedEntity : public RSEntity {
public:
    void InitFromRAM(uint8_t *data, size_t size, std::string name) override;
    static std::string models_dir;
    static std::string replacementPathFor(const std::string &entity_name);
    // OBJ groups lod0..lodN, game axes: x forward, y up, z lateral. Returns false if the file cannot be used.
    bool replaceGeometryFromOBJ(const std::string &path);
};
