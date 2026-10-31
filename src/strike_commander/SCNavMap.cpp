//
//  SCNavMap.cpp
//  libRealSpace
//
//  Created by Rémi LEONARD on 31/08/2024.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//
#include "precomp.h"

SCNavMap::SCNavMap(){
    this->navMap = nullptr;
    if (Game == nullptr) {
        Game = &GameEngine::instance();
    }
}
SCNavMap::~SCNavMap(){

}
/**
 * Checks for keyboard events and updates the game state accordingly.
 *
 * This function peeks at the SDL event queue to check for keyboard events.
 * If a key is pressed, it updates the game state based on the key pressed.
 * The supported keys are escape, space, and return.
 *
 * @return None
 */
void SCNavMap::checkKeyboard(void) {
    if (this->m_keyboard == nullptr) {
        return;
    }
    if (this->m_keyboard->isActionJustPressed(CreateAction(InputAction::NAVMAP_START, NavActionOfst::NAV_ESCAPE))) {
        Game->stopTopActivity();
    }
    if (this->m_keyboard->isActionJustPressed(CreateAction(InputAction::NAVMAP_START, NavActionOfst::NAV_ESCAPE2))) {
        Game->stopTopActivity();
    }
    if (this->m_keyboard->isActionJustPressed(InputAction::CONTROLLER_BUTTON_BACK)) {
        Game->stopTopActivity();
    }
    if (this->m_keyboard->isActionJustPressed(CreateAction(InputAction::NAVMAP_START, NavActionOfst::NAV_NEXT_WP))) {
        *this->current_nav_point = *this->current_nav_point+1;
        if (*this->current_nav_point > this->mission->waypoints.size()-1) {
            *this->current_nav_point = (uint8_t) this->mission->waypoints.size()-1;
        }
    }
    if (this->m_keyboard->isActionJustPressed(CreateAction(InputAction::NAVMAP_START, NavActionOfst::NAV_PREV_WP))) {
        *this->current_nav_point = *this->current_nav_point-1;
        if (*this->current_nav_point == 255) {
            *this->current_nav_point = 0;
        }
    }
    if (this->m_keyboard->isActionJustPressed(InputAction::CONTROLLER_BUTTON_A)) {
        *this->current_nav_point = *this->current_nav_point+1;
        if (*this->current_nav_point > this->mission->waypoints.size()-1) {
            *this->current_nav_point = (uint8_t) this->mission->waypoints.size()-1;
        }
    }
    if (this->m_keyboard->isActionJustPressed(InputAction::CONTROLLER_BUTTON_B)) {
        *this->current_nav_point = *this->current_nav_point-1;
        if (*this->current_nav_point == 255) {
            *this->current_nav_point = 0;
        }
    }
}
/**
 * Initializes the SCNavMap object.
 *
 * This function reads in the PALETTE.IFF file and sets the object's color palette.
 * It also reads in the NAMAP.IFF file and sets the object's navMap property.
 *
 * @return None
 */
