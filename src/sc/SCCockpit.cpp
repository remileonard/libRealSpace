//
//  SCCockpit.cpp
//  libRealSpace
//
//  Created by Rémi LEONARD on 02/09/2024.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//
#include "precomp.h"

Point2D rotateAroundPoint(Point2D point, Point2D center, float angle) {
    float x = (float)(point.x - center.x);
    float y = (float)(point.y - center.y);
    float newx = x * cos(angle) - y * sin(angle) + center.x;
    float newy = x * sin(angle) + y * cos(angle) + center.y;
    return {(int)newx, (int)newy};
}
Point2D rotate(Point2D point, float angle) {
    float newx = point.x * cos(angle) - point.y * sin(angle);
    float newy = point.x * sin(angle) + point.y * cos(angle);
    return {(int)newx, (int)newy};
}
SCCockpit::SCCockpit() {}
SCCockpit::~SCCockpit() {}
/**
 * SCCockpit::Init
 *
 * Initialize the cockpit object from the standard SC cockpit assets.
 *
 * This function reads the palette from the standard SC palette IFF file,
 * initializes an RSPalette object from it, and assigns the result to the
 * SCCockpit::palette member variable.
 *
 * It also initializes the cockpit object from the F16 cockpit IFF file
 * and assigns the result to the SCCockpit::cockpit member variable.
 */
void SCCockpit::Init() {
    IffLexer lexer;
    lexer.InitFromFile("PALETTE.IFF");
    RSPalette palette;
    palette.InitFromIFF(&lexer);
    this->palette = *palette.GetColorPalette();
    cockpit = new RSCockpit();
    TreEntry *cockpit_def =
        Assets.tres[AssetManager::TRE_OBJECTS]->GetEntryByName("..\\..\\DATA\\OBJECTS\\F16-CKPT.IFF");
    cockpit->InitFromRam(cockpit_def->data, cockpit_def->size);

    TreEntry *hud_def = Assets.tres[AssetManager::TRE_OBJECTS]->GetEntryByName("..\\..\\DATA\\OBJECTS\\HUD.IFF");
    hud = new RSHud();
    hud->InitFromRam(hud_def->data, hud_def->size);
    for (int i = 0; i < 36; i++) {
        HudLine line;
        line.start.x = 0;
        line.start.y = 0 + i * 20;
        line.end.x = 70;
        line.end.y = 0 + i * 20;
        horizon.push_back(line);
    }
    TreEntry *fontdef = Assets.tres[AssetManager::TRE_MISC]->GetEntryByName("..\\..\\DATA\\FONTS\\SHUDFONT.SHP");
    PakArchive *pak = new PakArchive();
    pak->InitFromRAM("HUDFONT", fontdef->data, fontdef->size);
    this->font = new RSFont();
    this->font->InitFromPAK(pak);

    TreEntry *bigfontdef = Assets.tres[AssetManager::TRE_MISC]->GetEntryByName("..\\..\\DATA\\FONTS\\HUDFONT.SHP");
    PakArchive *bigpak = new PakArchive();
    bigpak->InitFromRAM("HUDFONT", bigfontdef->data, bigfontdef->size);
    this->big_font = new RSFont();
    this->big_font->InitFromPAK(bigpak);
}
/**
 * SCCockpit::RenderAltitude
 *
 * Render the current altitude in the cockpit view.
 *
 * This function renders the current altitude in the cockpit view by drawing
 * a band of lines and numbers on the right side of the screen. The altitude
 * is rendered in meters, with the 1000s place on the left side of the band
 * and the 100s place on the right side of the band. The band is rendered
 * from the top of the screen (0) to the bottom of the screen (512), with
 * the 0 mark at the top of the screen and the 1000 mark at the bottom of
 * the screen.
 */

