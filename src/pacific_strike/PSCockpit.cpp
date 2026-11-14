#include "PSCockpit.h"
#include "PSEntity.h"

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
    this->comm_framebuffer = new FrameBuffer(320, 13);
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

void PSCockpit::RenderTachometer(PSCockpitInstrument *instrument, FrameBuffer *fb) {
    InstrumentView view = this->selectView(instrument);
    if (!view.active) {
        return;
    }
    int notch = this->player_plane->GetThrottle() / 10;
    int radius = view.center_x - view.left;
    this->drawNeedle(fb, view, (float)notch * 30.0f, radius * 3 / 4, 29);
}

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

float PSCockpit::heading() {
    float value = fmodf(360.0f - this->player_plane->yaw / 10.0f, 360.0f);
    if (value < 0.0f) {
        value += 360.0f;
    }
    return value;
}

float PSCockpit::rollAngle() {
    return this->player_plane->roll / 10.0f;
}

void PSCockpit::RenderCompass(PSCockpitInstrument *instrument, FrameBuffer *fb) {
    InstrumentView view = this->selectView(instrument);
    if (!view.active) {
        return;
    }
    int radius = view.center_x - view.left;
    this->drawNeedle(fb, view, floorf(this->heading()), radius * 3 / 4, 29);
}

void PSCockpit::RenderBank(PSCockpitInstrument *instrument, FrameBuffer *fb) {
    InstrumentView view = this->selectView(instrument);
    if (!view.active) {
        return;
    }
    int radius = view.center_x - view.left;
    this->drawNeedle(fb, view, floorf(this->rollAngle()), radius * 3 / 4, 29);
}

void PSCockpit::RenderRudder(PSCockpitInstrument *instrument, FrameBuffer *fb) {
    InstrumentView view = this->selectView(instrument);
    if (!view.active) {
        return;
    }
    int pedal = (int)(this->player_plane->rudder / 10.0f * 16.0f);
    int radius = view.center_x - view.left;
    this->drawNeedle(fb, view, (float)(pedal * 50 / 16), radius * 3 / 4, 29);
}

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
    if (this->ps_cockpit != nullptr && this->show_comm) {
        this->RenderCommMenu(fb);
    }
    if (this->ps_cockpit != nullptr) {
        Point2D comm_position;
        comm_position.x = 0;
        comm_position.y = 200;
        this->RenderCommMessages(comm_position, fb);
    }
    if (this->mouse_control) {
        Mouse.draw();
    }
    VGA.vSync();
    VGA.upscale = upscale;
    VGA.wide_screen = wide_screen;
}

std::vector<SCMissionActors *> PSCockpit::commReceivers() {
    std::vector<SCMissionActors *> receivers;
    SCMissionActors *player = this->current_mission->player;
    for (auto actor : this->current_mission->actors) {
        if (receivers.size() >= 8) {
            break;
        }
        if (actor == player || actor->plane == nullptr || actor->is_destroyed || !actor->is_active) {
            continue;
        }
        if (actor->team_id != player->team_id || actor->profile == nullptr || actor->profile->radi.opts.empty()) {
            continue;
        }
        receivers.push_back(actor);
    }
    return receivers;
}

bool PSCockpit::commOptionAvailable(char ask, SCMissionActors *receiver) {
    SCMissionActors *player = this->current_mission->player;
    PSBrain *brain = dynamic_cast<PSBrain *>(receiver->brain);
    bool airborne = !this->player_plane->on_ground;
    float distance = (this->player_plane->position - receiver->plane->position).Length();
    switch (ask) {
        case 'd':
            return true;
        case 'e':
            return !airborne;
        case 'f':
            return airborne && distance < 5000.0f;
        case 'g':
            return brain != nullptr && brain->in_formation;
        case 'h':
            return brain != nullptr && !brain->in_formation;
        case 'i':
            return player->target != nullptr;
        case 'j':
            return brain != nullptr && !brain->isFleeing();
        case 'l':
            return receiver->talkative;
        case 'm':
            return !receiver->talkative;
        case 'n':
            return airborne && distance < 5000.0f;
        default:
            return false;
    }
}

std::vector<char> PSCockpit::commOptions(SCMissionActors *receiver) {
    std::vector<char> options;
    for (char ask : receiver->profile->radi.opts) {
        if (this->commOptionAvailable(ask, receiver)) {
            options.push_back(ask);
        }
    }
    return options;
}

void PSCockpit::drawCommLine(FrameBuffer *fb, int row, std::string text) {
    int left = 90;
    int top = 40 + 11 * row;
    for (int y = top; y < top + 11; y++) {
        fb->line(left, y, left + 130, y, 0xC0);
    }
    RSFont *font = this->cockpit->PLAQ.fonts[1].font;
    Point2D position;
    position.x = left + 4;
    position.y = top + 2;
    fb->printText(font, &position, (char *)text.c_str(), 0, 0, (uint32_t)text.size(), 1, 2);
}

void PSCockpit::RenderCommMenu(FrameBuffer *fb) {
    if (this->cockpit->PLAQ.fonts.size() < 2) {
        return;
    }
    if (this->comm_target == 0 || this->comm_actor == nullptr) {
        this->drawCommLine(fb, 0, "Select Frequency");
        std::vector<SCMissionActors *> receivers = this->commReceivers();
        int row = 1;
        for (auto receiver : receivers) {
            this->drawCommLine(fb, row, std::to_string(row) + ": " + receiver->profile->radi.info.callsign);
            row++;
        }
        this->drawCommLine(fb, row, "No One");
        return;
    }
    this->drawCommLine(fb, 0, this->comm_actor->profile->radi.info.callsign);
    std::vector<char> options = this->commOptions(this->comm_actor);
    int row = 1;
    for (char ask : options) {
        std::string key(1, ask);
        auto text = this->current_mission->player->profile->radi.asks.find(key);
        std::string label = text != this->current_mission->player->profile->radi.asks.end() ? text->second : key;
        this->drawCommLine(fb, row, std::to_string(row) + ": " + label);
        row++;
    }
    this->drawCommLine(fb, row, "nothing");
}

void PSCockpit::SetCommActorTarget(int target) {
    this->comm_actor = nullptr;
    this->comm_target = 0;
    if (target == 0) {
        return;
    }
    std::vector<SCMissionActors *> receivers = this->commReceivers();
    if (target > (int)receivers.size()) {
        this->show_comm = false;
        return;
    }
    this->comm_actor = receivers[target - 1];
    this->comm_target = target;
    printf("PSCockpit: radio vers %s#%d\n", this->comm_actor->actor_name.c_str(), this->comm_actor->actor_id);
}

int PSCockpit::CommMessageId(int line) {
    if (this->comm_actor == nullptr) {
        return 0;
    }
    std::vector<char> options = this->commOptions(this->comm_actor);
    if (line < 1 || line > (int)options.size()) {
        return 0;
    }
    std::vector<char> &opts = this->comm_actor->profile->radi.opts;
    for (size_t i = 0; i < opts.size(); i++) {
        if (opts[i] == options[line - 1]) {
            return (int)i + 1;
        }
    }
    return 0;
}
