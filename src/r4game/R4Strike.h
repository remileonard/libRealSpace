//
//  R4Strike.h
//  libRealSpace
//
//  The flight on the scene graph of R4nkine. The graph lives as long as the flight: the terrain of the area, a node for each actor
//  and for each simulated object. The APP step sets their matrices from the state of the game once a frame; one pass draws the graph.
//  The effects (smoke, explosions, tracers) and the cockpit of the game go in the same pass as the graph.
//
#pragma once

#include "strike_commander/precomp.h"

#include "r4/render/renderer.h"
#include "r4/scene/node.h"

#include <memory>
#include <unordered_map>
#include <vector>

class R4Strike : public SCStrike {
public:
    // the masks of the layers of the graph, for the traversals
    enum Layer : uint32_t {
        LAYER_TERRAIN = 1u << 0,
        LAYER_ACTORS = 1u << 1,
        LAYER_SHOTS = 1u << 2,
        LAYER_PLAYER = 1u << 3
    };

    R4Strike();
    ~R4Strike() override;
    void runFrame() override;

    // the world of the flight, in libRealSpace's axes
    r4::scene::Node &world() {
        return *world_;
    }

protected:
    void afterCameraPlaced() override;
    void renderWorld() override;
    void renderPlayerPlane() override;

private:
    // a node that follows one object of the game
    struct Place {
        std::shared_ptr<r4::scene::Group> node;
        RSEntity *entity = nullptr;
        bool seen = false;
    };
    // the parts of a plane: its body, its wheels, its afterburner and its weapons under the hard points
    struct PlanePlace {
        std::shared_ptr<r4::scene::Group> node;
        std::shared_ptr<r4::scene::Group> body;
        std::shared_ptr<r4::scene::Group> wheels;
        std::shared_ptr<r4::scene::Group> afterburner;
        std::shared_ptr<r4::scene::Group> weapons;
        RSEntity *wheel = nullptr;
        std::vector<RSEntity *> loaded;
        bool seen = false;
    };

    void useArea();
    // APP: the matrices and the visibility of the nodes from the state of the game
    void app();
    void placePlane(SCPlane *plane, bool shown, uint32_t layer);
    void placeObject(SCMissionActors *actor);
    void placeShot(SCSimulatedObject *shot);
    // false: the game draws it itself (tracers, billboards, ejection)
    static bool inGraph(SCSimulatedObject *shot);
    bool playerPlaneShown() const;
    void setModel(r4::scene::Group &group, RSEntity *entity);
    template <class T> void dropUnseen(std::unordered_map<const void *, T> &places, r4::scene::Group &layer);

    RSArea *area_ = nullptr;
    std::shared_ptr<r4::scene::Group> world_;
    std::shared_ptr<r4::scene::Group> terrain_;
    std::shared_ptr<r4::scene::Group> actors_;
    std::shared_ptr<r4::scene::Group> shots_;
    std::unordered_map<const void *, PlanePlace> planes_;
    std::unordered_map<const void *, Place> objects_;
    std::unordered_map<const void *, Place> shotPlaces_;
    // what the APP step put in the graph this frame, for the effects that the game draws after it
    std::vector<SCMissionActors *> actorsShown_;
    std::vector<SCSimulatedObject *> shotsShown_;
};
