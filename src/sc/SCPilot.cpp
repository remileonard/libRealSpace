//
//  SCPilot.cpp
//  libRealSpace
//
//  Created by Rémi LEONARD on 23/09/2024.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//
#include <algorithm>
#include <cmath>
#include "precomp.h"
#include "SCPilot.h"

class PIDController {
public:
    PIDController(float kp, float ki, float kd)
        : kp(kp), ki(ki), kd(kd), prev_error(0), integral(0) {}

    float calculate(float setpoint, float measured_value, float dt) {
        float error = setpoint - measured_value;
        integral += error * dt;
        float derivative = (error - prev_error) / dt;
        prev_error = error;
        return kp * error + ki * integral + kd * derivative;
    }

private:
    float kp, ki, kd;
    float prev_error;
    float integral;
};

SCPilot::SCPilot() {
    target_speed = 0;
    target_climb = 0;
    target_azimut = 0;
}

SCPilot::~SCPilot() {}

void SCPilot::SetTargetWaypoint(Vector3D waypoint) {
    Vector2D weapoint_direction = {waypoint.x - plane->x,
                                   waypoint.z - plane->z};
    float az = (atan2f((float)weapoint_direction.y, (float)weapoint_direction.x) * 180.0f / (float)M_PI);
    if (turning) {
        return;
    }
    turning = true;
    az -= 360.0f;
    az += 90.0f;
    if (az > 360.0f) {
        az -= 360.0f;
    }
    while (az < 0.0f) {
        az += 360.0f;
    }
    this->target_azimut = az;
    target_climb = (int) waypoint.y;
    if (waypoint.y > plane->y || this->land) {
        
    } else {
        target_climb = (int) plane->y;
    }
}

void SCPilot::AutoPilot() {
    PIDController altitudeController(1.0f, 0.1f, 0.05f); // Adjust these values as needed
    PIDController azimuthController(1.0f, 0.1f, 0.05f); // Adjust these values as needed

    if (!this->alive) {
        return;
    }
    if (!this->plane->object->alive) {
        this->plane->Mthrust = 0;
        this->plane->s = 0.001f;
        this->plane->b = 0.001f;
        this->target_speed = 0;
        this->target_climb = 0;
        this->target_azimut = 0;
        this->plane->SetSpoilers();
        this->plane->vz /= 1.5f;
        this->plane->vy *=1.05f;
        this->plane->SetThrottle(0);
        this->alive = false;
    }
    if (this->plane == nullptr) {
        return;
    }
    if (this->plane->on_ground && this->target_climb == 0) {
        this->plane->SetThrottle(0);
        return;
    }
    if (this->plane->vz > this->target_speed) {
        this->plane->SetThrottle(100);
    } else {
        this->plane->SetThrottle(this->plane->GetThrottle() - 10);
    }


    float horizontal_distance = 1000.0f;
    float vertical_distance = this->target_climb - this->plane->y;
    float target_elevation_rad = atan2(vertical_distance, horizontal_distance);

    
    float dt = 0.1f; // Time step, adjust as needed
    float control_signal = altitudeController.calculate(target_elevation_rad, this->plane->pitch, dt);
    
    if (this->plane->pitch > target_elevation_rad) {
        if (std::abs(target_elevation_rad) > 0.01f) {
            this->plane->pitch -= 0.01f;
        } else {
            this->plane->pitch = target_elevation_rad;
        } 
    } else if (this->plane->pitch < target_elevation_rad) {
        if (std::abs(target_elevation_rad) > 0.01f) {
            this->plane->pitch += 0.01f;
        } else {
            this->plane->pitch = target_elevation_rad;
        }
    }

    
    float target_yaw = (float) ((360.0f - this->target_azimut) * M_PI / 180.0f);
    float target_roll = 0.0f;
    if (this->plane->yaw > target_yaw) {
        this->plane->roll = 5.78f;
        if (std::abs(this->plane->yaw-target_yaw) > 0.01f) {
            this->plane->yaw -= 0.01f;
        } else {
            this->plane->yaw = target_yaw;
        }
    } else if (this->plane->yaw < target_yaw) {
        this->plane->roll = 0.5f;
        if (std::abs(this->plane->yaw-target_yaw) > 0.01f) {
            this->plane->yaw += 0.01f;
        } else {
            this->plane->yaw = target_yaw;
        }
    } 
    
    if (std::abs(this->plane->yaw-target_yaw) < 0.1f) {
        target_roll = 0.0f;
        this->plane->yaw = target_yaw;
        this->plane->roll = 0.0f;
        turning = false;
    }
    
}

