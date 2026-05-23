//
//  SCShot.cpp
//  libRealSpace
//
//  Created by Rémi LEONARD on 23/08/2024.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//


#include "precomp.h"
#include "../engine/gametimer.h"
SCShot::SCShot() { 
    this->fps = SDL_GetTicks() / 100;
    if (Game == nullptr) {
        Game = &GameEngine::getInstance();
    }
}

SCShot::~SCShot() {}

/**
 * Checks for keyboard events and updates the game state accordingly.
 *
 * This function peeks at the SDL event queue to check for keyboard events.
 * If a key is pressed, it updates the game state based on the key pressed.
 * The supported keys are escape, space, and return.
 *
 * @return None
 */
void SCShot::checkKeyboard(void) {
    if (!Game)
        return;
    Keyboard* kb = Game->getKeyboard();
    if (!kb)
        return;
    if (kb->isActionJustPressed(InputAction::KEY_ESCAPE)) {
        Game->stopTopActivity();
    }
    if (Mouse.buttons[MouseButton::LEFT].event == MouseButton::PRESSED) {
        Mouse.buttons[MouseButton::LEFT].event = MouseButton::NONE;
        Game->stopTopActivity();
    }
}

/**
 * Initializes the SCShot object by loading the necessary IFF files and initializing the optionParser and
 * gameFlowParser. It also initializes the optShps and optPals objects by loading the OPTSHPS.PAK and OPTPALS.PAK files
 * respectively. Finally, it sets the efect pointer to nullptr and calls the createMiss() function.
 *
 * @throws None
 */
void SCShot::init() {
    
    TreEntry *optionIFF =
        Assets.GetEntryByName(Assets.option_filename);
    this->optionParser.InitFromRam(optionIFF->data, optionIFF->size);
    TreEntry *optShapEntry =
        Assets.GetEntryByName(Assets.optshps_filename);
    this->optShps.InitFromRAM("OPTSHPS.PAK", optShapEntry->data, optShapEntry->size);
    TreEntry *optPalettesEntry =
        Assets.GetEntryByName(Assets.optpals_filename);
    this->optPals.InitFromRAM("OPTPALS.PAK", optPalettesEntry->data, optPalettesEntry->size);
}

/**
 * Sets the shot ID of the SCShot object and updates the layers and palette accordingly.
 *
 * @param shotid The shot ID to set.
 *
 * @throws None
 */
void SCShot::SetShotId(uint8_t shotid) {
    if (this->optionParser.estb.find(shotid) == this->optionParser.estb.end()) {
        return;
    }
    for (auto s : this->optionParser.estb[shotid]->images) {
        shotBackground *bg = new shotBackground();
        bg->img = this->getShape(s->OptshapeID);
        this->layers.push_back(bg);
    }
    if (this->optionParser.estb[shotid]->palettes.size() < 1) {
        return;
    }
    uint8_t paltID = this->optionParser.estb[shotid]->palettes[0]->ID;
    this->rawPalette = this->optPals.GetEntry(paltID)->data;
    for (auto p : this->optionParser.estb[shotid]->palettes) {
        this->palettes.push_back(this->optPals.GetEntry(p->ID)->data);
    }
}

/**
 * Retrieves an RSImageSet object for the specified shape ID.
 *
 * @param shpid The ID of the shape to retrieve.
 *
 * @return A pointer to the RSImageSet object containing the shape data.
 *
 * @throws None.
 */
RSImageSet *SCShot::getShape(uint8_t shpid) {
    PakEntry *shapeEntry = this->optShps.GetEntry(shpid);
    PakArchive subPAK;
    if (!shapeEntry) {
        return nullptr;
    }
    subPAK.InitFromRAM("", shapeEntry->data, shapeEntry->size);
    RSImageSet *img = new RSImageSet();
    if (!subPAK.IsReady()) {
        img->InitFromPakEntry(this->optShps.GetEntry(shpid));
    } else {
        img->InitFromSubPakEntry(&subPAK);
    }
    return (img);
}

/**
 * Runs a single frame of the game, updating the game state and rendering the graphics.
 *
 * @param None
 *
 * @return None
 *
 * @throws None
 */
