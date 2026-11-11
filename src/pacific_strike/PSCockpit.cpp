#include "PSCockpit.h"
#include "PSEntity.h"

// Palette et fichier de cockpit, comme SCCockpit::init ; Pacific Strike n'a ni EJECT.PAK ni HUD.IFF.
void PSCockpit::init() {
    RSPalette palette;
    TreEntry *entries = (TreEntry *)Assets.GetEntryByName("..\\..\\DATA\\PALETTE\\PALETTE.IFF");
    if (entries != nullptr) {
        palette.initFromFileRam(entries->data, entries->size);
    }
    this->palette = *palette.GetColorPalette();
    this->cockpit = nullptr;
    this->ps_cockpit = nullptr;
    if (this->player_plane == nullptr || this->player_plane->object->entity == nullptr) {
        return;
    }
    std::string name = this->player_plane->object->entity->cockpit_name;
    std::transform(name.begin(), name.end(), name.begin(), ::toupper);
    TreEntry *cockpit_def = Assets.GetEntryByName("..\\..\\DATA\\OBJECTS\\" + name + ".IFF");
    if (cockpit_def == nullptr) {
        printf("PSCockpit: fichier de cockpit %s introuvable\n", name.c_str());
        return;
    }
    this->ps_cockpit = new PSCockpitDef();
    this->ps_cockpit->InitFromRam(cockpit_def->data, cockpit_def->size);
    this->cockpit = this->ps_cockpit;
    this->debug_framebuffer = new FrameBuffer(320, 200);
    printf("PSCockpit: %s charge, %d images ARTP, %zu instruments\n", name.c_str(), (int)this->ps_cockpit->ARTP.GetNumImages(), this->ps_cockpit->instruments.size());
}

PSCockpit::InstrumentView PSCockpit::selectView(PSCockpitInstrument *instrument) {
    InstrumentView view;
    if (this->face == CockpitFace::CP_BIG) {
        if (!instrument->has_second_box) {
            return view;
        }
        view.active = true;
        view.view = 1;
        view.left = instrument->second_left;
        view.top = instrument->second_top;
        view.right = instrument->second_right;
        view.bottom = instrument->second_bottom;
        view.center_x = instrument->second_point_x;
        view.center_y = instrument->second_point_y;
        return view;
    }
    view.active = true;
    view.view = 0;
    view.left = instrument->left;
    view.top = instrument->top;
    view.right = instrument->right;
    view.bottom = instrument->bottom;
    view.center_x = instrument->point_x;
    view.center_y = instrument->point_y;
    return view;
}

// sub_18A3E / sub_18BFD : aiguille depuis le centre, 0° vers le haut, sens des aiguilles d'une montre.
void PSCockpit::drawNeedle(FrameBuffer *fb, const InstrumentView &view, float angle_deg, int length, uint8_t color) {
    float angle = angle_deg * (float)M_PI / 180.0f;
    int end_x = view.center_x + (int)((float)length * sinf(angle));
    int end_y = view.center_y - (int)((float)length * cosf(angle));
    fb->line(view.center_x, view.center_y, end_x, end_y, color);
}

void PSCockpit::drawImage(FrameBuffer *fb, PSCockpitInstrument *instrument, size_t image, int x, int y) {
    if (image >= instrument->SHAP.GetNumImages()) {
        return;
    }
    RLEShape *shape = instrument->SHAP.GetShape(image);
    Point2D position;
    position.x = x;
    position.y = y;
    shape->SetPosition(&position);
    fb->drawShape(shape);
}

