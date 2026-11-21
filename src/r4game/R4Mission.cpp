//
//  R4Mission.cpp
//  libRealSpace
//
#include "R4Mission.h"
#include "R4Renderer.h"
#include "R4Screen.h"
#include "R4World.h"

#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cstdio>

namespace {

// SCStrike's RENDER_DISTANCE: the actors farther from the eye are not drawn
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

// SCStrike's RENDER_DISTANCE as a LOD under the node of an actor: the renderer draws its children up to this distance from the eye
std::shared_ptr<r4::scene::LOD> rangeOf(r4::scene::Group &node) {
    auto lod = r4::scene::make<r4::scene::LOD>();
    lod->setName("render distance");
    lod->setRanges({0, kRenderDistance});
    node.addChild(lod);
    return lod;
}

// the height of the origin of a plane above the bottom of its body and of its wheels down (the last child of the wheels, as
// SCPlane::wheel_index at 5): the physics puts its origin on groundlevel
float clearanceOf(const RSEntity *entity) {
    float lowest = 0.0f;
    for (const Point3D &v : entity->vertices) {
        lowest = std::min(lowest, v.y);
    }
    const size_t wheels = std::min<size_t>(5, entity->chld.size() > 0 ? entity->chld.size() - 1 : 0);
    if (wheels > 0 && entity->chld[wheels]->objct != nullptr) {
        for (const Point3D &v : entity->chld[wheels]->objct->vertices) {
            lowest = std::min(lowest, v.y);
        }
    }
    return -lowest;
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

R4Mission::R4Mission(std::string missionName, std::unordered_map<std::string, RSEntity *> *objCache) : SCMission(missionName, objCache) {
    world_ = group("mission (libRealSpace axes)");
    terrain_ = group("terrain");
    terrain_->setMask(LAYER_TERRAIN);
    // no callback in the terrain: the APP traversal does not go into its thousands of nodes
    terrain_->setAppMask(0);
    actors_ = group("actors");
    actors_->setMask(LAYER_ACTORS | LAYER_PLAYER);
    shots_ = group("simulated objects");
    shots_->setMask(LAYER_SHOTS);
    world_->addChild(terrain_);
    world_->addChild(actors_);
    world_->addChild(shots_);
    planeLayer_ = group("planes");
    actors_->addChild(planeLayer_);
    if (this->mission != nullptr) {
        const MISN &data = this->mission->mission_data;
        for (size_t a = 0; a < data.areas.size(); a++) {
            areas_.push_back(group(("area " + std::to_string(a) + ": " + data.areas[a]->AreaName).c_str()));
        }
        for (size_t s = 0; s < data.scenes.size(); s++) {
            for (uint16_t cast : data.scenes[s]->cast) {
                if (cast < data.parts.size()) {
                    sceneOfPart_.emplace(data.parts[cast]->id, (int)s);
                }
            }
        }
    }
    areas_.push_back(group("out of the areas"));
    for (const auto &area : areas_) {
        actors_->addChild(area);
    }
    triggers_ = group("triggers of the areas");
    triggers_->setMask(LAYER_TRIGGERS);
    triggers_->setAppMask(0);
    world_->addChild(triggers_);
    if (this->mission != nullptr) {
        for (size_t a = 0; a < this->mission->mission_data.areas.size(); a++) {
            addTrigger(a);
        }
    }
    // the ground of the game is the one that the renderer draws: RSArea::getY (physics, AI, scenes) takes the LVL0 triangles
    if (RSArea *area = this->area) {
        area->ground = [area](float x, float z, float &height) {
            return r4bridge::groundHeight(area, x, z, height);
        };
        // SCMission::loadMission put the origins of the planes on the ground of the blocks: their wheels on this ground, with its rule
        for (SCMissionActors *actor : this->actors) {
            if (actor->plane == nullptr || actor->object == nullptr || actor->object->entity == nullptr) {
                continue;
            }
            actor->plane->ground_clearance = clearanceOf(actor->object->entity);
            const float rest = area->getY(actor->plane->x, actor->plane->z) + actor->plane->ground_clearance;
            if (std::abs(rest - actor->plane->y) <= 10 || actor->plane->y < rest) {
                actor->plane->y = rest;
                actor->object->position.y = rest;
            }
        }
    }
    // SCMission loads the mission in its constructor: the graph of its area is made just after
    useArea();
    // the models of the actors, of their wheels and afterburners and of their stores, before the flight
    if (R4Renderer *r = activeRenderer()) {
        std::vector<RSEntity *> entities;
        for (SCMissionActors *actor : this->actors) {
            RSEntity *entity = actor->object != nullptr ? actor->object->entity : nullptr;
            if (entity == nullptr) {
                continue;
            }
            entities.push_back(entity);
            for (const RSEntity::CHLD *child : entity->chld) {
                entities.push_back(child->objct);
            }
            if (actor->plane != nullptr) {
                for (SCWeaponLoadoutHardPoint *hp : actor->plane->weaps_load) {
                    if (hp != nullptr) {
                        entities.push_back(hp->objct);
                    }
                }
            }
        }
        const Uint64 start = SDL_GetPerformanceCounter();
        r->preloadModels(entities);
        std::printf("[R4Mission] the models of %zu entities of the actors in %.0f ms\n", entities.size(), R4Screen::msSince(start));
    }
}

R4Mission::~R4Mission() = default;

void R4Mission::setModel(r4::scene::Node &node, RSEntity *entity) {
    node.removeAllChildren();
    R4Renderer *r = activeRenderer();
    if (!r || !entity || entity->vertices.empty()) {
        return;
    }
    if (r4::scene::NodePtr model = r->modelOf(entity)) {
        node.addChild(model);
    }
}

void R4Mission::useArea() {
    R4Renderer *r = activeRenderer();
    if (!r || this->area == area_) {
        return;
    }
    area_ = this->area;
    terrain_->removeAllChildren();
    // no tree nor building on the objects of the mission
    std::vector<r4::Vec3> keepOut;
    for (SCMissionActors *actor : this->actors) {
        // without vertices, the box of RSEntity::CalcBoundingBox stays at FLT_MAX and would keep the whole map out
        if (actor->object == nullptr || actor->object->entity == nullptr || actor->object->entity->vertices.empty()) {
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

void R4Mission::addTrigger(size_t index) {
    const AREA *area = this->mission->mission_data.areas[index];
    auto trigger = r4::scene::make<r4::scene::Trigger>();
    trigger->setName("area " + std::to_string(index) + ": " + area->AreaName);
    // its space: Z up (the axis of the tube), from the position of the area; libRealSpace's -Z is the north
    trigger->setMatrix(r4::translate({area->position.x, area->position.y, area->position.z}) * r4::axes::toYUpMatrix());
    if (area->AreaType == 'S') {
        trigger->setSphere(area->radius);
    } else if (area->AreaType == 'C') {
        trigger->setCylinder(area->radius, area->height);
    } else if (area->AreaType == 'B') {
        trigger->setBox({{-area->west, -area->south, 0}, {area->east, area->north, area->height}});
    } else {
        return;
    }
    trigger->onEnter = [this, index](uint32_t actor) {
        publishArea(actor, index, true);
    };
    trigger->onLeave = [this, index](uint32_t actor) {
        publishArea(actor, index, false);
    };
    triggers_->addChild(trigger);
}

void R4Mission::publishArea(uint32_t actor, size_t area, bool entered) {
    if (actor >= this->actors.size()) {
        return;
    }
    AREA *a = this->mission->mission_data.areas[area];
    std::printf("[R4Mission] %s %s area %zu %s\n", this->actors[actor]->actor_name.c_str(), entered ? "enters" : "leaves", area, a->AreaName);
    if (entered) {
        auto event = std::make_unique<MissionEventAreaEntered>();
        event->actor = this->actors[actor];
        event->area = a;
        event->mission = this;
        this->messageBus.publish(std::move(event));
    } else {
        auto event = std::make_unique<MissionEventAreaLeft>();
        event->actor = this->actors[actor];
        event->area = a;
        event->mission = this;
        this->messageBus.publish(std::move(event));
    }
}

bool R4Mission::inGraph(SCSimulatedObject *shot) {
    if (!shot->obj || shot->obj->vertices.empty()) {
        return false;
    }
    if (dynamic_cast<DumbSimulatedObject *>(shot) || dynamic_cast<EjectionSeatSimulatedObject *>(shot) || dynamic_cast<EjectedPilotSimulatedObject *>(shot)) {
        return false;
    }
    return true;
}

void R4Mission::placePlane(SCPlane *plane, SCMissionActors *actor) {
    if (!plane->object || !plane->object->entity) {
        return;
    }
    PlanePlace &place = planes_[plane];
    if (!place.node) {
        place.node = group("plane");
        place.range = rangeOf(*place.node);
        place.body = group("body");
        place.wheels = group("wheels");
        place.afterburner = group("afterburner");
        place.weapons = group("weapons");
        // the LOD draws one child: the parts in one group
        auto parts = group("parts");
        parts->addChild(place.body);
        parts->addChild(place.wheels);
        parts->addChild(place.afterburner);
        parts->addChild(place.weapons);
        place.range->addChild(parts);
        planeLayer_->addChild(place.node);
        place.parent = planeLayer_.get();
        PlanePlace *p = &place;
        place.node->setAppCallbacks([this, p, plane, actor](r4::scene::Node &) {
            updatePlane(*p, plane, actor);
        });
    }
    place.seen = true;
}

void R4Mission::updatePlane(PlanePlace &place, SCPlane *plane, SCMissionActors *actor) {
    // the player's plane is always in the graph: the view hides it in the cockpit
    const uint32_t layer = actor == nullptr ? (uint32_t)LAYER_PLAYER : actor->is_hidden ? 0u : (uint32_t)LAYER_ACTORS;
    place.node->setMask(layer);
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

void R4Mission::placeObject(SCMissionActors *actor) {
    Place &place = objects_[actor];
    MISN_PART *part = actor->object;
    if (!place.node) {
        place.node = group("object");
        place.range = rangeOf(*place.node);
        // its height in the game, once: its lowest vertex on the LVL0 triangles that the renderer draws, not RSArea::getY
        Vector3D orientation = {360.0f - (float)part->azymuth + 90.0f, (float)part->pitch, -(float)part->roll};
        part->position.y = r4bridge::onGround(this->area, part->entity, degreePlacement(part->position, orientation), false)[3].y;
        place.parent = &objectParent(part);
        place.parent->addChild(place.node);
        Place *p = &place;
        place.node->setAppCallbacks([this, p, actor](r4::scene::Node &) {
            updateObject(*p, actor);
        });
    }
    place.seen = true;
}

void R4Mission::updateObject(Place &place, SCMissionActors *actor) {
    MISN_PART *part = actor->object;
    if (place.entity != part->entity) {
        setModel(*place.range, part->entity);
        place.entity = part->entity;
    }
    Vector3D orientation = {360.0f - (float)part->azymuth + 90.0f, (float)part->pitch, -(float)part->roll};
    place.node->setMatrix(degreePlacement(part->position, orientation));
    place.node->setMask(LAYER_ACTORS);
}

void R4Mission::placeShot(SCSimulatedObject *shot) {
    shotList_.push_back(shot);
    if (!inGraph(shot)) {
        return;
    }
    Place &place = shotPlaces_[shot];
    if (!place.node) {
        place.node = group("simulated object");
        shots_->addChild(place.node);
        Place *p = &place;
        place.node->setAppCallbacks([this, p, shot](r4::scene::Node &) {
            updateShot(*p, shot);
        });
    }
    place.seen = true;
}

void R4Mission::updateShot(Place &place, SCSimulatedObject *shot) {
    if (place.entity != shot->obj) {
        setModel(*place.node, shot->obj);
        place.entity = shot->obj;
    }
    place.node->setMatrix(radianPlacement({shot->x, shot->y, shot->z}, {shot->azimuthf, shot->elevationf, 0.0f}));
}

r4::scene::Node &R4Mission::objectParent(const MISN_PART *part) {
    int area = (int)areas_.size() - 1;
    if (this->mission != nullptr) {
        const std::vector<AREA *> &areas = this->mission->mission_data.areas;
        for (size_t a = 0; a + 1 < areas_.size() && a < areas.size(); a++) {
            // on the ground, under or above the volume of the area
            Vector3D at = part->position;
            at.y = areas[a]->position.y;
            if (areas[a]->contains(at)) {
                area = (int)a;
                break;
            }
        }
    }
    auto scene = sceneOfPart_.find(part->id);
    if (scene == sceneOfPart_.end()) {
        return *areas_[area];
    }
    std::shared_ptr<r4::scene::Switch> &node = sceneNodes_[{area, scene->second}];
    if (!node) {
        node = r4::scene::make<r4::scene::Switch>();
        node->setName("scene " + std::to_string(scene->second));
        // a scene shows its objects once it has been activated
        const MISN_SCEN *data = this->mission->mission_data.scenes[scene->second];
        node->setAppCallbacks([data](r4::scene::Node &n) {
            static_cast<r4::scene::Switch &>(n).setValue(data->has_been_activated ? r4::scene::Switch::kAll : r4::scene::Switch::kNone);
        });
        areas_[area]->addChild(node);
    }
    return *node;
}

template <class T> void R4Mission::dropUnseen(std::unordered_map<const void *, T> &places, r4::scene::Group &layer) {
    for (auto it = places.begin(); it != places.end();) {
        if (!it->second.seen) {
            (it->second.parent ? *it->second.parent : layer).removeChild(it->second.node.get());
            it = places.erase(it);
        } else {
            it->second.seen = false;
            it++;
        }
    }
}

void R4Mission::app(const std::vector<SCSimulatedObject *> &shots, float tps) {
    const Uint64 start = SDL_GetPerformanceCounter();
    useArea();
    // SCStrike::renderWorld moved them while it drew them; here once a frame, before their nodes
    for (SCSimulatedObject *shot : shots) {
        if (shot->alive) {
            shot->Simulate(tps);
        }
    }
    shotList_.clear();
    SCPlane *player = this->player != nullptr ? this->player->plane : nullptr;
    if (player == nullptr) {
        return;
    }
    // the nodes of what appeared, with their callbacks, and without what is gone; every actor: the view chooses what it draws
    for (SCMissionActors *actor : this->actors) {
        if (actor->plane != nullptr) {
            if (actor->plane == player) {
                continue;
            }
            placePlane(actor->plane, actor);
            if (!actor->is_hidden && actor->plane->object->alive) {
                for (SCSimulatedObject *shot : actor->plane->weaps_object) {
                    placeShot(shot);
                }
            }
        } else if (actor->object->entity != nullptr && !actor->is_hidden) {
            placeObject(actor);
            for (SCSimulatedObject *shot : actor->weapons_shooted) {
                if (shot->alive) {
                    placeShot(shot);
                }
            }
        }
    }
    placePlane(player, nullptr);
    for (SCSimulatedObject *shot : player->weaps_object) {
        placeShot(shot);
    }
    for (SCSimulatedObject *shot : shots) {
        if (shot->alive) {
            placeShot(shot);
        }
    }
    dropUnseen(planes_, *planeLayer_);
    dropUnseen(objects_, *actors_);
    dropUnseen(shotPlaces_, *shots_);
    // the APP traversal: each node takes the state of its object of the game
    r4::scene::app(*world_);
    // the planes in the triggers of the areas, by their index in the actors
    std::vector<r4::scene::Tracker> trackers;
    for (size_t i = 0; i < this->actors.size(); i++) {
        SCPlane *plane = this->actors[i]->plane;
        if (plane != nullptr && !this->actors[i]->is_hidden) {
            trackers.push_back({(uint32_t)i, {plane->x, plane->y, plane->z}});
        }
    }
    r4::scene::checkTriggers(*triggers_, trackers);
    R4Screen::counters().appMs += R4Screen::msSince(start);
}
