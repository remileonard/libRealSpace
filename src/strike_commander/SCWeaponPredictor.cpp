#include "precomp.h"
#include "SCWeaponPredictor.h"

SCWeaponPredictor::SCWeaponPredictor() {
    this->trajectory.clear();
}

SCWeaponPredictor::~SCWeaponPredictor() {
    if (this->sim_object) {
        delete this->sim_object;
    }
}

Vector3D SCWeaponPredictor::PredictTrajectory(RSEntity* obj, SCMissionActors* shooter, 
                                           SCMissionActors* target, Vector3D position, 
                                           Vector3D velocity, SCMission* mission) {
    // Créer l'objet simulé du bon type en fonction de l'arme
    if (this->sim_object) {
        delete this->sim_object;
        this->sim_object = nullptr;
    }
    
    if (obj->entity_type == EntityType::tracer || obj->entity_type == EntityType::bomb) {
        this->sim_object = new GunSimulatedObject();
    } else {
        this->sim_object = new SCSimulatedObject();
    }
    
    // Configurer l'objet simulé avec les paramètres initiaux
    this->sim_object->is_simulated = true; // Indique que c'est une simulation
    this->sim_object->obj = obj;
    this->sim_object->shooter = shooter;
    this->sim_object->target = target;
    this->sim_object->mission = mission;
    this->sim_object->x = position.x;
    this->sim_object->y = position.y;
    this->sim_object->z = position.z;
    this->sim_object->vx = velocity.x;
    this->sim_object->vy = velocity.y;
    this->sim_object->vz = velocity.z;
    this->sim_object->weight = obj->weight_in_kg * 2.205f;
    
    // Sauvegarder les valeurs initiales
    float init_x = position.x;
    float init_y = position.y;
    float init_z = position.z;
    float init_vx = velocity.x;
    float init_vy = velocity.y;
    float init_vz = velocity.z;
    Vector3D initial_direction = velocity;
    initial_direction.Normalize();
    
    // Vider la trajectoire précédente
    this->trajectory.clear();
    this->trajectory.push_back({position.x, position.y, position.z});
    
    // Variables pour le calcul de la probabilité d'impact
    float closest_distance = FLT_MAX;
    Vector3D closest_point = {0, 0, 0};
    bool has_hit = false;
    bool has_hit_ground = false;
    
    // Simulation complète en utilisant l'objet simulé réel
    for (int i = 0; i < this->simulation_steps; i++) {
        // Utilisation du vrai code de simulation (pas de duplication)
        this->sim_object->Simulate(30); // 30 FPS standard
        
        // Enregistrement de la position pour la trajectoire
        this->trajectory.push_back({this->sim_object->x, this->sim_object->y, this->sim_object->z});
        
        // Si l'objet n'est plus en vie (a touché le sol ou explosé), on arrête la simulation
        if (!this->sim_object->alive) {
            has_hit_ground = true;
            break;
        }

        Vector3D target_pos = {0.0f, 0.0f, 0.0f};
        if (target && target->plane) {
            target_pos = {
                target->plane->x, 
                target->plane->y, 
                target->plane->z
            };
        } else if (target) {
            target_pos = {
                target->object->position.x,
                target->object->position.y,
                target->object->position.z
            };
        }
        // Si une cible est définie, calculer la distance
        if (target ) {

            Vector3D current_pos = {this->sim_object->x, this->sim_object->y, this->sim_object->z};
            float distance = (target_pos - current_pos).Norm();
            
            // Mettre à jour la distance la plus proche
            if (distance < closest_distance) {
                closest_distance = distance;
                closest_point = current_pos;
            }
            
            // Vérifier si l'impact est probable
            if (distance < obj->wdat->radius) {
                has_hit = true;
                break;
            }
        }
    }
    
    // Calcul de la probabilité d'impact
    if (target) {
        Vector3D target_pos = {0.0f, 0.0f, 0.0f};
        if (target->plane) {
            target_pos = {
                target->plane->x, 
                target->plane->y, 
                target->plane->z
            };
        } else {
            target_pos = {
                target->object->position.x,
                target->object->position.y,
                target->object->position.z
            };
        }
        if (obj->entity_type == EntityType::bomb) {
            // Pour les bombes, la probabilité dépend de l'impact au sol et du rayon d'explosion
            if (has_hit_ground) {
                Vector3D impact_pos = this->trajectory.back();
                float distance = (target_pos - impact_pos).Norm();
                this->hit_probability = 1.0f - (distance / (obj->wdat->radius * 1.5f));
                if (this->hit_probability < 0.0f) this->hit_probability = 0.0f;
                if (this->hit_probability > 1.0f) this->hit_probability = 1.0f;
            }
        } else {
            // Pour les projectiles directs et missiles
            this->hit_probability = 1.0f - (closest_distance / 50.0f);
            if (this->hit_probability < 0.0f) this->hit_probability = 0.0f;
            if (this->hit_probability > 1.0f) this->hit_probability = 1.0f;
            
            if (has_hit) this->hit_probability = 0.9f;
        }
    }
    
    // Calculer le vecteur de direction ajusté
    Vector3D adjusted_direction = initial_direction;
    
    if (target && !has_hit && closest_distance < 50.0f) {
        // Ajustement basé sur le point le plus proche atteint

        Vector3D target_pos = {
            0.0f, 0.0f, 0.0f
        };
        if (target->plane) {
            target_pos = {
                target->plane->x, 
                target->plane->y, 
                target->plane->z
            };
        } else {
            target_pos = {
                target->object->position.x,
                target->object->position.y,
                target->object->position.z
            };
        }
        // Vecteur d'erreur: différence entre la cible et le point le plus proche
        Vector3D error_vector = target_pos - closest_point;
        
        // Ajustement différent selon le type d'arme
        float adjustment_factor = 0.0f;
        
        if (obj->entity_type == EntityType::bomb) {
            // Ajustement minimal pour les bombes (principalement guidé par la gravité)
            adjustment_factor = 0.02f;
        } else if (obj->entity_type == EntityType::tracer) {
            // Ajustement moyen pour les projectiles
            adjustment_factor = 0.05f;
        } else {
            // Ajustement plus important pour les missiles
            adjustment_factor = 0.1f;
        }
        
        adjustment_factor *= (1.0f - (closest_distance / 50.0f));
        error_vector.Normalize();
        adjusted_direction = initial_direction + (error_vector * adjustment_factor);
        adjusted_direction.Normalize();
    }
    
    return adjusted_direction;
}

void SCWeaponPredictor::RenderTrajectory() {
    if (this->trajectory.size() < 2) return;
    SCRenderer &Renderer = SCRenderer::getInstance();
    // Couleur: rouge pour faible probabilité, vert pour haute
    Vector3D color = {1.0f - this->hit_probability, this->hit_probability, 0.0f};
    
    // Dessiner la trajectoire
    for (size_t i = 0; i < this->trajectory.size() - 1; i++) {
        Renderer.drawLine(this->trajectory[i], this->trajectory[i + 1], color);
    }
    
    // Marquer le point final
    if (!this->trajectory.empty()) {
        Vector3D last_point = this->trajectory.back();
        Renderer.drawPoint(last_point, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f});
    }
}