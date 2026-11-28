#include "precomp.h"
#include "../engine/gametimer.h"

void SCExplosion::render() {
    if (is_finished) return;
    float dt = GameTimer::getInstance().getDeltaTime();
    if (this->falling) {
        this->fall(dt);
        if (this->is_finished) {
            return;
        }
    }
    this->image_time += dt;
    while (this->image_time >= 7.0f * TUNED_FRAME_SECONDS) {
        this->image_time -= 7.0f * TUNED_FRAME_SECONDS;
        this->current_frame = this->current_frame + 1;
    }
    if (this->falling && !this->obj->animations.empty()) {
        this->current_frame = this->current_frame % (int)this->obj->animations.size();
    }
    if (this->current_frame >= this->obj->animations.size()) {
        this->is_finished = true; // Mark as finished if no more frames
        return;
    }
    Renderer.drawBillboard(
        this->position,
        obj->animations[this->current_frame],
        this->falling && this->obj->anim_size > 0.0f ? this->obj->anim_size : 50.0f
    );
}

// Chute d'un débris : gravité, frottement de l'air (coefficient ATMO, masse DYNM, densité de AIRDENS.TBL), comme
// AgravSimulatedObject ; fini quand il passe sous le sol.
void SCExplosion::fall(float dt) {
    if (!this->airdens.isLoaded()) {
        TreEntry *airdens_entry = AssetManager::getInstance().GetEntryByName("..\\..\\DATA\\AIRDENS.TBL");
        if (airdens_entry != nullptr) {
            this->airdens.initFromRam(airdens_entry->data, airdens_entry->size);
        }
    }
    Vector3D acceleration = {0.0f, -9.8f, 0.0f};
    float speed = this->velocity.Length();
    if (speed > 0.0f && this->obj->weight_in_kg > 0 && this->airdens.isLoaded()) {
        float dynamic_pressure = 0.5f * this->airdens.densityAt(this->position.y) * speed * speed;
        Vector3D direction = this->velocity;
        direction.Normalize();
        acceleration = acceleration - direction * (this->obj->drag_coefficient * dynamic_pressure / (float)this->obj->weight_in_kg);
    }
    this->velocity = this->velocity + acceleration * dt;
    this->position = this->position + this->velocity * dt;
    if (this->area != nullptr && this->position.y < this->area->getY(this->position.x, this->position.z)) {
        this->is_finished = true;
    }
}
SCExplosion::SCExplosion(RSEntity *obj, Vector3D position)
    : obj(obj), image_time(0.0f), position(position), current_frame(0), is_finished(false) {
    if (obj == nullptr) {
        is_finished = true; // If no object is provided, mark as finished
    }
}