// Altimeter_UpdateValue_18643 et Altimeter_Draw_18746 : altitude en pieds ; chiffres des dizaines de milliers et des
// milliers (images 2 à 11 en vue 0, 12 à 21 en vue 1) ; grande aiguille = altitude modulo 100 (3,6° par pied),
// petite aiguille = altitude modulo 1000 ÷ 10 (3,6°).
void PSCockpit::RenderAltimeter(PSCockpitInstrument *instrument, FrameBuffer *fb) {
    InstrumentView view = this->selectView(instrument);
    if (!view.active) {
        return;
    }
    int altitude = (int)floorf(this->player_plane->y * this->feet_per_meter * this->altimeter_scale);
    int ten_thousands = (altitude % 100000) / 10000;
    int thousands = (altitude % 10000) / 1000;
    if (view.view == 0) {
        this->drawImage(fb, instrument, (size_t)(ten_thousands + 2), view.center_x - 3, view.center_y + 2);
        this->drawImage(fb, instrument, (size_t)(thousands + 2), view.center_x + 1, view.center_y + 2);
    } else {
        this->drawImage(fb, instrument, (size_t)(ten_thousands + 12), view.center_x - 4, view.center_y + 2);
        this->drawImage(fb, instrument, (size_t)(thousands + 12), view.center_x + 1, view.center_y + 2);
    }
    int radius = view.center_x - view.left;
    float long_angle = (float)(altitude % 100) * 3.6f;
    float short_angle = (float)((altitude % 1000) / 10) * 3.6f;
    this->drawNeedle(fb, view, long_angle, radius * 3 / 4, 29);
    this->drawNeedle(fb, view, short_angle, radius / 2, 29);
}

// sub_18AF1 et sub_18B8A : vitesse en nœuds × 0,71875 °, au plus 359° ; aiguille de 3/4 du rayon.
void PSCockpit::RenderAirspeed(PSCockpitInstrument *instrument, FrameBuffer *fb) {
    InstrumentView view = this->selectView(instrument);
    if (!view.active) {
        return;
    }
    float knots = this->player_plane->worldVelocity().Length() * this->knots_per_ms;
    float angle = knots * (184.0f / 256.0f);
    if (angle > 359.0f) {
        angle = 359.0f;
    }
    int radius = view.center_x - view.left;
    this->drawNeedle(fb, view, angle, radius * 3 / 4, 29);
}

// sub_19859 et sub_198B4 : cran de la manette (bloc de commandes +1Eh, 0 à 10) × 30° ; aiguille de 3/4 du rayon.
void PSCockpit::RenderTachometer(PSCockpitInstrument *instrument, FrameBuffer *fb) {
    InstrumentView view = this->selectView(instrument);
    if (!view.active) {
        return;
    }
    int notch = this->player_plane->GetThrottle() / 10;
    int radius = view.center_x - view.left;
    this->drawNeedle(fb, view, (float)notch * 30.0f, radius * 3 / 4, 29);
}

// sub_18DA9 et sub_18E6D : vitesse verticale en pieds par seconde × 0,449°, moins 90° (zéro à 9 heures) ; en montée
// l'aiguille s'arrête à 90° (3 heures, 400 ft/s), en descente aussi à 90° par le bas.
void PSCockpit::RenderVerticalSpeed(PSCockpitInstrument *instrument, FrameBuffer *fb) {
    InstrumentView view = this->selectView(instrument);
    if (!view.active) {
        return;
    }
    float climb = this->player_plane->worldVelocity().y * this->feet_per_meter;
    int angle = (int)(climb * (115.0f / 256.0f)) - 90;
    if (angle < 0) {
        angle += 360;
    }
    if (climb < 0.0f) {
        if (angle < 90) {
            angle = 90;
        }
    } else if (angle > 90 && angle < 270) {
        angle = 90;
    }
    int radius = view.center_x - view.left;
    this->drawNeedle(fb, view, (float)angle, radius * 3 / 4, 29);
}

// sub_19AB1 et sub_19B7B : carburant restant ÷ capacité × 360° ; aiguille de 3/4 du rayon.
void PSCockpit::RenderFuel(PSCockpitInstrument *instrument, FrameBuffer *fb) {
    InstrumentView view = this->selectView(instrument);
    if (!view.active) {
        return;
    }
    JDYN *jdyn = this->player_plane->object->entity->jdyn;
    if (jdyn == nullptr || jdyn->fuel_capacity <= 0) {
        return;
    }
    float ratio = (float)this->player_plane->fuel / (float)jdyn->fuel_capacity;
    int radius = view.center_x - view.left;
    this->drawNeedle(fb, view, ratio * 360.0f, radius * 3 / 4, 29);
}

