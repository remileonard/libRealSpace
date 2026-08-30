#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include "SCCameraSequence.h"
#include "SCCameraEvent.h"

class RSWorld;
class SCPlane;
class SCMission;

//
// Descripteur de la vue active, consommé par SCStrike pour la composition du
// rendu (cockpit 2D / cockpit 3D / avion joueur visible). Ce que le directeur
// sait dire ; SCStrike croise avec l'état joueur (crash, éjection, airframe).
//
struct CameraViewDesc {
    View        view{View::FRONT};
    bool        first_person{false};   // vue pilote : avion joueur masqué
    bool        renders_cockpit{false};
    std::string cockpit_art;           // "F16-CKPT" (chunk CKPT du CAMR), sinon vide
    bool        scripted{false};       // séquence COMP en cours
};

//
// SCCameraDirector
// ----------------
// Possède la caméra logique du jeu. Miroir du registre de caméras 0x59CD de
// l'assembleur : reçoit des demandes de vue nommées (CameraViewRequest), exécute
// la séquence COMP active une frame à la fois, et publie CameraViewChanged à la
// fin d'une séquence (vue de reprise).
//
// Possédé par SCMission (durée de vie = celle de la mission). Tické à chaque
// frame de simulation par SCStrike (source de tick indépendante de
// MissionUpdateEvent, qui peut être supprimé en AUTO_PILOT / pause).
//
// SCStrike : director.tick(dt) ; si director.isRunningSequence() ->
// camera->SetPosition(&director.position()) ; camera->lookAt(&director.aimPoint(),
// &director.up()) ; puis compose le rendu d'après director.viewDesc().
//
class SCCameraDirector {
public:
    SCCameraDirector();
    ~SCCameraDirector();

    void init(RSWorld *world, SCPlane *player_entity);

    // Appelé une fois par frame de simulation, inconditionnellement.
    void tick(float dt);

    // Valides uniquement quand isRunningSequence() : à appliquer via
    // camera->SetPosition(&pos) ; camera->lookAt(&aim, &up).
    const Vector3D       &position() const;
    const Vector3D       &aimPoint() const;
    const Vector3D       &up() const;

    const CameraViewDesc &viewDesc() const;
    View                  currentView() const;
    bool                  isRunningSequence() const;

private:
    void onEvent(const EventMessage &event);
    void onViewRequest(const CameraViewRequest &request);

    // Retrouve une RSCameraSequence par nom dans le monde.
    const RSCameraSequence *findSequence(const std::string &name) const;
    // Résout une entité par nom ASCII ("PLAYER" ...) pour OP_IA_BIND_ENTITY.
    SCPlane *resolveEntity(const std::string &name) const;

    void setView(View view, const std::string &reason);
    void publishViewChanged(View view, const std::string &reason);

    long long subscription_id{-1};   // MessageBus::SubscriptionId

    RSWorld  *world{nullptr};
    SCPlane *player_entity{nullptr};

    View            current_view{View::FRONT};
    CameraViewDesc  view_desc;
    SCCameraSequence sequence;
    Vector3D        out_pos;
    Vector3D        out_aim;
    Vector3D        out_up;
};