void SCNavMap::init(){
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
    for (uint8_t i=0; i<255; i++) {
        Texel c = {i,0,0,255};
        this->objpal.SetColor(i, c);
    }
    
    TreEntry *nav_map = Assets.GetEntryByName(Assets.navmap_filename);
    this->navMap = new RSNavMap();
    this->navMap->InitFromRam(nav_map->data, nav_map->size);

    if (Assets.navmap_add_filename != "") {
        TreEntry *nav_map_add = Assets.GetEntryByName(Assets.navmap_add_filename);
        if (nav_map_add != nullptr) {
            RSNavMap *nav_map_add_obj = new RSNavMap();
            nav_map_add_obj->InitFromRam(nav_map_add->data, nav_map_add->size);
            for (auto &map: nav_map_add_obj->maps) {
                this->navMap->maps[map.first] = map.second;
            }
        }
    }
    m_keyboard = Game->getKeyboard();
    m_keyboard->registerAction(CreateAction(InputAction::NAVMAP_START, NavActionOfst::NAV_ESCAPE));
    m_keyboard->registerAction(CreateAction(InputAction::NAVMAP_START, NavActionOfst::NAV_NEXT_WP));
    m_keyboard->registerAction(CreateAction(InputAction::NAVMAP_START, NavActionOfst::NAV_PREV_WP));
    m_keyboard->registerAction(CreateAction(InputAction::NAVMAP_START, NavActionOfst::NAV_ESCAPE2));


    m_keyboard->bindKeyToAction(CreateAction(InputAction::NAVMAP_START, NavActionOfst::NAV_ESCAPE), SDL_SCANCODE_ESCAPE);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::NAVMAP_START, NavActionOfst::NAV_PREV_WP), SDL_SCANCODE_LEFT);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::NAVMAP_START, NavActionOfst::NAV_NEXT_WP), SDL_SCANCODE_RIGHT);
    m_keyboard->bindKeyToAction(CreateAction(InputAction::NAVMAP_START, NavActionOfst::NAV_ESCAPE2), SDL_SCANCODE_N);
    
}
/**
 * Sets the name of the current mission.
 *
 * This function takes a C-style string as its argument and sets the object's
 * name to that string.
 *
 * @param name the name of the current mission
 */
void SCNavMap::SetName(char *name) {
    this->name = new std::string(name);
    std::transform(this->name->begin(), this->name->end(), this->name->begin(), ::toupper);
}
/**
 * SCNavMap::RunFrame
 *
 * This function runs the frame of the navigation map.
 *
 * It clears the screen, sets the palette, draws the background and the shape
 * corresponding to the current mission, and draws the mouse.
 *
 * @return None
 */