void SCShot::runFrame(void) {
    if (!this->layers.size()) {
        Game->stopTopActivity();
        return;
    }
    checkKeyboard();
    VGA.activate();
    VGA.getFrameBuffer()->fillWithColor(0);
    ByteStream paletteReader;
    int fpsupdate = 0;
    fpsupdate = (SDL_GetTicks() / 10) - this->fps > 12;
    if (fpsupdate) {
        this->fps = (SDL_GetTicks() / 10);
    }

    bool ended = false;
    int cpt = 0;
    for (auto layer : this->layers) {
        if (cpt < this->palettes.size()) {
            paletteReader.Set((this->palettes[cpt]), 772);
            this->palette.ReadPatch(&paletteReader);
            VGA.setPalette(&this->palette);
        }

        if (layer->img->palettes.size() >= 1) {
            layer->img->palettes[0]->copyFrom(&this->palette);
            VGA.setPalette(layer->img->palettes[0]->GetColorPalette());
        } else {
            VGA.setPalette(&this->palette);
        }
        VGA.getFrameBuffer()->drawShape(layer->img->GetShape(layer->img->sequence[layer->frameCounter]));
        if (layer->img->sequence.size() > 1) {
            layer->frameCounter = layer->frameCounter + fpsupdate;
            if (layer->frameCounter >= layer->img->sequence.size()) {
                ended = true;
            }
        }
        cpt++;
    }
    for (size_t i = 0; i < CONV_TOP_BAR_HEIGHT; i++)
        VGA.getFrameBuffer()->fillLineColor(i, 0x00);

    for (size_t i = 0; i < CONV_BOTTOM_BAR_HEIGHT; i++)
        VGA.getFrameBuffer()->fillLineColor(199 - i, 0x00);
    Mouse.draw();

    VGA.vSync();
    if (ended) {
        Game->stopTopActivity();
    }
}

void EndMissionScene::init() {
    // opt shp id 143
    // opt shp id 141
    SCShot::init();
    this->scoringSprites = this->getShape(142);
    this->layers.clear();
    shotBackground *tmpbg = new shotBackground();
    tmpbg->img = this->getShape(143);
    if (tmpbg->img != nullptr) {   
        this->layers.push_back(tmpbg);
    }
    tmpbg = new shotBackground();
    tmpbg->img = this->getShape(141);
    if (tmpbg->img != nullptr) {
        this->layers.push_back(tmpbg);
    }
    
    PakEntry *paletteEntry = this->optPals.GetEntry(20);
    this->rawPalette = nullptr;
    if (paletteEntry != nullptr) {
        this->rawPalette = this->optPals.GetEntry(20)->data;
    }
    PakEntry *paletteEntry2 = this->optPals.GetEntry(19);
    if (paletteEntry2 != nullptr) {
        this->p2 = new VGAPalette();
        ByteStream paletteReader;
        paletteReader.Set((this->optPals.GetEntry(19)->data), 772);
        this->p2->ReadPatch(&paletteReader);
    }
}

void EndMissionScene::runFrame() {
    if (this->rawPalette == nullptr) {
        Game->stopTopActivity();
        return;
    }
    checkKeyboard();
    int fpsupdate = 0;
    fpsupdate = (SDL_GetTicks() / 10) - this->fps > 12;
    if (fpsupdate) {
        this->fps = (SDL_GetTicks() / 10);
    }
    Mixer.switchBank(0);
    Mixer.playMusic(27, 1);
    ByteStream paletteReader;
    paletteReader.Set((this->rawPalette), 772);
    this->palette.ReadPatch(&paletteReader);
    VGA.activate();
    VGA.getFrameBuffer()->fillWithColor(0);
    VGA.setPalette(&this->palette);
    shotBackground *layer = this->layers[this->part];
    switch (this->part) {
        case 0:
            VGA.getFrameBuffer()->drawShape(layer->img->GetShape(layer->img->sequence[1]));
            VGA.getFrameBuffer()->drawShape(layer->img->GetShape(layer->img->sequence[layer->frameCounter]));
            if (layer->img->sequence.size() > 1) {
                layer->frameCounter = (uint8_t)(layer->frameCounter + fpsupdate) % layer->img->sequence.size();
            }
            break;
        case 1:
            {
                VGA.setPalette(this->p2);
                VGA.getFrameBuffer()->drawShape(layer->img->GetShape(layer->img->sequence[0]));
                SCState &state = SCState::getInstance();
                int air_kill    = state.kill_board[PilotsId::PLAYER][KillBoardType::AIR_KILL];
                int ground_kill = state.kill_board[PilotsId::PLAYER][KillBoardType::GROUND_KILL];

                int mark_w = 16;
                int mark_h = 16;

                // seq_base : index du sprite ×1 pour ce type de kill (1=air, 4=ground)
                auto drawKills = [&](int kills, Point2D base_pos, int seq_base) {
                    int remaining = kills;
                    int col = 0, row = 0;
                    while (remaining > 0) {
                        if (col >= 8) { col = 0; row++; }

                        int seq_idx, marks_used;
                        if (remaining >= 10) { 
                            seq_idx = seq_base + 2;
                            marks_used = 10; 
                        } else if (remaining >= 5) {
                            seq_idx = seq_base + 1;
                            marks_used = 5;  
                        } else { 
                            seq_idx = seq_base;
                            marks_used = 1;
                        }

                        Point2D pos = { 
                            base_pos.x + col * mark_w,
                            base_pos.y + row * mark_h 
                        };
                        RLEShape *shape = this->scoringSprites->GetShape(this->scoringSprites->sequence[seq_idx]);
                        shape->SetPosition(&pos);
                        VGA.getFrameBuffer()->drawShape(shape);

                        remaining -= marks_used;
                        col++;
                    }
                };

                Point2D air_pos    = { 110, 75 };
                Point2D ground_pos = { 110, 75 + mark_h + 4 };
                drawKills(air_kill,    air_pos,    1);  // sprites 1,2,3
                drawKills(ground_kill, ground_pos, 4);  // sprites 4,5,6
            }
            break;
    }
    
    Mouse.draw();
    VGA.vSync();
}
void EndMissionScene::checkKeyboard(void) {
    if (!Game)
        return;
    Keyboard* kb = Game->getKeyboard();
    if (!kb)
        return;
    if (kb->isActionJustPressed(InputAction::KEY_ESCAPE)) {
        Game->stopTopActivity();
    }
    if (kb->isActionJustPressed(InputAction::KEY_RETURN) || kb->isActionJustPressed(InputAction::KEY_SPACE)) {
        if (this->part == 0) {
            this->part = 1;
        } else {
            Game->stopTopActivity();
        }
    }
    if (Mouse.buttons[MouseButton::LEFT].event == MouseButton::PRESSED) {
        Mouse.buttons[MouseButton::LEFT].event = MouseButton::NONE;
        if (this->part == 0) {
            this->part = 1;
        } else {
            Game->stopTopActivity();
        }
    }
}

