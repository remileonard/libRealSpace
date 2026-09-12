//
//  SCPilot.cpp
//  libRealSpace
//
//  Created by Rémi LEONARD on 23/09/2024.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//
#include <algorithm>
#include "precomp.h"
#include "../engine/gametimer.h"
#include "SCMissionEvent.h"

SCPilot::SCPilot() {
    target_speed = 0;
    target_climb = 0;
    target_azimut = 0;
}

SCPilot::~SCPilot() {}

void SCPilot::SetTargetWaypoint(Vector3D waypoint) {
    this->target_waypoint = {
        waypoint.x, waypoint.y, waypoint.z
    };
    Vector2D weapoint_direction = {
        waypoint.x - plane->x,
        waypoint.z - plane->z
    };
    float az = (atan2f((float)weapoint_direction.y, (float)weapoint_direction.x) * 180.0f / (float)M_PI);
    if (turning) {
        return;
    }
    //turning = true;
    az -= 360.0f;
    az += 90.0f;
    if (az > 360.0f) {
        az -= 360.0f;
    }
    while (az < 0.0f) {
        az += 360.0f;
    }
    this->target_azimut = az*10.0f;
    float ground_y = this->plane->area->getY(waypoint.x, waypoint.z);
    if (waypoint.y < ground_y) {
        waypoint.y = ground_y+1000;
    }
    float ground_level = this->plane->area->getY(this->plane->x, this->plane->z);
    if (this->plane->y < ground_y + 1000) {
        waypoint.y = ground_y + 1000;
    }
    target_climb = (int) waypoint.y;
    if (waypoint.y > plane->y || this->land) {
        
    } else {
        target_climb = (int) plane->y;
    }
}

float SCPilot::calculateMaxTurnRate(float airspeed, float maxG) {
    // Constantes
    const float G = 9.81f;          // Accélération gravitationnelle en m/s²
    const float KNOTS_TO_MS = 0.514444f;  // Facteur de conversion noeuds -> m/s
    
    // Vitesse en m/s
    float speed_ms = airspeed * KNOTS_TO_MS;
    
    // Calcul de l'angle de roulis maximum pour le facteur G donné
    float max_bank_angle_rad = acosf(1.0f / maxG);
    
    // Formule physique du taux de virage maximum (en degrés/seconde)
    float max_turn_rate_deg_sec = (G * tanf(max_bank_angle_rad)) / (M_PI * speed_ms / 180.0f);
    
    // Convertir en unités du jeu (dixièmes de degrés)
    return max_turn_rate_deg_sec * 10.0f;
}



/**
 * Retourne le delta time courant du moteur, avec une valeur de repli
 * cohérente avec la simulation si le timer n'est pas disponible.
 */
float SCPilot::getDeltaTime() {
    float dt = GameTimer::getInstance().getDeltaTime();
    if (dt <= 0.0f) {
        if (this->plane != nullptr && this->plane->tps > 0) {
            dt = 1.0f / this->plane->tps;
        } else {
            dt = 1.0f / 30.0f;
        }
    }
    return dt;
}

/**
 * Calcule l'angle de roulis maximum (en dixiemes de degres) que l'avion
 * peut soutenir pour son facteur de charge maximum.
 * bank = acos(1 / G)
 */
float SCPilot::maxBankForG(float maxG) {
    if (maxG <= 1.0f) {
        return 450.0f;
    }
    float bank_rad = acosf(1.0f / maxG);
    return radToDegree(bank_rad) * 10.0f;
}

/**
 * Gere les gaz pour atteindre la vitesse cible.
 * Rappel : vz est negatif quand l'avion avance, target_speed est negatif aussi.
 */
void SCPilot::controlThrottle() {
    if (this->plane->vz > this->target_speed) {
        this->throttle = 100;
    } else {
        this->throttle = this->plane->GetThrottle() - 10;
        if (this->throttle < 0) {
            this->throttle = 0;
        }
    }
}

/**
 * Pilote automatique pilotant l'avion comme un humain : il n'agit QUE sur le
 * manche (control_stick_x / control_stick_y) et sur les gaz. La physique de
 * SCJdynPlane applique ensuite les forces reelles (portance, G, inertie, stall).
 *
 * Trois cascades de boucles :
 *   - lateral  : cap -> inclinaison voulue -> manche gauche/droite
 *   - vertical : altitude -> vitesse verticale voulue -> assiette -> manche cabrer/piquer
 *   - vitesse  : vitesse cible -> gaz
 *
 * La cascade verticale surveille la vitesse verticale reelle (vy) pour anticiper
 * l'inertie et eviter que l'avion n'oscille autour de l'altitude voulue.
 */