// Cap de l'avion en degrés, de 0 à 359 (sub_5720C sur l'axe du nez ; dans libRealSpace : 360 − lacet).
float PSCockpit::heading() {
    float value = fmodf(360.0f - this->player_plane->yaw / 10.0f, 360.0f);
    if (value < 0.0f) {
        value += 360.0f;
    }
    return value;
}

// Roulis de l'avion en degrés (Matrix_RollAngle_59A30).
float PSCockpit::rollAngle() {
    return this->player_plane->roll / 10.0f;
}

// sub_19D7A et sub_19DC3 : aiguille au cap, 3/4 du rayon.
void PSCockpit::RenderCompass(PSCockpitInstrument *instrument, FrameBuffer *fb) {
    InstrumentView view = this->selectView(instrument);
    if (!view.active) {
        return;
    }
    int radius = view.center_x - view.left;
    this->drawNeedle(fb, view, floorf(this->heading()), radius * 3 / 4, 29);
}

// sub_19721 et sub_19765 : aiguille à l'angle de roulis, 3/4 du rayon.
void PSCockpit::RenderBank(PSCockpitInstrument *instrument, FrameBuffer *fb) {
    InstrumentView view = this->selectView(instrument);
    if (!view.active) {
        return;
    }
    int radius = view.center_x - view.left;
    this->drawNeedle(fb, view, floorf(this->rollAngle()), radius * 3 / 4, 29);
}

// sub_1906A et sub_190AE : palonnier du bloc de commandes (pleine butée = 16) × 50 ÷ 16 degrés ; 3/4 du rayon.
void PSCockpit::RenderRudder(PSCockpitInstrument *instrument, FrameBuffer *fb) {
    InstrumentView view = this->selectView(instrument);
    if (!view.active) {
        return;
    }
    int pedal = (int)(this->player_plane->rudder / 10.0f * 16.0f);
    int radius = view.center_x - view.left;
    this->drawNeedle(fb, view, (float)(pedal * 50 / 16), radius * 3 / 4, 29);
}

// sub_192AB, sub_1930C et sub_1941F : fond (image 0 en vue 0, 1 en vue 1) au coin du cadre, barre d'horizon = deux
// traits opposés depuis le centre à l'angle (−roulis − 90°), puis la maquette (image 2 ou 3) au centre. Le tangage
// n'est pas affiché.
void PSCockpit::RenderHorizon(PSCockpitInstrument *instrument, FrameBuffer *fb) {
    InstrumentView view = this->selectView(instrument);
    if (!view.active) {
        return;
    }
    this->drawImage(fb, instrument, (size_t)view.view, view.left, view.top);
    float angle = floorf(-this->rollAngle() - 90.0f);
    int radius = view.center_x - view.left;
    this->drawNeedle(fb, view, angle, radius * 3 / 4, 29);
    this->drawNeedle(fb, view, angle - 180.0f, radius * 3 / 4, 29);
    this->drawImage(fb, instrument, (size_t)(view.view + 2), view.center_x, view.center_y);
}

// Voyants GEAR, FLAP et STAL (sub_1A072, sub_1A1B3, sub_1A337) : image état (0 ou 1) en vue 0, état + 2 en vue 1, au
// coin du cadre.
void PSCockpit::RenderLamp(PSCockpitInstrument *instrument, FrameBuffer *fb, bool on) {
    InstrumentView view = this->selectView(instrument);
    if (!view.active) {
        return;
    }
    size_t image = on ? 1 : 0;
    if (view.view == 1) {
        image += 2;
    }
    this->drawImage(fb, instrument, image, view.left, view.top);
}

// Points d'emport de ce type qui portent un canon ; avec count_ammo, la somme de leurs munitions.
int PSCockpit::gunStations(int hardpoint_type, bool count_ammo) {
    int total = 0;
    for (auto station : this->player_plane->weaps_load) {
        if (station == nullptr || station->hpts_type != hardpoint_type) {
            continue;
        }
        if (PSEntity::weaponId(station->objct) != PS_WEAPON_GUN) {
            continue;
        }
        if (count_ammo) {
            total += station->nb_weap;
        } else {
            total++;
        }
    }
    return total;
}