void SCNavMap::runFrame(void) {
    this->checkKeyboard();
    bool upscaled = VGA.upscale;
    VGA.upscale = false;
    VGA.activate();
    VGA.getFrameBuffer()->clear();
    VGA.setPalette(&this->palette);
    RLEShape *map_shape = nullptr;
    if (this->navMap != nullptr && this->name != nullptr) {
        if (this->navMap->maps.count(*this->name)>0) {
            map_shape = this->navMap->maps[*this->name];
        } else if (this->navMap->maps.count(this->mission->mission->mission_data.world_filename)>0) {
            map_shape = this->navMap->maps[this->mission->mission->mission_data.world_filename];
        }
    }
    if (map_shape != nullptr) {
        // GlyphObject_MeasureAndDraw_7F587 : la carte, ce qui est dessus (NavMap_DrawMissionObjects_7EDCA), le cadre,
        // puis le panneau de droite.
        VGA.getFrameBuffer()->drawShape(map_shape);
        this->label_boxes.clear();
        if (show_waypoint) {
            this->drawAreaOutlines();
        }
        if (show_obj) {
            this->drawObjects();
        }
        if (show_waypoint) {
            this->drawAreaNames();
        }
        RLEShape *shape = this->navMap->background;
        if (shape != nullptr) {
            VGA.getFrameBuffer()->drawShape(shape);
        }

        std::string leader = "solo";
        if (this->mission->friendlies.size() > 0) {
            leader = this->mission->friendlies[0]->profile->radi.info.callsign;
        }
        std::string mission_name = this->mission->mission->mission_data.world_filename;
        std::transform(leader.begin(), leader.end(), leader.begin(), ::tolower);
        std::transform(mission_name.begin(), mission_name.end(), mission_name.begin(), ::tolower);
        int msg_newx = 257;
        int msg_newy = 10;
        Point2D mission_name_pos{248, 30};
        Point2D leader_pos{248, 50};

        VGA.getFrameBuffer()->printText_SM(
            this->navMap->font,
            &mission_name_pos,
            const_cast<char*>("mission"),
            0,
            0,
            7,1,this->navMap->font->GetShapeForChar('A')->GetWidth(), false, false);
        
        mission_name_pos.x = 260;
        mission_name_pos.y += this->navMap->font->GetShapeForChar('A')->GetHeight();

        VGA.getFrameBuffer()->printText_SM(
            this->navMap->font,
            &mission_name_pos,
            (char*) mission_name.c_str(),
            0,
            0,
            (int32_t)mission_name.size(),1,this->navMap->font->GetShapeForChar('A')->GetWidth(), false, false);
        leader_pos.y = mission_name_pos.y + this->navMap->font->GetShapeForChar('A')->GetHeight();
        VGA.getFrameBuffer()->printText_SM(
            this->navMap->font,
            &leader_pos,
            const_cast<char*>("leader"),
            0,
            0,
            6,1,this->navMap->font->GetShapeForChar('A')->GetWidth(), false, false);
        leader_pos.y += this->navMap->font->GetShapeForChar('A')->GetHeight();
        leader_pos.x = 260;
        VGA.getFrameBuffer()->printText_SM(
            this->navMap->font,
            &leader_pos,
            (char*) leader.c_str(),
            0,
            0,
            (int32_t)leader.size(),1,this->navMap->font->GetShapeForChar('A')->GetWidth(), false, false);
        if (this->mission->friendlies.size() > 1) {
            if (this->mission->friendlies[1]->profile != nullptr) {
                leader = this->mission->friendlies[1]->profile->radi.info.callsign;
            } else {
                leader = "wingman";
            }
            std::transform(leader.begin(), leader.end(), leader.begin(), ::tolower);
            leader_pos.y += this->navMap->font->GetShapeForChar('A')->GetHeight();
            leader_pos.x = 252;
            VGA.getFrameBuffer()->printText_SM(
                this->navMap->font,
                &leader_pos,
                const_cast<char*>("wingman"),
                0,
                0,
                7,1,this->navMap->font->GetShapeForChar('A')->GetWidth(), false, false);
            leader_pos.y += this->navMap->font->GetShapeForChar('A')->GetHeight();
            leader_pos.x = 260;
            VGA.getFrameBuffer()->printText_SM(
                this->navMap->font,
                &leader_pos,
                (char*) leader.c_str(),
                0,
                0,
                (int32_t)leader.size(),1,this->navMap->font->GetShapeForChar('A')->GetWidth(), false, false);
        }
        
        if (show_waypoint) {
            int cpt = 0;
            
            for (auto wp: this->mission->waypoints) {
                if (cpt == *this->current_nav_point) {
                    int msg_newx = 252;
                    int msg_newy = 80;
                    Point2D *msg_p1 = new Point2D({msg_newx, msg_newy});
                    
                    VGA.getFrameBuffer()->printText_SM(
                        this->navMap->font,
                        msg_p1,
                        const_cast<char*>("objective"),
                        0,
                        0,
                        9,
                        1,
                        this->navMap->font->GetShapeForChar('A')->GetWidth(),
                        false, false
                    );
                    
                    msg_p1->y += this->navMap->font->GetShapeForChar('A')->GetHeight();
                    msg_p1->x = msg_newx+5;
                    if (wp->objective != nullptr) {
                        std::transform(wp->objective->begin(), wp->objective->end(), wp->objective->begin(), ::tolower);
                        VGA.getFrameBuffer()->printText_SM(
                            this->navMap->font,
                            msg_p1,
                            (char*) wp->objective->c_str(),
                            0,
                            0,
                            (int32_t)wp->objective->size(),
                            1,
                            this->navMap->font->GetShapeForChar('A')->GetWidth(),
                            false, false
                        );
                    }

                    msg_newx = 255;
                    msg_newy = 135;
                    msg_p1 = new Point2D({msg_newx, msg_newy});
                    
                    VGA.getFrameBuffer()->printText_SM(
                        this->navMap->font,
                        msg_p1,
                        const_cast<char*>("notes"),
                        0,
                        0,
                        5,
                        1,
                        this->navMap->font->GetShapeForChar('A')->GetWidth(),
                        false, false
                    );
                    msg_p1->y += this->navMap->font->GetShapeForChar('A')->GetHeight();
                    msg_p1->x = msg_newx+3;
                    if (wp->message != nullptr) {
                        std::transform(wp->message->begin(), wp->message->end(), wp->message->begin(), ::tolower);
                        VGA.getFrameBuffer()->printText_SM(
                            this->navMap->font,
                            msg_p1,
                            (char*) wp->message->c_str(),
                            0,
                            0,
                            (int32_t)wp->message->size(),
                            1,
                            this->navMap->font->GetShapeForChar('A')->GetWidth(),
                            true, false
                        );
                    }
                }
                cpt++;
            }
        }
    }
    
    Mouse.draw();
    VGA.vSync();
    VGA.upscale = upscaled;
}

