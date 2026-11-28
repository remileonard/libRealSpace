#include "precomp.h"
#include "../engine/gametimer.h"

void SCExplosion::render() {
    if (is_finished) return;
    this->image_time += GameTimer::getInstance().getDeltaTime();
    while (this->image_time >= 7.0f * TUNED_FRAME_SECONDS) {
        this->image_time -= 7.0f * TUNED_FRAME_SECONDS;
        this->current_frame = this->current_frame + 1;
    }
    if (this->current_frame >= this->obj->animations.size()) {
        this->is_finished = true; // Mark as finished if no more frames
        return;
    }
    Renderer.drawBillboard(
        this->position,
        obj->animations[this->current_frame],
        50.0f
    );
}
SCExplosion::SCExplosion(RSEntity *obj, Vector3D position)
    : obj(obj), image_time(0.0f), position(position), current_frame(0), is_finished(false) {
    if (obj == nullptr) {
        is_finished = true; // If no object is provided, mark as finished
    }
}