// sub_1A3FE et sub_1A44F : munitions des canons du groupe 1 (points de type 0), 4 chiffres (milliers, centaines,
// dizaines, unités) ; image du chiffre en vue 0, chiffre + 10 en vue 1 ; un chiffre tous les (largeur + 3) ÷ 4 points.
void PSCockpit::RenderAmmo(PSCockpitInstrument *instrument, FrameBuffer *fb) {
    InstrumentView view = this->selectView(instrument);
    if (!view.active) {
        return;
    }
    int ammo = this->gunStations(0, true);
    int digits[4];
    digits[0] = ammo / 1000;
    digits[1] = (ammo / 100) % 10;
    digits[2] = (ammo / 10) % 10;
    digits[3] = ammo % 10;
    int step = (view.right - view.left + 3) / 4;
    int offset = view.view == 1 ? 10 : 0;
    for (int k = 0; k < 4; k++) {
        this->drawImage(fb, instrument, (size_t)(digits[k] + offset), view.left + k * step, view.top);
    }
}

// sub_1A691 et sub_1A74D : image 0 si l'arme choisie n'est pas un canon ; sinon (canons des groupes 1 et 2) ÷ 2 + 1,
// au plus la moitié du nombre d'images ; en vue 1, décalée de la moitié du nombre d'images. Au coin du cadre.
void PSCockpit::RenderArmedGuns(PSCockpitInstrument *instrument, FrameBuffer *fb) {
    InstrumentView view = this->selectView(instrument);
    if (!view.active) {
        return;
    }
    int half = (int)instrument->SHAP.GetNumImages() / 2;
    int image = 0;
    SCWeaponLoadoutHardPoint *selected = nullptr;
    if (this->player_plane->selected_weapon >= 0 && (size_t)this->player_plane->selected_weapon < this->player_plane->weaps_load.size()) {
        selected = this->player_plane->weaps_load[this->player_plane->selected_weapon];
    }
    if (selected != nullptr && PSEntity::weaponId(selected->objct) == PS_WEAPON_GUN) {
        image = (this->gunStations(0, false) + this->gunStations(4, false)) / 2 + 1;
        if (half <= image) {
            image = half;
        }
    }
    if (view.view == 1) {
        image += half;
    }
    this->drawImage(fb, instrument, (size_t)image, view.left, view.top);
}

// sub_1A7DB : ruban du compas de secours : image 0 (vue 0) ou 1 (vue 1) décalée de (second mot du point × cap ÷ 360)
// vers la gauche depuis le centre, découpée par le cadre.
void PSCockpit::RenderBackupCompass(PSCockpitInstrument *instrument, FrameBuffer *fb) {
    InstrumentView view = this->selectView(instrument);
    if (!view.active || (size_t)view.view >= instrument->SHAP.GetNumImages()) {
        return;
    }
    int shift = view.center_y * (int)floorf(this->heading()) / 360;
    RLEShape *shape = instrument->SHAP.GetShape((size_t)view.view);
    Point2D position;
    position.x = view.center_x - shift;
    position.y = view.top;
    shape->SetPosition(&position);
    fb->drawShapeWithBox(shape, view.left, view.right, view.top, view.bottom);
}

// sub_1BAD9 : viseur : image de la vue (0 ou 1) au coin du cadre. Le jeu la dessine à travers une table de teinte
// (ligne donnée par le premier mot du chunk GUNS), non portée.
void PSCockpit::RenderGunSight(PSCockpitInstrument *instrument, FrameBuffer *fb) {
    InstrumentView view = this->selectView(instrument);
    if (!view.active) {
        return;
    }
    this->drawImage(fb, instrument, (size_t)view.view, view.left, view.top);
}

PSCockpitDef *PSCockpit::definition() {
    return this->ps_cockpit;
}

