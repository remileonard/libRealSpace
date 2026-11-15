#pragma once
#include <cstdint>
#include <unordered_map>

class PSState {
public:
    static PSState &getInstance() {
        static PSState instance;
        return instance;
    }
    std::unordered_map<uint16_t, int16_t> flags;

    int16_t getFlag(uint16_t number);
    void setFlag(uint16_t number, int16_t value);
    void Reset();

private:
    PSState() = default;
};