void SCPilot::PilotPlane() {
    // Constantes de gains et tolérances
    const float azimuthTolerance = 0.05f;  // tolérance pour l'alignement azimutal (en radians)
    const float Kx_turn = 1.0f;            // gain sur control_stick_x pour induire le roll
    const float Ky_turn = 0.5f;            // gain sur control_stick_y pour accélérer le taux de rotation
    const float K_roll_level = 1.0f;       // gain pour ramener twist à zéro lorsque l'azimuth est atteint
    const float altitudeThreshold = 2.0f;  // tolérance sur l'altitude
    const float K_alt = 0.1f;              // gain sur control_stick_y pour le contrôle d'altitude
    const float speedTolerance = 5.0f;     // tolérance sur la vitesse
    const float K_thrust = 0.5f;           // gain sur la modification de thrust
    

    float desired_azimuth=this->target_azimut;
    float desired_altitude=this->target_climb;
    float desired_speed=this->target_speed;

    // -------------------- Contrôle de l'azimuth --------------------
    float azError = desired_azimuth - plane->azimuthf;
    // Normalisation de l'erreur pour être dans [-pi, pi]
    while (azError > M_PI)  azError -= 2.0f * M_PI;
    while (azError < -M_PI) azError += 2.0f * M_PI;
    
    float turningSignalX = 0.0f; // signal pour control_stick_x
    float turningSignalY = 0.0f; // signal pour control_stick_y (lié au contrôle de la rotation)
    // On définit un facteur de pondération qui, si l'erreur est grande,
    // privilégie la correction de l'azimuth.
    float turnInfluence = (std::min)(std::fabs(azError) / (float) M_PI, 1.0f);
    
    if (std::fabs(azError) > azimuthTolerance) {
        // Essayer de corriger l'azimuth en induisant un roll
        turningSignalX = (azError > 0 ? 1.0f : -1.0f) * Kx_turn;
        // On peut ajuster la vitesse d'orientation avec control_stick_y
        turningSignalY = -Ky_turn * azError;
    } else {
        // Azimuth atteint, on "nivelle" l'avion en annulant le twist
        turningSignalX = -K_roll_level * plane->twist;
        turningSignalY = 0.0f;
    }
    
    // -------------------- Contrôle de l'altitude --------------------
    float altitudeError = desired_altitude - plane->y;
    float altitudeSignal = 0.0f;
    // Pour éviter un looping, on n'applique le contrôle d'altitude que si l'erreur est significative
    if (std::fabs(altitudeError) > altitudeThreshold) {
        // Une altitude trop basse (altitudeError > 0) nécessite de monter :
        // une valeur négative de control_stick_y fera monter le nez.
        altitudeSignal = (altitudeError > 0) ? -K_alt : K_alt;
    }
    
    // -------------------- Combinaison des signaux --------------------
    // Si l'avion est en phase de tournant (erreur azimutale élevée),
    // on privilégie le signal de rotation; sinon, le contrôle en altitude.
    float combinedControlY = turnInfluence * turningSignalY + (1.0f - turnInfluence) * altitudeSignal;
    
    plane->control_stick_x = turningSignalX*100;
    plane->control_stick_y = combinedControlY*-300;
    
    // -------------------- Contrôle de la vitesse --------------------
    // Une valeur négative de vz indique une avance de l'avion.
    float currentSpeed = -plane->vz;
    float speedError = desired_speed - currentSpeed;
    float thrustAdjustment = (std::fabs(speedError) > speedTolerance) ? K_thrust * speedError : 0.0f;
    
    if (speedError < 0) {
        // Si l'avion est trop lent, on augmente la poussée
        plane->thrust += 100;
    } else if (speedError > 0) {
        // Si l'avion est trop rapide, on diminue la poussée
        plane->thrust += -100;
    }
    plane->thrust = (std::clamp)((float) plane->thrust, -100.0f, 100.0f);
}
