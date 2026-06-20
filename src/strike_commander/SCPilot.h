//
//  SCPilot.h
//  libRealSpace
//
//  Created by Rémi LEONARD on 23/09/2024.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//

#pragma once

#include "precomp.h"
class PIDController {
public:
    PIDController() : kp(0), ki(0), kd(0), i_limit(0), prev_error(0), integral(0) {};
    PIDController(float kp, float ki, float kd, float i_limit = 0.0f)
        : kp(kp), ki(ki), kd(kd), i_limit(i_limit), prev_error(0), integral(0) {};
    float update(float error, float dt) {
        if (dt <= 0.0f) {
            dt = 1.0f / 30.0f;
        }
        this->integral += error * dt;
        if (this->i_limit > 0.0f) {
            if (this->integral > this->i_limit) {
                this->integral = this->i_limit;
            }
            if (this->integral < -this->i_limit) {
                this->integral = -this->i_limit;
            }
        }
        float derivative = (error - this->prev_error) / dt;
        this->prev_error = error;
        return this->kp * error + this->ki * this->integral + this->kd * derivative;
    };
    void reset() {
        this->prev_error = 0.0f;
        this->integral = 0.0f;
    };

private:
    float kp;
    float ki;
    float kd;
    float i_limit;
    float prev_error;
    float integral;
};

class SCPilot {
private:
    bool alive{true};
    enum TurnState {
        TURN_NONE,
        TURN_LEFT,
        TURN_RIGHT
    };

    float turnRate;
    float calculateMaxTurnRate(float airspeed, float maxG);

    // Boucle laterale en cascade : cap -> inclinaison, inclinaison -> taux de roulis, taux -> manche
    PIDController heading_pid{1.0f, 0.0f, 0.0f, 0.0f};
    PIDController bank_pid{0.6f, 0.0f, 0.02f, 0.0f};
    PIDController roll_rate_pid{0.5f, 0.0f, 0.01f, 0.0f};
    // Boucle verticale en cascade : altitude -> vario, vario -> assiette, assiette -> manche
    PIDController vspeed_pid{1.5f, 0.0f, 0.05f, 0.0f};
    PIDController pitch_pid{1.2f, 0.0f, 0.08f, 0.0f};

    float getDeltaTime();
    float maxBankForG(float maxG);
    void controlThrottle();

public:
    Vector3D target_waypoint{0.0f, 0.0f, 0.0f};
    bool turning{false};
    int target_speed{0};
    int target_climb{0};
    float target_azimut{0.0f};
    float old_target_azimut{0.0f};
    float targetRoll{0.0f};
    bool land{false};
    TurnState turnState;
    SCPlane* plane{nullptr};
    SCMissionActors *actor{nullptr};
    SCPilot();
    ~SCPilot();
    void SetTargetWaypoint(Vector3D waypoint);
    void DirectAutoPilot();
    void AutoPilot();
    void FlyTo();
};