void SCCockpit::RenderAltitude() {
    Point2D alti_arrow = {160 + 25, 50 - 20};

    std::vector<Point2D> alti_band_roll;
    alti_band_roll.reserve(100);
    for (int i = 0; i < 100; i++) {
        Point2D p;
        p.x = 0;
        p.y = i * 10;
        alti_band_roll.push_back(p);
    }
    int cpt = 0;
    for (auto p : alti_band_roll) {
        Point2D p2 = p;
        p2.x = 194;
        p2.y = (29 + this->hud->small_hud->ALTI->SHAP->GetHeight() / 2) + p.y - (int)(this->altitude / 100);
        if (p2.y > 30 && p2.y < 29 + this->hud->small_hud->ALTI->SHAP->GetHeight()) {
            VGA.DrawText(this->font, &p2, (char *)std::to_string(cpt / 10.0f).c_str(), 0, 0, 3, 2, 2);
        }
        cpt += 10;
        Point2D alti = {160 + 30, 50 - 20};
        alti.y = p2.y;
        if (p2.y > 10 && p2.y < 29 + this->hud->small_hud->ALTI->SHAP->GetHeight()) {
            this->hud->small_hud->ALTI->SHAP->SetPosition(&alti);
            VGA.DrawShapeWithBox(this->hud->small_hud->ALTI->SHAP, 160 - 30, 160 + 40, 20,
                                 15 + this->hud->small_hud->ALTI->SHAP->GetHeight());
        }
    }
    Point2D alti_text = {160 + 25, 35 + this->hud->small_hud->ALTI->SHAP->GetHeight()};
    VGA.DrawText(this->font, &alti_text, (char *)std::to_string(this->altitude).c_str(), 0, 0, 5, 2, 2);

    Point2D gear = {160 + 25, alti_text.y + 5};
    if (this->gear) {
        VGA.DrawText(this->font, &gear, "GEARS", 0, 0, 5, 2, 2);
    }
    Point2D break_text = {160 + 25, gear.y + 5};
    if (this->airbrake) {
        VGA.DrawText(this->font, &break_text, "BREAKS", 0, 0, 6, 2, 2);
    }
}
/**
 * Renders the heading indicator in the cockpit view.
 *
 * This function renders the heading indicator in the cockpit view by drawing
 * a band of lines and numbers on the top of the screen. The heading is rendered
 * in degrees, with the 0 mark at the top of the screen and the 360 mark at the
 * bottom of the screen. The band is rendered from the top of the screen (0) to
 * the bottom of the screen (360), with the 0 mark at the top of the screen and
 * the 360 mark at the bottom of the screen.
 */
