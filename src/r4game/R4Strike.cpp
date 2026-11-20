//
//  R4Strike.cpp
//  libRealSpace
//
#include "R4Strike.h"
#include "R4Renderer.h"

#include <glm/gtc/type_ptr.hpp>

#include <algorithm>

namespace {

// SCStrike's RENDER_DISTANCE: the actors farther from the player are not drawn
constexpr float kRenderDistance = 40000.0f;

r4::Mat4 toMat4(Matrix &m) {
    return glm::make_mat4(&m.v[0][0]);
}

// the matrix of SCRenderer::drawModelWithChilds and drawModel(object, position, orientation): angles in radians, about Y, Z then X
r4::Mat4 radianPlacement(Vector3D position, Vector3D orientation) {
    Matrix m;
    m.Clear();
    m.Identity();
    m.translateM(position.x, position.y, position.z);
    m.rotateM(orientation.x, 0.0f, 1.0f, 0.0f);
    m.rotateM(orientation.y, 0.0f, 0.0f, 1.0f);
    m.rotateM(orientation.z, 1.0f, 0.0f, 0.0f);
    return toMat4(m);
}

// the matrix of SCRenderer::drawModel(object, lod, position, orientation): glRotatef in degrees, about Y, Z then X
r4::Mat4 degreePlacement(Vector3D position, Vector3D orientation) {
    r4::Mat4 m = r4::translate({position.x, position.y, position.z});
    m = m * r4::rotate(r4::radians(orientation.x), {0, 1, 0});
    m = m * r4::rotate(r4::radians(orientation.y), {0, 0, 1});
    m = m * r4::rotate(r4::radians(orientation.z), {1, 0, 0});
    return m;
}

std::shared_ptr<r4::scene::Group> group(const char *name) {
    auto g = r4::scene::make<r4::scene::Group>();
    g->setName(name);
    return g;
}

R4Renderer *activeRenderer() {
    R4Renderer *r = R4Renderer::current();
    if (r && r->active()) {
        return r;
    }
    return nullptr;
}

} // namespace

R4Strike::R4Strike() {
    world_ = group("flight (libRealSpace axes)");
    terrain_ = group("terrain");
    terrain_->setMask(LAYER_TERRAIN);
    actors_ = group("actors");
    actors_->setMask(LAYER_ACTORS | LAYER_PLAYER);
    shots_ = group("simulated objects");
    shots_->setMask(LAYER_SHOTS);
    world_->addChild(terrain_);
    world_->addChild(actors_);
    world_->addChild(shots_);
}

R4Strike::~R4Strike() = default;

void R4Strike::setModel(r4::scene::Group &node, RSEntity *entity) {
    node.removeAllChildren();
    R4Renderer *r = activeRenderer();
    if (!r || !entity || entity->vertices.empty()) {
        return;
    }
    if (r4::scene::NodePtr model = r->modelOf(entity)) {
        node.addChild(model);
    }
}

void R4Strike::useArea() {
    R4Renderer *r = activeRenderer();
    if (!r || this->area == area_) {
        return;
    }
    area_ = this->area;
    terrain_->removeAllChildren();
    actors_->removeAllChildren();
    shots_->removeAllChildren();
    planes_.clear();
    objects_.clear();
    shotPlaces_.clear();
    // no tree nor building on the objects of the mission
    std::vector<r4::Vec3> keepOut;
    for (SCMissionActors *actor : this->current_mission->actors) {
        if (actor->object == nullptr || actor->object->entity == nullptr) {
            continue;
        }
        BoudingBox *bb = actor->object->entity->GetBoudingBpx();
        float radius = std::max({std::abs(bb->min.x), std::abs(bb->max.x), std::abs(bb->min.z), std::abs(bb->max.z)});
        keepOut.push_back({actor->object->position.x, actor->object->position.z, std::max(radius * 1.2f, 40.0f)});
    }
    r->useArea(area_, &keepOut);
    for (const r4::scene::NodePtr &node : {r->terrain(), r->overlay(), r->forests(), r->buildings()}) {
        if (node) {
            terrain_->addChild(node);
        }
    }
}

