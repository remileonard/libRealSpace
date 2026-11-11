#pragma once
#include "../strike_commander/precomp.h"
#include "PSCockpitDef.h"

// Cockpit de Pacific Strike : fichier OBJECTS\<nom du chunk CKPT de l'avion>.IFF (PSCockpitDef), tableau de bord ARTP
// et instruments du FORM INST.
class PSCockpit : public SCCockpit {
protected:
    PSCockpitDef *ps_cockpit{ nullptr };
    // Facteurs du jeu (seg123 de PACIFIC.EXE) : mètres → pieds (dword_7979B), m/s → nœuds (dword_797A3), échelle de
    // l'altimètre (dword_79762, 1,0 par défaut, remplacée par un chunk de mission non identifié).
    float feet_per_meter{ 3.2773f };
    float knots_per_ms{ 1.9414f };
    float altimeter_scale{ 1.0f };

    // Cadre, centre et état de l'instrument pour la vue (sub_1792C) : vue 0 = premier cadre, vue 1 = second cadre.
    struct InstrumentView {
        bool active{ false };
        int left{ 0 };
        int top{ 0 };
        int center_x{ 0 };
        int center_y{ 0 };
        int right{ 0 };
        int bottom{ 0 };
        int view{ 0 };
    };
    InstrumentView selectView(PSCockpitInstrument *instrument);
    void drawNeedle(FrameBuffer *fb, const InstrumentView &view, float angle_deg, int length, uint8_t color);
    void drawImage(FrameBuffer *fb, PSCockpitInstrument *instrument, size_t image, int x, int y);
    void RenderAltimeter(PSCockpitInstrument *instrument, FrameBuffer *fb);
    void RenderAirspeed(PSCockpitInstrument *instrument, FrameBuffer *fb);
    void RenderTachometer(PSCockpitInstrument *instrument, FrameBuffer *fb);
    void RenderVerticalSpeed(PSCockpitInstrument *instrument, FrameBuffer *fb);
    void RenderFuel(PSCockpitInstrument *instrument, FrameBuffer *fb);
    void RenderCompass(PSCockpitInstrument *instrument, FrameBuffer *fb);
    void RenderBank(PSCockpitInstrument *instrument, FrameBuffer *fb);
    void RenderRudder(PSCockpitInstrument *instrument, FrameBuffer *fb);
    void RenderHorizon(PSCockpitInstrument *instrument, FrameBuffer *fb);
    void RenderLamp(PSCockpitInstrument *instrument, FrameBuffer *fb, bool on);
    void RenderAmmo(PSCockpitInstrument *instrument, FrameBuffer *fb);
    void RenderArmedGuns(PSCockpitInstrument *instrument, FrameBuffer *fb);
    void RenderBackupCompass(PSCockpitInstrument *instrument, FrameBuffer *fb);
    void RenderGunSight(PSCockpitInstrument *instrument, FrameBuffer *fb);
    float heading();
    float rollAngle();
    int gunStations(int hardpoint_type, bool count_ammo);

public:
    PSCockpit() = default;
    void init() override;
    void Render(CockpitFace face) override;
    PSCockpitDef *definition();
    size_t artImage(CockpitFace face);
    void RenderInstrument3D(PSCockpitInstrument *instrument, FrameBuffer *fb);
};
