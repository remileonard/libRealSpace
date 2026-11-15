#include "PSState.h"

int16_t PSState::getFlag(uint16_t number) {
    auto found = this->flags.find(number);
    if (found == this->flags.end()) {
        return 0;
    }
    if (number < 256) {
        return found->second;
    }
    return found->second != 0 ? 1 : 0;
}

void PSState::setFlag(uint16_t number, int16_t value) {
    if (number < 256) {
        this->flags[number] = value;
    } else {
        this->flags[number] = value != 0 ? 1 : 0;
    }
}

void PSState::Reset() {
    this->flags.clear();
}
