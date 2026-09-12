#pragma once
#include <string>
#include "../realspace/RSWorld.h"   // RSCameraType

class SCMissionActors;

//
// Interface d'une caméra du registre — CHASE/TARGET/ROTA (CAMR) aussi bien
// que les séquences scriptées COMP (STARTCAM/TAKEOFF/...). Miroir du
// registre ASM 0x59CD : chaque entrée y a un code de type (recherche façon
// Kneeboard_SelectByStateCode) ET un nom/ID (recherche façon
// Kneeboard_SelectByID/FindByID) — cf. CAMERA_SYSTEM.md §2.6/§3.3/§4bis.
//
// Une instance par entrée de registre RÉELLEMENT PRÉSENTE dans le monde
// chargé — cf. SCCameraDirector::init(), qui construit une caméra par
// RSCameraDef (CHASE/TARGET/ROTA) et par RSCameraSequence (COMP). Un type
// qui n'existe pas dans le fichier de la mission n'a simplement pas
// d'instance : findCameraByType()/findCameraByName() ne le trouveront pas,
// exactement comme Kneeboard_RenderByCode dans le jeu d'origine.
// SCCameraDirector::tick() ne fait plus que déléguer à la caméra active :
// ajouter une caméra n'y touche plus jamais.
//
class SCProceduralCamera {
public:
    virtual ~SCProceduralCamera() {}

    // RSCameraType géré par cette instance (RSCAM_CHAS, RSCAM_TARG,
    // RSCAM_COMP pour toute séquence scriptée...).
    virtual RSCameraType typeCode() const = 0;

    // Nom/ID dans le registre (ex. "STARTCAM"/"TAKEOFF" pour une séquence
    // COMP, "CHASECAM"/"ROTATCAM"/"AUTOTRAC" pour une entrée CAMR — tel que
    // déclaré dans le fichier). Défaut : chaîne vide.
    virtual const std::string &name() const;

    // Nom de l'entité sujet TEL QUE DÉCLARÉ DANS LE FICHIER
    // (RSCameraDef::subject, ex. "PLAYER") — utilisé par le directeur comme
    // repli quand la CameraViewRequest ne fournit pas explicitement de
    // sujet. Défaut : chaîne vide (séquences COMP : le sujet vient du
    // script/de la requête, pas d'un champ de définition).
    virtual const std::string &subjectName() const;

    // FOV vertical en degrés TEL QUE DÉCLARÉ DANS LE FICHIER
    // (RSCameraDef::fov, ex. 40 pour CHASECAM) — le champ est utilisable
    // directement, sans transformation (contrairement à viewX/Y/W/H : voir
    // le commentaire de RSCameraDef). Défaut : 45.0f (valeur non-zoom déjà
    // utilisée par SCStrike pour les vues sans backing fichier).
    virtual float fov() const;

    // Appelé UNE SEULE FOIS, au moment où l'on bascule SUR cette caméra
    // depuis une autre (pas à chaque frame) : pose l'état initial. `subject`
    // est n'importe quel acteur de mission (avion, bateau, bâtiment) — pas
    // forcément un SCPlane, une cible verrouillée peut être n'importe quel
    // RSEntity. `target` est optionnel : nullptr pour les caméras qui n'en
    // ont pas besoin (CHASE/ROTA/COMP) ; utilisé par TARGET comme point de
    // visée, distinct du sujet autour duquel la caméra se positionne — voir
    // CameraViewRequest.
    virtual void activate(SCMissionActors *subject, SCMissionActors *target) = 0;

    // Appelé à chaque frame tant que cette caméra est active ; écrit la
    // pose caméra dans les sorties du directeur.
    virtual void tick(float dt, Vector3D &out_pos, Vector3D &out_aim, Vector3D &out_up) = 0;
};