void MapShot::init() {
    SCShot::init();
    x_max = 960;
    y_max = 600;
    this->frameBuffer = new FrameBuffer(x_max, y_max);
    this->frameBuffer->fillWithColor(0);
    this->frameBufferA = new FrameBuffer(x_max, y_max);
    this->frameBufferA->fillWithColor(255);
    this->layers.clear();
    this->rawPalette = this->optPals.GetEntry(23)->data;
    std::unordered_map<uint8_t, std::tuple<uint16_t, uint16_t>> map = {
        {128, {177, 37}},
        {129, {177, 237}},
        {130, {497, 37}},
        {131, {497, 237}},
        {132, {0, 200}},
        {133, {177, 0}},
        {134, {0, 0}}
    };
    std::vector<uint8_t> shpids_to_paint = {134, 132, 133, 128, 129, 130, 131};

    for (auto i : shpids_to_paint) {
        shotBackground *tmpbg = new shotBackground();
        tmpbg->img = this->getShape(i);
        this->layers.push_back(tmpbg);
        uint8_t *tmp_fb = new uint8_t[320 * 200];
        size_t read = 0;
        tmpbg->img->GetShape(tmpbg->img->sequence[0])->Expand(tmp_fb, &read);
        this->mapics.push_back(tmp_fb);
        this->frameBuffer->blit(tmp_fb, std::get<0>(map[i]), std::get<1>(map[i]), 320, 200);
    }
    
    this->font = FontManager.GetFont("..\\..\\DATA\\FONTS\\OPTFONT.SHP");
}
void MapShot::SetPoints(std::vector<MAP_POINT *> *points) { 
    this->points = points;
    point_counter = 0;
    next_point_counter = 1;

    float dx = (float)((*this->points)[next_point_counter]->x - (*this->points)[point_counter]->x);
    float dy = (float)((*this->points)[next_point_counter]->y - (*this->points)[point_counter]->y);
    float dist = sqrtf(dx * dx + dy * dy);
    const float SPEED = 0.5f; // pixels par étape
    this->nb_etapes = (int)(dist / SPEED);
    if (this->nb_etapes < 1) this->nb_etapes = 1;

    point_x_move = dx / (float)this->nb_etapes;
    point_y_move = dy / (float)this->nb_etapes;
    current_x = (float)(*this->points)[point_counter]->x;
    current_y = (float)(*this->points)[point_counter]->y;

    this->cam_x = (float)(std::max)(0, (std::min)((int)current_x - 160, x_max - 320));
    this->cam_y = (float)(std::max)(0, (std::min)((int)current_y - 100, y_max - 200));
    this->x = (int)this->cam_x;
    this->y = (int)this->cam_y;
}

