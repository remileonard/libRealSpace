//
//  SCCockpit.cpp
//  libRealSpace
//
//  Created by Rémi LEONARD on 02/09/2024.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//
#include "precomp.h"
#include "SCCockpit.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <climits>

bool SCCockpit::project_to_screen(Vector3D coord, int &Xout, int &Yout) {
    Vector3D campos = this->cam->getPosition();
    Vector3DHomogeneous v = {coord.x, coord.y, coord.z, 1.0f};

    Matrix *mproj = this->cam->getProjectionMatrix();
    Matrix *mview = this->cam->getViewMatrix();

    Vector3DHomogeneous mcombined = mview->multiplyMatrixVector(v);
    Vector3DHomogeneous result = mproj->multiplyMatrixVector(mcombined);
    if (result.z > 0.0f) {
        float x = result.x / result.w;
        float y = result.y / result.w;

        Xout = (int)((x + 1.0f) * 160.0f);
        Yout = (int)((1.0f - y - 0.45f) * 100.0f) - 1;
        return true;
    }
    return false;
}
SCCockpit::SCCockpit() {}
SCCockpit::~SCCockpit() {
    if (this->hud_framebuffer) {
        delete this->hud_framebuffer;
        this->hud_framebuffer = nullptr;
    }
    if (this->mfd_right_framebuffer) {
        delete this->mfd_right_framebuffer;
        this->mfd_right_framebuffer = nullptr;
    }
    if (this->mfd_left_framebuffer) {
        delete this->mfd_left_framebuffer;
        this->mfd_left_framebuffer = nullptr;
    }
}
/**
 * SCCockpit::init
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
void SCCockpit::init() {
    RSPalette palette;
    TreEntry *entries = (TreEntry *)Assets.GetEntryByName("..\\..\\DATA\\PALETTE\\PALETTE.IFF");
    if (entries != nullptr) {
        palette.initFromFileRam(entries->data, entries->size);
    } else {
        FileData *f = Assets.GetFileData("PALETTE.IFF");
        if (f != nullptr) {
            palette.initFromFileData(f);
        }
    }
    this->palette = *palette.GetColorPalette();
    cockpit = nullptr;
    TreEntry *cockpit_def = nullptr;
    if (player_plane != nullptr) {
        if (player_plane->object->entity != nullptr) {
            RSEntity *e = player_plane->object->entity;
            cockpit_def = Assets.GetEntryByName("..\\..\\DATA\\OBJECTS\\" + e->cockpit_name + ".IFF");
        }
    }
    if (cockpit_def == nullptr) {
        if (player_plane->object->entity->name == "..\\..\\DATA\\OBJECTS\\F-16DES.IFF") {
            cockpit_def = Assets.GetEntryByName("..\\..\\DATA\\OBJECTS\\F16-CKPT.IFF");
        } else if (player_plane->object->entity->name == "..\\..\\DATA\\OBJECTS\\F-22.IFF") {
            cockpit_def = Assets.GetEntryByName("..\\..\\DATA\\OBJECTS\\F22-CKPT.IFF");
        } else if (player_plane->object->entity->name == "..\\..\\DATA\\OBJECTS\\F-22B.IFF") {
            cockpit_def = Assets.GetEntryByName("..\\..\\DATA\\OBJECTS\\F22-CKPT.IFF");
        }
    }
    if (cockpit_def != nullptr) {
        cockpit = new RSCockpit();
        cockpit->InitFromRam(cockpit_def->data, cockpit_def->size);
        TreEntry *hud_def = Assets.GetEntryByName("..\\..\\DATA\\OBJECTS\\HUD.IFF");
        if (hud_def != nullptr) {
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
            hud_framebuffer = new FrameBuffer(96, 93);
            mfd_right_framebuffer = new FrameBuffer(115, 95);
            mfd_left_framebuffer = new FrameBuffer(115, 95);
            raws_framebuffer = new FrameBuffer(36, 32);
            target_framebuffer = new FrameBuffer(320, 200);
            alti_framebuffer = new FrameBuffer(33, 29);
            speed_framebuffer = new FrameBuffer(36, 29);
            comm_framebuffer = new FrameBuffer(320, 13);
            this->font = FontManager.GetFont("..\\..\\DATA\\FONTS\\SHUDFONT.SHP");
            this->big_font = FontManager.GetFont("..\\..\\DATA\\FONTS\\HUDFONT.SHP");
        }
    }
    debug_framebuffer = new FrameBuffer(320, 200);
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

void SCCockpit::RenderAltitude(Point2D alti_top_left = {185, 30}, FrameBuffer *fb = nullptr) {
    if (!fb) {
        fb = VGA.getFrameBuffer();
    }
    std::vector<Point2D> alti_band_roll;
    float alti_in_feet = this->altitude * 3.28084f; // Convert meters to feet
    Point2D alti_size = {20, 35};

    Point2D bottom_right = {alti_top_left.x + alti_size.x, alti_top_left.y + alti_size.y};

    alti_band_roll.reserve(100);
    for (int i = 0; i < 100; i++) {
        Point2D p;
        p.x = 0;
        p.y = (100 - i) * 10;
        alti_band_roll.push_back(p);
    }
    int cpt = 1000;
    for (auto p : alti_band_roll) {
        Point2D p2 = {p.x, p.y};
        p2.x = alti_top_left.x + 9;
        p2.y = (alti_top_left.y + alti_size.y / 2) - p.y + (int)(alti_in_feet / 100);
        if (p2.y > alti_top_left.y && p2.y < bottom_right.y) {
            fb->printText(this->font, &p2, (char *)std::to_string(cpt / 10.0f).c_str(), 0, 0, 3, 2, 2);
        }
        cpt -= 10;
        Point2D alti = {alti_top_left.x + 1, p2.y};
        int sheight = this->hud->small_hud->ALTI->SHAP->GetHeight();
        // alti.y = alti_top_left.y;
        this->hud->small_hud->ALTI->SHAP->SetPosition(&alti);
        fb->drawShapeWithBox(this->hud->small_hud->ALTI->SHAP, alti_top_left.x, bottom_right.x, alti_top_left.y - 10,
                             bottom_right.y - 10);
    }
    Point2D alti_arrow = {alti_top_left.x - 3, alti_top_left.y + alti_size.y / 2};
    fb->line(alti_arrow.x, alti_arrow.y, alti_arrow.x + 1, alti_arrow.y, 223);
    alti_arrow.y -= this->hud->small_hud->ALTI->SHP2->GetHeight() / 2;
    this->hud->small_hud->ALTI->SHP2->SetPosition(&alti_arrow);
    fb->drawShape(this->hud->small_hud->ALTI->SHP2);
    Point2D alti_text = {alti_top_left.x, alti_top_left.y + this->hud->small_hud->ALTI->SHAP->GetHeight()};
    std::ostringstream oss;
    oss << std::setw(5) << std::setfill('0') << std::fixed << std::setprecision(1) << (alti_in_feet-(this->player_plane->groundlevel* 3.28084f)) / 1000.0f;
    std::string s = oss.str();
    std::string alti_str = "A"+s;
    fb->printText(this->font, &alti_text, (char *)alti_str.c_str(), 0, 0, alti_str.length(), 2, 2);
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
void SCCockpit::RenderHeading(Point2D heading_pos = {136, 90}, FrameBuffer *fb = nullptr) {
    if (!fb) {
        fb = VGA.getFrameBuffer();
    }
    this->hud->small_hud->HEAD->SHAP->SetPosition(&heading_pos);

    std::vector<Point2D> heading_points;
    std::string txt;
    heading_points.reserve(36);
    for (int i = 0; i < 36; i++) {
        Point2D p;
        p.x = 432 - (36 - i) * 12;
        p.y = heading_pos.y + 7;
        heading_points.push_back(p);
    }
    int headcpt = 0;
    int pixelcpt = 0;

    Point2D heading_size = {48, 14};
    Point2D bottom_right = {heading_pos.x + heading_size.x, heading_pos.y + heading_size.y};

    for (auto p : heading_points) {
        Point2D hp = p;
        hp.x = hp.x + (int)(this->heading * 1.2) + (heading_pos.x + heading_size.x / 2);

        if (hp.x < 0) {
            hp.x += 432;
        }
        if (hp.x > 432) {
            hp.x -= 432;
        }
        if (hp.x > heading_pos.x && hp.x < bottom_right.x) {
            Point2D txt_pos = hp;
            int toprint = headcpt;
            if (toprint == 36) {
                toprint = 0;
            }
            txt = std::to_string(toprint);
            fb->printText(this->font, &txt_pos, (char *)txt.c_str(), 0, 0, (uint32_t)txt.length(), 2, 2);
        }
        if (headcpt % 3 == 0) {
            Point2D headspeed;
            headspeed.x = hp.x - 6;
            headspeed.y = heading_pos.y;
            this->hud->small_hud->HEAD->SHAP->SetPosition(&headspeed);
            fb->drawShapeWithBox(this->hud->small_hud->HEAD->SHAP, heading_pos.x, bottom_right.x, heading_pos.y,
                                 bottom_right.y);
        }
        headcpt += 1;
    }
    Vector2D weapoint_direction = {this->weapoint_coords.x - this->player->position.x,
                                   this->weapoint_coords.y - this->player->position.z};
    float weapoint_azimut = (atan2f(weapoint_direction.y, weapoint_direction.x) * 180.0f / (float)M_PI);
    Point2D weapoint = {0, heading_pos.y - 3};

    weapoint_azimut -= 360;
    weapoint_azimut += 90;
    if (weapoint_azimut > 360) {
        weapoint_azimut -= 360;
    }
    while (weapoint_azimut < 0) {
        weapoint_azimut += 360;
    }
    this->way_az = weapoint_azimut;
    weapoint.x = weapoint.x + (int)(weapoint_azimut * 1.2f) - (int)((360 - this->heading) * 1.2f) +
                 (heading_pos.x + heading_size.x / 2);
    if (weapoint.x < 0) {
        weapoint.x += 432;
    }
    if (weapoint.x > 432) {
        weapoint.x -= 432;
    }

    this->hud->small_hud->HEAD->SHP2->SetPosition(&weapoint);
    fb->line((heading_pos.x + heading_size.x / 2), weapoint.y, (heading_pos.x + heading_size.x / 2), weapoint.y + 3,
             223);
    fb->drawShapeWithBox(this->hud->small_hud->HEAD->SHP2, heading_pos.x, bottom_right.x, heading_pos.y - 5,
                         bottom_right.y);
}
void SCCockpit::RenderSpeed(Point2D speed_top_left = {115, 30}, FrameBuffer *fb = nullptr) {
    if (!fb) {
        fb = VGA.getFrameBuffer();
    }
    std::vector<Point2D> speed_band_roll;
    Point2D speed_band_size = {20, 35};
    Point2D bottom_right = {speed_top_left.x + speed_band_size.x, speed_top_left.y + speed_band_size.y};

    std::string txt;
    speed_band_roll.reserve(30);
    for (int i = 0; i < 150; i++) {
        Point2D p;
        p.x = 0;
        p.y = (150 - i) * 10;
        speed_band_roll.push_back(p);
    }
    int cpt_speed = 1500;
    for (auto sp : speed_band_roll) {
        Point2D p = sp;
        p.x = speed_top_left.x + 5;
        p.y = (speed_top_left.y + this->hud->small_hud->ASPD->SHAP->GetHeight() / 2) - p.y + (int)this->speed;
        if (p.y > speed_top_left.y + 1 && p.y < speed_top_left.y + this->hud->small_hud->ASPD->SHAP->GetHeight()) {
            txt = std::to_string(cpt_speed);
            fb->printText(this->font, &p, (char *)txt.c_str(), 0, 0, (uint32_t)txt.length(), 2, 2);
        }
        if (cpt_speed % 5 == 0) {
            if (p.y > 10 && p.y < 29 + this->hud->small_hud->ASPD->SHAP->GetHeight()) {
                Point2D speed = {bottom_right.x - 5, speed_top_left.y};
                speed.y = p.y;
                this->hud->small_hud->ASPD->SHAP->SetPosition(&speed);
                fb->drawShapeWithBox(this->hud->small_hud->ASPD->SHAP, speed_top_left.x, bottom_right.x,
                                     speed_top_left.y - 10, bottom_right.y - 10);
            }
        }
        cpt_speed -= 10;
    }
    Point2D speed_arrow = {bottom_right.x, speed_top_left.y + speed_band_size.y / 2};
    fb->line(speed_arrow.x - 3, speed_arrow.y, speed_arrow.x, speed_arrow.y, 223);
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
void SCCockpit::RenderHudHorizonLinesSmall(Point2D center = {160, 50}, FrameBuffer *fb = nullptr) {
    if (!fb) {
        fb = VGA.getFrameBuffer();
    }

    const int dec = 0;

    const int width = 50;
    const int height = 78;

    int bx1 = center.x - width / 2;
    int bx2 = center.x + width / 2;
    int by1 = center.y - height / 2;
    int by2 = center.y + height / 2;

    int top = by1;
    int bottom = by2;
    int left = bx1 + 5;
    int ligne_width = width - 10;
    std::string txt;
    std::vector<HudLine> *hline = new std::vector<HudLine>();
    hline->resize(36);
    for (int i = 0; i < 36; i++) {
        hline->at(i).start.x = horizon[i].start.x;
        hline->at(i).start.y = (int)(horizon[i].start.y - ((360 - center.y) - this->pitch * 4)) % 720;
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

        l.start = l.start.rotateAroundPoint(center, this->roll * (float)M_PI / 180.0f);
        l.end = l.end.rotateAroundPoint(center, this->roll * (float)M_PI / 180.0f);

        l2.start = l2.start.rotateAroundPoint(center, this->roll * (float)M_PI / 180.0f);
        l2.end = l2.end.rotateAroundPoint(center, this->roll * (float)M_PI / 180.0f);
        txt = std::to_string(ladder);
        if (l.start.x > bx1 && l.start.x < bx2 && l.start.y > by1 && l.start.y < by2) {
            Point2D p = l.start;
            p.y -= 2;
            fb->printText(this->font, &p, (char *)txt.c_str(), 0, 0, (uint32_t)txt.length(), 2, 2);
        }
        if (l2.end.x > bx1 && l2.end.x < bx2 && l2.end.y > by1 && l2.end.y < by2) {
            Point2D p = l2.end;
            p.x = p.x - 8;
            p.y -= 2;
            fb->printText(this->font, &p, (char *)txt.c_str(), 0, 0, (uint32_t)txt.length(), 2, 2);
        }
        int color = 223;
        if (ladder == 0) {
            HudLine l3 = h;
            l3.start.x = l3.start.x + left - 5;
            l3.start.y = l3.start.y - dec;
            l3.end.y = l3.end.y - dec;
            l3.end.x = l3.start.x + ligne_width + 10;
            l3.start = l3.start.rotateAroundPoint( center, this->roll * (float)M_PI / 180.0f);
            l3.end = l3.end.rotateAroundPoint( center, this->roll * (float)M_PI / 180.0f);
            fb->lineWithBox(l3.start.x, l3.start.y, l3.end.x, l3.end.y, color, bx1, bx2, by1, by2);
        } else {
            int skip = 1;
            if (ladder < 0) {
                skip = 2;
            }
            fb->lineWithBoxWithSkip(l.start.x, l.start.y, l.end.x, l.end.y, color, bx1, bx2, by1, by2, skip);
            fb->lineWithBoxWithSkip(l2.start.x, l2.start.y, l2.end.x, l2.end.y, color, bx1, bx2, by1, by2, skip);
        }
        ladder = ladder - 5;
    }
    hline->clear();
    hline->shrink_to_fit();
    delete hline;
}

void SCCockpit::RenderMFDS(Point2D mfds, FrameBuffer *fb = nullptr) {
    if (this->cockpit->MONI.SHAP.data == nullptr) {
        return;
    }
    if (!fb) {
        fb = VGA.getFrameBuffer();
    }
    this->cockpit->MONI.SHAP.SetPosition(&mfds);
    for (int i = 0; i < this->cockpit->MONI.SHAP.GetHeight(); i++) {
        fb->line(mfds.x, mfds.y + i, mfds.x + this->cockpit->MONI.SHAP.GetWidth(), mfds.y + i, 2);
    }
    fb->drawShape(&this->cockpit->MONI.SHAP);
}
void SCCockpit::RenderTargetWithCam(Point2D top_left = {126, 5}, FrameBuffer *fb = nullptr) {
    if (!fb) {
        fb = VGA.getFrameBuffer();
    }
    Point2D hud_size = {68, 85};
    Point2D bottom_right = {top_left.x + hud_size.x, top_left.y + hud_size.y};

    if (this->target != nullptr) {
        int Xhud, Yhud;
        if (project_to_screen(this->target->position, Xhud, Yhud)) {

            // Check if the target is within HUD boundaries
            bool isInHud = (Xhud >= top_left.x && Xhud < bottom_right.x && Yhud >= top_left.y && Yhud < bottom_right.y);
            int hudX = Xhud - top_left.x;
            int hudY = Yhud - top_left.y;
            isInHud = (hudX >= 0 && hudX < fb->width && hudX >= 0 && hudX < fb->height);
            // If the target is within the HUD boundaries, draw a targeting indicator
            if (isInHud) {
                // Draw a targeting reticle at the position
                Point2D targetPoint = {hudX + top_left.x, hudY + top_left.y};
                RLEShape *targetShape = this->hud->small_hud->TARG->SHAPSET->GetShape(1);
                targetPoint.x = targetPoint.x - targetShape->GetWidth() / 2;
                targetPoint.y = targetPoint.y - targetShape->GetHeight() / 2;
                targetShape->SetPosition(&targetPoint);
                fb->drawShape(targetShape);
                if (this->target_in_range) {
                    RLEShape *inRangeShape = this->hud->small_hud->MISD->SHAP;
                    Point2D inRangePos = {hudX + top_left.x, hudY + top_left.y};
                    inRangePos.x = inRangePos.x - inRangeShape->GetWidth() / 2;
                    inRangePos.y = inRangePos.y - inRangeShape->GetHeight() / 2;
                    inRangeShape->SetPosition(&inRangePos);
                    fb->drawShape(inRangeShape);
                }
            }
        }
    }
}

static bool projectLocalAnglesToHud(const Vector3D &vLocal, FrameBuffer *fb, int &outX, int &outY) {
    // Repère avion/cockpit:
    // -Z = devant
    // +X = droite (supposé)
    // +Y = haut (supposé)
    const float forward = -vLocal.z;
    if (forward <= 0.001f)
        return false; // derrière ou trop proche du plan

    // Angles (radians)
    // az > 0 => à droite si +X est à droite
    const float az = atan2f(vLocal.x, forward);

    // el > 0 => vers le haut si +Y est vers le haut
    // (si ton Y est inversé, enlève le '-' ci-dessous)
    const float el = atan2f(vLocal.y, forward);

    // A TUNER: FOV angulaire du HUD (collimation approximée)
    const float hudFovX = 30.0f * (float)M_PI / 180.0f; // total
    const float hudFovY = 25.0f * (float)M_PI / 180.0f; // total

    const float nx = az / (hudFovX * 0.5f); // [-1..1]
    const float ny = el / (hudFovY * 0.5f); // [-1..1]

    const float cx = (fb->width - 1) * 0.5f;
    const float cy = (fb->height - 1) * 0.5f;

    outX = (int)(cx + nx * cx);
    outY = (int)(cy - ny * cy);
    return (outX >= 0 && outX < fb->width && outY >= 0 && outY < fb->height);
}

/**
 * projectToHUD3D - Projette un point 3D monde sur le HUD virtuel du cockpit 3D
 *
 * Cette fonction calcule où un point cible (comme un viseur de cannon) doit
 * apparaître sur le HUD en prenant en compte:
 * - La position de l'œil du pilote dans le cockpit
 * - La géométrie réelle du HUD (plan 3D)
 * - La direction depuis l'œil vers la cible
 *
 * Principe: On trace un rayon depuis l'œil vers la cible à l'infini,
 * et on calcule l'intersection avec le plan du HUD.
 *
 * @param targetLocal     Point cible en coordonnées locales de l'avion
 * @param eyeLocal        Position de l'œil du pilote en coordonnées locales
 * @param hudTopLeft      Coin supérieur gauche du HUD en coords locales (depuis renderVirtualCockpit)
 * @param hudTopRight     Coin supérieur droit du HUD
 * @param hudBottomRight  Coin inférieur droit du HUD
 * @param hudBottomLeft   Coin inférieur gauche du HUD
 * @param fb              FrameBuffer du HUD
 * @param outX, outY      Coordonnées de sortie en pixels HUD
 * @return true si le point est visible sur le HUD
 */
