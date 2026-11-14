#include "PSPlane.h"
#include "PSArea.h"

PSPlane::PSPlane(float LmaxDEF, float LminDEF, float Fmax, float Smax, float ELEVF_CSTE, float ROLLFF_CSTE, float s, float W, float fuel_weight, float Mthrust, float b, float ie_pi_AR, int MIN_LIFT_SPEED, RSArea *area, float x, float y, float z) : SCJetpPlane(LmaxDEF, LminDEF, Fmax, Smax, ELEVF_CSTE, ROLLFF_CSTE, s, W, fuel_weight, Mthrust, b, ie_pi_AR, MIN_LIFT_SPEED, area, x, y, z) {
}

// Facteur k de la puissance à basse vitesse : avion du joueur selon Flight Model:, objet du camp 255 selon Enemy AI:,
// 1 sinon.
float PSPlane::lowSpeedPowerFactor() {
    if (this->pilot == nullptr) {
        return 1.0f;
    }
    if (this->pilot->actor_name == "PLAYER") {
        if (this->flight_model == 0) {
            return 1.047f;
        }
        if (this->flight_model == 1) {
            return 1.074f;
        }
        return 1.098f;
    }
    if (this->pilot->team_id == 255) {
        if (this->enemy_ai == 0) {
            return 0.898f;
        }
        if (this->enemy_ai == 1) {
            return 0.949f;
        }
        return 1.0f;
    }
    return 1.0f;
}

// Aero_PropellerThrust_4710B : poussée de l'hélice, recalculée quand le cran change, sinon une image sur 8. Le plafond
// du cran par le gain moteur et l'arrêt sans carburant viennent de SCJetpPlane::computeThrust.
void PSPlane::computeThrust() {
    int max_notch = (int)lroundf(10.0f * this->g_engine);
    if (this->thrust > max_notch * 10) {
        this->thrust = max_notch * 10;
    }
    int notch = this->thrust / 10;
    this->thrust_frame++;
    if (notch != this->thrust_notch_computed || (this->thrust_frame % 8) == 0) {
        PSEntity *entity = static_cast<PSEntity *>(this->object->entity);
        float power = entity->engine_power_hp;
        float speed = sqrtf(this->vx * this->vx + this->vy * this->vy + this->vz * this->vz);
        float reference_speed = entity->reference_speed_ms;
        if (speed < 0.65625f * reference_speed) {
            power = power * this->lowSpeedPowerFactor();
        }
        if (speed > reference_speed) {
            power = power / 2.0f;
        }
        float rated_altitude = entity->rated_altitude_m;
        float ceiling = entity->ceiling_m;
        if (this->y > rated_altitude && ceiling > rated_altitude) {
            power = power - power * (this->y - rated_altitude) / (ceiling - rated_altitude);
        }
        float floor_speed = entity->stall_speed_ms / 2.0f;
        float v = speed > floor_speed ? speed : floor_speed;
        float force = 0.0f;
        if (v > 0.0f) {
            force = 0.8477f * 550.0f * power / (v * 3.2773f) * 4.4453f * (float)notch / 10.0f;
        }
        if (force < 0.0f) {
            force = 0.0f;
        }
        this->thrust_force = force * this->g_engine;
        if (notch != this->thrust_notch_computed) {
            printf("PSPlane: %s cran %d, vitesse %.1f m/s, altitude %.0f m, puissance %.0f ch, poussee %.0f N\n", this->pilot != nullptr ? this->pilot->actor_name.c_str() : "?", notch, speed, this->y, power, this->thrust_force);
        }
        this->thrust_notch_computed = notch;
    }
    if (this->fuel_kg <= 0.0f) {
        this->thrust_force = 0.0f;
    }
    this->thrust_vector = Vector3D(0.0f, 0.0f, -this->thrust_force);
}

PSPlane::PSWeaponMask PSPlane::weaponMask(RSEntity *weapon) {
    switch (PSEntity::weaponId(weapon)) {
    case PS_WEAPON_BOMB_100LB:
        return PS_MASK_BOMB_100LB;
    case PS_WEAPON_BOMB_500LB:
        return PS_MASK_BOMB_500LB;
    case PS_WEAPON_ROCKET:
        return PS_MASK_ROCKET;
    case PS_WEAPON_BOMB_1000LB:
        return PS_MASK_BOMB_1000LB;
    case PS_WEAPON_TORPEDO:
        return PS_MASK_TORPEDO;
    case PS_WEAPON_NAPALM:
        return PS_MASK_NAPALM;
    case PS_WEAPON_GUN:
        return PS_MASK_GUN;
    default:
        return PS_MASK_NONE;
    }
}

void PSPlane::Shoot(int weapon_hard_point_id, SCMissionActors *target, SCMission *mission) {
    this->firePacific(weapon_hard_point_id, target, mission);
}