void SCPilot::FlyTo() {
    if (this->plane == nullptr) {
        return;
    }
    if (!this->alive) {
        return;
    }

    // Avion detruit : sequence de chute
    if (!this->plane->object->alive) {
        this->plane->Mthrust = 0;
        this->plane->s = 0.001f;
        this->plane->b = 0.001f;
        this->target_speed = 0;
        this->target_climb = 0;
        this->target_azimut = 0;
        this->plane->SetSpoilers();
        this->plane->vz /= 1.5f;
        this->plane->vy *= 1.05f;
        this->plane->SetThrottle(0);
        this->control_stick_x = 0;
        this->control_stick_y = 0;
        this->alive = false;
        return;
    }

    if (!this->plane->on_ground && this->plane->GetWheel()) {
        this->gear = 0;
    }
    if (this->plane->on_ground && this->target_climb == 0) {
        this->throttle = 0;
        return;
    }

    float dt = this->getDeltaTime();
    this->controlThrottle();

    float roll_signed = signedRoll(this->plane->roll);

    // ============ PROTECTION ANTI-STALL (priorite absolue) ============
    if (this->plane->wing_stall > 0 && !this->plane->on_ground) {
        this->plane->SetThrottle(100);
        this->control_stick_y = 80; // pousser pour reprendre de la vitesse
        // Remettre les ailes a plat (roll_signed -> 0).
        // control_stick_x > 0 fait DIMINUER roll_signed ; amortissement de meme signe que roll_speed.
        float recover_stick = 0.50f * roll_signed + 1.0f * this->plane->roll_speed;
        this->control_stick_x = std::clamp((int)recover_stick, -160, 160);
        return;
    }

    // ============ LATERAL: cap -> angle de bank -> manche ============
    //
    // Conventions REELLES (verifiees) :
    //   control_stick_x > 0  => roule a DROITE => roll_signed DIMINUE (roll_speed < 0)
    //   control_stick_x < 0  => roule a GAUCHE => roll_signed AUGMENTE (roll_speed > 0)
    //   roll_signed : + = aile gauche basse, - = aile droite basse
    //   cap affiche = 360 - yaw/10  => virer a droite (cap+) <=> yaw diminue
    //
    float target_yaw  = norm3600(3600.0f - this->target_azimut);
    float heading_err = signed1800(target_yaw - this->plane->yaw);

    float bank_limit = this->maxBankForG(this->plane->object->entity->jdyn->max_g);
    bank_limit = std::clamp(bank_limit, 250.0f, 600.0f);
    if (this->actor != nullptr &&
        this->actor->current_command == prog_op::OP_SET_OBJ_DEFEND_TARGET) {
        bank_limit = std::clamp(bank_limit * 1.25f, 250.0f, 900.0f);
    }

    // Angle de bank vise (convention roll_signed) : virer a droite => bank NEGATIF.
    const float HEADING_DEADZONE = 15.0f; // 1.5 deg : cap considere atteint

    // --- Serrage du virage : par le ROULIS (le tangage ne sert qu'a l'altitude) ---
    float turn_demand = std::clamp((fabsf(heading_err) - HEADING_DEADZONE) / 250.0f, 0.0f, 1.0f);
    float g_scale = this->plane->object->entity->jdyn->max_g / 10.0f;
    float skill   = (this->actor && this->actor->profile) ? (this->actor->profile->ai.atrb.AA / 16.0f) : 0.5f;

    // On incline davantage pour tourner plus fort (la portance sup. part a l'horizontale).
    float extra_bank = 250.0f * turn_demand * g_scale * (0.6f + 0.8f * skill);
    float hard_bank_limit = std::clamp(bank_limit + extra_bank, 250.0f, 850.0f);
    float bank_cmd = std::clamp(heading_err * 2.5f, -hard_bank_limit, hard_bank_limit);
    if (fabsf(heading_err) < HEADING_DEADZONE) {
        bank_cmd = 0.0f;
    }
    if (bank_cmd < -1.0f) {
        this->turnState = TURN_RIGHT;
    } else if (bank_cmd > 1.0f)  {
        this->turnState = TURN_LEFT;
    } else {
        this->turnState = TURN_NONE;
    }

    // bank_err > 0 => il faut AUGMENTER roll_signed (rouler a gauche) => stick NEGATIF.
    // D'ou le -Kp. Amortissement +Kd*roll_speed : un stick de meme signe que roll_speed
    // s'oppose a la rotation (roll_speed < 0 en roulant a droite => freine).
    float bank_err  = bank_cmd - roll_signed;
    float roll_stick = -0.50f * bank_err + 1.0f * this->plane->roll_speed;
    this->control_stick_x = std::clamp((int)roll_stick, -160, 160);

    // ============ VERTICAL: altitude -> vario -> assiette -> manche (inchange) ============
    const float pitch_stick_sign = 1.0f;
    float lead_time  = 0.70f;
    float lead_ticks = std::clamp(lead_time / dt, 4.0f, 24.0f);

    float predicted_y = this->plane->y + this->plane->vy * lead_ticks;
    float alt_err     = (float)this->target_climb - predicted_y;

    float vario_cmd = std::clamp(alt_err * 0.08f, -150.0f, 200.0f);
    float vario_err = vario_cmd - this->plane->vy;

    // Feed-forward de facteur de charge : tirer juste ce qu'il faut pour tenir
    // l'altitude a l'inclinaison courante => pas de montee parasite.
    float bank_rad       = tenthOfDegreeToRad(fabsf(roll_signed));
    float load_factor_ff = 1.0f / (std::max)(0.20f, cosf(bank_rad)) - 1.0f;
    float pitch_cmd = 1.35f * vario_err + load_factor_ff * 120.0f;
    pitch_cmd = std::clamp(pitch_cmd, -220.0f, 320.0f);
    
    float pitch_err = pitch_cmd - this->plane->pitch;
    float desired_pitch_speed = 0.6f * pitch_err - 2.00f * this->plane->pitch_speed;

    float pitch_stick = pitch_stick_sign * desired_pitch_speed * 3.0f;
    this->control_stick_y = std::clamp((int)pitch_stick, -160, 160);


    PlaneControlEvent planeControlEvent;
    planeControlEvent.plane = this->plane;
    planeControlEvent.control_stick_x = this->control_stick_x;
    planeControlEvent.control_stick_y = this->control_stick_y;
    planeControlEvent.throttle = this->throttle;
    planeControlEvent.flaps = this->flap;
    planeControlEvent.spoilers = this->spoilers;
    planeControlEvent.wheel = this->gear;
    MessageBus::getInstance().publish(std::make_unique<PlaneControlEvent>(planeControlEvent));
}