static bool projectToHUD3D(const Vector3D &targetLocal, const Vector3D &eyeLocal, const Vector3D &hudTopLeft,
                           const Vector3D &hudTopRight, const Vector3D &hudBottomRight, const Vector3D &hudBottomLeft,
                           FrameBuffer *fb, int &outX, int &outY) {
    // Direction depuis l'œil vers la cible (normalisée pour un HUD collimaté à l'infini)
    Vector3D dir = targetLocal - eyeLocal;
    float dirLen = dir.Length();
    if (dirLen < 0.001f)
        return false;
    dir = dir * (1.0f / dirLen);

    // La cible doit être devant (X positif dans le repère cockpit où X pointe vers l'avant)
    // Note: adapter selon votre convention d'axes
    if (dir.x <= 0.001f)
        return false;

    // Calcul du plan du HUD à partir de ses 4 coins
    // Le HUD est défini par hudTopLeft, hudTopRight, hudBottomRight, hudBottomLeft
    // On utilise 2 vecteurs du plan pour calculer la normale
    Vector3D hudRight = hudTopRight - hudTopLeft;
    Vector3D hudDown = hudBottomLeft - hudTopLeft;

    // Normale du plan HUD (produit vectoriel)
    Vector3D normal;
    normal.x = hudRight.y * hudDown.z - hudRight.z * hudDown.y;
    normal.y = hudRight.z * hudDown.x - hudRight.x * hudDown.z;
    normal.z = hudRight.x * hudDown.y - hudRight.y * hudDown.x;

    float normalLen = normal.Length();
    if (normalLen < 0.001f)
        return false;
    normal = normal * (1.0f / normalLen);

    // Équation du plan: normal · (P - hudTopLeft) = 0
    // D = -normal · hudTopLeft
    float D = -(normal.x * hudTopLeft.x + normal.y * hudTopLeft.y + normal.z * hudTopLeft.z);

    // Intersection rayon-plan
    // Rayon: P = eyeLocal + t * dir
    // Plan: normal · P + D = 0
    // => normal · (eyeLocal + t * dir) + D = 0
    // => t = -(normal · eyeLocal + D) / (normal · dir)

    float denom = normal.x * dir.x + normal.y * dir.y + normal.z * dir.z;
    if (fabsf(denom) < 0.0001f)
        return false; // Rayon parallèle au plan

    float t = -(normal.x * eyeLocal.x + normal.y * eyeLocal.y + normal.z * eyeLocal.z + D) / denom;

    // Le HUD doit être devant l'œil
    if (t < 0.0f)
        return false;

    // Point d'intersection sur le plan du HUD
    Vector3D hitPoint;
    hitPoint.x = eyeLocal.x + t * dir.x;
    hitPoint.y = eyeLocal.y + t * dir.y;
    hitPoint.z = eyeLocal.z + t * dir.z;

    // Convertir le point d'intersection en coordonnées UV du HUD [0, 1]
    // Vecteur du coin supérieur gauche vers le point d'intersection
    Vector3D toHit = hitPoint - hudTopLeft;

    // Projection sur les axes du HUD
    float hudWidth = hudRight.Length();
    float hudHeight = hudDown.Length();

    if (hudWidth < 0.001f || hudHeight < 0.001f)
        return false;

    // Normaliser les vecteurs du HUD
    Vector3D hudRightNorm = hudRight * (1.0f / hudWidth);
    Vector3D hudDownNorm = hudDown * (1.0f / hudHeight);

    // Coordonnées UV (projection du point sur les axes du HUD)
    float u = (toHit.x * hudRightNorm.x + toHit.y * hudRightNorm.y + toHit.z * hudRightNorm.z) / hudWidth;
    float v = (toHit.x * hudDownNorm.x + toHit.y * hudDownNorm.y + toHit.z * hudDownNorm.z) / hudHeight;

    // Convertir en coordonnées pixels
    outX = (int)(u * (fb->width - 1));
    outY = (int)(v * (fb->height - 1));

    // Vérifier si dans les limites du HUD
    return (outX >= 0 && outX < fb->width && outY >= 0 && outY < fb->height);
}

/**
 * projectCannonSightToHUD - Version simplifiée pour le viseur de cannon
 *
 * Utilise une approche angulaire adaptée au cockpit 3D:
 * - Calcule la direction vers la cible depuis la position du cannon (pas l'œil)
 * - Projette cette direction sur le HUD selon son champ de vision angulaire
 * - Tient compte de la parallaxe entre l'œil du pilote et le cannon
 *
 * @param targetWorld     Point cible en coordonnées monde
 * @param planeFromWorld  Matrice inverse de transformation avion (monde -> local)
 * @param eyeLocal        Position de l'œil en coordonnées locales cockpit
 * @param cannonOffset    Offset angulaire du cannon en radians (azimut, élévation)
 * @param fb              FrameBuffer du HUD
 * @param outX, outY      Coordonnées de sortie en pixels
 * @return true si le point est visible sur le HUD
 */
