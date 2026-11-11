#pragma once
#include "../realspace/RSEntity.h"

// Objet de Pacific Strike : lecture de RSEntity, plus le chunk KIND et les classes d'objets que Strike Commander
// n'a pas (SHIP, CARR, TORP, NPLM, LSON, EJEC), associées à un type de libRealSpace.
class PSEntity : public RSEntity {
public:
    // Classe du fichier (FORM sous OBJT) : JETP, SHIP, CARR...
    std::string object_class;
    // Chunk KIND : type cherché par les scripts de mission ; 22 quand le chunk est absent (PACIFIC.EXE).
    uint8_t kind{ 22 };

protected:
    void parseREAL_OBJT(uint8_t *data, size_t size) override;
    void parseClassKind(std::string class_name, uint8_t *data, size_t size);
    void parseKIND(uint8_t *data, size_t size);
    void parseREAL_OBJT_SHIP(std::string class_name, uint8_t *data, size_t size);
    void parseREAL_OBJT_TORP(std::string class_name, uint8_t *data, size_t size);
    void setPacificEntityType();
};
