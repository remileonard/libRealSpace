//
//  R4Strike.cpp
//  libRealSpace
//
#include "R4Strike.h"
#include "R4Renderer.h"
#include "R4Screen.h"

#include <algorithm>

namespace {

// SCStrike's RENDER_DISTANCE: the effects of the actors farther from the player are not drawn
constexpr float kRenderDistance = 40000.0f;

R4Renderer *activeRenderer() {
    R4Renderer *r = R4Renderer::current();
    if (r && r->active()) {
        return r;
    }
    return nullptr;
}

} // namespace

// [Sound] engine_volume (0 to 128, as R4ConfigPanel; -1: as SCStrike, 5 of 128): the volume of the engine channel of the player
R4Strike::R4Strike() {
    int volume = (std::min)(128, Config::getInstance().getInt("Sound", "engine_volume", 100));
    if (volume >= 0) {
        this->engine_channel_volume = volume;
    }
    printf("R4Strike: engine sound volume %d of 128\n", this->engine_channel_volume);
}

R4Strike::~R4Strike() = default;

SCMission *R4Strike::createMission(char const *missionName) {
    return new R4Mission(missionName, &object_cache);
}

R4Mission *R4Strike::mission() const {
    return dynamic_cast<R4Mission *>(this->current_mission);
}

void R4Strike::runFrame() {
    const Uint64 start = SDL_GetPerformanceCounter();
    const double flushBefore = R4Screen::counters().flushMs;
    R4Renderer *r = activeRenderer();
    if (r && this->cockpit && this->cockpit->cockpit) {
        r->noShadows(this->cockpit->cockpit->REAL.OBJS);
    }
    SCStrike::runFrame();
    R4Screen::counters().gameMs += R4Screen::msSince(start);
    R4Screen::counters().flightFlushMs += R4Screen::counters().flushMs - flushBefore;
}

bool R4Strike::playerPlaneShown() const {
    switch (this->camera_mode) {
    case View::CAM_DIRECTOR:
    case View::MISSILE_CAM:
    case View::TARGET:
    case View::OBJECT:
    case View::AUTO_PILOT:
    case View::CRASH_VIEW_P3:
    case View::FOLLOW:
    case View::EJECT_VIEW_P2:
    case View::EJECT_VIEW_P3:
        return true;
    default:
        return false;
    }
}

void R4Strike::renderScene(int32_t viewportW, int32_t viewportH, float projVerticalOffset, bool forceVirtualCockpit) {
    const Uint64 start = SDL_GetPerformanceCounter();
    this->viewport_h_ = (std::max)(1, viewportH);
    SCStrike::renderScene(viewportW, viewportH, projVerticalOffset, forceVirtualCockpit);
    R4Screen::counters().sceneMs += R4Screen::msSince(start);
}

void R4Strike::afterCameraPlaced() {
    R4Mission *m = mission();
    if (!activeRenderer() || !m || this->show_bbox) {
        return;
    }
    // the APP step runs at each frame, also in pause: SCMission::update does not
    m->app(this->simulated_objects, this->player_plane->tps);
}