void PSPlane::ShootDirect(int weapon_hard_point_id, SCMissionActors *target, SCMission *mission) {
    this->firePacific(weapon_hard_point_id, target, mission);
}

// SCPlane::launchDirect, puis le projectile créé est déplacé de la position de l'avion à celle du point d'emport :
// coordonnées HPTS (24.8, repère de l'objet : x = envergure, z = nez, y = haut) tournées comme le modèle de l'avion
// (lacet + 90° autour de Y, tangage autour de Z, −roulis autour de X ; objet : x = nez, y = haut, z = envergure).
void PSPlane::launchFromHardpoint(int hardpoint, SCMissionActors *target, SCMission *mission) {
    size_t before = this->weaps_object.size();
    Vector3D local(this->weaps_load[hardpoint]->position.z / 256.0f, this->weaps_load[hardpoint]->position.y / 256.0f, this->weaps_load[hardpoint]->position.x / 256.0f);
    this->launchDirect(hardpoint, target, mission);
    if (this->weaps_object.size() <= before) {
        return;
    }
    Matrix orientation;
    orientation.Identity();
    orientation.rotateM(tenthOfDegreeToRad(this->yaw + 900.0f), 0, 1, 0);
    orientation.rotateM(tenthOfDegreeToRad(this->pitch), 0, 0, 1);
    orientation.rotateM(tenthOfDegreeToRad(-this->roll), 1, 0, 0);
    Vector3D offset;
    offset.x = local.x * orientation.v[0][0] + local.y * orientation.v[1][0] + local.z * orientation.v[2][0];
    offset.y = local.x * orientation.v[0][1] + local.y * orientation.v[1][1] + local.z * orientation.v[2][1];
    offset.z = local.x * orientation.v[0][2] + local.y * orientation.v[1][2] + local.z * orientation.v[2][2];
    SCSimulatedObject *projectile = this->weaps_object.back();
    projectile->x += offset.x;
    projectile->y += offset.y;
    projectile->z += offset.z;
}

// Tir de l'arme du point d'emport avec les objets de libRealSpace (SCPlane::launchDirect), selon la catégorie du
// WDAT : 13 = canon, tous les points qui portent cette arme font feu ensemble (WeaponSystem_LaunchFromStation_3E2EC) ;
// 9 = bombe, lâchée du point le plus chargé qui porte cette arme ; 8 = roquette, lancée sans cible (non guidée).
void PSPlane::firePacific(int weapon_hard_point_id, SCMissionActors *target, SCMission *mission) {
    if (weapon_hard_point_id < 0 || (size_t)weapon_hard_point_id >= this->weaps_load.size()) {
        return;
    }
    SCWeaponLoadoutHardPoint *selected = this->weaps_load[weapon_hard_point_id];
    if (selected == nullptr || selected->objct == nullptr || selected->objct->wdat == nullptr) {
        return;
    }
    RSEntity *launcher = selected->objct;
    int slot = this->weaponTimerSlot(launcher);
    if (slot < 0 || this->weapon_timers[slot] > 0.0f) {
        return;
    }
    if (launcher->wdat->launch_class == 13) {
        for (int i = 0; i < (int)this->weaps_load.size(); i++) {
            SCWeaponLoadoutHardPoint *station = this->weaps_load[i];
            if (station == nullptr || station->objct != launcher || station->hpts_type != selected->hpts_type || station->nb_weap <= 0) {
                continue;
            }
            this->launchFromHardpoint(i, target, mission);
        }
    } else if (launcher->wdat->launch_class == 9) {
        int fullest = -1;
        int count = 0;
        for (int i = 0; i < (int)this->weaps_load.size(); i++) {
            SCWeaponLoadoutHardPoint *station = this->weaps_load[i];
            if (station != nullptr && station->objct == launcher && station->nb_weap > count) {
                fullest = i;
                count = station->nb_weap;
            }
        }
        if (fullest < 0) {
            return;
        }
        this->launchFromHardpoint(fullest, target, mission);
        printf("PSPlane: %s lache %s du point %d, reste %d\n", this->object->member_name.c_str(), selected->name.c_str(), fullest, this->weaps_load[fullest]->nb_weap);
    } else if (launcher->wdat->launch_class == 8) {
        int fullest = -1;
        int count = 0;
        for (int i = 0; i < (int)this->weaps_load.size(); i++) {
            SCWeaponLoadoutHardPoint *station = this->weaps_load[i];
            if (station != nullptr && station->objct == launcher && station->nb_weap > count) {
                fullest = i;
                count = station->nb_weap;
            }
        }
        if (fullest < 0) {
            return;
        }
        this->launchFromHardpoint(fullest, nullptr, mission);
        printf("PSPlane: %s tire %s du point %d, reste %d\n", this->object->member_name.c_str(), selected->name.c_str(), fullest, this->weaps_load[fullest]->nb_weap);
    } else {
        return;
    }
    this->weapon_timers[slot] = launcher->wdat->fire_interval;
}

