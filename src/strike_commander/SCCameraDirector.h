#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include "SCCameraSequence.h"
#include "SCCameraEvent.h"
#include "SCProceduralCamera.h"

class RSWorld;
class SCMissionActors;
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
// l'assembleur : reçoit des demandes de vue nommées (CameraViewRequest) et
// délègue tout le calcul à une SCProceduralCamera — cf. SCProceduralCamera.h.
// Une instance par entrée RÉELLEMENT PRÉSENTE dans le monde chargé, créée
// dans init() : une par RSCameraDef (CHASE/TARGET/ROTA/...) via
// createProceduralCamera(), une par RSCameraSequence (COMP) via
// SCSequenceCamera — jamais construite en dur, toujours pilotée par les
// données du fichier WRLD. active_camera pointe toujours vers l'une d'elles
// (jamais nullptr — une caméra "vide" par défaut) : tick() ne fait plus
// qu'un seul appel polymorphe, fermé pour toujours, quelle que soit la
// caméra ajoutée.
//
// Possédé par SCMission (durée de vie = celle de la mission). Tické via
// MissionUpdateEvent (delta_time réel) — donc suspendu en pause / AUTO_PILOT,
// comme le reste de la simulation.
//
// La vue active (COMP ou procédurale) est toujours publiée comme
// View::CAM_DIRECTOR ; SCStrike compare currentView() à la frame précédente
// et bascule camera_mode tout seul (aucune modif requise côté SCStrike) :
// camera->SetPosition(&director.position()) ; camera->lookAt(&director.aimPoint(),
// &director.up()) ; puis compose le rendu d'après director.viewDesc().
//
class SCCameraDirector {
public:
    SCCameraDirector();
    ~SCCameraDirector();

    void init(RSWorld *world, SCMissionActors *player_entity);

    // Appelé par onEvent() sur chaque MissionUpdateEvent (dt réel) ; simple
    // délégation polymorphe à la caméra active (jamais nullptr).
    void tick(float dt);

    // Valides quand currentView() == View::CAM_DIRECTOR : appliquer via
    // camera->SetPosition(&pos) ; camera->lookAt(&aim, &up).
    const Vector3D       &position() const;
    const Vector3D       &aimPoint() const;
    const Vector3D       &up() const;

    // FOV vertical (degrés) de la caméra active, tel que lu dans le fichier
    // (RSCameraDef::fov) — SCStrike l'applique à Renderer.camera.fovy quand
    // currentView() == View::CAM_DIRECTOR, à la place de son toggle
    // zoom_cockpit habituel (qui reste la source pour les vues non
    // scriptées, sans backing fichier).
    float fov() const;

    static const bool s_debug=false;   // true = trace chaque appel à fov() au stdout

    const CameraViewDesc &viewDesc() const;
    View                  currentView() const;

    // Type et sujet de la caméra active — permet à l'appelant (ex. SCStrike,
    // pour inverser subject/target d'un appui F7 à l'autre) de connaître
    // l'état courant sans dupliquer un flag séparé qui pourrait se
    // désynchroniser : le directeur reste la seule source de vérité.
    RSCameraType     activeCameraType() const;
    SCMissionActors *activeSubject() const;

private:
    void onEvent(const EventMessage &event);
    // Aiguilleur : une CameraViewRequest -> retrouve la SCProceduralCamera
    // correspondante (par nom pour une séquence COMP, par typeCode pour une
    // entrée CAMR — miroir de Kneeboard_SelectByID / SelectByStateCode),
    // sinon une vue brute sans backing fichier (FRONT/LEFT/RIGHT/REAR...).
    // Ajouter une caméra ne fait jamais grossir cette fonction : la
    // résolution reste par nom/type, générique.
    void onViewRequest(const CameraViewRequest &request);

    // Fabrique la SCProceduralCamera correspondant à def->typeCode (une par
    // RSCameraDef du monde, cf. init()). nullptr si le type n'est pas
    // (encore) câblé côté C++ (CKPT/VICT/WEAP/CONT) — l'entrée du fichier
    // est alors ignorée plutôt que de planter.
    SCProceduralCamera *createProceduralCamera(const RSCameraDef *def) const;

    SCProceduralCamera *findCameraByType(RSCameraType type_code) const;
    SCProceduralCamera *findCameraByName(const std::string &name) const;

    // Bascule sur `cam` (jamais nullptr) : activate() seulement si on
    // change réellement de caméra, publie le changement de vue.
    void activateCamera(SCProceduralCamera *cam, SCMissionActors *subject, SCMissionActors *target);
    void activateSimpleView(View view);

    // Résout une entité par nom ASCII ("PLAYER" ...) pour OP_IA_BIND_ENTITY /
    // RSCameraDef::subject.
    SCMissionActors *resolveEntity(const std::string &name) const;

    void publishViewChanged(View view, const std::string &reason);

    long long subscription_id{-1};   // MessageBus::SubscriptionId

    RSWorld  *world{nullptr};
    SCMissionActors *player_entity{nullptr};

    View            current_view{View::FRONT};
    CameraViewDesc  view_desc;
    Vector3D        out_pos;
    Vector3D        out_aim;
    Vector3D        out_up;

    // Une instance par entrée de registre réellement chargée depuis le
    // monde (une par RSCameraDef, une par RSCameraSequence — cf. init()),
    // possédée par le directeur (détruites dans ~SCCameraDirector).
    // active_camera pointe vers l'une d'elles, ou vers null_camera (vue
    // simple / aucune vue procédurale) — jamais nullptr.
    std::vector<SCProceduralCamera *> procedural_cameras;
    SCProceduralCamera                *null_camera{nullptr};
    SCProceduralCamera                *active_camera{nullptr};

    // Sujet/cible passés au dernier activate() réel — permet à
    // activateCamera() de détecter un changement à caméra inchangée (ex.
    // TARGET : une seule instance, verrouiller une autre cible ou inverser
    // subject/target doit réinitialiser la caméra, pas être ignoré parce
    // que c'est "la même caméra").
    SCMissionActors *active_subject{nullptr};
    SCMissionActors *active_target{nullptr};
};
