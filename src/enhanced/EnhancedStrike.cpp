#include "../strike_commander/precomp.h"
#include "EnhancedStrike.h"
#include "EnhancedMission.h"
#include <cstdio>
#include "../engine/SCRenderer.h"
#include "../realspace/RSEntity.h"
#include "../realspace/RSMission.h"
#include "../strike_commander/SCPlane.h"

using namespace enhanced;

SCMission *EnhancedStrike::createMission(char const *missionName) {
    return new EnhancedMission(missionName, &object_cache);
}

const size_t EnhancedStrike::SMOKE_POINTS = 17;
const float EnhancedStrike::SMOKE_MIN_DAMAGE = 0.2f;
const float EnhancedStrike::FIRE_MIN_DAMAGE = 0.7f;

float EnhancedStrike::damageLevel(SCPlane *plane) const {
    float total = 0.0f;
    float lost = 0.0f;
    for (const auto &sys : plane->object->entity->sysm) {
        auto health = plane->system_health.find(sys.first);
        if (health == plane->system_health.end()) {
            continue;
        }
        for (const auto &sub : sys.second) {
            auto current = health->second.find(sub.first);
            if (current == health->second.end() || sub.second == 0) {
                continue;
            }
            total += (float)sub.second;
            lost += (float)(sub.second - std::min<uint16_t>(sub.second, current->second));
        }
    }
    return total > 0.0f ? std::min(1.0f, lost / total) : 0.0f;
}

// Same composition as SCRenderer::drawModelWithChilds: translate * yaw * pitch * roll
Mat4 EnhancedStrike::modelMatrix(SCPlane *plane, Vector3D position) const {
    Mat4 model;
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(position.x, position.y, position.z);
    glRotatef((plane->yaw + 900.0f) / 10.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(plane->pitch / 10.0f, 0.0f, 0.0f, 1.0f);
    glRotatef(-plane->roll / 10.0f, 1.0f, 0.0f, 0.0f);
    glGetFloatv(GL_MODELVIEW_MATRIX, model.m);
    glPopMatrix();
    return model;
}

void EnhancedStrike::forgetOldTrails(float now) {
    for (auto it = this->smoke.begin(); it != this->smoke.end();) {
        if (now - it->second.lastSeen > 6.0f) {
            it = this->smoke.erase(it);
        } else {
            ++it;
        }
    }
}

// The original only smokes destroyed aircraft; here damaged ones leave a trail too, burning when badly hit.
void EnhancedStrike::updateSmoke(SCPlane *plane, Vector3D position, float now, const float *view) {
    const float damage = plane->object->alive ? this->damageLevel(plane) : 0.0f;
    SmokeTrail &trail = this->smoke[plane];
    trail.lastSeen = now;
    const bool emitting = damage >= SMOKE_MIN_DAMAGE && !plane->crached;
    int stage = 0;
    if (emitting) {
        stage = damage >= FIRE_MIN_DAMAGE ? 2 : 1;
    }
    if (stage != trail.stage) {
        printf("[enhanced] smoke %p: %s (damage %.2f)\n", (const void *)plane, stage == 2 ? "fire" : stage == 1 ? "smoke" : "off", damage);
        trail.stage = stage;
    }
    if (now - trail.lastSample >= 1.0f / 30.0f) {
        trail.lastSample = now;
        if (emitting) {
            Vec3 p = vec3(position.x, position.y, position.z);
            if (!trail.points.empty() && length(trail.points.front() - p) > 60.0f) {
                trail.points.clear();
            }
            trail.points.insert(trail.points.begin(), p);
            if (trail.points.size() > SMOKE_POINTS) {
                trail.points.pop_back();
            }
        } else if (!trail.points.empty()) {
            trail.points.pop_back();
        }
    }
    if (emitting) {
        this->damageSmoke.draw(trail.points, damage, damage >= FIRE_MIN_DAMAGE, view);
    } else {
        this->damageSmoke.draw(trail.points, SMOKE_MIN_DAMAGE, false, view);
    }
}

void EnhancedStrike::afterPlaneRendered(SCPlane *plane, Vector3D position, int thrust) {
    if (plane->object == nullptr || plane->object->entity == nullptr) {
        return;
    }
    RSEntity *entity = plane->object->entity;
    const float now = effectTime();
    float view[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, view);
    const Mat4 model = this->modelMatrix(plane, position);

    this->forgetOldTrails(now);
    if (plane->object->alive && !entity->chld.empty()) {
        Vec3 nozzle = vec3((float)entity->chld[0]->z / 250.0f, (float)entity->chld[0]->y / 250.0f, (float)entity->chld[0]->x / 250.0f);
        this->exhaust.draw(plane, model, nozzle, thrust, view);
    }
    this->vortices.draw(plane, entity, model, view, plane->g_load);
    this->updateSmoke(plane, position, now, view);
}
