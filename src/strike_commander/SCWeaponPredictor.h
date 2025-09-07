#pragma once
#include "SCSimulatedObject.h"


class SCWeaponPredictor {
public:
    SCWeaponPredictor();
    ~SCWeaponPredictor();
    
    // Stocke les positions simulées pour analyse et visualisation
    std::vector<Vector3D> trajectory;
    
    // Type d'arme à simuler
    EntityType weapon_type;
    
    // Nombre d'étapes de simulation à effectuer
    int simulation_steps = 120;
    
    // Objet réel utilisé pour la simulation
    SCSimulatedObject* sim_object = nullptr;
    
    // Simule le comportement de l'arme et renvoie un vecteur de direction ajusté
    Vector3D PredictTrajectory(RSEntity* obj, SCMissionActors* shooter, SCMissionActors* target, 
                            Vector3D position, Vector3D velocity, SCMission* mission);
    
    // Permet de visualiser la trajectoire simulée
    void RenderTrajectory();
    
    // Probabilité d'impact calculée après simulation
    float hit_probability = 0.0f;
};