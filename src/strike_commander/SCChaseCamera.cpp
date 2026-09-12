#include "precomp.h"
#include "SCChaseCamera.h"

bool SCChaseCamera::s_debug = true;

const char *SCChaseCamera::debugLabel() const {
    return "[CHASE]";
}

bool SCChaseCamera::debugEnabled() const {
    return SCChaseCamera::s_debug;
}
