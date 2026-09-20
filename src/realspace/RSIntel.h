#pragma once

#include "../commons/IFFSaxLexer.h"
#include "../commons/Maths.h"
#include "../commons/Matrix.h"

class RSIntel {
private:
    void parseINTL(uint8_t *data, size_t size);
    void parseINTL_VERS(uint8_t *data, size_t size);
    void parseINTL_NUMS(uint8_t *data, size_t size);

public:
    uint16_t version{0};
    float unknown_offset{0.0f};
    int16_t unknown_a{0};
    int16_t unknown_b{0};
    Vector3D formation_offset{0.0f, 0.0f, 0.0f};
    Vector3D unknown_vector{0.0f, 0.0f, 0.0f};
    float range_gun{0.0f};
    int32_t range_medium{0};
    int32_t range_far{0};
    int32_t range_close{0};
    int32_t range_long{0};
    int32_t range_ground{0};
    int32_t unknown_range{0};
    float unknown_value{0.0f};
    bool status_flag{false};

    RSIntel();
    ~RSIntel();
    void InitFromRAM(uint8_t *data, size_t size);
};