// GlyphObject_Helper3_7E72B : x = gauche + (c0 + demi-taille) / échelle X, y = haut + hauteur - (c1 + demi-taille) /
// échelle Y, avec échelle X = demi-taille / (largeur / 2), échelle Y = demi-taille / (hauteur / 2) et c1 = -z.
Point2D SCNavMap::worldToMap(float x, float z) {
    float half = this->navMap->world_half_size;
    float scale_x = half / (float)(this->navMap->map_width / 2);
    float scale_y = half / (float)(this->navMap->map_height / 2);
    Point2D p;
    p.x = this->navMap->map_left + (int)floorf((x + half) / scale_x);
    p.y = this->navMap->map_top + this->navMap->map_height - (int)floorf((half - z) / scale_y);
    return p;
}

// Zone d'une scène tracée sur la carte : scène active, zone présente, nom qui ne commence pas par « . ».
AREA *SCNavMap::sceneArea(MISN_SCEN *scene) {
    std::vector<AREA *> &areas = this->missionObj->mission_data.areas;
    if (scene->is_active == 0) {
        return nullptr;
    }
    uint16_t area_index = (uint16_t)scene->area_id;
    if (area_index >= areas.size()) {
        return nullptr;
    }
    AREA *area = areas[area_index];
    if (area->AreaName[0] == '.') {
        return nullptr;
    }
    return area;
}

// Expr_ResolveStringEntry_7F5D5 : pour un ordre de navigation, zone du SPOT de l'ordre ; ici, celle du point de
// navigation courant.
AREA *SCNavMap::objectiveArea() {
    std::vector<AREA *> &areas = this->missionObj->mission_data.areas;
    if (*this->current_nav_point >= this->mission->waypoints.size()) {
        return nullptr;
    }
    SCMissionWaypoint *wp = this->mission->waypoints[*this->current_nav_point];
    if (wp->spot == nullptr || wp->spot->area_id < 0 || wp->spot->area_id >= (int)areas.size()) {
        return nullptr;
    }
    return areas[wp->spot->area_id];
}

// Liste lue par Parser_FindDuplicateSymbol (scénario +104) : ici, les zones des points de navigation.
bool SCNavMap::isNavigationArea(AREA *area) {
    std::vector<AREA *> &areas = this->missionObj->mission_data.areas;
    for (auto wp : this->mission->waypoints) {
        if (wp->spot == nullptr || wp->spot->area_id < 0 || wp->spot->area_id >= (int)areas.size()) {
            continue;
        }
        if (areas[wp->spot->area_id] == area) {
            return true;
        }
    }
    return false;
}

// GlyphObject_Helper5_7EB1C : rectangle occupé, 32 au plus.
void SCNavMap::reserveBox(int left, int top, int width, int height) {
    if (this->label_boxes.size() >= 32) {
        return;
    }
    LabelBox box;
    box.left = left;
    box.top = top;
    box.right = left + width - 1;
    box.bottom = top + height - 1;
    this->label_boxes.push_back(box);
}

