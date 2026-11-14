#pragma once
#include "../strike_commander/precomp.h"
#include "../strike_commander/SCJetpPlane.h"
#include "PSEntity.h"

// Avion de Pacific Strike : modèle de vol de SCJetpPlane, avec la poussée de l'hélice et la répartition des armes de
// PACIFIC.EXE (strike_commander_re/analysis/pacific/README.md § 9).
class PSPlane : public SCJetpPlane {
protected:
    // Masque d'arme (sub_ADEA0 de ovr311), testé par les points d'emport.
    enum PSWeaponMask : uint8_t {
        PS_MASK_NONE = 0,
        PS_MASK_BOMB_100LB = 1,
        PS_MASK_BOMB_500LB = 2,
        PS_MASK_BOMB_1000LB = 4,
        PS_MASK_TORPEDO = 8,
        PS_MASK_ROCKET = 16,
        PS_MASK_NAPALM = 32,
        PS_MASK_GUN = 64
    };
    // Options de l'écran des options : 0 Ace, 1 Veteran, 2 Rookie (valeurs par défaut du jeu).
    int flight_model{ 1 };
    int enemy_ai{ 0 };
    int thrust_notch_computed{ -1 };
    int thrust_frame{ 0 };

    static PSWeaponMask weaponMask(RSEntity *weapon);
    int loadOnHardpoint(size_t hardpoint, RSEntity::WEAPS *weapon, int count);
    float lowSpeedPowerFactor();

public:
    PSPlane(float LmaxDEF, float LminDEF, float Fmax, float Smax, float ELEVF_CSTE, float ROLLFF_CSTE, float s, float W, float fuel_weight, float Mthrust, float b, float ie_pi_AR, int MIN_LIFT_SPEED, RSArea *area, float x, float y, float z);
    void computeThrust() override;
    void InitLoadout() override;
    int isOnRunWay() override;
    void Shoot(int weapon_hard_point_id, SCMissionActors *target, SCMission *mission) override;
    void ShootDirect(int weapon_hard_point_id, SCMissionActors *target, SCMission *mission) override;
    void firePacific(int weapon_hard_point_id, SCMissionActors *target, SCMission *mission);
    void launchFromHardpoint(int hardpoint, SCMissionActors *target, SCMission *mission);
};