void R4Strike::renderWorld() {
    R4Renderer *r = activeRenderer();
    R4Mission *m = mission();
    if (!r || !m || this->show_bbox) {
        SCStrike::renderWorld();
        return;
    }
    // the sky, the graph of the mission and the effects of the game go in the same pass of the renderer
    r->beginWorld(this->area);
    r->addScene(m->world());
    // the plane of the player is in the graph; this view hides it in the cockpit
    r->setCameraMask(~(uint32_t)R4Mission::LAYER_TRIGGERS & (playerPlaneShown() ? 0xffffffffu : ~(uint32_t)R4Mission::LAYER_PLAYER));

    // the effects, in the order of SCStrike::renderWorld, on the depth of the graph, up to its RENDER_DISTANCE
    for (SCMissionActors *actor : this->current_mission->actors) {
        if (actor->is_hidden || (actor->plane == nullptr && actor->object->entity == nullptr) || actor->plane == this->player_plane) {
            continue;
        }
        const Vector3D at = actor->plane != nullptr ? Vector3D(actor->plane->x, actor->plane->y, actor->plane->z) : actor->object->position;
        const Vector3D d = {at.x - this->player_plane->x, at.y - this->player_plane->y, at.z - this->player_plane->z};
        if (d.Length() > kRenderDistance) {
            continue;
        }
        if (actor->plane != nullptr) {
            if (!actor->plane->object->alive) {
                actor->plane->RenderSmoke();
            }
            this->afterPlaneRendered(actor->plane, Vector3D(actor->plane->x, actor->plane->y, actor->plane->z), actor->plane->getThrust());
        } else if (actor->smoke != nullptr) {
            actor->smoke->render(camera->getPosition());
        }
    }
    auto &explosions = this->current_mission->explosions;
    auto finished = [](const auto &e) {
        return e->is_finished;
    };
    explosions.erase(std::remove_if(explosions.begin(), explosions.end(), finished), explosions.end());
    for (auto expl : explosions) {
        expl->render();
    }
    Vector3D centerPoint = camera->getPosition() + camera->getForward() * 10.0f;
    r->drawPoint(centerPoint, {0.0f, 1.0f, 0.0f}, {0, 0, 0}, {0.0f, 0.0f, 0.0f});
    r->drawPoint(this->cockpit->targetImpactPointWorld, {0.0f, 1.0f, 1.0f}, {0, 0, 0}, {0.0f, 0.0f, 0.0f});
    for (SCSimulatedObject *shot : m->shots()) {
        if (!R4Mission::inGraph(shot)) {
            shot->Render();
        } else if (!dynamic_cast<GunSimulatedObject *>(shot)) {
            shot->smokeSet().render(camera->getPosition());
        }
    }
    drawTargetSquare();
}

void R4Strike::renderPlayerPlane() {
    if (!activeRenderer() || !mission() || this->show_bbox) {
        SCStrike::renderPlayerPlane();
        return;
    }
    if (!this->player_plane->object->alive) {
        this->player_plane->RenderSmoke();
    }
    this->afterPlaneRendered(this->player_plane, Vector3D(this->player_plane->x, this->player_plane->y, this->player_plane->z), this->player_plane->getThrust());
}

void R4Strike::drawTargetSquare() {
    R4Renderer *r = activeRenderer();
    if (this->cockpit == nullptr || this->cockpit->target == nullptr) {
        return;
    }
    const Vector3D at = this->cockpit->target->position;
    const Vector3D eye = camera->getPosition();
    const Vector3D fwd = camera->getForward();
    const float dx = at.x - eye.x, dy = at.y - eye.y, dz = at.z - eye.z;
    const float depth = dx * fwd.x + dy * fwd.y + dz * fwd.z;
    if (depth <= 0.0f) {
        return;
    }
    // colonnes de la matrice de vue : axes droite et haut de la caméra
    const Matrix *v = camera->getViewMatrix();
    const Vector3D right = {v->v[0][0], v->v[1][0], v->v[2][0]};
    const Vector3D up = {v->v[0][1], v->v[1][1], v->v[2][1]};
    const float s = 8.0f * depth * tanf(degreeToRad(Renderer.camera.fovy) * 0.5f) / (this->viewport_h_ * 0.5f);

    auto corner = [&](float a, float b) {
        return Vector3D{at.x + (right.x * a + up.x * b) * s,
                        at.y + (right.y * a + up.y * b) * s,
                        at.z + (right.z * a + up.z * b) * s};
    };
    const Vector3D c[4] = {corner(-1, -1), corner(1, -1), corner(1, 1), corner(-1, 1)};
    const Vector3D green = {0.0f, 1.0f, 0.0f};
    for (int i = 0; i < 4; i++) {
        r->drawLine(c[i], c[(i + 1) % 4], green, {0, 0, 0}, {0, 0, 0});
    }
}