// Image d'un instrument pour le cockpit 3D (méthode +28 de chaque instrument dans PACIFIC.EXE). Instruments à aiguille
// (sub_18945, sub_18CB0, sub_19EE9, sub_199A0, sub_18F59, sub_19C69, sub_1919A) : image 1 en (0, 0), puis une aiguille
// depuis le second point ramené au coin du second cadre, de longueur second point − bord gauche. Horizon (sub_19589) :
// image 1, barre depuis le milieu de l'image, image 3 au second point. Voyants : GEAR et STAL image état, FLAP image
// état + 2 (sub_19FFA, sub_1A2BF, sub_1A139). AGNS : son image de la vue 0 (sub_1A792). BANK, AMMO et BCOM n'ont pas
// d'image 3D.
void PSCockpit::RenderInstrument3D(PSCockpitInstrument *instrument, FrameBuffer *fb) {
    fb->fillWithColor(255);
    InstrumentView view;
    view.active = true;
    view.view = 1;
    view.left = 0;
    view.top = 0;
    view.center_x = instrument->second_point_x - instrument->second_left;
    view.center_y = instrument->second_point_y - instrument->second_top;
    int radius = instrument->second_point_x - instrument->second_left;
    float angle = 0.0f;
    bool needle = false;
    switch (instrument->type) {
    case PS_INST_ALTI: {
        int altitude = (int)floorf(this->player_plane->y * this->feet_per_meter * this->altimeter_scale);
        angle = (float)(altitude % 100) * 3.6f;
        needle = true;
    } break;
    case PS_INST_AIRS:
        angle = this->player_plane->worldVelocity().Length() * this->knots_per_ms * (184.0f / 256.0f);
        if (angle > 359.0f) {
            angle = 359.0f;
        }
        needle = true;
        break;
    case PS_INST_COMP:
        angle = floorf(this->heading());
        needle = true;
        break;
    case PS_INST_TACH:
        angle = (float)(this->player_plane->GetThrottle() / 10) * 30.0f;
        needle = true;
        break;
    case PS_INST_VASI: {
        float climb = this->player_plane->worldVelocity().y * this->feet_per_meter;
        int vertical = (int)(climb * (115.0f / 256.0f)) - 90;
        if (vertical < 0) {
            vertical += 360;
        }
        if (climb < 0.0f) {
            if (vertical < 90) {
                vertical = 90;
            }
        } else if (vertical > 90 && vertical < 270) {
            vertical = 90;
        }
        angle = (float)vertical;
        needle = true;
    } break;
    case PS_INST_FUEL: {
        JDYN *jdyn = this->player_plane->object->entity->jdyn;
        if (jdyn != nullptr && jdyn->fuel_capacity > 0) {
            angle = (float)this->player_plane->fuel / (float)jdyn->fuel_capacity * 360.0f;
        }
        needle = true;
    } break;
    case PS_INST_RUDR: {
        int pedal = (int)(this->player_plane->rudder / 10.0f * 16.0f);
        angle = (float)(pedal * 50 / 16);
        needle = true;
    } break;
    case PS_INST_HORI: {
        this->drawImage(fb, instrument, 1, 0, 0);
        InstrumentView middle = view;
        middle.center_x = fb->width / 2;
        middle.center_y = fb->height / 2;
        float bar = floorf(-this->rollAngle() - 90.0f);
        this->drawNeedle(fb, middle, bar, radius, 29);
        this->drawNeedle(fb, middle, bar - 180.0f, radius, 29);
        this->drawImage(fb, instrument, 3, view.center_x, view.center_y);
    } break;
    case PS_INST_GEAR:
        this->drawImage(fb, instrument, this->player_plane->GetWheel() != 0 ? 1 : 0, 0, 0);
        break;
    case PS_INST_STAL:
        this->drawImage(fb, instrument, this->player_plane->wing_stall != 0 ? 1 : 0, 0, 0);
        break;
    case PS_INST_FLAP:
        this->drawImage(fb, instrument, this->player_plane->GetFlaps() != 0 ? 3 : 2, 0, 0);
        break;
    case PS_INST_AGNS: {
        int half = (int)instrument->SHAP.GetNumImages() / 2;
        int image = 0;
        SCWeaponLoadoutHardPoint *selected = nullptr;
        if (this->player_plane->selected_weapon >= 0 && (size_t)this->player_plane->selected_weapon < this->player_plane->weaps_load.size()) {
            selected = this->player_plane->weaps_load[this->player_plane->selected_weapon];
        }
        if (selected != nullptr && PSEntity::weaponId(selected->objct) == PS_WEAPON_GUN) {
            image = (this->gunStations(0, false) + this->gunStations(4, false)) / 2 + 1;
            if (half <= image) {
                image = half;
            }
        }
        this->drawImage(fb, instrument, (size_t)image, 0, 0);
    } break;
    default:
        break;
    }
    if (needle) {
        this->drawImage(fb, instrument, 1, 0, 0);
        this->drawNeedle(fb, view, angle, radius, 29);
    }
}

