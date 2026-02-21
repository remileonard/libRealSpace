#pragma once
int mrandom(int maxr);
void gl_sincos(float a, float *b, float *c);
class SCJdynPlane : public SCPlane {
protected:

    float fuel_max {0.0f};

    float airspeed_in_ms {0.0f};
    void updatePosition();
    void updateAcceleration();
    void updateVelocity();
    void updateForces();
    void updateSpeedOfSound();
    void checkStatus();
    void computeLift();
    void computeDrag();
    void computeGravity();
    void computeThrust();
    void processInput();
public:
    SCJdynPlane();
    SCJdynPlane(float LmaxDEF, float LminDEF, float Fmax, float Smax, float ELEVF_CSTE, float ROLLFF_CSTE, float s, float W,
            float fuel_weight, float Mthrust, float b, float ie_pi_AR, int MIN_LIFT_SPEED,
            RSArea *area, float x, float y, float z);
    ~SCJdynPlane();
    void Simulate();
};