void SCCockpit::RenderHeading() {
    Point2D heading_pos = {140, 86};
    this->hud->small_hud->HEAD->SHAP->SetPosition(&heading_pos);

    std::vector<Point2D> heading_points;
    heading_points.reserve(36);
    for (int i = 0; i < 36; i++) {
        Point2D p;
        p.x = 432 - i * 12;
        p.y = heading_pos.y + 7;
        heading_points.push_back(p);
    }
    int headcpt = 0;
    int pixelcpt = 0;
    int headleft = 160 - 24;
    int headright = 160 + 24;
    for (auto p : heading_points) {
        Point2D hp = p;
        hp.x = hp.x - (int)(this->heading * 1.2) + 160;

        if (hp.x < 0) {
            hp.x += 432;
        }
        if (hp.x > 432) {
            hp.x -= 432;
        }
        if (hp.x > headleft && hp.x < headright) {
            Point2D txt_pos = hp;
            int toprint = headcpt;
            if (toprint == 36) {
                toprint = 0;
            }
            VGA.DrawText(this->font, &txt_pos, (char *)std::to_string(toprint).c_str(), 0, 0, 3, 2, 2);
        }
        if (headcpt % 3 == 0) {
            Point2D headspeed;
            headspeed.x = hp.x - 6;
            headspeed.y = heading_pos.y;
            this->hud->small_hud->HEAD->SHAP->SetPosition(&headspeed);
            VGA.DrawShapeWithBox(this->hud->small_hud->HEAD->SHAP, headleft, headright, 80, 120);
        }
        headcpt += 1;
    }
    Vector2D weapoint_direction = {this->weapoint_coords.x - this->player->x,
                                   this->weapoint_coords.y - this->player->y};
    float weapoint_azimut = (atan2f((float)weapoint_direction.y, (float)weapoint_direction.x) * 180.0f / (float)M_PI);
    Point2D weapoint = {0, heading_pos.y - 3};

    weapoint_azimut -= 360;
    weapoint_azimut += 90;
    if (weapoint_azimut > 360) {
        weapoint_azimut -= 360;
    }
    while (weapoint_azimut < 0) {
        weapoint_azimut += 360;
    }
    weapoint.x = weapoint.x + (int)(weapoint_azimut * 1.2f) - (int)((360 - this->heading) * 1.2f) + 160;
    if (weapoint.x < 0) {
        weapoint.x += 432;
    }
    if (weapoint.x > 432) {
        weapoint.x -= 432;
    }

    this->hud->small_hud->HEAD->SHP2->SetPosition(&weapoint);
    VGA.line(160, weapoint.y, 160, weapoint.y + 3, 223);
    VGA.DrawShapeWithBox(this->hud->small_hud->HEAD->SHP2, headleft, headright, 80, 120);
}
void SCCockpit::RenderSpeed() {
    std::vector<Point2D> speed_band_roll;
    speed_band_roll.reserve(30);
    for (int i = 0; i < 150; i++) {
        Point2D p;
        p.x = 0;
        p.y = i * 10;
        speed_band_roll.push_back(p);
    }
    int cpt_speed = 0;
    for (auto sp : speed_band_roll) {
        Point2D p = sp;
        p.x = 118;
        p.y = (29 + this->hud->small_hud->ASPD->SHAP->GetHeight() / 2) + p.y - (int)this->speed;
        if (p.y > 30 && p.y < 29 + this->hud->small_hud->ASPD->SHAP->GetHeight()) {
            VGA.DrawText(this->font, &p, (char *)std::to_string(cpt_speed).c_str(), 0, 0, 3, 2, 2);
        }
        if (cpt_speed % 5 == 0) {
            if (p.y > 10 && p.y < 29 + this->hud->small_hud->ASPD->SHAP->GetHeight()) {
                Point2D speed = {160 - 30, 50 - 20};
                speed.y = p.y;
                this->hud->small_hud->ASPD->SHAP->SetPosition(&speed);
                VGA.DrawShapeWithBox(this->hud->small_hud->ASPD->SHAP, 160 - 30, 160 + 30, 20,
                                     15 + this->hud->small_hud->ASPD->SHAP->GetHeight());
            }
        }
        cpt_speed += 10;
    }

    Point2D speed_text = {125, 35 + this->hud->small_hud->ASPD->SHAP->GetHeight()};
    VGA.DrawText(this->font, &speed_text, (char *)std::to_string(this->speed).c_str(), 0, 0, 3, 2, 2);
    Point2D throttle_text = {125, speed_text.y + 5};
    VGA.DrawText(this->font, &throttle_text, (char *)std::to_string(this->throttle).c_str(), 0, 0, 3, 2, 2);
    Point2D flaps = {125, throttle_text.y + 5};
    if (this->flaps) {
        VGA.DrawText(this->font, &flaps, "FLAPS", 0, 0, 5, 2, 2);
    }
}
/**
 * SCCockpit::RenderHudHorizonLinesSmall
 *
 * Renders the horizon lines of the Heads-Up Display (HUD) in the lower
 * left corner of the screen. The horizon lines are used to indicate the
 * orientation of the aircraft with respect to the horizon.
 *
 * The horizon lines are drawn with a set of HudLine objects, which are
 * transformed by the roll angle of the aircraft. The lines are also
 * clipped to the bounds of the HUD display area.
 *
 * This function is called by the SCCockpit::Render method.
 */
