#include "precomp.h"
#include "SCTargetCamera.h"

bool SCTargetCamera::s_debug = true;

const char *SCTargetCamera::debugLabel() const {
    return "[TARGET]";
}

bool SCTargetCamera::debugEnabled() const {
    return SCTargetCamera::s_debug;
}