static bool projectCannonSightToHUD(const Vector3D &targetWorld, const Matrix &planeFromWorld, const Vector3D &eyeLocal,
                                    const Vector2D &cannonAngularOffset, // (azimut_offset, elevation_offset) en radians
                                    FrameBuffer *fb, int &outX, int &outY) {
    // Transformer la cible dans le repère local de l'avion
    Vector3D worldPos = targetWorld;
    Vector3D targetLocal = worldPos.transformPoint(planeFromWorld);

    // Direction depuis l'origine de l'avion vers la cible dans le repère local
    Vector3D toTarget = targetLocal;

    // Dans le repère cockpit:
    // -Z = devant l'avion (forward)
    // X = droite/gauche (positif = droite)
    // Y = vers le haut (positif = haut)

    // La composante -Z doit être positive (cible devant)
    const float forward = -toTarget.z;
    if (forward <= 0.001f)
        return false;

    // Calcul des angles azimut (horizontal) et élévation (vertical)
    // depuis l'axe de visée (-Z) du cockpit
    float az = atan2f(toTarget.x, forward); // Angle horizontal (X = droite/gauche)
    float el = atan2f(toTarget.y, forward); // Angle vertical (Y = haut/bas)

    // Appliquer l'offset angulaire du cannon
    // Le cannon pointe légèrement différemment de l'axe de l'avion
    az += cannonAngularOffset.x; // Correction azimut
    el += cannonAngularOffset.y; // Correction élévation (cannon plus bas = élévation négative)

    float fovX = 2*atanf((1.35f - (-1.22f))/2.0f / 8.0f); 
    float fovY = 2*atanf((2.0f - (-0.8f))/2.0f / 8.0f);
    const float hudFovX = fovX; // ~24° horizontal
    const float hudFovY = fovY; // ~26° vertical

    // Normaliser les angles dans [-1, 1]
    const float nx = az / (hudFovX * 0.5f);
    const float ny = el / (hudFovY * 0.5f);

    // Centre du HUD en pixels
    const float cx = (fb->width - 1) * 0.5f;
    const float cy = (fb->height - 1) * 0.5f;

    // Convertir en coordonnées pixels
    // Note: Y est inversé car l'origine du framebuffer est en haut
    outX = (int)(cx + nx * cx);
    outY = (int)(cy - ny * cy);

    return (outX >= 0 && outX < fb->width && outY >= 0 && outY < fb->height);
}
void SCCockpit::RenderTargetingReticle(FrameBuffer *fb) {
    if (!fb) {
        fb = VGA.getFrameBuffer();
    }
    if (!this->player_plane) {
        return;
    }
    float target_distance = 0.0f;
    const float dt = (this->player_plane->tps > 0) ? (1.0f / (float)this->player_plane->tps) : (1.0f / 60.0f);

    Vector3D avion_pos {
        this->player_plane->x,
        this->player_plane->y,
        this->player_plane->z
    };
    Vector3D planeVelWorld = {
        (this->player_plane->x - this->player_plane->last_px),
        (this->player_plane->y - this->player_plane->last_py),
        (this->player_plane->z - this->player_plane->last_pz)
    };
    
    float timeOfFlight = 0.5f;
    float debutTimeOfFlight = timeOfFlight;
    
    float projectile_speed = 250.0f * (this->player_plane->tps / 60.0f);
    
    Vector3D initial_trust{0, 0, 0};

    initial_trust = this->player_plane->getWeaponIntialVector(projectile_speed);
    float projectile_speed_world = initial_trust.Length();
    // === LEAD ANGLE: vitesse de la cible ===
    Vector3D targetVelocityWorld = {0.0f, 0.0f, 0.0f};
    Vector3D predictedTargetPos = (this->target != nullptr) 
        ? this->target->position 
        : avion_pos;
    float newDist = 0.0f;
    if (this->target != nullptr) {
        Vector3D toTarget = {
            this->target->position.x - avion_pos.x,
            this->target->position.y - avion_pos.y,
            this->target->position.z - avion_pos.z
        };
        target_distance = toTarget.Length();
        
        // Temps de vol vers la cible courante
        float tof = target_distance / projectile_speed_world;
        int nbsteps_predict = tof * this->player_plane->tps;
        timeOfFlight = tof;
        debutTimeOfFlight = timeOfFlight;
        // Récupérer la vitesse de la cible depuis l'acteur mission
        // On cherche l'acteur correspondant à this->target
        SCMissionActors *targetActor = nullptr;
        for (auto actor : this->current_mission->enemies) {
            if (actor->object == this->target && actor->is_active) {
                targetActor = actor;
                break;
            }
        }
        if (targetActor == nullptr) {
            for (auto actor : this->current_mission->friendlies) {
                if (actor->object == this->target && actor->is_active) {
                    targetActor = actor;
                    break;
                }
            }
        }

        if (targetActor != nullptr && targetActor->plane != nullptr) {
            
            /**/
            Vector3D targetVelocityWorld = {
                (targetActor->plane->x - targetActor->plane->last_px) / dt,
                (targetActor->plane->y - targetActor->plane->last_py) / dt,
                (targetActor->plane->z - targetActor->plane->last_pz) / dt
            };
        }

        // Position prédite de la cible à l'instant d'impact (lead)
        // On itère: la position prédite change le ToF, qui change la position prédite
        // 2-3 itérations suffisent pour converger
        for (int iter = 0; iter < 3; iter++) {
            predictedTargetPos = {
                this->target->position.x + targetVelocityWorld.x * tof,
                this->target->position.y + targetVelocityWorld.y * tof,
                this->target->position.z + targetVelocityWorld.z * tof
            };
            // Recalcul du ToF avec la nouvelle distance prédite
            Vector3D toPredict = {
                predictedTargetPos.x - avion_pos.x,
                predictedTargetPos.y - avion_pos.y,
                predictedTargetPos.z - avion_pos.z
            };
            newDist = toPredict.Length();
            tof = (projectile_speed_world > 0.0f) ? (newDist / projectile_speed_world) : 0.0f;
            nbsteps_predict = tof * this->player_plane->tps;
        }
        
        timeOfFlight = tof;
        //nbsteps_predict = timeOfFlight * this->player_plane->tps;
        // Vitesse RELATIVE de la cible par rapport à l'avion
        /*Vector3D relativeVelocity = {
            targetVelocityWorld.x - planeVelWorld.x * dt,
            targetVelocityWorld.y - planeVelWorld.y * dt,
            targetVelocityWorld.z - planeVelWorld.z * dt
        };
        for (int iter = 0; iter < 3; iter++) {
            predictedTargetPos = {
                this->target->position.x + relativeVelocity.x * nbsteps_predict,
                this->target->position.y + relativeVelocity.y * nbsteps_predict,
                this->target->position.z + relativeVelocity.z * nbsteps_predict
            };
            Vector3D toPredict = predictedTargetPos - avion_pos;
            float newDist = toPredict.Length();
            timeOfFlight = newDist / projectile_speed_world;
        }*/
    }

    int nbsteps = timeOfFlight * this->player_plane->tps;
    GunSimulatedObject *weap = new GunSimulatedObject();
    
    Vector3D omegaLocal = this->player_plane->angular_velocity;
    
    Matrix &ptw = this->player_plane->ptw;
    Vector3D omegaStep = {
        ptw.v[0][0] * omegaLocal.x + ptw.v[1][0] * omegaLocal.y + ptw.v[2][0] * omegaLocal.z,
        ptw.v[0][1] * omegaLocal.x + ptw.v[1][1] * omegaLocal.y + ptw.v[2][1] * omegaLocal.z,
        ptw.v[0][2] * omegaLocal.x + ptw.v[1][2] * omegaLocal.y + ptw.v[2][2] * omegaLocal.z
    };

    initial_trust = this->player_plane->getWeaponIntialVector(projectile_speed);
    Vector3D planeDispAccum{0, 0, 0};
    
    weap->obj = this->player_plane->weaps_load[0]->objct;
    weap->x = this->player_plane->x + planeDispAccum.x;
    weap->y = this->player_plane->y + planeDispAccum.y;
    weap->z = this->player_plane->z + planeDispAccum.z;
    weap->vx = initial_trust.x;
    weap->vy = initial_trust.y;
    weap->vz = initial_trust.z;
    weap->weight = this->player_plane->weaps_load[0]->objct->weight_in_kg * 2.205f;
    weap->azimuthf = this->player_plane->azimuthf;
    weap->elevationf = this->player_plane->elevationf;
    weap->target = nullptr;

    Vector3D impact{0, 0, 0};
    Vector3D velo{0, 0, 0};
    bool firstIter = true;
    float bulletSpeedAfterShot = 0.0f;
    for (int i = 0; i < nbsteps; i++) {
        // 1. Accumuler le déplacement avion
        planeDispAccum = planeDispAccum + planeVelWorld;
        planeVelWorld = planeVelWorld.rotateByAxis(omegaStep);

        std::tie(impact, velo) = weap->ComputeTrajectory(this->player_plane->tps);
        
        Vector3D finalPos = {
            impact.x + planeDispAccum.x,
            impact.y + planeDispAccum.y,
            impact.z + planeDispAccum.z
        };

        weap->x = impact.x;
        weap->y = impact.y;
        weap->z = impact.z;
        
        weap->vx = velo.x;
        weap->vy = velo.y;
        weap->vz = velo.z;

        if (firstIter) {
            bulletSpeedAfterShot = velo.Length();
            firstIter = false;
        }
    }

    const Vector3D impactWorld = {
        impact.x + planeDispAccum.x,
        impact.y + planeDispAccum.y,
        impact.z + planeDispAccum.z
    };
    this->targetImpactPointWorld = impactWorld;

    Matrix planeFromWorld = this->player_plane->ptw.invertRigidBodyMatrixLocal();

    int Xdraw = 0, Ydraw = 0;
    if (projectCannonSightToHUD(impactWorld, planeFromWorld, this->hud_eye_world, this->cannonAngularOffset, fb, Xdraw, Ydraw)) {
        Point2D gun_piper= {Xdraw, Ydraw};
        RLEShape *s = this->hud->small_hud->LCOS->SHAPSET->GetShape(0);
        int shapeWidth = s->GetWidth();
        int shapeHeight = s->GetHeight();
        gun_piper.x -= shapeWidth / 2;
        gun_piper.y -= shapeHeight / 2;
        s->SetPosition(&gun_piper);
        fb->drawShapeWithBox(s, 0, 320, 0,
                             200);
        int distance_to_target_index = target_distance*3.2808399f / 1000;
        if (distance_to_target_index > 12 ) {
            distance_to_target_index = 12;
        }
        std::unordered_map<int, int> distance_to_target_shape = {
            {0, 0},
            {1, 1},
            {2, 2},
            {3, 3},
            {4, 4},
            {5, 5},
            {6, 6},
            {7, 7},
            {8, 8},
            {9, 10},
            {10, 11},
            {11, 12},
            {12, 9}
        };
        std::unordered_map<int, int> distance_shape_offset = {
            {2, 6},
            {4, 6},
            {6, 4},
            {8, 2}
        };
        if (distance_to_target_index > 0) {
            RLEShape *distance = this->hud->small_hud->LCOS->SHAPSET->GetShape(distance_to_target_shape[distance_to_target_index]);
            gun_piper= {Xdraw, Ydraw};
            int shapeWidth = s->GetWidth();
            int shapeHeight = s->GetHeight();
            int dwidth = distance->GetWidth();
            int dheight = distance->GetHeight();
            int distanceShapeWidth = distance->GetWidth();
            
            gun_piper.x -= shapeWidth / 2;
            gun_piper.y -= shapeHeight / 2;
            gun_piper.x += distance_shape_offset[dwidth];
            gun_piper.y += 2;
            distance->SetPosition(&gun_piper);
            fb->drawShapeWithBox(distance, 0, 320, 0,
                                 200);
        }
    }

    // === Affichage du LEAD DIAMOND sur la cible prédite ===
    // On projette la position future de la cible (avec lead)
    if (this->target != nullptr) {
        int Xlead = 0, Ylead = 0;
        if (projectCannonSightToHUD(predictedTargetPos, planeFromWorld, this->hud_eye_world, this->cannonAngularOffset, fb, Xlead, Ylead)) {
            // Dessine un losange/diamond autour de la position prédite de la cible
            int diamond_size = 4;
            fb->lineWithBox(Xlead, Ylead - diamond_size, Xlead + diamond_size, Ylead, 223, 0, 320, 0, 200);
            fb->lineWithBox(Xlead + diamond_size, Ylead, Xlead, Ylead + diamond_size, 223, 0, 320, 0, 200);
            fb->lineWithBox(Xlead, Ylead + diamond_size, Xlead - diamond_size, Ylead, 223, 0, 320, 0, 200);
            fb->lineWithBox(Xlead - diamond_size, Ylead, Xlead, Ylead - diamond_size, 223, 0, 320, 0, 200);
        }
    }

    if (debug_print) {
        Point2D debugPos = {5, 10};
        std::string debugTxt5 = "Angular speed: " + std::to_string(this->player_plane->angular_velocity.x) + ", " + std::to_string(this->player_plane->angular_velocity.y) + ", " + std::to_string(this->player_plane->angular_velocity.z);
        debug_framebuffer->printText(this->big_font, &debugPos, (char *)debugTxt5.c_str(), 0, 0, (uint32_t)debugTxt5.length(), 2, 2);

        std::string debugTxt = "TOF: " + std::to_string(timeOfFlight) + "s nbsteps: " + std::to_string(nbsteps);
        debugPos.x = 5;
        debugPos.y += 8;
        debug_framebuffer->printText(this->big_font, &debugPos, (char *)debugTxt.c_str(), 0, 0, (uint32_t)debugTxt.length(), 2, 2);
        std::string debugTxt2 = "Dist: " + std::to_string((int)target_distance) + "m predict: " + std::to_string((int)newDist) + "m";
        debugPos.x = 5;
        debugPos.y += 8;
        debug_framebuffer->printText(this->big_font, &debugPos, (char *)debugTxt2.c_str(), 0, 0, (uint32_t)debugTxt2.length(), 2, 2);
        std::string debugTxt3 = "Debug TOF: " + std::to_string(debutTimeOfFlight) + "s";
        debugPos.x = 5;
        debugPos.y += 8;
        debug_framebuffer->printText(this->big_font, &debugPos, (char *)debugTxt3.c_str(), 0, 0, (uint32_t)debugTxt3.length(), 2, 2);
        float predictedImpactDistance = (this->targetImpactPointWorld - avion_pos).Length();
        std::string debugTxt4 = "Predicted Impact Dist: " + std::to_string((int)predictedImpactDistance) + "m bullet speed: " + std::to_string((int)projectile_speed_world) + " m/s after shot: " + std::to_string((int)bulletSpeedAfterShot) + " m/s";
        debugPos.x = 5;
        debugPos.y += 8;
        debug_framebuffer->printText(this->big_font, &debugPos, (char *)debugTxt4.c_str(), 0, 0, (uint32_t)debugTxt4.length(), 2, 2);
    }
    delete weap;
}
void SCCockpit::RenderBombSight(FrameBuffer *fb) {
    // Par défaut, on dessine dans la texture HUD si elle existe
    if (!fb) {
        fb = (this->hud_framebuffer != nullptr) ? this->hud_framebuffer : VGA.getFrameBuffer();
    }
    if (!this->player_plane) {
        return;
    }

    // Zone de visée dans le HUD
    const Point2D center{fb->width / 2, fb->height / 2};
    const int width = 50;
    const int height = 60;

    const int bx1 = center.x - width / 2;
    const int bx2 = center.x + width / 2;
    const int by1 = center.y - height / 2;
    const int by2 = center.y + height / 2;

    GunSimulatedObject *weap = new GunSimulatedObject();

    
    float thrustMagnitude = 50.0f * ((float)this->player_plane->tps / 60.0f); // comme avant

    Vector3D initial_trust{0, 0, 0};
    initial_trust = this->player_plane->getWeaponIntialVector(thrustMagnitude);
    weap->obj = this->player_plane->weaps_load[this->player_plane->selected_weapon]->objct;

    weap->x = this->player_plane->x;
    weap->y = this->player_plane->y;
    weap->z = this->player_plane->z;
    weap->vx = initial_trust.x;
    weap->vy = initial_trust.y;
    weap->vz = initial_trust.z;

    weap->weight = this->player_plane->weaps_load[this->player_plane->selected_weapon]->objct->weight_in_kg * 2.205f;
    weap->azimuthf = this->player_plane->yaw;
    weap->elevationf = this->player_plane->pitch;
    weap->target = nullptr;
    weap->mission = this->current_mission;

    Vector3D impact{0, 0, 0};
    Vector3D velo{0, 0, 0};
    std::tie(impact, velo) = weap->ComputeTrajectoryUntilGround(this->player_plane->tps);

    Vector3D impactWorld = {impact.x, impact.y, impact.z};
    this->targetImpactPointWorld = impactWorld;
    const Matrix planeFromWorld = this->player_plane->ptw.invertRigidBodyMatrixLocal();

    const Vector2D bombAngularOffset = {0.0f, 0.0f};

    int Xhud = 0, Yhud = 0;
    if (projectCannonSightToHUD(impactWorld, planeFromWorld, this->hud_eye_world, this->cannonAngularOffset, fb, Xhud, Yhud)) {
        fb->plot_pixel(center.x, center.y, 223);
        fb->lineWithBox(center.x, center.y, Xhud, Yhud, 223, bx1, bx2, by1, by2);
        fb->plot_pixel(Xhud, Yhud, 223);
        fb->circle_slow(Xhud, Yhud, 6, 90);
    }

    delete weap;
}
void SCCockpit::RenderMFDSWeapon(Point2D pmfd_right, FrameBuffer *fb = nullptr) {
    if (this->cockpit->MONI.SHAP.data == nullptr) {
        return;
    }
    if (!fb) {
        fb = VGA.getFrameBuffer();
    }
    std::string txt;
    this->RenderMFDS(pmfd_right, fb);
    Point2D pmfd_right_center = {pmfd_right.x + this->cockpit->MONI.SHAP.GetWidth() / 2,
                                 pmfd_right.y + this->cockpit->MONI.SHAP.GetHeight() / 2};
    Point2D pmfd_right_weapon = {pmfd_right_center.x - this->cockpit->MONI.MFDS.WEAP.ARTS.GetShape(0)->GetWidth() / 2,
                                 pmfd_right_center.y - 10 -
                                     this->cockpit->MONI.MFDS.WEAP.ARTS.GetShape(0)->GetHeight() / 2};
    this->cockpit->MONI.MFDS.WEAP.ARTS.GetShape(0)->SetPosition(&pmfd_right_weapon);
    fb->drawShape(this->cockpit->MONI.MFDS.WEAP.ARTS.GetShape(0));
    std::unordered_map<int, int> weapons_shape = {
        {1, 1},
        {2, 1},
        {3, 5},
        {4, 7},
        {5, 9},
        {6, 11},
        {7, 13},    
        {8, 15},
        {9, 3}
    };
    if (this->player_plane->object->entity->hpts.size() > 1) {
        for (int indice = 1; indice < this->player_plane->object->entity->hpts.size() / 2 + 1; indice++) {
            if (this->player_plane->weaps_load[indice] == nullptr) {
                continue;
            }
            int weapon_id = this->player_plane->weaps_load[indice]->objct->wdat->weapon_id;
            int nb_weap = this->player_plane->weaps_load[indice]->nb_weap;
            int i = 4 - indice;
            int selected = indice == this->player_plane->selected_weapon;
            RLEShape *shape = this->cockpit->MONI.MFDS.WEAP.ARTS.GetShape(weapons_shape[weapon_id] + selected);
            int32_t s_width = shape->GetWidth();
            int32_t s_height = shape->GetHeight();

            if (nb_weap > 0) {
                Point2D pmfd_right_weapon_hp = {pmfd_right_center.x - 15 - s_width / 2 - i * 9,
                                                pmfd_right_center.y - 18 - s_height / 2 + i * 9};
                shape->SetPosition(&pmfd_right_weapon_hp);
                fb->drawShape(shape);
                txt = std::to_string(nb_weap);
                Point2D pmfd_right_weapon_hp_text = {pmfd_right_weapon_hp.x + s_width / 2 + 1,
                                                     pmfd_right_weapon_hp.y + s_height + 6};
                fb->printText(this->big_font, &pmfd_right_weapon_hp_text, (char *)txt.c_str(), 0, 0,
                              (uint32_t)txt.length(), 2, 2);
            }
            if (this->player_plane->weaps_load[this->player_plane->object->entity->hpts.size() - indice] == nullptr) {
                continue;
            }
            nb_weap = this->player_plane->weaps_load[this->player_plane->object->entity->hpts.size() - indice]->nb_weap;
            selected = 9 - indice == this->player_plane->selected_weapon;
            shape = this->cockpit->MONI.MFDS.WEAP.ARTS.GetShape(weapons_shape[weapon_id] + selected);
            if (nb_weap > 0) {
                Point2D pmfd_right_weapon_hp_left = {pmfd_right_center.x + 13 - s_width / 2 + i * 9,
                                                     pmfd_right_center.y - 18 - s_height / 2 + i * 9};
                shape->SetPosition(&pmfd_right_weapon_hp_left);
                fb->drawShape(shape);
                txt = std::to_string(nb_weap);
                Point2D pmfd_right_weapon_hp_text_left = {pmfd_right_weapon_hp_left.x + s_width / 2 - 1,
                                                          pmfd_right_weapon_hp_left.y + s_height + 6};
                fb->printText(this->big_font, &pmfd_right_weapon_hp_text_left, (char *)txt.c_str(), 0, 0,
                              (uint32_t)txt.length(), 2, 2);
            }
        }
    }

    int sel_weapon_id = 0;
    int sel_nb_weap = 0;
    if (this->player_plane->weaps_load[this->player_plane->selected_weapon] != nullptr) {
        sel_weapon_id = this->player_plane->weaps_load[this->player_plane->selected_weapon]->objct->wdat->weapon_id;
        for (auto weap : this->player_plane->weaps_load) {
            if (weap != nullptr && weap->objct->wdat->weapon_id == sel_weapon_id) {
                sel_nb_weap += weap->nb_weap;
            }
        }
    }

    Point2D pmfd_right_weapon_gun{pmfd_right_weapon.x - 8 +
                                      this->cockpit->MONI.MFDS.WEAP.ARTS.GetShape(0)->GetWidth() / 2,
                                  pmfd_right_weapon.y + 6};
    txt = std::to_string(sel_nb_weap);
    fb->printText(this->big_font, &pmfd_right_weapon_gun, (char *)txt.c_str(), 0, 0, (uint32_t)txt.length(), 2, 2);

    Point2D pmfd_right_weapon_radar{pmfd_right_weapon.x, pmfd_right_weapon.y + 5};
    fb->printText(this->big_font, &pmfd_right_weapon_radar, const_cast<char *>("NORM"), 0, 0, 4, 2, 2);

    Point2D pmfd_right_weapon_selected{pmfd_right_weapon.x + 12 +
                                           this->cockpit->MONI.MFDS.WEAP.ARTS.GetShape(0)->GetWidth() / 2,
                                       pmfd_right_weapon.y + 5};
    fb->printText(this->big_font, &pmfd_right_weapon_selected, (char *)weapon_names[static_cast<weapon_ids>(sel_weapon_id)].c_str(), 0, 0,
                  (uint32_t)weapon_names[static_cast<weapon_ids>(sel_weapon_id)].length(), 2, 2);

    Point2D pmfd_right_weapon_chaff{pmfd_right_weapon.x - 7 +
                                        this->cockpit->MONI.MFDS.WEAP.ARTS.GetShape(0)->GetWidth() / 2,
                                    pmfd_right_weapon.y + 4 * 9};
    fb->printText(this->big_font, &pmfd_right_weapon_chaff, const_cast<char *>("C:30"), 0, 0, 4, 2, 2);

    Point2D pmfd_right_weapon_flare{pmfd_right_weapon.x - 7 +
                                        this->cockpit->MONI.MFDS.WEAP.ARTS.GetShape(0)->GetWidth() / 2,
                                    pmfd_right_weapon.y + 5 * 9};
    fb->printText(this->big_font, &pmfd_right_weapon_flare, const_cast<char *>("F:30"), 0, 0, 4, 2, 2);
}