void SCCockpit::RenderHudHorizonLinesSmall() {

    const int dec = 40;

    const Point2D center{160, 50};
    const int width = 50;
    const int height = 60;

    int bx1 = center.x - width / 2;
    int bx2 = center.x + width / 2;
    int by1 = center.y - height / 2;
    int by2 = center.y + height / 2;

    int top = by1;
    int bottom = by2;
    int left = bx1 + 5;
    int ligne_width = width - 10;

    std::vector<HudLine> *hline = new std::vector<HudLine>();
    hline->resize(36);
    for (int i = 0; i < 36; i++) {
        hline->at(i).start.x = horizon[i].start.x;
        hline->at(i).start.y = (int)(horizon[i].start.y - (270 - this->pitch * 4)) % 720;
        if (hline->at(i).start.y < 0) {
            hline->at(i).start.y += 720;
        }
        hline->at(i).end.x = horizon[i].end.x;
        hline->at(i).end.y = hline->at(i).start.y;
    }
    int ladder = 90;
    for (auto h : *hline) {
        HudLine l = h;
        HudLine l2 = h;
        l.start.x = l.start.x + left;
        l.start.y = l.start.y - dec;
        l.end.y = l.end.y - dec;
        l.end.x = l.start.x + ligne_width / 4;

        l2.start.x = l.start.x + ligne_width - ligne_width / 4;
        l2.start.y = l2.start.y - dec;
        l2.end.y = l2.end.y - dec;
        l2.end.x = l.start.x + ligne_width;

        l.start = rotateAroundPoint(l.start, center, this->roll * (float)M_PI / 180.0f);
        l.end = rotateAroundPoint(l.end, center, this->roll * (float)M_PI / 180.0f);

        l2.start = rotateAroundPoint(l2.start, center, this->roll * (float)M_PI / 180.0f);
        l2.end = rotateAroundPoint(l2.end, center, this->roll * (float)M_PI / 180.0f);

        if (l.start.x > bx1 && l.start.x < bx2 && l.start.y > by1 && l.start.y < by2) {
            Point2D p = l.start;
            p.y -= 2;
            VGA.DrawText(this->font, &p, (char *)std::to_string(ladder).c_str(), 0, 0, 3, 2, 2);
        }
        if (l2.end.x > bx1 && l2.end.x < bx2 && l2.end.y > by1 && l2.end.y < by2) {
            Point2D p = l2.end;
            p.x = p.x - 8;
            p.y -= 2;
            VGA.DrawText(this->font, &p, (char *)std::to_string(ladder).c_str(), 0, 0, 3, 2, 2);
        }
        int color = 223;
        if (ladder == 0) {
            HudLine l3 = h;
            l3.start.x = l3.start.x + left - 5;
            l3.start.y = l3.start.y - dec;
            l3.end.y = l3.end.y - dec;
            l3.end.x = l3.start.x + ligne_width + 10;
            l3.start = rotateAroundPoint(l3.start, center, this->roll * (float)M_PI / 180.0f);
            l3.end = rotateAroundPoint(l3.end, center, this->roll * (float)M_PI / 180.0f);
            VGA.lineWithBox(l3.start.x, l3.start.y, l3.end.x, l3.end.y, color, bx1, bx2, by1, by2);
        } else {
            int skip = 1;
            if (ladder < 0) {
                skip = 2;
            }
            VGA.lineWithBoxWithSkip(l.start.x, l.start.y, l.end.x, l.end.y, color, bx1, bx2, by1, by2, skip);
            VGA.lineWithBoxWithSkip(l2.start.x, l2.start.y, l2.end.x, l2.end.y, color, bx1, bx2, by1, by2, skip);
        }
        ladder = ladder - 5;
    }
}

