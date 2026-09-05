//
//  RSAirdens.cpp
//  libRealSpace
//

#include "precomp.h"
#include "RSAirdens.h"
#include "../commons/ByteStream.h"

#include <cmath>

RSAirdens::RSAirdens() {
}

RSAirdens::~RSAirdens() {
}

void RSAirdens::initFromRam(uint8_t *data, size_t size) {
    this->table.clear();
    if (data == nullptr || size < 4)
        return;

    ByteStream stream(data, size);
    size_t count = size / 4;
    this->table.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        uint32_t raw = stream.ReadUInt32LE();
        this->table.push_back((float)raw);
    }
}

float RSAirdens::rawAt(float altitude_m) const {
    if (this->table.empty())
        return 0.0f;
    if (altitude_m < 0.0f)
        altitude_m = 0.0f;

    float idx = altitude_m / kBandMeters;
    size_t i = (size_t)idx;
    if (i >= this->table.size() - 1)
        return this->table.back();

    float frac = idx - (float)i;
    return this->table[i] + (this->table[i + 1] - this->table[i]) * frac;
}

float RSAirdens::densityAt(float altitude_m) const {
    if (this->table.empty() || this->table[0] <= 0.0f) {
        // Repli : l'exponentielle standard colle a la table a <1% pres.
        if (altitude_m < 0.0f)
            altitude_m = 0.0f;
        return kSeaLevelDensity * expf(-altitude_m / 8000.0f);
    }
    return kSeaLevelDensity * this->rawAt(altitude_m) / this->table[0];
}