void SCCockpit::RenderMFDSRadar(Point2D pmfd_left, float range, int mode, FrameBuffer *fb = nullptr) {
    if (!fb) {
        fb = VGA.getFrameBuffer();
    }
    switch (mode) {
    case RadarMode::AARD:
        this->RenderMFDSRadarImplementation(pmfd_left, range, "AIR", true, fb);
        break;
    case RadarMode::AGRD:
        this->RenderMFDSRadarImplementation(pmfd_left, range, "GROUND", false, fb);
        break;
    case RadarMode::ASST:
        this->RenderMFDSRadarSingleTargetImplementation(pmfd_left, range, "SINGLE", false, fb);
        break;
    case RadarMode::AFRD:
        break;
    default:
        break;
    }
}

void SCCockpit::RenderMFDSRadarSingleTargetImplementation(Point2D pmfd_left, float range, const char *mode_name, bool air_mode,
                                              FrameBuffer *fb = nullptr) {
    if (!fb) {
        fb = VGA.getFrameBuffer();
    }
    // Calcul des positions et dimensions
    Point2D pmfd_center = {
        pmfd_left.x + this->cockpit->MONI.SHAP.GetWidth() / 2,
        pmfd_left.y + this->cockpit->MONI.SHAP.GetHeight() / 2,
    };
    this->RenderMFDS(pmfd_left, fb);

    // Dimensions du radar
    Point2D radar_size = {100, 80};
    Point2D bottom_right = {pmfd_left.x + radar_size.x, pmfd_left.y + radar_size.y};

    // Choisir les ressources en fonction du mode
    auto &radar_arts = this->cockpit->MONI.MFDS.AARD.ARTS;
    int bg_shape_index = 3;

    // Position du fond du radar
    Point2D radar_bg_pos = pmfd_left;

    
    radar_bg_pos.x += this->cockpit->MONI.SHAP.GetWidth() / 2 - radar_arts.GetShape(bg_shape_index)->GetWidth() / 2;
    
    radar_bg_pos.y += this->cockpit->MONI.SHAP.GetHeight() / 2 - radar_arts.GetShape(bg_shape_index)->GetHeight() / 2;

    // Dessine le fond du radar
    radar_arts.GetShape(bg_shape_index)->SetPosition(&radar_bg_pos);
    fb->drawShape(radar_arts.GetShape(bg_shape_index));

    // Affiche les infos textuelles
    Point2D pmfd_text = {radar_bg_pos.x + 10, radar_bg_pos.y + 2};
    std::string rzoom_str = "10";
    switch (this->radar_zoom) {
    case 1:
        rzoom_str = "10";
        break;
    case 2:
        rzoom_str = "20";
        break;
    case 3:
        rzoom_str = "40";
        break;
    case 4:
        rzoom_str = "80";
        break;
    default:
        rzoom_str = "10";
        break;
    }
    fb->printText(this->big_font, &pmfd_text, (char *)rzoom_str.c_str(), 0, 0, 2, 2, 2);

    std::string mode_text = " " + std::string(mode_name) + " ";
    fb->printText(this->big_font, &pmfd_text, const_cast<char *>(mode_text.c_str()), 0, 0, (int)mode_text.length(), 2,
                  2);

    // Position du joueur comme centre du radar
    Vector2D center = {this->player->position.x, this->player->position.z};

    // Préparation pour la rotation
    int heading = (int)this->heading;
    heading = (heading + 360) % 360; // Normalisation entre 0-359
    float headingRad = heading / 180.0f * (float)M_PI;

    // Fonction pour dessiner un contact sur le radar
    auto drawContact = [&](SCMissionActors *object, bool isFriendly, bool isDestroyed) {
        // Vérifier si l'entité correspond au mode actuel
        bool valid_entity;
        if (!object->is_active)
            return;

        Vector2D objPos = {object->object->position.x, object->object->position.z};

        // Rotation selon le heading du joueur
        Vector2D rotatedPos = objPos.rotateAroundPoint(center, headingRad);
        Vector2D relativePos = {rotatedPos.x - center.x, rotatedPos.y - center.y};

        // Vérification de la distance
        float distance = sqrtf(relativePos.x * relativePos.x + relativePos.y * relativePos.y);
        if (distance >= range)
            return;

        // Échelle et position sur l'écran
        float scale = 60.0f / range;
        Point2D screenPos = {pmfd_center.x + (int)(relativePos.x * scale),
                             pmfd_center.y + (int)(relativePos.y * scale)};

        // Vérifie si le contact est dans les limites du MFD
        if (screenPos.x <= pmfd_left.x || screenPos.x >= pmfd_left.x + this->cockpit->MONI.SHAP.GetWidth() ||
            screenPos.y <= pmfd_left.y || screenPos.y >= pmfd_left.y + this->cockpit->MONI.SHAP.GetHeight())
            return;

        // Sélectionne l'icône appropriée
        int iconIndex;
        
        iconIndex = 10;
    
        // Dessine l'icône
        radar_arts.GetShape(iconIndex)->SetPosition(&screenPos);
        fb->drawShapeWithBox(radar_arts.GetShape(iconIndex), pmfd_left.x, bottom_right.x, pmfd_left.y, bottom_right.y);
        Point2D targetPos = {screenPos.x - 2, screenPos.y - 1};
        std::string altitude_str = std::to_string((int)object->object->position.y);
        std::string heading_str = std::to_string((int)object->object->azymuth);
        std::string speed_str = std::to_string(object->plane != nullptr ? (int)object->plane->airspeed: 0);
        Point2D textPos = {targetPos.x + 10, targetPos.y};
        fb->printText(this->big_font, &textPos, (char *)altitude_str.c_str(), 0, 0, (uint32_t)altitude_str.length(), 1, 1);
        textPos.x = targetPos.x + 10;
        textPos.y = textPos.y + 6;
        fb->printText(this->big_font, &textPos, (char *)heading_str.c_str(), 0, 0, (uint32_t)heading_str.length(), 1, 1);
        textPos.x = targetPos.x + 10;
        textPos.y = textPos.y + 6;
        fb->printText(this->big_font, &textPos, (char *)speed_str.c_str(), 0, 0, (uint32_t)speed_str.length(), 1, 1);
    };

    // Dessine les ennemis 
    for (auto actor : this->current_mission->enemies) {
        if (actor->object == this->target) {
            drawContact(actor, false, actor->is_destroyed);
        }
    }

    // Dessine les alliés (sauf le joueur)
    for (auto actor : this->current_mission->friendlies) {
        if (actor->object == this->target) {
            drawContact(actor, true, actor->is_destroyed);
        }
    }
}

