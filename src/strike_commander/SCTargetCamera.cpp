#include "precomp.h"
#include "SCTargetCamera.h"

bool SCTargetCamera::s_debug = true;

RSCameraType SCTargetCamera::typeCode() const {
    return RSCAM_TARG;
}

const char *SCTargetCamera::debugLabel() const {
    return "[TARGET]";
}

bool SCTargetCamera::debugEnabled() const {
    return SCTargetCamera::s_debug;
}