void MapShot::runFrame(void) {
    checkKeyboard();
    ByteStream paletteReader;
    paletteReader.Set((this->rawPalette), 772);
    this->palette.ReadPatch(&paletteReader);
    VGA.activate();
    VGA.getFrameBuffer()->fillWithColor(255);
    VGA.setPalette(&this->palette);
    
    this->fp_counter++;

    bool ended = false;
    
    if (this->nb_etapes < 0) {
        point_counter++;
        if (point_counter >= this->points->size()) {
            point_counter = 0;
        }
        next_point_counter = point_counter + 1;
        if (next_point_counter >= this->points->size()) {
            next_point_counter = 0;
            ended = true;
        }
        current_x = (float)(*this->points)[point_counter]->x;
        current_y = (float)(*this->points)[point_counter]->y;

        
        float dx = (float)((*this->points)[next_point_counter]->x - (*this->points)[point_counter]->x);
        float dy = (float)((*this->points)[next_point_counter]->y - (*this->points)[point_counter]->y);
        float dist = sqrtf(dx * dx + dy * dy);
        const float SPEED = 0.5f;
        this->nb_etapes = (int)(dist / SPEED);
        if (this->nb_etapes < 1) this->nb_etapes = 1;

        point_x_move = dx / (float)this->nb_etapes;
        point_y_move = dy / (float)this->nb_etapes;
    }

    float dt = GameTimer::getInstance().getDeltaTime();
    this->move_accumulator += dt;

    const float MOVE_INTERVAL = 1.0f / 30.0f;
    if (this->move_accumulator >= MOVE_INTERVAL) {
        this->move_accumulator -= MOVE_INTERVAL;
        this->nb_etapes--;
        original_pos.x = current_x;
        original_pos.y = current_y;
        current_x += point_x_move;
        current_y += point_y_move;
    }

    const int MARGIN_X = 80;
    const int MARGIN_Y = 50;
    int screen_x = (int)current_x - this->x;
    int screen_y = (int)current_y - this->y;

    // Les marges déclenchent une cible, mais on part de la position courante par défaut
    float target_x = this->cam_x;
    float target_y = this->cam_y;

    if (screen_x < MARGIN_X) {
        target_x = current_x - MARGIN_X;
    } else if (screen_x > 320 - MARGIN_X) {
        target_x = current_x - (320 - MARGIN_X);
    }
    if (screen_y < MARGIN_Y) {
        target_y = current_y - MARGIN_Y;
    } else if (screen_y > 200 - MARGIN_Y) {
        target_y = current_y - (200 - MARGIN_Y);
    }

    target_x = (std::max)(0.0f, (std::min)(target_x, (float)(x_max - 320)));
    target_y = (std::max)(0.0f, (std::min)(target_y, (float)(y_max - 200)));

    const float CAM_SPEED = 0.25f;
    float lerp_t = 1.0f - expf(-CAM_SPEED * dt);
    this->cam_x += (target_x - this->cam_x) * lerp_t;
    this->cam_y += (target_y - this->cam_y) * lerp_t;
    this->x = (int)this->cam_x;
    this->y = (int)this->cam_y;
    //frameBufferA->framebuffer[(int)current_y * x_max + (int)current_x] = 246;
    if (original_pos.x != 0 && original_pos.y != 0) {
        frameBufferA->lineThick(original_pos.x, original_pos.y, current_x, current_y, 246, 3);
    }
    VGA.getFrameBuffer()->blitLargeBuffer(frameBuffer->framebuffer, x_max, y_max, this->x, this->y, 0, 0, 320, 200);
    float rx = (this->x / (x_max / 320.0f));
    float ry = (this->y / (y_max / 200.0f)) + 100;
    
    VGA.getFrameBuffer()->blitLargeBuffer(frameBufferA->framebuffer, x_max, y_max, this->x, this->y, 0, 0, 320, 200);
    Mouse.draw();
    for (auto p: *this->points) {
        //VGA.getFrameBuffer()->plot_pixel((int)p->x, (int)p->y, 0);
        //VGA.getFrameBuffer()->line((int)original_pos.x, (int)original_pos.y, (int)p->x, (int)p->y, 0);
        VGA.getFrameBuffer()->printText_SM(
            this->font,
            new Point2D{p->x-this->x, p->y-this->y},
            (char*)p->label.c_str(),
            90,
            0,
            (uint32_t) p->label.size(),
            1,
            this->font->GetShapeForChar('A')->GetWidth(),
            false, false
        );
    }
    VGA.vSync();
    if (ended) {
        Game->stopTopActivity();
    }
}