void SCCockpit::RenderMFDSRadarImplementation(Point2D pmfd_left, float range, const char *mode_name, bool air_mode,
                                              FrameBuffer *fb = nullptr) {
    if (!fb) {
        fb = VGA.getFrameBuffer();
    }
    // Calcul des positions et dimensions
    Point2D pmfd_center = {
        pmfd_left.x + this->cockpit->MONI.SHAP.GetWidth() / 2,
        pmfd_left.y + this->cockpit->MONI.SHAP.GetHeight() / 2,
    };
    this->RenderMFDS(pmfd_left, fb);

    // Dimensions du radar
    Point2D radar_size = {100, 80};
    Point2D bottom_right = {pmfd_left.x + radar_size.x, pmfd_left.y + radar_size.y};

    // Choisir les ressources en fonction du mode
    auto &radar_arts = air_mode ? this->cockpit->MONI.MFDS.AARD.ARTS : this->cockpit->MONI.MFDS.AGRD.ARTS;
    int bg_shape_index = air_mode ? 4 : 1;

    // Position du fond du radar
    Point2D radar_bg_pos = pmfd_left;

    if (air_mode) {
        radar_bg_pos.x +=
            -5 + this->cockpit->MONI.SHAP.GetWidth() / 2 - radar_arts.GetShape(bg_shape_index)->GetWidth() / 2;
    } else {
        radar_bg_pos.x += this->cockpit->MONI.SHAP.GetWidth() / 2 - radar_arts.GetShape(bg_shape_index)->GetWidth();
    }

    radar_bg_pos.y += this->cockpit->MONI.SHAP.GetHeight() / 2 - radar_arts.GetShape(bg_shape_index)->GetHeight() / 2;

    // Dessine le fond du radar
    radar_arts.GetShape(bg_shape_index)->SetPosition(&radar_bg_pos);
    fb->drawShape(radar_arts.GetShape(bg_shape_index));

    // Affiche les infos textuelles
    Point2D pmfd_text = {radar_bg_pos.x + 10, radar_bg_pos.y + 2};
    std::string rzoom_str = "10";
    switch (this->radar_zoom) {
    case 1:
        rzoom_str = "10";
        break;
    case 2:
        rzoom_str = "20";
        break;
    case 3:
        rzoom_str = "40";
        break;
    case 4:
        rzoom_str = "80";
        break;
    default:
        rzoom_str = "10";
        break;
    }
    fb->printText(this->big_font, &pmfd_text, (char *)rzoom_str.c_str(), 0, 0, 2, 2, 2);

    std::string mode_text = " " + std::string(mode_name) + " ";
    fb->printText(this->big_font, &pmfd_text, const_cast<char *>(mode_text.c_str()), 0, 0, (int)mode_text.length(), 2,
                  2);

    // Affiche le "360" uniquement en mode air
    if (air_mode) {
        fb->printText(this->big_font, &pmfd_text, const_cast<char *>("360"), 0, 0, 3, 2, 2);
    }

    // Position du joueur comme centre du radar
    Vector2D center = {this->player->position.x, this->player->position.z};

    // Préparation pour la rotation
    int heading = (int)this->heading;
    heading = (heading + 360) % 360; // Normalisation entre 0-359
    float headingRad = heading / 180.0f * (float)M_PI;

    // Fonction pour dessiner un contact sur le radar
    auto drawContact = [&](SCMissionActors *object, bool isFriendly, bool isDestroyed) {
        // Vérifier si l'entité correspond au mode actuel
        bool valid_entity;
        if (!object->is_active)
            return;
        if (air_mode) {
            valid_entity = (object->object->entity->entity_type == EntityType::jet);
        } else {
            valid_entity = (object->object->entity->entity_type == EntityType::ground ||
                            object->object->entity->entity_type == EntityType::ornt ||
                            object->object->entity->entity_type == EntityType::swpn);
        }

        if (!valid_entity)
            return;

        Vector2D objPos = {object->object->position.x, object->object->position.z};

        // Rotation selon le heading du joueur
        Vector2D rotatedPos = objPos.rotateAroundPoint(center, headingRad);
        Vector2D relativePos = {rotatedPos.x - center.x, rotatedPos.y - center.y};

        // Vérification de la distance
        float distance = sqrtf(relativePos.x * relativePos.x + relativePos.y * relativePos.y);
        if (distance >= range)
            return;

        // Échelle et position sur l'écran
        float scale = 60.0f / range;
        Point2D screenPos = {pmfd_center.x + (int)(relativePos.x * scale),
                             pmfd_center.y + (int)(relativePos.y * scale)};

        // Vérifie si le contact est dans les limites du MFD
        if (screenPos.x <= pmfd_left.x || screenPos.x >= pmfd_left.x + this->cockpit->MONI.SHAP.GetWidth() ||
            screenPos.y <= pmfd_left.y || screenPos.y >= pmfd_left.y + this->cockpit->MONI.SHAP.GetHeight())
            return;

        // Sélectionne l'icône appropriée
        int iconIndex;
        if (air_mode) {
            if (isFriendly) {
                iconIndex = isDestroyed ? 8 : 9;
            } else {
                iconIndex = isDestroyed ? 5 : 0;
            }
        } else {
            // En mode sol, tous les icônes sont les mêmes (index 3)
            iconIndex = 3;
        }

        // Dessine l'icône
        radar_arts.GetShape(iconIndex)->SetPosition(&screenPos);
        fb->drawShapeWithBox(radar_arts.GetShape(iconIndex), pmfd_left.x, bottom_right.x, pmfd_left.y, bottom_right.y);

        // Si c'est la cible actuelle, dessine le marqueur de cible
        if (object->object == this->target) {
            Point2D targetPos = {screenPos.x - 2, screenPos.y - 1};
            // Utilise toujours l'icône de cible du mode air
            this->cockpit->MONI.MFDS.AARD.ARTS.GetShape(2)->SetPosition(&targetPos);
            fb->drawShapeWithBox(this->cockpit->MONI.MFDS.AARD.ARTS.GetShape(2), pmfd_left.x, bottom_right.x,
                                 pmfd_left.y, bottom_right.y);
        }
    };

    // Dessine les ennemis 
    for (auto actor : this->current_mission->enemies) {
        drawContact(actor, false, actor->is_destroyed);
    }

    // Dessine les alliés (sauf le joueur)
    for (auto actor : this->current_mission->friendlies) {
        if (actor != this->current_mission->player) {
            drawContact(actor, true, actor->is_destroyed);
        }
    }
}

void SCCockpit::RenderRAWS(Point2D pmfd_left = {84, 112}, FrameBuffer *fb = nullptr) {
    if (this->cockpit->MONI.INST.RAWS.NORM.data == nullptr) {
        return;
    }
    if (!fb) {
        fb = VGA.getFrameBuffer();
    }
    Point2D raws_size = {this->cockpit->MONI.INST.RAWS.NORM.GetWidth(), this->cockpit->MONI.INST.RAWS.NORM.GetHeight()};
    if (this->cockpit->MONI.INST.RAWS.x != 0 && this->cockpit->MONI.INST.RAWS.y != 0) {
        if (pmfd_left.x != 0 && pmfd_left.y != 0) {
            pmfd_left.x = this->cockpit->MONI.INST.RAWS.x;
            pmfd_left.y = this->cockpit->MONI.INST.RAWS.y;
        }
    }
    Point2D bottom_right = {pmfd_left.x + raws_size.x, pmfd_left.y + raws_size.y};
    this->cockpit->MONI.INST.RAWS.NORM.SetPosition(&pmfd_left);
    fb->drawShape(&this->cockpit->MONI.INST.RAWS.NORM);
    int heading = (int)this->heading;
    heading = (heading + 360) % 360; // Normalisation entre 0-359
    float headingRad = heading / 180.0f * (float)M_PI;

    for (auto contact : this->current_mission->actors) {
        if (contact->is_active && contact->object->entity->entity_type == EntityType::jet) {
            Vector2D contact_pos = {contact->object->position.x, contact->object->position.z};
            Vector2D center = {this->player->position.x, this->player->position.z};
            Vector2D roa_dir = {contact_pos.x - center.x, contact_pos.y - center.y};

            float distance = roa_dir.Length();
            roa_dir.Normalize();
            const float max_range = 30000.0f; // 30 km
            if (distance < max_range) {
                float scale = 10.0f;
                scale = (distance / max_range) * this->cockpit->MONI.INST.RAWS.NORM.GetWidth() /
                        2; // Ajustement de l'échelle
                Point2D p = {(int)(roa_dir.x * scale), (int)(roa_dir.y * scale)};
                Point2D rotatedPos = p.rotateAroundPoint({0, 0}, headingRad);
                Point2D raw_pos = {pmfd_left.x + (raws_size.x / 2) + rotatedPos.x,
                                   pmfd_left.y + (raws_size.y / 2) + rotatedPos.y};
                this->cockpit->MONI.INST.RAWS.SYMB.GetShape(29)->SetPosition(&raw_pos);
                fb->drawShape(this->cockpit->MONI.INST.RAWS.SYMB.GetShape(29));
                // fb->plot_pixel((int)p.x, (int)p.y, 223);
            }
        }
    }
}

void SCCockpit::RenderMFDSComm(Point2D pmfd_left, int mode, FrameBuffer *fb = nullptr) {
    if (this->cockpit->MONI.SHAP.data == nullptr) {
        return;
    }
    if (!fb) {
        fb = VGA.getFrameBuffer();
    }
    for (int i = 0; i < this->cockpit->MONI.SHAP.GetHeight(); i++) {
        fb->line(pmfd_left.x, pmfd_left.y + i, pmfd_left.x + this->cockpit->MONI.SHAP.GetWidth(), pmfd_left.y + i, 2);
    }
    Vector2D center = {this->player->position.x, this->player->position.y};
    Point2D pmfd_text = {pmfd_left.x + 20, pmfd_left.y + 20};
    Point2D pfmd_title = {pmfd_text.x + 12, pmfd_text.y};
    fb->printText(this->big_font, &pfmd_title, const_cast<char *>("Comm mode:"), 120, 0, 10, 2, 2);
    pfmd_title.y += 10;
    pfmd_title.x = pmfd_left.x + 20;
    fb->printText(this->big_font, &pfmd_title, const_cast<char *>("Select frequency"), 0, 0, 16, 2, 2);
    pmfd_text.y += 20;
    if (mode == 0) {
        int cpt = 1;
        for (auto ai : this->current_mission->friendlies) {
            Vector2D ai_position = {ai->object->position.x, ai->object->position.z};
            Vector2D roa_dir = {ai_position.x - center.x, ai_position.y - center.y};

            float distance = sqrtf((float)(roa_dir.x * roa_dir.x) + (float)(roa_dir.y * roa_dir.y));
            if (ai->actor_name != "PLAYER" && ai->is_active && ai->profile != nullptr) {
                std::string name_str = std::to_string(cpt) + ". " + ai->actor_name;
                Point2D pfmd_entry = {pmfd_text.x, pmfd_text.y};
                fb->printText(this->big_font, &pfmd_entry, (char *)name_str.c_str(), 120, 0,
                              (uint32_t)name_str.length(), 2, 2);
                pmfd_text.y += 10;
                cpt++;
            }
        }
        if (cpt == 1) {
            pfmd_title.y += 25;
            pfmd_title.x = pmfd_left.x + 32;
            fb->printText(this->big_font, &pfmd_title, const_cast<char *>("NO RECIVER"), 120, 0, 10, 2, 2);
        }
    } else if (mode > 0) {
        int cpt = 1;
        if (this->comm_actor != nullptr && this->comm_actor->profile != nullptr) {
            auto ai = this->comm_actor;
            int cpt_message = 1;
            for (auto asks : ai->profile->radi.opts) {
                std::string toask = std::string("") + asks;
                std::string request = this->current_mission->player->profile->radi.asks.at(toask);
                Point2D pfmd_entry = {pmfd_text.x, pmfd_text.y};
                std::string asks_str = std::to_string(cpt_message) + ". " + request;
                fb->printText(this->big_font, &pfmd_entry, (char *)asks_str.c_str(), 124, 0,
                              (uint32_t)asks_str.length(), 2, 2);
                pmfd_text.y += 6;
                cpt_message++;
            }
        }
    }
    this->cockpit->MONI.SHAP.SetPosition(&pmfd_left);
    fb->drawShape(&this->cockpit->MONI.SHAP);
}

