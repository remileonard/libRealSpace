#pragma once
//
//  SCStrike.h
//  libRealSpace
//
//  Created by fabien sanglard on 1/28/2014.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//
#include "precomp.h"
#include <map>

#ifndef __libRealSpace__SCStrike__
#define __libRealSpace__SCStrike__

#define SCSTRIKE_MAX_MISSIONS 47
static const char *mission_list[] = {
    "MISN-1A.IFF",  "MISN-1B.IFF",  "MISN-1C.IFF",  "MISN-2A.IFF",  "MISN-3A.IFF",  "MISN-3B.IFF",  "MISN-3C.IFF",
    "MISN-4A.IFF",  "MISN-4B.IFF",  "MISN-4C.IFF",  "MISN-5A.IFF",  "MISN-5B.IFF",  "MISN-5BB.IFF", "MISN-5C.IFF",
    "MISN-6A.IFF",  "MISN-6B.IFF",  "MISN-6C.IFF",  "MISN-6D.IFF",  "MISN-7A.IFF",  "MISN-7B.IFF",  "MISN-7C.IFF",
    "MISN-7D.IFF",  "MISN-8A.IFF",  "MISN-8B.IFF",  "MISN-8C.IFF",  "MISN-9AA.IFF", "MISN-9A.IFF",  "MISN-9B.IFF",
    "MISN-10A.IFF", "MISN-10B.IFF", "MISN-10C.IFF", "MISN-10D.IFF", "MISN-11A.IFF", "MISN-11B.IFF", "MISN-11C.IFF",
    "MISN-12A.IFF", "MISN-12B.IFF", "MISN-13A.IFF", "MISN-13B.IFF", "MISN-14A.IFF", "MISN-14B.IFF", "MISN-15A.IFF",
    "MISN-6X.IFF",  "MISN-8X.IFF",  "MISN-10X.IFF", "MISN-11X.IFF", "TEMPLATE.IFF"
};
static int mission_idx = 0;

/**
 * @class SCStrike
 * @brief Implements the main game logic for Strike Commander
 *
 * This class implements the main game logic for Strike Commander. It handles
 * mission selection, keyboard input, game simulation, and rendering.
 */
class SCStrike : public IActivity, public Creatable<SCStrike> {
protected:
    uint8_t camera_mode;
    uint8_t nav_point_id{0};
    uint8_t current_target{0};
    uint8_t current_object_to_view{0};

    int32_t mfd_timeout{0};

    int engine_channel_volume{5};
    int world_lod{BLOCK_LOD_MAX};
    int max_stick_x{0};
    int max_stick_y{0};
    int stick_sensitivity{1};
    int eye_y{3};
    int radio_mission_timer{0};
    int music{2};
    
    bool air_weapons_mode{false};
    bool mouse_control;
    bool pause_simu{false};
    bool options_pause_before{ false };
    bool options_mouse_before{ false };
    bool autopilot{false};
    bool show_bbox{false};
    bool follow_dynamic{false};
    bool zoom_cockpit{false};
    bool rudder_pressed{false};
    bool axis_stick{false};
    
    float throttle_axis{-2.0f};
    float end_panel_time{-1.0f};
    float yaw{0.0f};
    float autopilot_timeout{0.0f};
    float crash_cam_distance{20.0f};
    float crash_cam_travel{0.1f};
    float eject_cam_y_offset{15.0f};
    float eject_cam_z_offset{-20.0f};
    float autopilot_target_azimuth{0};
    float counter{0.0f};
    float mouseAxisAccumX{0.0f};
    float mouseAxisAccumY{0.0f};

    Point3D camera_pos{0.0f, 0.0f, 0.0f};
    Point3D target_camera_pos{0.0f, 0.0f, 0.0f};
    Point3D light{4.0f, 4.0f, 4.0f};
    Point3D new_position{0.0f, 0.0f, 0.0f};

    Point2D pilote_lookat{0, 0};
    Vector2D gunsight_hud_offset{0.0f, -0.077f};

    std::string miss_file_name;
    
    SCFlightOptions *flight_options{ nullptr };
    
    Camera *camera;
    Quaternion new_orientation;

    RSArea *area;
    SCNavMap *nav_screen;
    SCPlane *player_plane;
    SCCockpit *cockpit;
    SCPilot pilot;
    RSProf *player_prof;
    SCState &GameState = SCState::getInstance();
    SCVirtualCockpit *virtual_cockpit{nullptr};
    std::vector<SCSimulatedObject*> simulated_objects;
    SCSimulatedObject* ejected_object{nullptr};
    std::unordered_map<std::string, RSEntity *> object_cache;
    SCMissionActors *target{nullptr};
    SCMission *current_mission{nullptr};

