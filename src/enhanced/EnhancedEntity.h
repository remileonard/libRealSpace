#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "../realspace/RSEntity.h"

// RSEntity that replaces its airframe geometry with an OBJ file of the same base name when one is found.
class EnhancedEntity : public RSEntity {
public:
    void InitFromRAM(uint8_t *data, size_t size, std::string name) override;
    static std::string models_dir;
    static std::string replacementPathFor(const std::string &entity_name);
    // OBJ groups lod0..lodN, game axes: x forward, y up, z lateral. Returns false if the file cannot be used.
    bool replaceGeometryFromOBJ(const std::string &path);

    /** Switches all replaced aircraft between the OBJ airframe and the original geometry, also for models loaded later. */
    static void setReplacementsActive(bool active);

private:
    // everything replaceGeometryFromOBJ() changes, so the entity can switch between the two at any time
    struct Geometry {
        std::vector<Vector3D> vertices;
        std::vector<Triangle> triangles;
        std::vector<Lod> lods;
        std::unordered_map<uint16_t, Attr *> attrs;
        std::vector<uvxyEntry> uvs;
        std::vector<Quads *> quads;
        std::vector<qmapuvxyEntry *> qmapuvs;
        std::vector<int32_t> childY;
    };
    void takeGeometry(Geometry &geometry) const;
    void putGeometry(const Geometry &geometry);
    void useReplacement(bool active);

    Geometry original;
    Geometry replacement;
    bool replaced{false};
    bool active{false};
    static bool wanted;
    static std::vector<EnhancedEntity *> replacedEntities;
};