void SCCockpit::RenderMFDSDamage(Point2D pmfd_left, FrameBuffer *fb) {
    if (!fb) {
        fb = VGA.getFrameBuffer();
    }
    this->RenderMFDS(pmfd_left, fb);
    Point2D damage_pos = {pmfd_left.x +25, pmfd_left.y +15};
    RLEShape *damage_shape = this->cockpit->MONI.MFDS.DAMG.ARTS.GetShape(0);
    damage_shape->SetPosition(&damage_pos);
    fb->drawShape(damage_shape);
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
void SCCockpit::Render(CockpitFace face) {
    this->face = face;
    FrameBuffer *fb{nullptr};
    bool upscale = false;

    VGA.activate();
    VGA.setPalette(&this->palette);
    upscale = VGA.upscale;
    VGA.upscale = false;
    fb = VGA.getFrameBuffer();
    fb->clear();
    if (debug_print) {
        debug_framebuffer->clear();
    }
    this->cannonAngularOffset = {0.0f, 0.0f};
    if (cockpit != nullptr) {
        if (this->face  == CockpitFace::CP_FRONT) {
            if (this->hud != nullptr) {
                this->RenderTextTags({this->hud->small_hud->HINF->center_x,this->hud->small_hud->HINF->center_y}, fb);
                //fb->blit(this->hud_framebuffer->framebuffer, 111, 9, this->hud_framebuffer->width,
                //            this->hud_framebuffer->height);
            }
            if (this->target != this->player) {
                this->RenderTargetWithCam();
            }
        }
        if (this->face == CockpitFace::CP_BIG) {
            this->RenderTextTags({this->hud->large_hud->HINF->center_x,this->hud->large_hud->HINF->center_y}, fb);
        }
        fb->drawShape(this->cockpit->ARTP.GetShape(this->face));
        if (this->face  == CockpitFace::CP_FRONT || this->face  == CockpitFace::CP_BIG) {
            if (this->player_plane->weaps_load.size() > 0 &&
                this->player_plane->weaps_load[this->player_plane->selected_weapon] != nullptr) {
                if (this->radar_mode != RadarMode::ASST) {
                    switch (
                        this->player_plane->weaps_load[this->player_plane->selected_weapon]->objct->wdat->weapon_id) {
                    case ID_20MM:
                        this->radar_mode = RadarMode::AARD;
                        break;
                    case ID_AIM9J:
                    case ID_AIM9M:
                    case ID_AIM120:
                        this->radar_mode = RadarMode::AARD;
                        break;
                    case ID_MK20:
                    case ID_MK82:
                    case ID_DURANDAL:
                        this->radar_mode = RadarMode::AGRD;
                        break;
                    case ID_AGM65D:
                    case ID_GBU15:
                        this->radar_mode = RadarMode::AGRD;
                        break;
                    case ID_LAU3:
                        this->radar_mode = RadarMode::AGRD;
                        break;
                    }
                }
            }
            if (this->face ==CockpitFace::CP_FRONT) {
                this->RenderRAWS({84, 112}, fb);
                this->RenderAlti({161, 166}, fb);
                this->RenderSpeedOmetter({125, 166}, fb);
            }
            Point2D pmfd_right = {0, 200 - this->cockpit->MONI.SHAP.GetHeight()};
            Point2D pmfd_left = {320 - this->cockpit->MONI.SHAP.GetWidth() - 1,
                                    200 - this->cockpit->MONI.SHAP.GetHeight()};
            Point2D pmfd;
            bool mfds = false;
            if (this->show_radars) {
                if (!mfds) {
                    pmfd = pmfd_left;
                    mfds = true;
                } else {
                    pmfd = pmfd_right;
                }
                float range = 0.0f;
                switch (this->radar_zoom) {
                case 1:
                    range = 18520.0f;
                    break;
                case 2:
                    range = 18520.0f * 2.0f;
                case 3:
                    range = 18520.0f * 4.0f;
                    break;
                case 4:
                    range = 18520.0f * 8.0f;
                    break;
                }
                this->RenderMFDSRadar(pmfd, range, this->radar_mode);
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
            if (this->show_comm) {
                if (!mfds) {
                    pmfd = pmfd_left;
                    mfds = true;
                } else {
                    pmfd = pmfd_right;
                }
                this->RenderMFDSComm(pmfd, this->comm_target);
            }
            if (this->show_damage) {
                if (!mfds) {
                    pmfd = pmfd_left;
                    mfds = true;
                } else {
                    pmfd = pmfd_right;
                }
                this->RenderMFDSDamage(pmfd, fb);
            }
            if (this->show_cam) {
                if (!mfds) {
                    pmfd = pmfd_left;
                    mfds = true;
                } else {
                    pmfd = pmfd_right;
                }
                this->RenderMFDSCamera(pmfd, fb);
            }
        }
        this->RenderCommMessages({0,200}, fb);
    }
    if (this->mouse_control) {
        Mouse.draw();
    }
    if (debug_print) {  
        VGA.getFrameBuffer()->blitWithMask(debug_framebuffer->framebuffer, 0, 0, debug_framebuffer->width, debug_framebuffer->height,255);
    }
    VGA.vSync();
    VGA.upscale = upscale;
}
void SCCockpit::Update() {
    this->yaw_speed = this->yaw - (this->player_plane->azimuthf / 10.0f);
    this->pitch_speed = this->pitch - (this->player_plane->elevationf / 10.0f);
    this->roll_speed = this->roll - (this->player_plane->twist / 10.0f);
    this->pitch = this->player_plane->elevationf / 10.0f;
    this->roll = this->player_plane->twist / 10.0f;
    this->yaw = this->player_plane->azimuthf / 10.0f;
    this->speed = (float)this->player_plane->airspeed;
    this->mach = this->player_plane->mach;
    this->g_limit = this->player_plane->Lmax;
    this->g_load = this->player_plane->g_load;
    this->throttle = this->player_plane->GetThrottle();
    this->altitude = this->player_plane->y;
    this->heading = 360 - (this->player_plane->azimuthf / 10.0f);
    this->gear = this->player_plane->GetWheel();
    this->flaps = this->player_plane->GetFlaps() > 0;
    this->airbrake = this->player_plane->GetSpoilers() > 0;
    this->player = this->player_plane->object;
    this->weapoint_coords.x = this->current_mission->waypoints[*this->nav_point_id]->spot->position.x;
    this->weapoint_coords.y = this->current_mission->waypoints[*this->nav_point_id]->spot->position.z;

    Vector2D weapoint_direction = {this->weapoint_coords.x - this->player->position.x,
                                   this->weapoint_coords.y - this->player->position.z};
    float weapoint_azimut = (atan2f(weapoint_direction.y, weapoint_direction.x) * 180.0f / (float)M_PI);

    weapoint_azimut -= 360;
    weapoint_azimut += 90;
    if (weapoint_azimut > 360) {
        weapoint_azimut -= 360;
    }
    while (weapoint_azimut < 0) {
        weapoint_azimut += 360;
    }
    this->way_az = weapoint_azimut;

    float distance_to_waypoint = (this->weapoint_coords - Vector2D{this->player->position.x, this->player->position.z}).Length();
    float radar_altitude = (this->player_plane->y - this->player_plane->groundlevel) * 3.28084f;
    std::ostringstream oss;
    
    
    
    std::string txt;
    int weapons_count = 0;
    if (this->player_plane->weaps_load.size() == 0) {
        txt = "NO WEAP";
    } else {
        if (this->player_plane->weaps_load[this->player_plane->selected_weapon] != nullptr) {
            int weapon_id = this->player_plane->weaps_load[this->player_plane->selected_weapon]->objct->wdat->weapon_id;
            for (auto weap : this->player_plane->weaps_load) {
                if (weap != nullptr && weap->objct->wdat->weapon_id == weapon_id) {
                    weapons_count += weap->nb_weap;
                }
            }
            if (this->target != nullptr) {
                uint32_t weap_range =
                    this->player_plane->weaps_load[this->player_plane->selected_weapon]->objct->wdat->effective_range;
                Vector3D dist_to_target = this->target->position - this->player_plane->object->position;
                float distance = dist_to_target.Length();
                if (distance <= weap_range) {
                    this->target_in_range = true;
                }
            } else {
                this->target_in_range = false;
            }
            if (weapon_names.find(static_cast<weapon_ids>(weapon_id)) != weapon_names.end()) {
                txt = weapon_names[static_cast<weapon_ids>(weapon_id)];
            } else {
                txt = "UNKNOWN";
            }
        } else {
            txt = "NO WEAP";
        }
    }
    this->hud_text_tags["NUMW"]=std::to_string(weapons_count) + " "+txt;
    this->hud_text_tags["HUDM"]="HUDM";
    
    if (this->target != nullptr) {
        Vector3D dist_to_target = this->target->position - this->player_plane->position;
        float distance = dist_to_target.Length();

        oss.str("");
        oss << std::setw(3) << std::setfill('0') << std::fixed << std::setprecision(2) << distance / 1000.0f;
        this->hud_text_tags["TARG"]= "R "+oss.str();
        SCMissionActors *targetActor = nullptr;
        for (auto actor : this->current_mission->enemies) {
            if (actor->object == this->target && actor->is_active) {
                targetActor = actor;
                break;
            }
        }
        if (targetActor == nullptr) {
            for (auto actor : this->current_mission->friendlies) {
                if (actor->object == this->target && actor->is_active) {
                    targetActor = actor;
                    break;
                }
            }
        }
        float target_velocity = targetActor != nullptr ? targetActor->plane != nullptr ? targetActor->plane->airspeed : 0.0f : 0.0f;
        oss.str("");
        oss << std::setw(3) << std::setfill('0') << std::fixed << std::setprecision(2) << target_velocity;
        this->hud_text_tags["CLSR"]= "C "+oss.str();
        if (this->target_in_range) {
            this->hud_text_tags["IRNG"]= "IN RNG";
        } else {
            this->hud_text_tags["IRNG"]= "";
        }
    } else {
        this->hud_text_tags["TARG"] = "";
        this->hud_text_tags["CLSR"]="";
        this->hud_text_tags["IRNG"]= "";
        
    }
    oss.str("");
    oss << std::setw(2) << std::fixed << std::setprecision(1) << this->g_load;
    this->hud_text_tags["GFRC"]= oss.str()+"G";
    oss.str("");
    oss << std::setw(2) << std::fixed << std::setprecision(1) << (float)this->player_plane->object->entity->jdyn->MAX_G;
    this->hud_text_tags["MAXG"]= oss.str()+"G";
    oss.str("");
    oss << std::setw(3) << std::fixed << std::setprecision(2) << this->mach;
    std::string speed_buffer = oss.str();
    this->hud_text_tags["MACH"]= oss.str()+"M";
    oss.str("");
    oss.clear();
    oss << std::setw(4) << std::setfill('0') << std::fixed << std::setprecision(1) << distance_to_waypoint / 1000.0f;
    this->hud_text_tags["WAYP"]= "D"+oss.str();
    oss.str("");
    oss.clear();
    oss << std::setw(4) << std::setfill('0') << std::fixed << std::setprecision(1) << radar_altitude / 1000.0f;
    this->hud_text_tags["RALT"]= "R"+oss.str();

    this->hud_text_tags["LNDG"]= this->player_plane->GetWheel() > 0 ? "GEAR" : "";
    this->hud_text_tags["FLAP"]= this->player_plane->GetFlaps() > 0 ? "FLAP" : "";
    this->hud_text_tags["SPDB"]= this->player_plane->GetSpoilers() > 0 ? "BREAK" : "";
    this->hud_text_tags["THRO"]= std::to_string(this->player_plane->GetThrottle());
    this->hud_text_tags["CALA"]= "T";
}

void SCCockpit::RenderHUD() {
    FrameBuffer *hud = this->hud_framebuffer;
    if (hud == nullptr) {
        return;
    }
    hud->fillWithColor(255);
    this->RenderHudHorizonLinesSmall({46, 44}, hud);
    this->RenderAltitude({72, 24}, hud);
    this->RenderSpeed({0, 24}, hud);
    this->RenderHeading({22, 82}, hud);

    std::string txt;
    char speed_buffer[10];
    Point2D g_load_text = {7, 14};
    snprintf(speed_buffer, sizeof(speed_buffer), "%.2f", this->g_load);
    txt = speed_buffer;
    hud->printText(this->font, &g_load_text, (char *)txt.c_str(), 0, 0, (uint32_t)txt.length(), 2, 2);
    Point2D g_limit_text = {7, g_load_text.y + 5};
    snprintf(speed_buffer, sizeof(speed_buffer), "%.2f", this->g_limit);
    txt = speed_buffer;
    hud->printText(this->font, &g_limit_text, (char *)txt.c_str(), 0, 0, (uint32_t)txt.length(), 2, 2);

    Point2D speed_text = {7, 24 + 40};
    snprintf(speed_buffer, sizeof(speed_buffer), "%.3f", this->mach);
    txt = speed_buffer;
    hud->printText(this->font, &speed_text, (char *)txt.c_str(), 0, 0, (uint32_t)txt.length(), 2, 2);
    Point2D throttle_text = {7, speed_text.y + 5};
    txt = std::to_string(this->throttle);
    hud->printText(this->font, &throttle_text, (char *)txt.c_str(), 0, 0, (uint32_t)txt.length(), 2, 2);

    Point2D inrange_text = {7, throttle_text.y + 5};
    Point2D weapons_text = {7 + 16, throttle_text.y + 10};
    Point2D weapons_count_text = {7, weapons_text.y};
    int weapons_count = 0;
    bool in_range = false;
    if (this->player_plane->weaps_load.size() == 0) {
        txt = "NO WEAP";
    } else {
        if (this->player_plane->weaps_load[this->player_plane->selected_weapon] != nullptr) {
            int weapon_id = this->player_plane->weaps_load[this->player_plane->selected_weapon]->objct->wdat->weapon_id;
            for (auto weap : this->player_plane->weaps_load) {
                if (weap != nullptr && weap->objct->wdat->weapon_id == weapon_id) {
                    weapons_count += weap->nb_weap;
                }
            }
            if (this->target != nullptr) {
                uint32_t weap_range =
                    this->player_plane->weaps_load[this->player_plane->selected_weapon]->objct->wdat->effective_range;
                Vector3D dist_to_target = this->target->position - this->player_plane->object->position;
                float distance = dist_to_target.Length();
                if (distance <= weap_range) {
                    this->target_in_range = true;
                }
            } else {
                this->target_in_range = false;
            }
            if (weapon_names.find(static_cast<weapon_ids>(weapon_id)) != weapon_names.end()) {
                txt = weapon_names[static_cast<weapon_ids>(weapon_id)];
            } else {
                txt = "UNKNOWN";
            }
        } else {
            txt = "NO WEAP";
        }
    }
    hud->printText(this->font, &weapons_text, const_cast<char *>(txt.c_str()), 0, 0, (uint32_t)txt.length(), 2, 2);
    Point2D center = {hud->width / 2, hud->height / 2};
    center.x -= this->hud->small_hud->LADD->VECT->SHAP->GetWidth() / 2;
    center.y -= this->hud->small_hud->LADD->VECT->SHAP->GetHeight() / 2;
    center.x += (int)(this->player_plane->ax * 1000.0f);
    center.y -= (int)(this->player_plane->ay * 100.0f);
    this->hud->small_hud->LADD->VECT->SHAP->SetPosition(&center);
    hud->drawShapeWithBox(this->hud->small_hud->LADD->VECT->SHAP, 0, hud_framebuffer->width, 0,
                          hud_framebuffer->height);
    if (weapons_count > 0) {
        txt = std::to_string(weapons_count);
    } else {
        txt = "0";
    }
    hud->printText(this->font, &weapons_count_text, (char *)txt.c_str(), 0, 0, (uint32_t)txt.length(), 2, 2);
    if (this->target_in_range) {
        hud->printText(this->font, &inrange_text, const_cast<char *>("IN RANGE"), 0, 0, 8, 2, 2);
    }
    if (this->target != nullptr) {
        Vector3D dist_to_target = this->target->position - this->player_plane->object->position;
        float distance = dist_to_target.Length();
        std::ostringstream oss;
        oss << std::setw(5) << std::setfill('0') << std::fixed << std::setprecision(1) << distance / 1000.0f;
        std::string s = "R"+oss.str();
        Point2D target_distance_text = {72, inrange_text.y};
        hud->printText(this->font, &target_distance_text, (char *)s.c_str(), 0, 0, (uint32_t)s.length(), 2, 2);
    }
    Point2D gear = {75, 9};
    Point2D flaps = {75, gear.y + 5};
    Point2D break_text = {75, flaps.y + 5};
    if (this->gear) {
        hud->printText(this->font, &gear, const_cast<char *>("GEARS"), 0, 0, 5, 2, 2);
    }

    if (this->flaps) {
        hud->printText(this->font, &flaps, const_cast<char *>("FLAPS"), 0, 0, 5, 2, 2);
    }

    if (this->airbrake) {
        hud->printText(this->font, &break_text, const_cast<char *>("BREAKS"), 0, 0, 6, 2, 2);
    }
    Point2D pcenter = {hud->width / 2, hud->height / 2};
    hud->plot_pixel(pcenter.x, pcenter.y, 223);

    // Reticle de gun: dessiné dans le même framebuffer que le HUD (important pour cockpit 3D/VR).
    if (this->player_plane->weaps_load.size() > 0 &&
        this->player_plane->weaps_load[this->player_plane->selected_weapon] != nullptr) {
        int weapon_id = this->player_plane->weaps_load[this->player_plane->selected_weapon]->objct->wdat->weapon_id;
        if (weapon_id == ID_20MM) {
            this->RenderTargetingReticle(hud);
        }
        if (weapon_id == ID_MK20 || weapon_id == ID_MK82 || weapon_id == ID_DURANDAL) {
            this->RenderBombSight(hud);
        }
    }
    float distance = 0.0f;
    Vector2D planepos = {this->player_plane->position.x, this->player_plane->position.z};
    Vector2D dist_to_waypoint = this->weapoint_coords - planepos;
    distance = dist_to_waypoint.Length();
    std::ostringstream oss;
    oss << std::setw(5) << std::setfill('0') << std::fixed << std::setprecision(1) << distance / 1000.0f;
    std::string s = "D"+oss.str();
    Point2D target_distance_text = {72, 24+45};
    hud->printText(this->font, &target_distance_text, (char *)s.c_str(), 0, 0, (uint32_t)s.length(), 2, 2);
}
void SCCockpit::RenderAlti(Point2D pmfd_left = {177, 179}, FrameBuffer *fb = nullptr) {
    if (!fb) {
        fb = VGA.getFrameBuffer();
    }
    if (this->cockpit->MONI.INST.ALTI.ARTS.GetNumImages() == 0) {
        return;
    }
    if (this->cockpit->MONI.INST.ALTI.x != 0 && this->cockpit->MONI.INST.ALTI.y != 0) {
        if (pmfd_left.x != 0 && pmfd_left.y != 0) {
            pmfd_left.x = this->cockpit->MONI.INST.ALTI.x;
            pmfd_left.y = this->cockpit->MONI.INST.ALTI.y;
        }
    }
    RLEShape *shape = this->cockpit->MONI.INST.ALTI.ARTS.GetShape(0);
    Point2D raws_size = {shape->GetWidth(), shape->GetHeight()};
    Point2D bottom_right = {pmfd_left.x + raws_size.x, pmfd_left.y + raws_size.y};
    shape->SetPosition(&pmfd_left);
    fb->drawShape(shape);
    // Calculate altitude in feet
    float altiInFeet = this->altitude * 3.28084f;

    // Calculate angles for each needle
    // 1000s needle (full circle = 10,000 feet)
    float thousandsAngle = (altiInFeet / 10000.0f) * 2.0f * (float)M_PI;
    // 100s needle (full circle = 1,000 feet)
    float hundredsAngle = (fmodf(altiInFeet, 1000.0f) / 1000.0f) * 2.0f * (float)M_PI;

    // Calculate center position of the altimeter
    Point2D center = {pmfd_left.x + raws_size.x / 2, pmfd_left.y + raws_size.y / 2};

    // Calculate needle lengths
    int thousandsLength = raws_size.x / 2 - 5;
    int hundredsLength = raws_size.x / 2 - 6;

    // Calculate needle endpoints
    Point2D thousandsEnd = {center.x + (int)(thousandsLength * sinf(thousandsAngle)),
                            center.y - (int)(thousandsLength * cosf(thousandsAngle))};

    Point2D hundredsEnd = {center.x + (int)(hundredsLength * sinf(hundredsAngle)),
                           center.y - (int)(hundredsLength * cosf(hundredsAngle))};

    // Draw needles
    fb->line(center.x, center.y, thousandsEnd.x, thousandsEnd.y, 223); // Thousands needle
    fb->line(center.x, center.y, hundredsEnd.x, hundredsEnd.y, 90);    // Hundreds needle (different color)
}
void SCCockpit::RenderSpeedOmetter(Point2D pmfd_left = {125, 166}, FrameBuffer *fb = nullptr) {
    if (!fb) {
        fb = VGA.getFrameBuffer();
    }
    if (this->cockpit->MONI.INST.AIRS.ARTS.GetNumImages() == 0) {
        return;
    }
    if (this->cockpit->MONI.INST.AIRS.x != 0 && this->cockpit->MONI.INST.AIRS.y != 0) {
        if (pmfd_left.x != 0 && pmfd_left.y != 0) {
            pmfd_left.x = this->cockpit->MONI.INST.AIRS.x;
            pmfd_left.y = this->cockpit->MONI.INST.AIRS.y;
        }
    }
    RLEShape *shape = this->cockpit->MONI.INST.AIRS.ARTS.GetShape(0);
    Point2D raws_size = {shape->GetWidth(), shape->GetHeight()};
    Point2D bottom_right = {pmfd_left.x + raws_size.x, pmfd_left.y + raws_size.y};
    shape->SetPosition(&pmfd_left);
    fb->drawShape(shape);
    // Calculate speed in knots
    float speedInKnots = this->speed;
    // Calculate angle for the needle
    float speedAngle = (speedInKnots / 1500.0f) * 2.0f * (float)M_PI; // Assuming max speed is 600 knots
    // Calculate center position of the speedometer
    Point2D center = {pmfd_left.x + raws_size.x / 2, pmfd_left.y + raws_size.y / 2};
    // Calculate needle length
    int needleLength = raws_size.x / 2 - 5; // Adjusted for the size of the speedometer
    // Calculate needle endpoint
    Point2D needleEnd = {center.x + (int)(needleLength * sinf(speedAngle)),
                         center.y - (int)(needleLength * cosf(speedAngle))};
    // Draw the needle
    fb->line(center.x, center.y, needleEnd.x, needleEnd.y, 223); // Draw the speed needle
}

bool SCCockpit::RenderCommMessages(Point2D pmfd_text, FrameBuffer *fb) {
    bool hasMessage = false;
    if (fb == nullptr) {
        fb = VGA.getFrameBuffer();
    }
    if (this->current_mission->radio_messages.size() > 0) {
        hasMessage = true;
        if (this->radio_mission_timer == 0) {
            if (this->current_mission->radio_messages[0]->sound != nullptr) {
                Mixer.playSoundVoc(this->current_mission->radio_messages[0]->sound->data,
                                   this->current_mission->radio_messages[0]->sound->size);
            }
            this->radio_mission_timer = 400;
        }

        RLEShape *background_message = this->cockpit->PLAQ.shapes.GetShape(2);
        Point2D b_message_pos = {pmfd_text.x, pmfd_text.y - background_message->GetHeight() - 1};
        background_message->SetPosition(&b_message_pos);
        Point2D radio_text = {4, b_message_pos.y + 9};
        fb->drawShape(background_message);
        RSFont *fnt = this->cockpit->PLAQ.fonts[1].font;
        std::string radio_message = this->current_mission->radio_messages[0]->message;
        std::transform(radio_message.begin(), radio_message.end(), radio_message.begin(), ::toupper);
        fb->printText(fnt, &radio_text, (char *)radio_message.c_str(), 0, 0, (uint32_t)radio_message.size(), 2, 2);
        if (this->radio_mission_timer > 0) {
            this->radio_mission_timer--;
        }
        if (this->radio_mission_timer <= 0) {
            this->current_mission->radio_messages.erase(this->current_mission->radio_messages.begin());
            this->radio_mission_timer = 0;
        }
    }
    return hasMessage;
}

void SCCockpit::RenderMFDSCamera(Point2D pmfd_left, FrameBuffer *fb) {
    SCRenderer &renderer = SCRenderer::getInstance();
    
    if (!fb) {
        fb = VGA.getFrameBuffer();
    }


    // Récupérer les dimensions de la texture
    int texWidth  = 128;
    int texHeight = 128;

    int mdfs_height = this->cockpit->MONI.SHAP.GetHeight()-20;
    int mdfs_width = this->cockpit->MONI.SHAP.GetWidth()-8;
    if (texWidth <= 0 || texHeight <= 0) {
        return;
    }

    // Lire les pixels RGBA depuis la texture OpenGL
    std::vector<uint8_t> rgbaPixels(texWidth * texHeight * 4);
    
    glBindTexture(GL_TEXTURE_2D, renderer.texture);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgbaPixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    // Créer un buffer RGBA redimensionné (nearest-neighbor)
    std::vector<uint8_t> resizedRGBA(mdfs_width * mdfs_height * 4);
    for (int dy = 0; dy < mdfs_height; dy++) {
        for (int dx = 0; dx < mdfs_width; dx++) {
            int sx = (int)(dx * texWidth  / (float)mdfs_width);
            int sy = (int)(dy * texHeight / (float)mdfs_height);
            sx = (std::min)(sx, texWidth  - 1);
            sy = (std::min)(sy, texHeight - 1);

            // Inverser l'axe Y : OpenGL a l'origine en bas, le FrameBuffer en haut
            int sy_flipped = (texHeight - 1) - sy;

            int srcIdx = (sy_flipped * texWidth + sx) * 4;
            int dstIdx = (dy * mdfs_width + dx) * 4;
            resizedRGBA[dstIdx + 0] = rgbaPixels[srcIdx + 0];
            resizedRGBA[dstIdx + 1] = rgbaPixels[srcIdx + 1];
            resizedRGBA[dstIdx + 2] = rgbaPixels[srcIdx + 2];
            resizedRGBA[dstIdx + 3] = rgbaPixels[srcIdx + 3];
        }
    }

    // Construire la LUT si la palette a changé
    if (this->palette_lut_dirty) {
        this->BuildPaletteLUT();
    }

    // Convertir le buffer RGBA redimensionné en buffer indexé palette 8 bits via LUT
    std::vector<uint8_t> indexedBuffer(mdfs_width * mdfs_height);

    for (int i = 0; i < mdfs_width * mdfs_height; i++) {
        uint8_t r = resizedRGBA[i * 4 + 0];
        uint8_t g = resizedRGBA[i * 4 + 1];
        uint8_t b = resizedRGBA[i * 4 + 2];

        // Quantifier sur 5 bits et former la clé
        uint32_t key = (((uint32_t)r >> 3) << 10) | (((uint32_t)g >> 3) << 5) | ((uint32_t)b >> 3);
        indexedBuffer[i] = this->palette_lut[key];
    }

    // Blitter le buffer indexé dans le FrameBuffer à la position pmfd_left
    fb->blit(indexedBuffer.data(), pmfd_left.x+13, pmfd_left.y+11, mdfs_width, mdfs_height);
    this->cockpit->MONI.SHAP.SetPosition(&pmfd_left);
    fb->drawShape(&this->cockpit->MONI.SHAP);
    Point2D center = {pmfd_left.x + mdfs_width / 2, pmfd_left.y + mdfs_height / 2};
    this->cockpit->MONI.MFDS.GCAM.ARTS.GetShape(0)->SetPosition(&center);
    fb->drawShape(this->cockpit->MONI.MFDS.GCAM.ARTS.GetShape(0));
}
void SCCockpit::BuildPaletteLUT() {
    VGAPalette &pal = this->palette;
    this->palette_lut.clear();
    // Précalcule pour chaque combinaison r,g,b réduite (5 bits par composante = 32768 entrées)
    for (int r = 0; r < 256; r += 8) {
        for (int g = 0; g < 256; g += 8) {
            for (int b = 0; b < 256; b += 8) {
                int bestIndex = 128; // Index de départ dans la plage verte
                int bestDist  = INT_MAX;

                // Convertir RGB en luminosité
                float luminance = 0.299f * r + 0.587f * g + 0.114f * b;

                // Chercher uniquement dans la plage [128-159] (dégradé de vert)
                for (int c = 128; c <= 159; c++) {
                    int dr = r - (int)pal.colors[c].r;
                    int dg = g - (int)pal.colors[c].g;
                    int db = b - (int)pal.colors[c].b;
                    int dist = dr * dr + dg * dg + db * db;
                    if (dist < bestDist) {
                        bestDist  = dist;
                        bestIndex = c;
                        if (dist == 0) break;
                    }
                }
                // Clé = r5g5b5 packed sur 15 bits
                uint32_t key = ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
                this->palette_lut[key] = (uint8_t)bestIndex;
            }
        }
    }
    this->palette_lut_dirty = false;
}
void SCCockpit::SetCommActorTarget(int target) {
    if (target == 0) {
        this->comm_actor = nullptr;
    } else {
        int cpt = 1;
        for (auto ai : this->current_mission->friendlies) {
            if (ai->actor_name != "PLAYER" && ai->is_active) {
                if (cpt == target) {
                    this->comm_actor = ai;
                    this->comm_target = cpt;
                    break;
                }
                cpt++;
            }
        }
    }
}
void SCCockpit::printTTAG(Point2D pos, HUD_POS &tag, std::string name, FrameBuffer *fb, RSFont *font) {
    Point2D tag_pos = {pos.x+tag.x, pos.y+tag.y+font->GetShapeForChar('0')->GetHeight()};
    if (tag.z == 0) {
        return;
    }
    std::string value = this->hud_text_tags[name];
    fb->printText(font, &tag_pos, (char *)value.c_str(), 0, 0, (uint32_t)value.size(), 2, 2);
}
void SCCockpit::RenderTextTags(Point2D position, FrameBuffer *fb) {
    if (!fb) {
        fb = VGA.getFrameBuffer();
    }
    Point2D alti{0,0};
    switch (this->face) {
        case CockpitFace::CP_FRONT:
            printTTAG(position, this->hud->small_hud->TTAG->CLSR, "CLSR", fb, this->font);
            printTTAG(position, this->hud->small_hud->TTAG->TARG, "TARG", fb, this->font);
            printTTAG(position, this->hud->small_hud->TTAG->NUMW, "NUMW", fb, this->font);
            printTTAG(position, this->hud->small_hud->TTAG->HUDM, "HUDM", fb, this->font);
            printTTAG(position, this->hud->small_hud->TTAG->IRNG, "IRNG", fb, this->font);
            printTTAG(position, this->hud->small_hud->TTAG->GFRC, "GFRC", fb, this->font);
            printTTAG(position, this->hud->small_hud->TTAG->MAXG, "MAXG", fb, this->font);
            printTTAG(position, this->hud->small_hud->TTAG->MACH, "MACH", fb, this->font);
            printTTAG(position, this->hud->small_hud->TTAG->WAYP, "WAYP", fb, this->font);
            printTTAG(position, this->hud->small_hud->TTAG->RALT, "RALT", fb, this->font);
            printTTAG(position, this->hud->small_hud->TTAG->LNDG, "LNDG", fb, this->font);
            printTTAG(position, this->hud->small_hud->TTAG->FLAP, "FLAP", fb, this->font);
            printTTAG(position, this->hud->small_hud->TTAG->SPDB, "SPDB", fb, this->font);
            printTTAG(position, this->hud->small_hud->TTAG->THRO, "THRO", fb, this->font);
            printTTAG(position, this->hud->small_hud->TTAG->CALA, "CALA", fb, this->font);
            alti = {position.x+this->hud->small_hud->ALTI->x, position.y+this->hud->small_hud->ALTI->y};
            
            this->RenderAltiBandRoll(alti, fb, this->font, this->hud->small_hud->ALTI);
            alti = {position.x+this->hud->small_hud->ASPD->x, position.y+this->hud->small_hud->ASPD->y};
            
            this->RenderSpeedBandRoll(alti, fb, this->font, this->hud->small_hud->ASPD);
            alti = {position.x+this->hud->small_hud->HEAD->x, position.y+this->hud->small_hud->HEAD->y};
            this->RenderHeadingCompas(alti, fb, this->font,this->hud->small_hud->HEAD);
            this->RenderPitchLadder(position, {90, 80}, fb, this->hud->small_hud->LADD, this->font);

        break;
        case CockpitFace::CP_BIG:
            printTTAG(position, this->hud->large_hud->TTAG->CLSR, "CLSR", fb, this->big_font);
            printTTAG(position, this->hud->large_hud->TTAG->TARG, "TARG", fb, this->big_font);
            printTTAG(position, this->hud->large_hud->TTAG->NUMW, "NUMW", fb, this->big_font);
            printTTAG(position, this->hud->large_hud->TTAG->HUDM, "HUDM", fb, this->big_font);
            printTTAG(position, this->hud->large_hud->TTAG->IRNG, "IRNG", fb, this->big_font);
            printTTAG(position, this->hud->large_hud->TTAG->GFRC, "GFRC", fb, this->big_font);
            printTTAG(position, this->hud->large_hud->TTAG->MAXG, "MAXG", fb, this->big_font);
            printTTAG(position, this->hud->large_hud->TTAG->MACH, "MACH", fb, this->big_font);
            printTTAG(position, this->hud->large_hud->TTAG->WAYP, "WAYP", fb, this->big_font);
            printTTAG(position, this->hud->large_hud->TTAG->RALT, "RALT", fb, this->big_font);
            printTTAG(position, this->hud->large_hud->TTAG->LNDG, "LNDG", fb, this->big_font);
            printTTAG(position, this->hud->large_hud->TTAG->FLAP, "FLAP", fb, this->big_font);
            printTTAG(position, this->hud->large_hud->TTAG->SPDB, "SPDB", fb, this->big_font);
            printTTAG(position, this->hud->large_hud->TTAG->THRO, "THRO", fb, this->big_font);
            printTTAG(position, this->hud->large_hud->TTAG->CALA, "CALA", fb, this->big_font);
            alti = {position.x+this->hud->large_hud->ALTI->x, position.y+this->hud->large_hud->ALTI->y};
            
            this->RenderAltiBandRoll(alti, fb, this->big_font, this->hud->large_hud->ALTI);
            alti = {position.x+this->hud->large_hud->ASPD->x, position.y+this->hud->large_hud->ASPD->y};
            
            this->RenderSpeedBandRoll(alti, fb, this->big_font, this->hud->large_hud->ASPD);
            alti = {position.x+this->hud->large_hud->HEAD->x, position.y+this->hud->large_hud->HEAD->y};
            this->RenderHeadingCompas(alti, fb, this->big_font,this->hud->large_hud->HEAD);
            this->RenderPitchLadder(position, {90, 80}, fb, this->hud->large_hud->LADD, this->big_font);


        break;
        default:
        break;
    }
    
}

void SCCockpit::RenderAltiBandRoll(Point2D alti_top_left, FrameBuffer *fb, RSFont *sfont, CHUD_SHAPE *alti_band) {
    if (!fb) fb = VGA.getFrameBuffer();

    float alti_in_feet = this->altitude * 3.28084f;
    Point2D alti_size = {alti_band->width, alti_band->height};
    Point2D bottom_right = {alti_top_left.x + alti_size.x, alti_top_left.y + alti_size.y};
    int center_y = alti_top_left.y + alti_size.y / 2;

    // Plage d'altitude visible (en pieds) depuis le haut et bas de la fenêtre
    float half_view_feet = ((float)alti_size.y / 2.0f) * (500.0f / alti_band->step);

    // Première graduation en dessous de la zone visible, arrondie à 500 pieds
    int first_grad = (int)floorf((alti_in_feet - half_view_feet) / 500.0f) * 500;

    for (int grad_feet = first_grad; grad_feet <= (int)(alti_in_feet + half_view_feet) + 500; grad_feet += 500) {
        // Conversion altitude → Y pixel
        float y = center_y - (grad_feet - alti_in_feet) / 500.0f * alti_band->step;
        int iy = (int)y;

        if (iy < alti_top_left.y || iy > bottom_right.y+alti_band->step) continue;

        // Tick mark
        Point2D tick = {alti_top_left.x, iy-alti_band->SHAP->GetHeight()};
        alti_band->SHAP->SetPosition(&tick);
        fb->drawShapeWithBox(alti_band->SHAP, alti_top_left.x, bottom_right.x,
                             alti_top_left.y-alti_band->step, bottom_right.y-alti_band->step);
        // Label : valeur en milliers de pieds, 1 décimale (ex: "-02.5")
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << (grad_feet / 1000.0f);
        std::string txt = oss.str();
        Point2D label = {alti_top_left.x + 6, iy};
        if (iy >= alti_top_left.y && iy <= bottom_right.y) {
            fb->printText(sfont, &label, (char *)txt.c_str(), 0, 0, (uint32_t)txt.length(), 2, 2);
        }
    }
    // Flèche centrale (altitude courante)
    Point2D alti_arrow = {alti_top_left.x - 4, center_y};
    fb->line(alti_arrow.x, alti_arrow.y, alti_arrow.x +3, alti_arrow.y, 223);
}

void SCCockpit::RenderSpeedBandRoll(Point2D speed_top_left, FrameBuffer *fb, RSFont *sfont, CHUD_SHAPE *speed_band) {
    if (!fb) fb = VGA.getFrameBuffer();

    float speed_in_knots = this->speed;
    Point2D speed_size = {speed_band->width, speed_band->height};
    Point2D bottom_right = {speed_top_left.x + speed_size.x, speed_top_left.y + speed_size.y};
    int center_y = speed_top_left.y + speed_size.y / 2;

    float half_view_knots = ((float)speed_size.y / 2.0f) * (50.0f / speed_band->step);

    int first_grad = (int)floorf((speed_in_knots - half_view_knots) / 50.0f) * 50;
    if (first_grad < 0) first_grad = 0;
    int txt_width = sfont->GetShapeForChar('0')->GetWidth() * 2 + 4;
    for (int grad_knots = first_grad; grad_knots <= (int)(speed_in_knots + half_view_knots) + 50; grad_knots += 50) {
        float y = center_y - (grad_knots - speed_in_knots) / 50.0f * speed_band->step;
        int iy = (int)y;

        if (iy < speed_top_left.y || iy > bottom_right.y + speed_band->step) continue;

        Point2D tick = {speed_top_left.x+txt_width, iy - speed_band->SHAP->GetHeight()};
        speed_band->SHAP->SetPosition(&tick);
        fb->drawShapeWithBox(speed_band->SHAP, speed_top_left.x, bottom_right.x,
                             speed_top_left.y - speed_band->step, bottom_right.y - speed_band->step);

        std::string txt = std::to_string(grad_knots/10);
        Point2D label = {speed_top_left.x, iy};
        if (iy >= speed_top_left.y && iy <= bottom_right.y) {
            fb->printText(sfont, &label, (char *)txt.c_str(), 0, 0, (uint32_t)txt.length(), 2, 2);
        }
    }

    Point2D speed_arrow = {speed_top_left.x+txt_width+2, center_y};
    fb->line(speed_arrow.x, speed_arrow.y, speed_arrow.x + (txt_width/2), speed_arrow.y, 223);
}
void SCCockpit::RenderHeadingCompas(Point2D heading_top_left, FrameBuffer *fb, RSFont *sfont, CHUD_SHAPE *heading_band) {
    if (!fb) fb = VGA.getFrameBuffer();

    float current_heading = this->heading; // en degrés [0, 360[
    Point2D heading_size = {heading_band->width, heading_band->height};
    Point2D bottom_right = {heading_top_left.x + heading_size.x, heading_top_left.y + heading_size.y};
    int center_x = heading_top_left.x + heading_size.x / 2;

    // Plage de degrés visible depuis le centre jusqu'au bord
    float half_view_deg = ((float)heading_size.x / 2.0f) * (10.0f / heading_band->step);
    int txt_height = sfont->GetShapeForChar('0')->GetHeight();
    int first_grad = (int)ceilf((current_heading + half_view_deg) / 10.0f) * 10;

    for (int grad_deg = first_grad; grad_deg >= (int)(current_heading - half_view_deg) - 10; grad_deg -= 10) {
        int display_deg = ((grad_deg % 360) + 360) % 360;

        float x = center_x + (grad_deg - current_heading) / 10.0f * heading_band->step;
        int ix = (int)x;

        if (ix < heading_top_left.x || ix > bottom_right.x + heading_band->step) continue;

        Point2D tick = {ix - heading_band->SHAP->GetWidth() / 2, heading_top_left.y};
        heading_band->SHAP->SetPosition(&tick);
        fb->drawShapeWithBox(heading_band->SHAP, heading_top_left.x - heading_band->step, bottom_right.x,
                             heading_top_left.y, bottom_right.y);

        std::string txt = std::to_string(display_deg/10);
        
        Point2D label = {ix - (int)(txt.length()) * 2, heading_top_left.y + heading_band->SHAP->GetHeight() + txt_height};
        if (ix >= heading_top_left.x && ix <= bottom_right.x) {
            fb->printText(sfont, &label, (char *)txt.c_str(), 0, 0, (uint32_t)txt.length(), 2, 2);
        }
    }

    // Flèche centrale (cap courant)
    Point2D heading_arrow = {center_x, heading_top_left.y - 3};
    fb->line(heading_arrow.x, heading_arrow.y, heading_arrow.x, heading_arrow.y + 6, 223);

    // Différence angulaire normalisée [-180, 180]
    float way_diff = fmodf(this->way_az - current_heading + 540.0f, 360.0f) - 180.0f;
    float way_x = center_x + way_diff / 10.0f * heading_band->step;
    int iway_x = (int)way_x;

    // Dessiner l'indicateur seulement s'il est dans la zone visible
    if (iway_x >= heading_top_left.x && iway_x <= bottom_right.x) {
        // ex: flèche différente de SHP2 ou même SHP2 repositionné
        Point2D way_arrow = {iway_x+(heading_band->SHP2->GetWidth() / 2), heading_top_left.y - heading_band->SHP2->GetHeight()};
        heading_band->SHP2->SetPosition(&way_arrow);
        fb->drawShape(heading_band->SHP2);
    }
}
void SCCockpit::RenderPitchLadder(Point2D center, Point2D clip_size, FrameBuffer *fb, SLADD *ladd, RSFont *ft) {
    if (!fb)   fb = VGA.getFrameBuffer();
    if (!ladd) return;

    const int bx1 = center.x - clip_size.x / 2;
    const int bx2 = center.x + clip_size.x / 2;
    const int by1 = center.y - clip_size.y / 2;
    const int by2 = center.y + clip_size.y / 2;

    const float rollRad   = this->roll * (float)M_PI / 180.0f;
    const float pixPerDeg = (float)ladd->ladd_height / 5.0f;
    const int   halfGap   = ladd->ladd_space_in_line / 2;
    const int   color     = 223;

    std::string txt;

    for (int angle = 90; angle >= -90; angle -= 5) {
        const int lineY = center.y + (int)((this->pitch - (float)angle) * pixPerDeg);

        if (angle == 0) {
            // Horizon line: solid, slightly wider than the other bars
            Point2D hStart = { center.x - ladd->ladd_half_width - 5, lineY };
            Point2D hEnd   = { center.x + ladd->ladd_half_width + 5, lineY };
            hStart = hStart.rotateAroundPoint(center, rollRad);
            hEnd   = hEnd.rotateAroundPoint(center, rollRad);
            fb->lineWithBox(hStart.x, hStart.y, hEnd.x, hEnd.y, color, bx1, bx2, by1, by2);
            continue;
        }

        // Left and right bar segments (pre-rotation)
        Point2D lStart = { center.x - ladd->ladd_half_width-halfGap, lineY };
        Point2D lEnd   = { center.x - halfGap,               lineY };
        Point2D rStart = { center.x + halfGap,               lineY };
        Point2D rEnd   = { center.x + ladd->ladd_half_width+halfGap, lineY };

        // Tick endpoints (perpendicular, pointing toward horizon)
        const int tickDir = (angle > 0) ? ladd->ladd_tick_height : -ladd->ladd_tick_height;
        Point2D ltTick = { lStart.x+1, lineY + tickDir };
        Point2D rtTick = { rEnd.x,   lineY + tickDir };

        // Apply roll rotation to all points
        lStart = lStart.rotateAroundPoint(center, rollRad);
        lEnd   = lEnd.rotateAroundPoint(center, rollRad);
        rStart = rStart.rotateAroundPoint(center, rollRad);
        rEnd   = rEnd.rotateAroundPoint(center, rollRad);
        ltTick = ltTick.rotateAroundPoint(center, rollRad);
        rtTick = rtTick.rotateAroundPoint(center, rollRad);

        // Below horizon → dashed lines
        const int skip = (angle < 0) ? 2 : 1;
        fb->lineWithBoxWithSkip(lStart.x, lStart.y, lEnd.x, lEnd.y, color, bx1, bx2, by1, by2, skip);
        fb->lineWithBoxWithSkip(rStart.x, rStart.y, rEnd.x, rEnd.y, color, bx1, bx2, by1, by2, skip);

        // Tick marks at outer ends
        if (ladd->ladd_tick_height != 0) {
            fb->lineWithBox(lStart.x+1, lStart.y, ltTick.x, ltTick.y, color, bx1, bx2, by1, by2);
            fb->lineWithBox(rEnd.x,   rEnd.y,   rtTick.x, rtTick.y, color, bx1, bx2, by1, by2);
        }

        
        txt = std::to_string(std::abs(angle));
        if (lStart.x > bx1 && lStart.x < bx2 && lStart.y > by1 && lStart.y < by2) {
            Point2D p = lStart;
            p.x -= ladd->ladd_text_spacer;
            p.y -= 2;
            fb->printText(ft, &p, (char *)txt.c_str(), 0, 0, (uint32_t)txt.length(), 2, 2);
        }
        if (rEnd.x > bx1 && rEnd.x < bx2 && rEnd.y > by1 && rEnd.y < by2) {
            Point2D p = rEnd;
            p.x += ladd->ladd_text_spacer;
            p.y -= 2;
            fb->printText(ft, &p, (char *)txt.c_str(), 0, 0, (uint32_t)txt.length(), 2, 2);
        }
        
    }
}