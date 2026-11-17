//
//  EnhancedOrbit.h
//  libRealSpace
//
//  Mouse orbit for the external cameras (chase F2, target F7, rotating cameras of the camera director, missile camera F8).
//  A mixin over a flight activity (SCStrike or PSStrike), so Strike Commander and Pacific Strike share it:
//    class EnhancedStrike : public EnhancedOrbit<SCStrike>
//    class EnhancedPSStrike : public EnhancedOrbit<PSStrike>
//  The derived class calls orbitInit() once, orbitAfterInput(panelOpen) from afterInput() and orbitAfterCameraPlaced() from
//  afterCameraPlaced(). The mouse position orbits the camera around the plane without limit (one screen width = 360 degrees),
//  the wheel zooms, the middle button resets the zoom. While the orbit camera owns the mouse, the SDL relative mouse mode is on.
//
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include "../strike_commander/precomp.h"
#include "../strike_commander/SCStrike.h"
#include "../engine/EventManager.h"
#include "../strike_commander/SCMission.h"
#include "../strike_commander/SCPlane.h"

template <class Base>
class EnhancedOrbit : public Base {
protected:
    // [Video] camera_mouse_invert_y and camera_mouse_orbit
    void orbitInit() {
        Config &cfg = Config::getInstance();
        this->orbitInvertY = cfg.getString("Video", "camera_mouse_invert_y", "1") != "0";
        printf("[enhanced] camera mouse orbit: invert_y=%d\n", this->orbitInvertY ? 1 : 0);
        this->orbitEnabled = cfg.getString("Video", "camera_mouse_orbit", "1") != "0";
        printf("[enhanced] outside camera mouse orbit: %s\n", this->orbitEnabled ? "on" : "off");
    }

    // panelOpen: a panel (options, question) owns the mouse
    void orbitAfterInput(bool panelOpen) {
        if ((int)this->camera_mode != this->orbitLastView) {
            this->orbitLastView = (int)this->camera_mode;
            this->orbitZoom = 1.0f;
        }
        const bool allowed = this->orbitAllowed();
        if (allowed != this->orbitActive) {
            printf("[enhanced] camera mouse orbit: %s (view %d)\n", allowed ? "on" : "off", (int)this->camera_mode);
            this->orbitActive = allowed;
        }
        this->setOrbitRelativeMouse(allowed && !panelOpen);
        if (allowed && panelOpen) {
            // a panel is open: the mouse belongs to the panel, the camera keeps its current orbit
            if (this->mouse_control) {
                this->player_plane->control_stick_x = 0;
                this->player_plane->control_stick_y = 0;
            }
            return;
        }
        if (!allowed) {
            this->orbitYaw = 0.0f;
            this->orbitPitch = 0.0f;
            return;
        }
        this->updateOrbit();
        // the mouse position steers the camera here, not the plane (a key press has already switched mouse_control off)
        if (this->mouse_control) {
            this->player_plane->control_stick_x = 0;
            this->player_plane->control_stick_y = 0;
        }
    }

    // places the camera on its orbit around the plane (or the missile)
    void orbitAfterCameraPlaced() {
        if (!this->orbitActive || !this->orbitAllowed()) {
            return;
        }
        Vector3D pivot;
        if (this->camera_mode == View::MISSILE_CAM) {
            this->player_plane->weaps_object.back()->GetPosition(&pivot);
        } else {
            SCMissionActors *subject = this->current_mission->camera_director->activeSubject();
            if (subject == nullptr) {
                return;
            }
            if (subject->plane != nullptr) {
                pivot = Vector3D(subject->plane->x, subject->plane->y, subject->plane->z);
            } else {
                pivot = subject->object->position;
            }
        }
        Point3D current = this->camera->getPosition();
        Vector3D position = this->orbitedPosition(pivot, Vector3D(current.x, current.y, current.z));
        this->camera->SetPosition(&position);
        if (this->camera_mode == View::MISSILE_CAM) {
            this->camera->lookAt(&pivot);
        } else {
            Vector3D up = this->current_mission->camera_director->up();
            this->camera->lookAt(&pivot, &up);
        }
        if (this->orbitTraceFrames > 0) {
            printf("[enhanced] orbit: pivot (%.1f %.1f %.1f) camera (%.1f %.1f %.1f) yaw %.2f pitch %.2f zoom %.2f\n", pivot.x, pivot.y,
                   pivot.z, position.x, position.y, position.z, this->orbitYaw, this->orbitPitch, this->orbitZoom);
            this->orbitTraceFrames--;
        }
    }

