//
//  R4Mission.h
//  libRealSpace
//
//  The mission on the scene graph of R4nkine. SCMission loads the mission and runs it; R4Mission keeps its graph, made when the
//  mission is loaded: the terrain of the area, a node for each actor and for each simulated object, in libRealSpace's axes.
//  Its APP step makes the nodes of what appears and drops those of what is gone; then the APP traversal runs the callback of each
//  node, which takes the state of its object of the game (matrix, wheels, afterburner, stores, model). R4Strike draws it.
//  The planes are in one group: they go from an area to another. The other objects are below the node of the area where they are,
//  and those of a scene below a Switch of this scene, on when the scene has been activated. A trigger by area of the mission, with
//  its volume (sphere, tube or box), puts MissionEventAreaEntered and MissionEventAreaLeft of the planes on the message bus.
//
#pragma once

#include "strike_commander/precomp.h"

#include "r4/render/renderer.h"
#include "r4/scene/node.h"

#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

class R4Mission : public SCMission {
public:
    // the masks of the layers of the graph, for the traversals
    enum Layer : uint32_t {
        LAYER_TERRAIN = 1u << 0,
        LAYER_ACTORS = 1u << 1,
        LAYER_SHOTS = 1u << 2,
        LAYER_PLAYER = 1u << 3,
        LAYER_TRIGGERS = 1u << 4
    };

    R4Mission(std::string missionName, std::unordered_map<std::string, RSEntity *> *objCache);
    ~R4Mission() override;

    // the world of the mission, in libRealSpace's axes
    const std::shared_ptr<r4::scene::Group> &world() const {
        return world_;
    }
    // APP: the shots of SCStrike move, then the nodes take the state of the game; tps: the frames per second of the player's plane
    void app(const std::vector<SCSimulatedObject *> &shots, float tps);
    // the simulated objects of the last APP step: the game draws those out of the graph, and the smoke of the others
    const std::vector<SCSimulatedObject *> &shots() const {
        return shotList_;
    }
    // false: the game draws it itself (tracers, billboards, ejection)
    static bool inGraph(SCSimulatedObject *shot);

private:
    // a node that follows one object of the game
    struct Place {
        std::shared_ptr<r4::scene::Group> node;
        // under the node: the actor is drawn up to kRenderDistance from the eye
        std::shared_ptr<r4::scene::LOD> range;
        RSEntity *entity = nullptr;
        bool seen = false;
        // the node that holds it, when it is not the layer
        r4::scene::Node *parent = nullptr;
    };
    // the parts of a plane: its body, its wheels, its afterburner and its weapons under the hard points
    struct PlanePlace {
        std::shared_ptr<r4::scene::Group> node;
        std::shared_ptr<r4::scene::LOD> range;
        std::shared_ptr<r4::scene::Group> body;
        std::shared_ptr<r4::scene::Group> wheels;
        std::shared_ptr<r4::scene::Group> afterburner;
        std::shared_ptr<r4::scene::Group> weapons;
        RSEntity *wheel = nullptr;
        std::vector<RSEntity *> loaded;
        bool seen = false;
        r4::scene::Node *parent = nullptr;
    };

    // the terrain, the runways, the forests and the buildings of the area, made once
    void useArea();
    // a trigger with the volume of an area, as AREA::contains
    void addTrigger(size_t index);
    void publishArea(uint32_t actor, size_t area, bool entered);
    // the node of an object of the game, made when it appears, with an APP callback that updates it
    void placePlane(SCPlane *plane, SCMissionActors *actor);
    void placeObject(SCMissionActors *actor);
    void placeShot(SCSimulatedObject *shot);
    // the APP callbacks; actor: null for the player's plane
    void updatePlane(PlanePlace &place, SCPlane *plane, SCMissionActors *actor);
    void updateObject(Place &place, SCMissionActors *actor);
    void updateShot(Place &place, SCSimulatedObject *shot);
    // the area of the object where it is (not the one of the scene that brings it), and the Switch of its scene
    r4::scene::Node &objectParent(const MISN_PART *part);
    void setModel(r4::scene::Node &node, RSEntity *entity);
    template <class T> void dropUnseen(std::unordered_map<const void *, T> &places, r4::scene::Group &layer);

    RSArea *area_ = nullptr;
    std::shared_ptr<r4::scene::Group> world_;
    std::shared_ptr<r4::scene::Group> terrain_;
    std::shared_ptr<r4::scene::Group> actors_;
    std::shared_ptr<r4::scene::Group> shots_;
    std::shared_ptr<r4::scene::Group> planeLayer_;
    std::shared_ptr<r4::scene::Group> triggers_;
    // a node by area of the mission, then one for the objects out of all areas
    std::vector<std::shared_ptr<r4::scene::Group>> areas_;
    // the Switch of a scene in an area: (area, scene)
    std::map<std::pair<int, int>, std::shared_ptr<r4::scene::Switch>> sceneNodes_;
    // the scene that brings a part, by the id of the part: the first one whose cast has it
    std::unordered_map<int, int> sceneOfPart_;
    std::unordered_map<const void *, PlanePlace> planes_;
    std::unordered_map<const void *, Place> objects_;
    std::unordered_map<const void *, Place> shotPlaces_;
    std::vector<SCSimulatedObject *> shotList_;
};