void R4Strike::runFrame() {
    // the area before the view of the target camera, which also draws it
    useArea();
    SCStrike::runFrame();
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

bool R4Strike::inGraph(SCSimulatedObject *shot) {
    if (!shot->obj || shot->obj->vertices.empty()) {
        return false;
    }
    if (dynamic_cast<DumbSimulatedObject *>(shot) || dynamic_cast<EjectionSeatSimulatedObject *>(shot) || dynamic_cast<EjectedPilotSimulatedObject *>(shot)) {
        return false;
    }
    return true;
}

void R4Strike::placePlane(SCPlane *plane, bool shown, uint32_t layer) {
    if (!plane->object || !plane->object->entity) {
        return;
    }
    PlanePlace &place = planes_[plane];
    if (!place.node) {
        place.node = group("plane");
        place.body = group("body");
        place.wheels = group("wheels");
        place.afterburner = group("afterburner");
        place.weapons = group("weapons");
        place.node->addChild(place.body);
        place.node->addChild(place.wheels);
        place.node->addChild(place.afterburner);
        place.node->addChild(place.weapons);
        actors_->addChild(place.node);
    }
    place.seen = true;
    place.node->setMask(shown ? layer : 0u);
    if (!shown) {
        return;
    }
    RSEntity *entity = plane->object->entity;
    if (place.body->children().empty()) {
        setModel(*place.body, entity);
    }
    Vector3D orientation = {tenthOfDegreeToRad(plane->yaw + 900), tenthOfDegreeToRad(plane->pitch), -tenthOfDegreeToRad(plane->roll)};
    place.node->setMatrix(radianPlacement({plane->x, plane->y, plane->z}, orientation));

    const int wheel = plane->getWheelIndex();
    RSEntity *wheels = nullptr;
    if (wheel != 0 && (int)entity->chld.size() > wheel) {
        wheels = entity->chld[wheel]->objct;
    }
    if (wheels != place.wheel) {
        setModel(*place.wheels, wheels);
        place.wheel = wheels;
    }

    const int thrust = plane->getThrust();
    const bool burning = thrust > 50 && !entity->chld.empty();
    place.afterburner->setMask(burning ? 0xffffffffu : 0u);
    if (burning) {
        if (place.afterburner->children().empty()) {
            setModel(*place.afterburner, entity->chld[0]->objct);
        }
        const RSEntity::CHLD *c = entity->chld[0];
        place.afterburner->setMatrix(r4::translate({c->z, c->y, c->x}) * r4::scale({1 + thrust / 100.0f, 1, 1}));
    }

    // the stores under the hard points, laid out as SCPlane::Render
    std::vector<RSEntity *> loaded;
    std::vector<r4::Vec3> positions;
    for (SCWeaponLoadoutHardPoint *hp : plane->weaps_load) {
        if (!hp || hp->hpts_type == 0) {
            continue;
        }
        const float decy = hp->hpts_type == 4 ? 0.0f : 0.5f;
        const Vector3D path[6] = {{0, -2 * decy, 0}, {decy, -decy, 0}, {-decy, -decy, 0}, {0, -2 * decy, -2 * decy}, {decy, -decy, -2 * decy}, {-decy, -decy, -2 * decy}};
        Vector3D position = hp->position;
        for (int i = 0; i < hp->nb_weap && i < 6; i++) {
            positions.push_back({position.z / 250 + path[i].z, position.y / 250 + path[i].y, -position.x / 250 + path[i].x});
            loaded.push_back(hp->objct);
            position.y -= 0.5f;
        }
    }
    if (loaded != place.loaded) {
        place.weapons->removeAllChildren();
        for (RSEntity *store : loaded) {
            auto slot = group("store");
            setModel(*slot, store);
            place.weapons->addChild(slot);
        }
        place.loaded = loaded;
    }
    for (size_t i = 0; i < positions.size(); i++) {
        place.weapons->children()[i]->setMatrix(r4::translate(positions[i]));
    }
}

void R4Strike::placeObject(SCMissionActors *actor) {
    Place &place = objects_[actor];
    if (!place.node) {
        place.node = group("object");
        actors_->addChild(place.node);
    }
    place.seen = true;
    MISN_PART *part = actor->object;
    if (place.entity != part->entity) {
        setModel(*place.node, part->entity);
        place.entity = part->entity;
    }
    Vector3D orientation = {360.0f - (float)part->azymuth + 90.0f, (float)part->pitch, -(float)part->roll};
    place.node->setMatrix(degreePlacement(part->position, orientation));
    place.node->setMask(LAYER_ACTORS);
}

void R4Strike::placeShot(SCSimulatedObject *shot) {
    shotsShown_.push_back(shot);
    if (!inGraph(shot)) {
        return;
    }
    Place &place = shotPlaces_[shot];
    if (!place.node) {
        place.node = group("simulated object");
        shots_->addChild(place.node);
    }
    place.seen = true;
    if (place.entity != shot->obj) {
        setModel(*place.node, shot->obj);
        place.entity = shot->obj;
    }
    place.node->setMatrix(radianPlacement({shot->x, shot->y, shot->z}, {shot->azimuthf, shot->elevationf, 0.0f}));
}

template <class T> void R4Strike::dropUnseen(std::unordered_map<const void *, T> &places, r4::scene::Group &layer) {
    for (auto it = places.begin(); it != places.end();) {
        if (!it->second.seen) {
            layer.removeChild(it->second.node.get());
            it = places.erase(it);
        } else {
            it->second.seen = false;
            it++;
        }
    }
}

void R4Strike::app() {
    useArea();
    actorsShown_.clear();
    shotsShown_.clear();
    for (SCMissionActors *actor : this->current_mission->actors) {
        if (actor->is_hidden) {
            continue;
        }
        if (actor->plane != nullptr) {
            if (actor->plane == this->player_plane) {
                continue;
            }
            Vector3D d = {actor->plane->x - this->player_plane->x, actor->plane->y - this->player_plane->y, actor->plane->z - this->player_plane->z};
            if (d.Length() > kRenderDistance) {
                continue;
            }
            placePlane(actor->plane, true, LAYER_ACTORS);
            actorsShown_.push_back(actor);
            if (actor->plane->object->alive) {
                for (SCSimulatedObject *shot : actor->plane->weaps_object) {
                    placeShot(shot);
                }
            }
        } else if (actor->object->entity != nullptr) {
            Vector3D d = {actor->object->position.x - this->player_plane->x, actor->object->position.y - this->player_plane->y, actor->object->position.z - this->player_plane->z};
            if (d.Length() > kRenderDistance) {
                continue;
            }
            placeObject(actor);
            actorsShown_.push_back(actor);
            for (SCSimulatedObject *shot : actor->weapons_shooted) {
                if (shot->alive) {
                    placeShot(shot);
                }
            }
        }
    }
    placePlane(this->player_plane, playerPlaneShown(), LAYER_PLAYER);
    for (SCSimulatedObject *shot : this->player_plane->weaps_object) {
        placeShot(shot);
    }
    for (SCSimulatedObject *shot : this->simulated_objects) {
        if (shot->alive) {
            placeShot(shot);
        }
    }
    // the planes out of sight keep their node, hidden, until they leave the mission
    for (auto &[key, place] : planes_) {
        if (!place.seen) {
            place.node->setMask(0u);
        }
    }
    for (SCMissionActors *actor : this->current_mission->actors) {
        if (actor->plane != nullptr) {
            auto it = planes_.find(actor->plane);
            if (it != planes_.end()) {
                it->second.seen = true;
            }
        }
    }
    auto player = planes_.find(this->player_plane);
    if (player != planes_.end()) {
        player->second.seen = true;
    }
    dropUnseen(planes_, *actors_);
    dropUnseen(objects_, *actors_);
    dropUnseen(shotPlaces_, *shots_);
}

void R4Strike::afterCameraPlaced() {
    if (!activeRenderer() || this->show_bbox) {
        return;
    }
    // SCStrike::renderWorld moves them while it draws them; here it is done once a frame, before the APP step
    for (SCSimulatedObject *shot : this->simulated_objects) {
        if (shot->alive) {
            shot->Simulate(this->player_plane->tps);
        }
    }
    app();
}

void R4Strike::renderWorld() {
    R4Renderer *r = activeRenderer();
    if (!r || this->show_bbox) {
        SCStrike::renderWorld();
        return;
    }
    // the sky, the graph of the flight and the effects of the game go in the same pass of the renderer
    r->beginWorld(this->area);
    r->addScene(world_);

    // the effects, in the order of SCStrike::renderWorld, on the depth of the graph
    for (SCMissionActors *actor : actorsShown_) {
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
    for (SCSimulatedObject *shot : shotsShown_) {
        if (!inGraph(shot)) {
            shot->Render();
        } else if (!dynamic_cast<GunSimulatedObject *>(shot)) {
            shot->smokeSet().render(camera->getPosition());
        }
    }
}

void R4Strike::renderPlayerPlane() {
    if (!activeRenderer() || this->show_bbox) {
        SCStrike::renderPlayerPlane();
        return;
    }
    if (!this->player_plane->object->alive) {
        this->player_plane->RenderSmoke();
    }
    this->afterPlaneRendered(this->player_plane, Vector3D(this->player_plane->x, this->player_plane->y, this->player_plane->z), this->player_plane->getThrust());
}