    Keyboard *m_keyboard{nullptr};
    View last_director_view{View::FRONT};
    
    typedef void (SCStrike::*KeyHandler)();
    struct KeyCommand {
        KeyHandler handler;
        bool held{false};
        bool when_disabled{false};
    };
    
    static std::map<int, KeyCommand> buildKeyCommandTable();
    RSEntity * loadWeapon(std::string name);
    void setThrottlePreset(int throttle);
    void sendCommRadioMessage(int message);
    void renderVirtualCockpit();
    void initCockpit();
    void autopilotCompute();
    void findTarget();
    void registerSimulatorInputs();
    void openFlightOptions();
    void closeFlightOptions();
    void renderFlightOptions();
    void stopAllSounds(const char *reason);
    void endMission();
    void restartMission();
    void updateEndPanel(SCFlightOptions::ExitMode mode);

    virtual const std::map<int, KeyCommand> &keyCommandTable();
    virtual bool confirmEndMission() { return true; }

    virtual void afterCameraPlaced() {}
    virtual void afterCockpitSceneRendered() {}
    virtual void afterFrameRendered() {}
    virtual void renderScene(int32_t viewportW, int32_t viewportH, float projVerticalOffset, bool forceVirtualCockpit);
    virtual void renderWorld();
    virtual void renderPlayerPlane();
    virtual void afterPlaneRendered(SCPlane *plane, Vector3D position, int thrust) {}
    virtual void afterInput() {}
    virtual void keyEndMission();
    virtual void keyRudderLeft();
    virtual void keyRudderRight();
    virtual void keyThrottleUp();
    virtual void keyThrottleDown();
    virtual void keyFirePrimary();
    virtual void keyToggleMouse();
    virtual void keyAutopilot();
    virtual void keyLandingGear();
    virtual void keyToggleBrakes();
    virtual void keyToggleFlaps();
    virtual void keyTargetNearest();
    virtual void keyMfdRadar();
    virtual void keyMfdDamage();
    virtual void keyMfdWeapons();
    virtual void keyShowNavMap();
    virtual void keyChaff();
    virtual void keyFlare();
    virtual void keyRadarZoomIn();
    virtual void keyRadarZoomOut();
    virtual void keyCommRadio();
    virtual void keyEject();
    virtual void keyViewTarget();
    virtual void keyViewBehind();
    virtual void keyViewCockpit();
    virtual void keyRadarModeToggle();
    virtual void keyViewWeapons();
    virtual void keyLookForward();
    virtual void keyLookLeft();
    virtual void keyLookRight();
    virtual void keyLookBehind();
    virtual void keyMfdTargetCamera();
    virtual void keyWeaponModeToggle();
    virtual void keyInfiniteAmmoToggle();
    virtual void keySingleTargetMode();
    virtual void keyThrottle10();
    virtual void keyThrottle20();
    virtual void keyThrottle30();
    virtual void keyThrottle40();
    virtual void keyThrottle50();
    virtual void keyThrottle60();
    virtual void keyThrottle70();
    virtual void keyThrottle80();
    virtual void keyThrottle90();
    virtual void keyThrottle100();
    virtual void keyCommRadioM1();
    virtual void keyCommRadioM2();
    virtual void keyCommRadioM3();
    virtual void keyCommRadioM4();
    virtual void keyCommRadioM5();
    virtual void keyCommRadioM6();
    virtual void keyCommRadioM7();
    virtual void keyCommRadioM8();
    virtual void keyPause();
    virtual void keyPitchUp();
    virtual void keyPitchDown();
    virtual void keyRollLeft();
    virtual void keyRollRight();
    virtual void keyEyesOnTarget();
    virtual void keySpecKey1();
    virtual void keySpecKey2();
public:
    float verticalOffset{0.45f};
    bool player_loadout{false};
    
    SCStrike();
    ~SCStrike();
    void init() override;
    void start(void) override;
    void setCameraFront();
    void setCameraFollow(SCPlane *plane);
    void setCameraRLR();
    void setCameraRLRUpDown();
    void setCameraLookat(Vector3D obj_pos);
    void setCameraLookat(Vector3D obj_pos, Vector3D offset);
    void checkKeyboard(void);
    void runFrame(void) override;

    virtual void setMission(char const *missionName);
    virtual void firePrimary();
    virtual void selectNextWeapon();
    virtual SCMission *createMission(char const *missionName);
    virtual SCCockpit *createCockpit();
    virtual SCVirtualCockpit *createVirtualCockpit(SCCockpit *cockpit);
};

#endif /* defined(__libRealSpace__SCStrike__) */
