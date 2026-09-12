#include "precomp.h"
#include "SCProceduralCamera.h"

const std::string &SCProceduralCamera::name() const {
    static const std::string empty;
    return empty;
}

const std::string &SCProceduralCamera::subjectName() const {
    static const std::string empty;
    return empty;
}

float SCProceduralCamera::fov() const {
    return 45.0f;
}