void SCCockpit::RenderMFDS(Point2D mfds) {

    this->cockpit->MONI.SHAP.SetPosition(&mfds);
    for (int i = 0; i < this->cockpit->MONI.SHAP.GetHeight(); i++) {
        VGA.line(mfds.x, mfds.y + i, mfds.x + this->cockpit->MONI.SHAP.GetWidth() - 1, mfds.y + i, 0);
    }
    VGA.DrawShape(&this->cockpit->MONI.SHAP);
}
void SCCockpit::RenderTargetWithCam() {
    if (this->target != nullptr) {
        Vector3D campos = this->cam->GetPosition();
        Vector3DHomogeneous v = {(float)this->target->x, (float)this->target->z, -(float)this->target->y, 1.0f};

        Matrix *mproj = this->cam->GetProjectionMatrix();
        Matrix *mview = this->cam->GetViewMatrix();

        Vector3DHomogeneous mcombined = mview->multiplyMatrixVector(v);
        Vector3DHomogeneous result = mproj->multiplyMatrixVector(mcombined);
        if (result.z > 0.0f) {
            float x = result.x / result.w;
            float y = result.y / result.w;

            int Xhud = (int)((x + 1.0f) * 160.0f);
            int Yhud = (int)((1.0f - y) * 100.0f) - 1;

            if (Xhud > 0 && Xhud < 320 && Yhud > 0 && Yhud < 200) {
                Point2D p = {Xhud, Yhud};
                VGA.lineWithBox((int)p.x - 5, (int)p.y - 5, (int)p.x + 5, (int)p.y - 5, 223, 160 - 34, 160 + 34, 5, 90);
                VGA.lineWithBox((int)p.x + 5, (int)p.y - 5, (int)p.x + 5, (int)p.y + 5, 223, 160 - 34, 160 + 34, 5, 90);
                VGA.lineWithBox((int)p.x + 5, (int)p.y + 5, (int)p.x - 5, (int)p.y + 5, 223, 160 - 34, 160 + 34, 5, 90);
                VGA.lineWithBox((int)p.x - 5, (int)p.y + 5, (int)p.x - 5, (int)p.y - 5, 223, 160 - 34, 160 + 34, 5, 90);
            }
        }
    }
}
void SCCockpit::RenderMFDSWeapon(Point2D pmfd_right) {
    this->RenderMFDS(pmfd_right);
    Point2D pmfd_right_center = {pmfd_right.x + this->cockpit->MONI.SHAP.GetWidth() / 2,
                                 pmfd_right.y + this->cockpit->MONI.SHAP.GetHeight() / 2};
    Point2D pmfd_right_weapon = {pmfd_right_center.x - this->cockpit->MONI.MFDS.WEAP.ARTS.GetShape(0)->GetWidth() / 2,
                                 pmfd_right_center.y - 10 -
                                     this->cockpit->MONI.MFDS.WEAP.ARTS.GetShape(0)->GetHeight() / 2};
    this->cockpit->MONI.MFDS.WEAP.ARTS.GetShape(0)->SetPosition(&pmfd_right_weapon);
    VGA.DrawShape(this->cockpit->MONI.MFDS.WEAP.ARTS.GetShape(0));
    for (int i = 0; i < 4; i++) {
        Point2D pmfd_right_weapon_hp = {
            pmfd_right_center.x - 15 - this->cockpit->MONI.MFDS.WEAP.ARTS.GetShape(1)->GetWidth() / 2 - i * 9,
            pmfd_right_center.y - 18 - this->cockpit->MONI.MFDS.WEAP.ARTS.GetShape(1)->GetHeight() / 2 + i * 9};
        this->cockpit->MONI.MFDS.WEAP.ARTS.GetShape(1)->SetPosition(&pmfd_right_weapon_hp);
        VGA.DrawShape(this->cockpit->MONI.MFDS.WEAP.ARTS.GetShape(1));
        Point2D pmfd_right_weapon_hp_text = {
            pmfd_right_weapon_hp.x + this->cockpit->MONI.MFDS.WEAP.ARTS.GetShape(1)->GetWidth() / 2 + 1,
            pmfd_right_weapon_hp.y + this->cockpit->MONI.MFDS.WEAP.ARTS.GetShape(1)->GetHeight() + 6};
        VGA.DrawText(this->big_font, &pmfd_right_weapon_hp_text, (char *)std::to_string(i).c_str(), 0, 0, 3, 2, 2);

        Point2D pmfd_right_weapon_hp_left = {
            pmfd_right_center.x + 13 - this->cockpit->MONI.MFDS.WEAP.ARTS.GetShape(1)->GetWidth() / 2 + i * 9,
            pmfd_right_center.y - 18 - this->cockpit->MONI.MFDS.WEAP.ARTS.GetShape(1)->GetHeight() / 2 + i * 9};
        this->cockpit->MONI.MFDS.WEAP.ARTS.GetShape(1)->SetPosition(&pmfd_right_weapon_hp_left);
        VGA.DrawShape(this->cockpit->MONI.MFDS.WEAP.ARTS.GetShape(1));
        Point2D pmfd_right_weapon_hp_text_left = {
            pmfd_right_weapon_hp_left.x + this->cockpit->MONI.MFDS.WEAP.ARTS.GetShape(1)->GetWidth() / 2 - 1,
            pmfd_right_weapon_hp_left.y + this->cockpit->MONI.MFDS.WEAP.ARTS.GetShape(1)->GetHeight() + 6};
        VGA.DrawText(this->big_font, &pmfd_right_weapon_hp_text_left, (char *)std::to_string(i).c_str(), 0, 0, 3, 2, 2);
    }

    Point2D pmfd_right_weapon_gun{pmfd_right_weapon.x - 8 +
                                      this->cockpit->MONI.MFDS.WEAP.ARTS.GetShape(0)->GetWidth() / 2,
                                  pmfd_right_weapon.y + 6};
    VGA.DrawText(this->big_font, &pmfd_right_weapon_gun, "1000", 0, 0, 4, 2, 2);

    Point2D pmfd_right_weapon_radar{pmfd_right_weapon.x, pmfd_right_weapon.y + 5};
    VGA.DrawText(this->big_font, &pmfd_right_weapon_radar, "NORM", 0, 0, 4, 2, 2);

    Point2D pmfd_right_weapon_selected{pmfd_right_weapon.x + 12 +
                                           this->cockpit->MONI.MFDS.WEAP.ARTS.GetShape(0)->GetWidth() / 2,
                                       pmfd_right_weapon.y + 5};
    VGA.DrawText(this->big_font, &pmfd_right_weapon_selected, "GUN", 0, 0, 4, 2, 2);

    Point2D pmfd_right_weapon_chaff{pmfd_right_weapon.x - 7 +
                                        this->cockpit->MONI.MFDS.WEAP.ARTS.GetShape(0)->GetWidth() / 2,
                                    pmfd_right_weapon.y + 4 * 9};
    VGA.DrawText(this->big_font, &pmfd_right_weapon_chaff, "C:30", 0, 0, 4, 2, 2);

    Point2D pmfd_right_weapon_flare{pmfd_right_weapon.x - 7 +
                                        this->cockpit->MONI.MFDS.WEAP.ARTS.GetShape(0)->GetWidth() / 2,
                                    pmfd_right_weapon.y + 5 * 9};
    VGA.DrawText(this->big_font, &pmfd_right_weapon_flare, "F:30", 0, 0, 4, 2, 2);
}
void SCCockpit::RenderMFDSRadar(Point2D pmfd_left, float range, int mode) {
    Point2D pmfd_center = {
        pmfd_left.x + this->cockpit->MONI.SHAP.GetWidth() / 2,
        pmfd_left.y + this->cockpit->MONI.SHAP.GetHeight() / 2,
    };
    this->RenderMFDS(pmfd_left);
    pmfd_left.x +=
        -5 + this->cockpit->MONI.SHAP.GetWidth() / 2 - this->cockpit->MONI.MFDS.AARD.ARTS.GetShape(4)->GetWidth() / 2;
    pmfd_left.y +=
        this->cockpit->MONI.SHAP.GetHeight() / 2 - this->cockpit->MONI.MFDS.AARD.ARTS.GetShape(4)->GetHeight() / 2;
    this->cockpit->MONI.MFDS.AARD.ARTS.GetShape(4)->SetPosition(&pmfd_left);
    VGA.DrawShape(this->cockpit->MONI.MFDS.AARD.ARTS.GetShape(4));

    Vector2D center = {this->player->x, -this->player->y};
    
    for (auto parts : this->parts) {
        if (parts == this->player) {
            continue;
        }
        Vector2D part = {parts->x, parts->y};

        // rotate part according to player heading
        int heading = 180-this->heading;
        if (heading < 0) {
            heading += 360;
        }
        if (heading > 360) {
            heading -= 360;
        }
        Vector2D roa = rotateAroundPoint(part, center, heading/180.0f*(float)M_PI);
        Vector2D roa_dir = {roa.x-center.x, roa.y-center.y};

        float distance = sqrtf((float) (roa_dir.x * roa_dir.x) + (float) (roa_dir.y * roa_dir.y));
        if (distance < range) {
            float scale = 60.0f/range;
            
            Point2D p2 = {pmfd_center.x + (int) (roa_dir.x * scale), pmfd_center.y + (int) (roa_dir.y * scale)};
            if (p2.x > pmfd_left.x && p2.x < pmfd_left.x + this->cockpit->MONI.SHAP.GetWidth() &&
                p2.y > pmfd_left.y && p2.y < pmfd_left.y + this->cockpit->MONI.SHAP.GetHeight()) {
                    this->cockpit->MONI.MFDS.AARD.ARTS.GetShape(0)->SetPosition(&p2);
                    VGA.DrawShape(this->cockpit->MONI.MFDS.AARD.ARTS.GetShape(0));
                    if (parts == this->target) {
                        Point2D p3 = {p2.x - 2, p2.y - 1};
                        this->cockpit->MONI.MFDS.AARD.ARTS.GetShape(2)->SetPosition(&p3);
                        VGA.DrawShape(this->cockpit->MONI.MFDS.AARD.ARTS.GetShape(2));
                    }
            }
        }
    }
}
/**
 * Render the cockpit in its current state.
 *
 * If the face number is non-negative, renders the cockpit in its 2D
 * representation using the VGA graphics mode. Otherwise, renders the
 * cockpit in its 3D representation using the OpenGL graphics mode.
 *
 * @param face The face number of the cockpit to render, or -1 to render
 * in 3D.
 */