// Image ARTP d'une vue. Les 6 entrées du chunk INFO du cockpit décrivent les 6 images dans l'ordre : trois vues vers
// l'avant (cap 0), puis l'arrière (cap 180), la droite (cap −90) et la gauche (cap 90). La première vue avant porte
// les instruments dans leur premier cadre, la deuxième dans leur second cadre ; la troisième n'a pas de vue
// correspondante dans libRealSpace.
size_t PSCockpit::artImage(CockpitFace face) {
    switch (face) {
    case CockpitFace::CP_FRONT:
        return 0;
    case CockpitFace::CP_BIG:
        return 1;
    case CockpitFace::CP_REAR:
        return 3;
    case CockpitFace::CP_LEFT:
        return 5;
    case CockpitFace::CP_RIGHT:
        return 4;
    default:
        return 0;
    }
}

// Copie réduite de SCCockpit::Render : tableau de bord de la vue (les cadrans y sont déjà dessinés), puis les aiguilles
// et chiffres des instruments en vue avant et en vue CP_BIG.
void PSCockpit::Render(CockpitFace face) {
    this->face = face;
    VGA.activate();
    VGA.setPalette(&this->palette);
    bool upscale = VGA.upscale;
    bool wide_screen = VGA.wide_screen;
    VGA.upscale = false;
    VGA.wide_screen = false;
    FrameBuffer *fb = VGA.getFrameBuffer();
    fb->clear();
    if (this->ps_cockpit != nullptr && this->artImage(this->face) < this->ps_cockpit->ARTP.GetNumImages()) {
        fb->drawShape(this->ps_cockpit->ARTP.GetShape(this->artImage(this->face)));
        if (this->face == CockpitFace::CP_FRONT || this->face == CockpitFace::CP_BIG) {
            for (auto instrument : this->ps_cockpit->instruments) {
                switch (instrument->type) {
                case PS_INST_GUNS:
                    this->RenderGunSight(instrument, fb);
                    break;
                case PS_INST_ALTI:
                    this->RenderAltimeter(instrument, fb);
                    break;
                case PS_INST_AIRS:
                    this->RenderAirspeed(instrument, fb);
                    break;
                case PS_INST_COMP:
                    this->RenderCompass(instrument, fb);
                    break;
                case PS_INST_TACH:
                    this->RenderTachometer(instrument, fb);
                    break;
                case PS_INST_VASI:
                    this->RenderVerticalSpeed(instrument, fb);
                    break;
                case PS_INST_FUEL:
                    this->RenderFuel(instrument, fb);
                    break;
                case PS_INST_BANK:
                    this->RenderBank(instrument, fb);
                    break;
                case PS_INST_HORI:
                    this->RenderHorizon(instrument, fb);
                    break;
                case PS_INST_GEAR:
                    this->RenderLamp(instrument, fb, this->player_plane->GetWheel() != 0);
                    break;
                case PS_INST_FLAP:
                    this->RenderLamp(instrument, fb, this->player_plane->GetFlaps() != 0);
                    break;
                case PS_INST_STAL:
                    this->RenderLamp(instrument, fb, this->player_plane->wing_stall != 0);
                    break;
                case PS_INST_AMMO:
                    this->RenderAmmo(instrument, fb);
                    break;
                case PS_INST_AGNS:
                    this->RenderArmedGuns(instrument, fb);
                    break;
                case PS_INST_BCOM:
                    this->RenderBackupCompass(instrument, fb);
                    break;
                case PS_INST_RUDR:
                    this->RenderRudder(instrument, fb);
                    break;
                }
            }
        }
    }
    if (this->mouse_control) {
        Mouse.draw();
    }
    VGA.vSync();
    VGA.upscale = upscale;
    VGA.wide_screen = wide_screen;
}
