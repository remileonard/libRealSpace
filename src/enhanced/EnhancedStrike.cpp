#include "../strike_commander/precomp.h"
#include "EnhancedStrike.h"
#include "EnhancedMission.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include "../engine/EventManager.h"
#include "../engine/SCRenderer.h"
#include "../realspace/RSEntity.h"
#include "../realspace/RSMission.h"
#include "../strike_commander/SCPlane.h"

using namespace enhanced;

SCCockpit *EnhancedStrike::createCockpit() {
    return new EnhancedCockpit();
}

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
    if (this->waterSplashEnabled && !this->splashDrawnThisFrame) {
        this->splashDrawnThisFrame = true;
        this->waterSplash.draw(view);
    }
    this->updateSmoke(plane, position, now, view);
}

void EnhancedStrike::init() {
    SCStrike::init();
    Config &cfg = Config::getInstance();
    this->orbitInvertY = cfg.getString("Video", "camera_mouse_invert_y", "1") != "0";
    printf("[enhanced] camera mouse orbit: invert_y=%d\n", this->orbitInvertY ? 1 : 0);
    this->cockpit3d = cfg.getString("Video", "cockpit", "2d") == "3d";
    this->cockpitFov = std::min(100.0f, std::max(30.0f, (float)atof(cfg.getString("Video", "cockpit_fov", "45").c_str())));
    printf("[enhanced] cockpit: %s, fov %.0f\n", this->cockpit3d ? "3d (forward, left, right and rear view)" : "2d", this->cockpitFov);
    this->waterSplashEnabled = cfg.getString("Video", "water_splash", "0") == "1";
    printf("[enhanced] water splash: %s\n", this->waterSplashEnabled ? "on" : "off");
    this->debugKeys = getenv("NEOSC_DEBUG_KEYS") != nullptr;
    printf("[enhanced] debug keys (F12 damage stages): %s\n", this->debugKeys ? "on" : "off (set NEOSC_DEBUG_KEYS=1)");
}

// F12 steps the player's aircraft through damage stages (repaired, light, heavy), never destroyed
void EnhancedStrike::debugDamage() {
    static const float remaining[3] = {1.0f, 0.65f, 0.25f};
    const bool down = SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_F12] != 0;
    const bool pressed = down && !this->debugF12Down;
    this->debugF12Down = down;
    if (!pressed) {
        return;
    }
    this->debugDamageStage = (this->debugDamageStage + 1) % 3;
    const float keep = remaining[this->debugDamageStage];
    for (auto &sys : this->player_plane->object->entity->sysm) {
        for (auto &sub : sys.second) {
            this->player_plane->system_health[sys.first][sub.first] = (uint16_t)(sub.second * keep);
        }
    }
    printf("[enhanced] debug damage stage %d (remaining health %.2f)\n", this->debugDamageStage, keep);
}

// Chase (F2), target (F7) and orbit cameras of the camera director, and the missile camera (F8)
bool EnhancedStrike::orbitAllowed() const {
    if (this->camera_mode == View::MISSILE_CAM) {
        return !this->player_plane->weaps_object.empty() && this->player_plane->weaps_object.back() != nullptr;
    }
    if (this->camera_mode != View::CAM_DIRECTOR) {
        return false;
    }
    RSCameraType type = this->current_mission->camera_director->activeCameraType();
    return type == RSCameraType::RSCAM_CHAS || type == RSCameraType::RSCAM_TARG || type == RSCameraType::RSCAM_ROTA;
}

// The mouse position orbits the camera around the plane (screen edge = 180 degrees), wheel zooms, middle button resets
void EnhancedStrike::updateOrbit() {
    const float halfWidth = (float)std::max(1, Screen->logical_width / 2);
    const float halfHeight = (float)std::max(1, Screen->logical_height / 2);
    float mx = m_keyboard->getActionValue(CreateAction(InputAction::SIM_START, SimActionOfst::MOUSE_X));
    float my = m_keyboard->getActionValue(CreateAction(InputAction::SIM_START, SimActionOfst::MOUSE_Y));
    float nx = std::min(1.0f, std::max(-1.0f, (mx - Screen->logical_width / 2) / halfWidth));
    float ny = std::min(1.0f, std::max(-1.0f, (my - Screen->logical_height / 2) / halfHeight));
    this->orbitYaw = nx * (float)M_PI;
    this->orbitPitch = std::min(1.35f, std::max(-0.9f, (this->orbitInvertY ? -ny : ny) * 1.35f));
    int wheel = EventManager::getInstance().getMouseWheel();
    if (wheel != 0) {
        this->orbitZoom = std::min(3.0f, std::max(0.35f, this->orbitZoom * powf(0.9f, (float)wheel)));
    }
    if (m_keyboard->isActionJustPressed(InputAction::MOUSE_MIDDLE)) {
        this->orbitZoom = 1.0f;
    }
}