    bool orbitEnabled{true};

private:
    // Chase (F2), target (F7) and orbit cameras of the camera director, and the missile camera (F8)
    bool orbitAllowed() const {
        if (!this->orbitEnabled) {
            return false;
        }
        if (this->camera_mode == View::MISSILE_CAM) {
            return !this->player_plane->weaps_object.empty() && this->player_plane->weaps_object.back() != nullptr;
        }
        if (this->camera_mode != View::CAM_DIRECTOR) {
            return false;
        }
        RSCameraType type = this->current_mission->camera_director->activeCameraType();
        return type == RSCameraType::RSCAM_CHAS || type == RSCameraType::RSCAM_TARG || type == RSCameraType::RSCAM_ROTA;
    }

    // The mouse movement orbits the camera around the plane without limit (screen width = 360 degrees), wheel zooms,
    // middle button resets. The relative mouse mode keeps the movement coming when the pointer would hit the screen edge.
    void updateOrbit() {
        const float halfWidth = (float)(std::max)(1, this->Screen->logical_width / 2);
        const float halfHeight = (float)(std::max)(1, this->Screen->logical_height / 2);
        int dx = 0;
        int dy = 0;
        EventManager::getInstance().getMouseMotion(dx, dy);
        this->orbitYaw += (float)dx * (float)M_PI / halfWidth;
        this->orbitYaw = remainderf(this->orbitYaw, 2.0f * (float)M_PI);
        const float pitchStep = (float)dy * 1.35f / halfHeight;
        this->orbitPitch += this->orbitInvertY ? -pitchStep : pitchStep;
        this->orbitPitch = (std::min)(1.35f, (std::max)(-0.9f, this->orbitPitch));
        int wheel = EventManager::getInstance().getMouseWheel();
        if (wheel != 0) {
            this->orbitZoom = (std::min)(3.0f, (std::max)(0.35f, this->orbitZoom * powf(0.9f, (float)wheel)));
        }
        if (this->m_keyboard->isActionJustPressed(InputAction::MOUSE_MIDDLE)) {
            this->orbitZoom = 1.0f;
        }
    }

    // Endless mouse movement while the orbit camera owns the mouse, normal pointer otherwise (panels)
    void setOrbitRelativeMouse(bool on) {
        if (on == this->orbitRelativeMouse) {
            return;
        }
        this->orbitRelativeMouse = on;
        SDL_SetRelativeMouseMode(on ? SDL_TRUE : SDL_FALSE);
        printf("[enhanced] orbit relative mouse: %s\n", on ? "on" : "off");
    }

    // Yaw about the world vertical, then raise/lower the camera, distance scaled by the zoom
    Vector3D orbitedPosition(Vector3D pivot, Vector3D position) const {
        Vector3D offset = position - pivot;
        const float distance = offset.Length();
        if (distance < 1e-3f) {
            return position;
        }
        Vector3D axis = offset * (-1.0f / distance);
        const float c = cosf(this->orbitYaw);
        const float s = sinf(this->orbitYaw);
        Vector3D turned(axis.x * c + axis.z * s, axis.y, -axis.x * s + axis.z * c);
        const float cp = cosf(this->orbitPitch);
        const float sp = sinf(this->orbitPitch);
        axis = Vector3D(turned.x * cp, turned.y * cp - sp, turned.z * cp);
        axis.Normalize();
        return pivot - axis * (distance * this->orbitZoom);
    }

    bool orbitActive{false};
    bool orbitInvertY{true};
    bool orbitRelativeMouse{false};
    float orbitYaw{0.0f};
    float orbitPitch{0.0f};
    float orbitZoom{1.0f};
    int orbitLastView{-1};
    int orbitTraceFrames{5};
};