// sub_AE513 de ovr311 : charge au plus count armes sur un point d'emport libre ; capacité selon le type du point
// (HPTS) : 0 et 4 = canon, toute la quantité ; 1 = roquette ×1 ; 2 = bombe de 100 lb ×1 ; 3 = bombe de 500 ou
// 1000 lb, torpille ou napalm ×1. Rend le nombre chargé.
int PSPlane::loadOnHardpoint(size_t hardpoint, RSEntity::WEAPS *weapon, int count) {
    if (this->weaps_load[hardpoint] != nullptr) {
        return 0;
    }
    PSWeaponMask mask = weaponMask(weapon->objct);
    int capacity = 0;
    switch (this->object->entity->hpts[hardpoint]->id) {
    case 0:
    case 4:
        if (mask == PS_MASK_GUN) {
            capacity = count;
        }
        break;
    case 1:
        if (mask == PS_MASK_ROCKET) {
            capacity = 1;
        }
        break;
    case 2:
        if (mask == PS_MASK_BOMB_100LB) {
            capacity = 1;
        }
        break;
    case 3:
        if (mask == PS_MASK_BOMB_500LB || mask == PS_MASK_BOMB_1000LB || mask == PS_MASK_TORPEDO || mask == PS_MASK_NAPALM) {
            capacity = 1;
        }
        break;
    default:
        break;
    }
    int loaded = count < capacity ? count : capacity;
    if (loaded <= 0) {
        return 0;
    }
    SCWeaponLoadoutHardPoint *station = new SCWeaponLoadoutHardPoint();
    station->objct = weapon->objct;
    station->nb_weap = loaded;
    station->hpts_type = this->object->entity->hpts[hardpoint]->id;
    station->name = weapon->name;
    station->position = Vector3D((float)this->object->entity->hpts[hardpoint]->x, (float)this->object->entity->hpts[hardpoint]->y, (float)this->object->entity->hpts[hardpoint]->z);
    station->hud_pos.x = 0;
    station->hud_pos.y = 0;
    this->weaps_load[hardpoint] = station;
    return loaded;
}

// sub_AEC5A de ovr311 : le premier canon de la liste WPNS va sur tous les points de type 0, les canons suivants sur
// les points de type 4, chacun avec toute sa quantité. Les autres armes se répartissent depuis les deux bouts de la
// liste HPTS : quand les deux points ont le même type, chacun reçoit la moitié du reste ; sinon le premier reçoit tout
// et seul lui avance.
void PSPlane::InitLoadout() {
    std::vector<HPTS *> &hardpoints = this->object->entity->hpts;
    this->weaps_load.resize(hardpoints.size());
    for (size_t i = 0; i < this->weaps_load.size(); i++) {
        this->weaps_load[i] = nullptr;
    }
    uint8_t gun_hardpoint_type = 0;
    for (auto weapon : this->object->entity->weaps) {
        if (weapon->objct == nullptr) {
            continue;
        }
        if (weaponMask(weapon->objct) == PS_MASK_GUN) {
            for (size_t i = 0; i < hardpoints.size(); i++) {
                if (hardpoints[i]->id == gun_hardpoint_type) {
                    this->loadOnHardpoint(i, weapon, weapon->nb_weap);
                }
            }
            gun_hardpoint_type = 4;
            continue;
        }
        int remaining = weapon->nb_weap;
        int low = 0;
        int high = (int)hardpoints.size() - 1;
        while (remaining > 0 && low <= high) {
            if (low != high && hardpoints[low]->id == hardpoints[high]->id) {
                int half = remaining >> 1;
                remaining -= this->loadOnHardpoint(low, weapon, half);
                low++;
                remaining -= this->loadOnHardpoint(high, weapon, half);
                high--;
            } else {
                remaining -= this->loadOnHardpoint(low, weapon, remaining);
                low++;
            }
        }
        if (remaining > 0) {
            printf("PSPlane: %s, %d %s non charges, aucun point d'emport ne les accepte\n", this->object->member_name.c_str(), remaining, weapon->name.c_str());
        }
    }
    for (size_t i = 0; i < this->weaps_load.size(); i++) {
        if (this->weaps_load[i] != nullptr) {
            printf("PSPlane: %s point %zu (type %d) : %d %s\n", this->object->member_name.c_str(), i, this->weaps_load[i]->hpts_type, this->weaps_load[i]->nb_weap, this->weaps_load[i]->name.c_str());
        }
    }
}

// Le pont d'un porte-avions est une piste : l'avion qui s'y pose ne s'écrase pas (Aircraft_ContactWithObject_3CA69).
// C'est la carte (PSArea) qui sait s'il y a un pont sous l'avion.
int PSPlane::isOnRunWay() {
    float deck = 0.0f;
    if (static_cast<PSArea *>(this->area)->carrierDeckAt(this->x, this->z, deck)) {
        return 1;
    }
    return SCPlane::isOnRunWay();
}