// [Video] cockpit = 3d: the forward and the left/right/rear views use the 3D virtual cockpit with the head fixed in that
// direction (the mouse keeps flying the aircraft). The explicit cockpit key keeps the classic free mouse look.
void EnhancedStrike::updateCockpit3D() {
    if (!this->cockpit3d || this->cockpit == nullptr) {
        return;
    }
    if (this->m_keyboard->isActionJustPressed(CreateAction(InputAction::SIM_START, SimActionOfst::VIEW_COCKPIT))) {
        this->cockpitLookFixed = false;
        return;
    }
    float yaw = -1.0f;
    switch (this->camera_mode) {
    case View::FRONT:
        yaw = 0.0f;
        break;
    case View::LEFT:
        yaw = 270.0f;
        break;
    case View::RIGHT:
        yaw = 90.0f;
        break;
    case View::REAR:
        yaw = 180.0f;
        break;
    default:
        break;
    }
    if (yaw >= 0.0f) {
        if (!this->cockpitLookFixed || this->cockpitLookYaw != yaw) {
            printf("[enhanced] 3D cockpit view, head yaw %.0f\n", yaw);
        }
        this->camera_mode = View::REAL;
        this->cockpit->is_3d_cockpit = true;
        this->cockpitLookFixed = true;
        this->cockpitLookYaw = yaw;
    } else if (this->camera_mode != View::REAL) {
        this->cockpitLookFixed = false;
    }
    if (this->camera_mode == View::REAL && this->cockpitLookFixed) {
        this->pilote_lookat.x = this->cockpitLookYaw;
        this->pilote_lookat.y = 0;
    }
}

void EnhancedStrike::afterInput() {
    if (this->debugKeys) {
        this->debugDamage();
    }
    this->updateCockpit3D();
    if ((int)this->camera_mode != this->orbitLastView) {
        this->orbitLastView = (int)this->camera_mode;
        this->orbitZoom = 1.0f;
    }
    const bool allowed = this->orbitAllowed();
    if (allowed != this->orbitActive) {
        printf("[enhanced] camera mouse orbit: %s (view %d)\n", allowed ? "on" : "off", (int)this->camera_mode);
        this->orbitActive = allowed;
    }
    if (!allowed) {
        this->orbitYaw = 0.0f;
        this->orbitPitch = 0.0f;
        return;
    }
    this->updateOrbit();
    // the mouse position steers the camera here, not the plane (a key press has already switched mouse_control off)
    if (this->mouse_control) {
        this->player_plane->control_stick_x = 0;
        this->player_plane->control_stick_y = 0;
    }
}

// Yaw about the world vertical, then raise/lower the camera, distance scaled by the zoom
Vector3D EnhancedStrike::orbitedPosition(Vector3D pivot, Vector3D position) const {
    Vector3D offset = position - pivot;
    const float distance = offset.Length();
    if (distance < 1e-3f) {
        return position;
    }
    Vector3D axis = offset * (-1.0f / distance);
    const float c = cosf(this->orbitYaw);
    const float s = sinf(this->orbitYaw);
    Vector3D turned(axis.x * c + axis.z * s, axis.y, -axis.x * s + axis.z * c);
    const float cp = cosf(this->orbitPitch);
    const float sp = sinf(this->orbitPitch);
    axis = Vector3D(turned.x * cp, turned.y * cp - sp, turned.z * cp);
    axis.Normalize();
    return pivot - axis * (distance * this->orbitZoom);
}

void EnhancedStrike::afterCameraPlaced() {
    this->splashDrawnThisFrame = false;
    if (this->waterSplashEnabled) {
        this->waterSplash.scan(this->current_mission);
    }
    if (!this->zoom_cockpit && (this->camera_mode == View::REAL || this->camera_mode == View::CONTROLLER_LOOK ||
                                this->camera_mode == View::EYE_ON_TARGET)) {
        Renderer.camera.fovy = this->cockpitFov;
        Renderer.camera.update();
    }
    if (!this->orbitActive || !this->orbitAllowed()) {
        return;
    }
    Vector3D pivot;
    if (this->camera_mode == View::MISSILE_CAM) {
        this->player_plane->weaps_object.back()->GetPosition(&pivot);
    } else {
        SCMissionActors *subject = this->current_mission->camera_director->activeSubject();
        if (subject == nullptr) {
            return;
        }
        if (subject->plane != nullptr) {
            pivot = Vector3D(subject->plane->x, subject->plane->y, subject->plane->z);
        } else {
            pivot = subject->object->position;
        }
    }
    Point3D current = this->camera->getPosition();
    Vector3D position = this->orbitedPosition(pivot, Vector3D(current.x, current.y, current.z));
    this->camera->SetPosition(&position);
    if (this->camera_mode == View::MISSILE_CAM) {
        this->camera->lookAt(&pivot);
    } else {
        Vector3D up = this->current_mission->camera_director->up();
        this->camera->lookAt(&pivot, &up);
    }
    if (this->orbitTraceFrames > 0) {
        printf("[enhanced] orbit: pivot (%.1f %.1f %.1f) camera (%.1f %.1f %.1f) yaw %.2f pitch %.2f zoom %.2f\n", pivot.x, pivot.y,
               pivot.z, position.x, position.y, position.z, this->orbitYaw, this->orbitPitch, this->orbitZoom);
        this->orbitTraceFrames--;
    }
}

void EnhancedStrike::afterCockpitSceneRendered() {
    if (this->player_plane == nullptr || this->player_plane->object == nullptr || this->player_plane->object->entity == nullptr) {
        return;
    }
    float view[16];
    float proj[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, view);
    glGetFloatv(GL_PROJECTION_MATRIX, proj);
    const Vector3D &light = SCRenderer::getInstance().object_light_direction;
    this->cockpitGlass.draw(view, proj, vec3(light.x, light.y, light.z), this->damageLevel(this->player_plane));
}