// GlyphObject_Helper7_7EBCA : dans la fenêtre des textes (x 25 à 241, y 23 à 177) et sans recouvrement, bornes
// incluses.
bool SCNavMap::boxIsFree(int left, int top, int width, int height) {
    int right = left + width - 1;
    int bottom = top + height - 1;
    if (left < 25 || right > 241 || top < 23 || bottom > 177) {
        return false;
    }
    for (auto &box : this->label_boxes) {
        if (box.right >= left && box.left <= right && box.bottom >= top && box.top <= bottom) {
            return false;
        }
    }
    return true;
}

// GlyphObject_Helper6_7EB72 : le rectangle est ramené dans la fenêtre des textes.
void SCNavMap::clampBox(int &left, int &top, int width, int height) {
    if (left + width - 1 > 241) {
        left = 241 - width + 1;
    }
    if (top + height - 1 > 177) {
        top = 177 - height + 1;
    }
    if (left < 25) {
        left = 25;
    }
    if (top < 23) {
        top = 23;
    }
}

// GlyphObject_Helper8_7EC45 : six places autour du point (x, y), la première libre est gardée et réservée ; avec
// label_first_try_only, seule la première est essayée et rien n'est écrit si elle est prise ; sinon, la dernière
// place est prise sans contrôle. Le texte est en majuscules (lettre de base « A » de la police).
void SCNavMap::drawLabel(int x, int y, std::string text, uint8_t label_color) {
    std::transform(text.begin(), text.end(), text.begin(), ::toupper);
    FrameBuffer *fb = VGA.getFrameBuffer();
    RSFont *font = this->navMap->font;
    int width = fb->getJustifiedTextWidth(font, text, 0);
    int height = font->GetShapeForChar('A')->GetHeight() + 1;
    int half_width = width / 2;
    int left = 0;
    int top = 0;
    for (int attempt = 0;; attempt++) {
        bool forced = false;
        switch (attempt) {
        case 0:
            left = x - half_width;
            top = y + height / 2 / 2;
            break;
        case 1:
            left = x - half_width;
            top = y - height - height / 2 / 2;
            break;
        case 2:
            left = x + 3;
            top = y - height / 2;
            break;
        case 3:
            left = x - width - 3;
            top = y - height / 2;
            break;
        case 4:
            left = x - half_width;
            top = y + height;
            break;
        case 5:
            left = x - half_width;
            top = y - 2 * height;
            break;
        default:
            left = x + 3;
            top = y - height / 2;
            forced = true;
            break;
        }
        if (forced) {
            break;
        }
        this->clampBox(left, top, width, height);
        if (this->boxIsFree(left, top, width, height)) {
            this->reserveBox(left, top, width, height);
            break;
        }
        if (this->label_first_try_only) {
            return;
        }
    }
    Point2D origin;
    origin.x = left;
    origin.y = top;
    fb->printText(font, &origin, (char *)text.c_str(), label_color, 0, (uint32_t)text.size(), 0, 0,
                  TEXT_JUSTIFY_LEFT, 320);
    this->label_first_try_only = false;
}

// NavMap_DrawMissionObjects_7EDCA, 1re boucle : pour chaque scène tracée, point de référence et contour, couleur 215,
// 238 pour la zone de l'objectif. Sphère et cylindre : ellipse (Render_DrawEllipseOutline_61612) ; boîte : 4 traits
// d'ouest en est et du nord au sud (Render_FillClippedRect_61960).
void SCNavMap::drawAreaOutlines() {
    FrameBuffer *fb = VGA.getFrameBuffer();
    float half = this->navMap->world_half_size;
    float scale_x = half / (float)(this->navMap->map_width / 2);
    float scale_y = half / (float)(this->navMap->map_height / 2);
    AREA *objective = this->objectiveArea();
    for (auto scene : this->missionObj->mission_data.scenes) {
        AREA *area = this->sceneArea(scene);
        if (area == nullptr) {
            continue;
        }
        uint8_t outline = area == objective ? 238 : 215;
        Point2D p = this->worldToMap(area->position.x, area->position.z);
        fb->plot_pixel(p.x, p.y, outline);
        if (area->AreaType == 'B') {
            int west = (int)floorf(area->west / scale_x);
            int east = (int)floorf(area->east / scale_x);
            int north = (int)floorf(area->north / scale_y);
            int south = (int)floorf(area->south / scale_y);
            fb->line(p.x - west, p.y - north, p.x + east, p.y - north, outline);
            fb->line(p.x - west, p.y + south, p.x + east, p.y + south, outline);
            fb->line(p.x - west, p.y - north, p.x - west, p.y + south, outline);
            fb->line(p.x + east, p.y - north, p.x + east, p.y + south, outline);
        } else {
            int rx = (int)floorf(area->radius / scale_x);
            int ry = (int)floorf(area->radius / scale_y);
            fb->ellipse(p.x, p.y, rx, ry, outline);
        }
    }
}

