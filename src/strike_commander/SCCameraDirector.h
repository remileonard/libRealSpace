#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include "SCCameraSequence.h"
#include "SCCameraEvent.h"
#include "SCProceduralCamera.h"

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
// Possédé par SCMission (durée de vie = celle de la mission). Tické via
// MissionUpdateEvent (delta_time réel) — donc suspendu en pause / AUTO_PILOT,
// comme le reste de la simulation. Gère deux familles de vues :
//   - séquences scriptées COMP (STARTCAM...)         -> this->sequence
//   - vues procédurales du registre CAMR (CHASE...)  -> procedural_cameras
// Une caméra procédurale = une SCProceduralCamera (cf. SCProceduralCamera.h),
// une par RSCameraType, enregistrée une fois dans le constructeur. Ajouter
// une caméra (TARGET, ROTA...) = écrire une sous-classe + l'enregistrer ;
// tick() et activateProceduralCamera() ci-dessous ne changent plus jamais.
//
// Dans les deux cas la vue active est publiée comme View::CAM_DIRECTOR ;
// SCStrike compare currentView() à la frame précédente et bascule
// camera_mode tout seul (aucune modif requise côté SCStrike).
//
// SCStrike : si isRunningSequence() ou vue procédurale active ->
// camera->SetPosition(&director.position()) ; camera->lookAt(&director.aimPoint(),
// &director.up()) ; puis compose le rendu d'après director.viewDesc().
//
class SCCameraDirector {
public:
    SCCameraDirector();
    ~SCCameraDirector();

    void init(RSWorld *world, SCPlane *player_entity);

    // Appelé par onEvent() sur chaque MissionUpdateEvent (dt réel) ; ne fait
    // rien si aucune vue COMP/procédurale n'est active.
    void tick(float dt);

    // Valides quand isRunningSequence() ou qu'une vue procédurale (CHASE...)
    // est active — currentView() vaut alors View::CAM_DIRECTOR : appliquer via
    // camera->SetPosition(&pos) ; camera->lookAt(&aim, &up).
    const Vector3D       &position() const;
    const Vector3D       &aimPoint() const;
    const Vector3D       &up() const;

    const CameraViewDesc &viewDesc() const;
    View                  currentView() const;
    bool                  isRunningSequence() const;

private:
    void onEvent(const EventMessage &event);
    // Aiguilleur : une CameraViewRequest -> soit une séquence COMP nommée,
    // soit une entrée du registre CAMR (world->cameras, cherchée par
    // typeCode — miroir de Kneeboard_SelectByStateCode), soit une vue brute
    // sans backing fichier (FRONT/LEFT/RIGHT/REAR...).
    void onViewRequest(const CameraViewRequest &request);

    // Retrouve l'entrée CAMR d'un type donné (RSCameraType) dans le monde
    // chargé. nullptr si ce type n'existe pas dans la mission (comme le jeu
    // d'origine : Kneeboard_RenderByCode ne trouve rien -> pas de bascule).
    const RSCameraDef *findCameraDef(RSCameraType type_code) const;

    // Retrouve la SCProceduralCamera enregistrée pour ce type, nullptr si
    // aucune (type reconnu par le fichier mais pas encore câblé ici).
    SCProceduralCamera *findProceduralCamera(RSCameraType type_code) const;

    // Coupe la caméra procédurale active (active_procedural = nullptr) avant
    // d'activer une vue simple ou une séquence COMP.
    void deactivateProceduralViews();

    void activateSimpleView(View view);
    void activateSequence(const CameraViewRequest &request);
    void tickSequence(float dt);

    // Dispatch une entrée CAMR trouvée par findCameraDef() vers la
    // SCProceduralCamera de son typeCode ; resolveEntity(entry.subject)
    // donne le sujet (au lieu d'un "PLAYER" en dur). Type reconnu par le
    // fichier mais pas encore câblé -> repli sur activateSimpleView(request.view).
    void activateProceduralCamera(const RSCameraDef &entry, const CameraViewRequest &request);

    // Retrouve une RSCameraSequence par nom dans le monde.
    const RSCameraSequence *findSequence(const std::string &name) const;
    // Résout une entité par nom ASCII ("PLAYER" ...) pour OP_IA_BIND_ENTITY /
    // RSCameraDef::subject.
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

    // Une instance par RSCameraType procédural câblé, possédée par le
    // directeur (détruites dans ~SCCameraDirector). active_procedural
    // pointe vers l'une d'elles (ou nullptr si COMP/vue simple active).
    std::vector<SCProceduralCamera *> procedural_cameras;
    SCProceduralCamera               *active_procedural{nullptr};
};