void SCCockpit::Render(int face) {
    if (face >= 0) {
        VGA.SwithBuffers();
        VGA.Activate();
        VGA.Clear();
        VGA.SetPalette(&this->palette);
        VGA.DrawShape(this->cockpit->ARTP.GetShape(face));
        if (face == 0) {
            this->RenderHudHorizonLinesSmall();
            this->RenderAltitude();
            this->RenderSpeed();
            this->RenderHeading();
            if (this->target != this->player) {
                this->RenderTargetWithCam();
            }
            VGA.plot_pixel(161, 50, 223);

            Point2D pmfd_right = {0, 200 - this->cockpit->MONI.SHAP.GetHeight()};
            Point2D pmfd_left = {320 - this->cockpit->MONI.SHAP.GetWidth(), 200 - this->cockpit->MONI.SHAP.GetHeight()};
            Point2D pmfd;
            bool mfds = false;
            if (this->show_radars) {
                if (!mfds) {
                    pmfd = pmfd_left;    
                    mfds = true;
                } else {
                    pmfd = pmfd_right;
                }
                this->RenderMFDSRadar(pmfd, this->radar_zoom*20000.0f, 0);
            }
            if (this->show_weapons) {
                if (!mfds) {
                    pmfd = pmfd_left;    
                    mfds = true;
                } else {
                    pmfd = pmfd_right;
                }
                this->RenderMFDSWeapon(pmfd);
            }
        }
        VGA.VSync();
        VGA.SwithBuffers();
    }
}