//
//  SCCockpit.h
//  libRealSpace
//
//  Created by Rémi LEONARD on 02/09/2024.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//
#pragma once
#ifndef __libRealSpace__SCCockpit__
#define __libRealSpace__SCCockpit__

#include "precomp.h"


struct HudLine {
    Point2D start;
    Point2D end;
};

class SCCockpit {
private:
    AssetManager &Assets = AssetManager::getInstance();
    RSFontManager &FontManager = RSFontManager::getInstance();
    RSMixer &Mixer = RSMixer::getInstance();
    RSVGA &VGA = RSVGA::getInstance();
    SCMouse &Mouse = SCMouse::getInstance();
    std::vector<HudLine> horizon;
    bool project_to_screen(Vector3D coord, int &Xout, int &Yout);
    void IdentifyRAWSContact(SCMissionActors *actor, FrameBuffer *fb, float headingRad, Point2D pmfd_left, Point2D raws_size, bool is_zoomed, int rsize);
    void RenderTargetingReticle(FrameBuffer *fb, CHUD_SHAPE *reticleShape, Point2D hudTopLeft, Point2D hudBottomRight, Point2D hudCenter);
    void RenderBombSight(FrameBuffer* fb, Point2D hudTopLeft, Point2D hudBottomRight, Point2D hudCenter);
    void RenderMFDS(Point2D mfds, FrameBuffer *fb);
    void RenderMissileHud(Point2D position, FrameBuffer *fb, CHUD *hud, Point2D hudTopLeft, Point2D hudBottomRight, Point2D hudCenter);
    void RenderMFDSRadarImplementation(Point2D pmfd_left, float range, const char* mode_name, bool air_mode, FrameBuffer *fb);
    void RenderMFDSRadarSingleTargetImplementation(Point2D pmfd_left, float range, const char *mode_name, bool air_mode, FrameBuffer *fb);
    void BuildPaletteLUT();
    void printTTAG(Point2D pos, HUD_POS &tag, std::string name, FrameBuffer *fb, RSFont *font);
    RSFont *font;
    RSFont *big_font;
    int radio_mission_timer{0};
    std::unordered_map<uint32_t, uint8_t> palette_lut;
    std::unordered_map<std::string, std::string> hud_text_tags;
    bool palette_lut_dirty = true;
    int current_weapon_id{-1};
    Camera cockpit_camera;
    
public:
    VGAPalette palette;
    RSCockpit* cockpit{nullptr};
    RSHud* hud{nullptr};
    FrameBuffer *hud_framebuffer{nullptr};
    FrameBuffer *mfd_right_framebuffer{nullptr};
    FrameBuffer *mfd_left_framebuffer{nullptr};
    FrameBuffer *raws_framebuffer{nullptr};
    FrameBuffer *target_framebuffer{nullptr};
    FrameBuffer *alti_framebuffer{nullptr};
    FrameBuffer *speed_framebuffer{nullptr};
    FrameBuffer *comm_framebuffer{nullptr};
    FrameBuffer *debug_framebuffer{nullptr};
    float pitch{0.0f};
    float roll{0.0f};
    float yaw{0.0f};

    float speed{0.0f};
    float mach{0.0f};
    float altitude{0.0f};
    float heading{0.0f};

    float g_limit{0.0f};
    float g_load{0.0f};
    float pitch_speed{0.0f};
    float yaw_speed{0.0f};
    float roll_speed{0.0f};

    bool gear{false};
    bool flaps{false};
    bool airbrake{false};
    bool mouse_control{false};
    bool show_radars{false};
    bool show_weapons{false};
    bool show_damage{false};
    bool show_comm{false};
    bool show_cam{false};
    bool target_in_range{false};
    RadarMode radar_mode{RadarMode::AARD};
    int radar_zoom{1};
    int throttle{0};
    int comm_target{0};
    SCMissionActors *comm_actor{nullptr};
    float way_az{0};
    MISN_PART *target{nullptr};
    MISN_PART *player{nullptr};
    RSProf *player_prof{nullptr};
    std::vector<MISN_PART *> parts;
    std::vector<SCAiPlane *> ai_planes;
    Camera *cam;
    CockpitFace face{CockpitFace::CP_FRONT};
    Vector2D weapoint_coords;
    SCPlane *player_plane;
    SCMission *current_mission;
    uint8_t *nav_point_id{nullptr};
    Hud_weapon_mode weapon_mode{Hud_weapon_mode::WM_HUD_NONE};
    Vector3D hud_eye_world = {0.0f, 0.0f, 0.0f};
    bool has_hud_eye_world = false;
    bool debug_print{false};
    bool big_cockpit{false};
    bool is_3d_cockpit{false};
    // Offset angulaire pour le viseur cannon (en radians)
    // x = azimut, y = élévation
    // En 2D: {0, 0}, en 3D: ajuster selon la géométrie
    Vector2D cannonAngularOffset = {0.0f, 0.0f};
    Vector3D targetImpactPointWorld = {0.0f, 0.0f, 0.0f};
    SCCockpit();
    ~SCCockpit();
    void init( );
    void Update();
    void Render(CockpitFace face);
    void RenderHUD();
    void RenderHUD(Point2D position, FrameBuffer *fb);
    void RenderMFDSWeapon(Point2D pmfd_right, FrameBuffer *fb);
    void RenderMFDSRadar(Point2D pmfd_left, float range, int mode, FrameBuffer *fb);
    void RenderMFDSComm(Point2D pmfd_left, int mode, FrameBuffer *fb);
    void RenderMFDSDamage(Point2D pmfd_left, FrameBuffer *fb);
    void RenderRAWS(Point2D pmfd_left, FrameBuffer *fb);
    void RenderRAWSBig(Point2D pmfd_left, FrameBuffer *fb);
    void RenderTargetWithCam(Point2D top_left, FrameBuffer *fb);
    void RenderAlti(Point2D alti_pos, FrameBuffer *fb);
    void RenderSpeedOmetter(Point2D speed_top_left, FrameBuffer *fb);
    bool RenderCommMessages(Point2D pmfd_text, FrameBuffer *fb);
    void RenderMFDSCamera(Point2D pmfd_left, FrameBuffer *fb);
    void SetCommActorTarget(int target);
    void RenderTextTags(Point2D position, FrameBuffer *fb, CHUD *hud, RSFont *font);
    void RenderAltiBandRoll(Point2D alti_top_left, FrameBuffer *fb, RSFont *font, CHUD_SHAPE *alti_band_roll);
    void RenderSpeedBandRoll(Point2D speed_top_left, FrameBuffer *fb, RSFont *sfont, CHUD_SHAPE *speed_band);
    void RenderHeadingCompas(Point2D heading_top_left, FrameBuffer *fb, RSFont *sfont, CHUD_SHAPE *heading_compas);
    void RenderPitchLadder(Point2D center, Point2D clip_size, FrameBuffer *fb, SLADD *ladd, RSFont *ft);
};
#endif