// NavMap_DrawMissionObjects_7EDCA, 2e boucle : objets marqués (bit 4 de +0x39) déjà créés. Point de couleur 199 pour
// le joueur, 230 pour un ennemi, 31 sinon, qui réserve 3 x 3 pixels ; second point à droite, ou pour un avion point
// noir à l'opposé de sa vitesse (seuil 0,25) ; nom en couleur 31.
void SCNavMap::drawObjects() {
    FrameBuffer *fb = VGA.getFrameBuffer();
    for (auto actor : this->mission->actors) {
        if (!actor->on_nav_map) {
            continue;
        }
        if (actor->is_active == 0 && actor != this->mission->player) {
            continue;
        }
        Point2D p = this->worldToMap(actor->object->position.x, actor->object->position.z);
        uint8_t marker = 31;
        if (actor == this->mission->player) {
            marker = 199;
        } else if (actor->team_id == 255) {
            marker = 230;
        }
        fb->plot_pixel(p.x, p.y, marker);
        this->reserveBox(p.x - 1, p.y - 1, 3, 3);
        int tail_x = 1;
        int tail_y = 0;
        uint8_t tail = marker;
        if (actor->plane != nullptr) {
            Vector3D velocity = actor->plane->worldVelocity();
            float east = velocity.x;
            float north = -velocity.z;
            tail_x = 0;
            if (east > 0.25f) {
                tail_x = -1;
            } else if (east < -0.25f) {
                tail_x = 1;
            }
            tail_y = 0;
            if (north > 0.25f) {
                tail_y = 1;
            } else if (north < -0.25f) {
                tail_y = -1;
            }
            tail = 0;
        }
        fb->plot_pixel(p.x + tail_x, p.y + tail_y, tail);
        std::string name = (actor->profile != nullptr) ? actor->profile->radi.info.callsign : "FRIEND";
        this->label_first_try_only = false;
        this->drawLabel(p.x, p.y, name, 34);
    }
}

// NavMap_DrawMissionObjects_7EDCA, 3e boucle : nom de chaque zone tracée, couleur 223, 237 pour une zone de point de
// navigation, 199 pour celle de l'objectif ; première place sous le point, puis 10 pixels plus bas, puis 10 plus haut,
// puis toutes les places (byte_6D5DE).
void SCNavMap::drawAreaNames() {
    AREA *objective = this->objectiveArea();
    for (auto scene : this->missionObj->mission_data.scenes) {
        AREA *area = this->sceneArea(scene);
        if (area == nullptr) {
            continue;
        }
        uint8_t name_color = 134;
        if (this->isNavigationArea(area)) {
            name_color = 134;
        }
        if (area == objective) {
            name_color = 0;
        }
        Point2D p = this->worldToMap(area->position.x, area->position.z);
        std::string name = area->AreaName;
        this->label_first_try_only = true;
        this->drawLabel(p.x, p.y, name, name_color);
        if (this->label_first_try_only) {
            this->drawLabel(p.x, p.y + 10, name, name_color);
        }
        if (this->label_first_try_only) {
            this->drawLabel(p.x, p.y - 10, name, name_color);
        }
        if (this->label_first_try_only) {
            this->label_first_try_only = false;
            this->drawLabel(p.x, p.y, name, name_color);
        }
    }
}
