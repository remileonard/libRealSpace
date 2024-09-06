//
//  SCGameFlow.cpp
//  libRealSpace
//
//  Created by Rémi LEONARD on 16/08/2024.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//

#include "precomp.h"

#include <imgui.h>
#include <imgui_impl_opengl2.h>
#include <imgui_impl_sdl2.h>

#define EFECT_OPT_CONV 0
#define EFECT_OPT_SCEN 1
#define EFECT_OPT_MISS 22
#define EFECT_OPT_MIS2 13
#define EFECT_OPT_SHOT 8
#define EFECT_OPT_FLYM 12

/**
 * \brief Test if a 2D point is inside a quad.
 *
 * This is the "Ray Casting Algorithm" also known as the "Even-Odd Rule Algorithm"
 * (https://en.wikipedia.org/wiki/Point_in_polygon#Ray_casting_algorithm).
 *
 * \param p The point to test.
 * \param quad The quad to test against.
 * \return true if the point is inside the quad, false otherwise.
 */
bool isPointInQuad(const Point2D &p, const std::vector<Point2D *> *quad) {
    int intersections = 0;
    for (size_t i = 0; i < quad->size(); ++i) {
        Point2D a = *quad->at(i);
        Point2D b = *quad->at((i + 1) % quad->size());

        if ((a.y > p.y) != (b.y > p.y)) {
            double atX = (double)(b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x;
            if (p.x < atX) {
                intersections++;
            }
        }
    }
    return (intersections % 2) != 0;
}

/**
 * SCGameFlow constructor.
 *
 * Initializes the SCGameFlow object with default values.
 *
 * @return None
 *
 * @throws None
 */
SCGameFlow::SCGameFlow() {
    this->current_miss = 0;
    this->current_scen = 0;
    this->efect = nullptr;
    this->currentOptCode = 0;
    this->fps = SDL_GetTicks() / 10;
}

SCGameFlow::~SCGameFlow() {}

/**
 * Handles a click event on a sprite.
 *
 * @param id The ID of the sprite that was clicked.
 *
 * @return None
 */
void SCGameFlow::clicked(uint8_t id) {
    if (this->sprites[id]->efect != nullptr) {
        printf("clicked on %d\n", id);
        this->efect = this->sprites[id]->efect;
        this->currentSpriteId = id;
        this->currentOptCode = 0;
        this->runEffect();
    }
}
SCZone *SCGameFlow::CheckZones(void) {
    for (size_t i = 0; i < zones.size(); i++) {

        SCZone *zone = zones[i];
        if (zone->quad != nullptr) {
            if (isPointInQuad(Mouse.GetPosition(), zone->quad)) {
                Mouse.SetMode(SCMouse::VISOR);

                // If the mouse button has just been released: trigger action.
                if (Mouse.buttons[MouseButton::LEFT].event == MouseButton::RELEASED)
                    zone->OnAction();
                Point2D p = {160 - static_cast<int32_t>(zone->label->length() / 2) * 8, 180};
                VGA.DrawText(FontManager.GetFont(""), &p, (char *)zone->label->c_str(), 64, 0,
                              static_cast<uint32_t>(zone->label->length()), 3, 5);
                return zone;
            }
        }
        if (Mouse.GetPosition().x > zone->position.x && Mouse.GetPosition().x < zone->position.x + zone->dimension.x &&
            Mouse.GetPosition().y > zone->position.y && Mouse.GetPosition().y < zone->position.y + zone->dimension.y) {
            // HIT !
            Mouse.SetMode(SCMouse::VISOR);

            // If the mouse button has just been released: trigger action.
            if (Mouse.buttons[MouseButton::LEFT].event == MouseButton::RELEASED)
                zone->OnAction();
            Point2D p = {160 - ((int32_t)(zone->label->length() / 2) * 8), 180};
            VGA.DrawText(FontManager.GetFont(""), &p, (char *)zone->label->c_str(), 64, 0,
                          static_cast<uint32_t>(zone->label->length()), 3, 5);

            return zone;
        }
    }

    Mouse.SetMode(SCMouse::CURSOR);
    return NULL;
    return nullptr;
}
/**
 * Runs the current effect based on the efect vector and currentOptCode.
 *
 * Iterates through the efect vector, executing different actions based on the opcode.
 * Actions include playing conversations, scenes, and missions, as well as handling unknown opcodes.
 *
 * @return None
 */
void SCGameFlow::runEffect() {
    if (this->currentSpriteId > 0 && this->sprites.size() > 0 && this->sprites[this->currentSpriteId]->cliked) {
        // play the animation first
        return;
    }
    if (this->efect == nullptr) {
        return;
    }
    for (int i = 0; i < this->efect->size(); i++) {
        this->currentOptCode++;
        switch (this->efect->at(i)->opcode) {
        case EFECT_OPT_CONV: {
            printf("PLAYING CONV %d\n", this->efect->at(i)->value);
            SCConvPlayer *conv = new SCConvPlayer();
            conv->Init();
            conv->SetID(this->efect->at(i)->value);
            this->convs.push(conv);
            
        } break;
        case EFECT_OPT_SCEN:
            for (int j = 0; j < this->gameFlowParser.game.game[this->current_miss]->scen.size(); j++) {
                if (this->gameFlowParser.game.game[this->current_miss]->scen.at(j)->info.ID ==
                    this->efect->at(i)->value) {
                    this->current_scen = j;
                    this->currentSpriteId = 0;
                    printf("PLAYING SCEN %d\n", this->current_scen);
                    this->createScen();
                }
            }
            break;
        case EFECT_OPT_MISS: {
            this->next_miss = this->efect->at(i)->value;
            printf("NEXT MISS %d\n", this->next_miss);
        } break;
        case EFECT_OPT_MIS2:
            //this->next_miss = this->efect->at(i)->value;
            printf("PLAYING MIS2 %d\n", this->efect->at(i)->value);
            break;
        case EFECT_OPT_SHOT: {
            printf("PLAYING SHOT %d\n", this->efect->at(i)->value);
            SCShot *sht = new SCShot();
            sht->Init();
            sht->SetShotId(this->efect->at(i)->value);
            this->cutsenes.push(sht);
        }
        break;
        case EFECT_OPT_FLYM: {
            uint8_t flymID = this->efect->at(i)->value;
            printf("PLAYING FLYM %d\n", flymID);
            printf("Mission Name %s\n", this->gameFlowParser.game.mlst->data[flymID]->c_str());
            this->missionToFly = (char *)malloc(13);
            sprintf(this->missionToFly, "%s.IFF", this->gameFlowParser.game.mlst->data[flymID]->c_str());
            strtoupper(this->missionToFly, this->missionToFly);
            SCStrike *fly = new SCStrike();
            fly->Init();
            fly->SetMission(this->missionToFly);
            this->missionToFly = nullptr;
            fly_mission.push(fly);
        } break;
        default:
            printf("Unkown opcode :%d, %d\n", this->efect->at(i)->opcode, this->efect->at(i)->value);
            break;
        };

    } 
    this->efect = nullptr;
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
void SCGameFlow::CheckKeyboard(void) {
    // Keyboard
    SDL_Event keybEvents[1];
    int numKeybEvents = SDL_PeepEvents(keybEvents, 1, SDL_PEEKEVENT, SDL_KEYDOWN, SDL_KEYDOWN);
    for (int i = 0; i < numKeybEvents; i++) {
        SDL_Event *event = &keybEvents[i];

        switch (event->key.keysym.sym) {
        case SDLK_ESCAPE: {
            Game.StopTopActivity();
            break;
        }
        case SDLK_SPACE:
            this->current_miss++;
            this->current_scen = 0;
            if (this->current_miss >= this->gameFlowParser.game.game.size()) {
                this->current_miss = 0;
            }
            this->efect = nullptr;
            this->currentOptCode = 0;
            this->createMiss();
            break;
        case SDLK_RETURN:
            this->current_scen++;
            if (this->current_scen >= this->gameFlowParser.game.game[this->current_miss]->scen.size()) {
                this->current_scen = 0;
                this->current_miss++;
                if (this->current_miss >= this->gameFlowParser.game.game.size()) {
                    this->current_miss = 0;
                }
            }
            this->createMiss();
            return;
            break;
        default:
            break;
        }
    }
}

/**
 * Initializes the SCGameFlow object by loading the necessary IFF files and initializing the optionParser and
 * gameFlowParser. It also initializes the optShps and optPals objects by loading the OPTSHPS.PAK and OPTPALS.PAK files
 * respectively. Finally, it sets the efect pointer to nullptr and calls the createMiss() function.
 *
 * @throws None
 */
void SCGameFlow::Init() {

    TreEntry *gameflowIFF =
        Assets.tres[AssetManager::TRE_GAMEFLOW]->GetEntryByName("..\\..\\DATA\\GAMEFLOW\\GAMEFLOW.IFF");
    TreEntry *optionIFF =
        Assets.tres[AssetManager::TRE_GAMEFLOW]->GetEntryByName("..\\..\\DATA\\GAMEFLOW\\OPTIONS.IFF");
    this->optionParser.InitFromRam(optionIFF->data, optionIFF->size);
    this->gameFlowParser.InitFromRam(gameflowIFF->data, gameflowIFF->size);

    TreEntry *optShapEntry =
        Assets.tres[AssetManager::TRE_GAMEFLOW]->GetEntryByName("..\\..\\DATA\\GAMEFLOW\\OPTSHPS.PAK");
    this->optShps.InitFromRAM("OPTSHPS.PAK", optShapEntry->data, optShapEntry->size);
    TreEntry *optPalettesEntry =
        Assets.tres[AssetManager::TRE_GAMEFLOW]->GetEntryByName("..\\..\\DATA\\GAMEFLOW\\OPTPALS.PAK");
    this->optPals.InitFromRAM("OPTPALS.PAK", optPalettesEntry->data, optPalettesEntry->size);
    this->efect = nullptr;
    this->createMiss();
}

/**
 * Creates a new miss in the game flow.
 *
 * This function initializes the efect, sprites, and zones for the current miss.
 * It also sets up the layer, raw palette, and foreground palette.
 *
 * @return None
 *
 * @throws None
 */
void SCGameFlow::createMiss() {
    this->missionToFly = nullptr;
    this->next_miss = 0;
    this->sprites.clear();
    this->zones.clear();
    this->layers.clear();
    this->convs = std::queue<SCConvPlayer *>();
    this->cutsenes = std::queue<SCShot *>();
    this->fly_mission = std::queue<SCStrike *>();
    printf("current miss : %d, current_scen %d\n", this->current_miss, this->current_scen);
    if (this->gameFlowParser.game.game[this->current_miss]->efct != nullptr) {
        this->efect = this->gameFlowParser.game.game[this->current_miss]->efct;
    }

    printf("efect size %zd\n", this->efect->size());
    this->createScen();
    this->runEffect();
}
/**
 * Creates a new scene in the current miss.
 *
 * This function initializes the layers, sprites, and zones for the current scene.
 * It also sets up the raw palette and foreground palette.
 *
 * @return None
 *
 * @throws None
 */
void SCGameFlow::createScen() {
    if (this->gameFlowParser.game.game[this->current_miss]->scen.size() > 0) {
        uint8_t optionScenID = this->gameFlowParser.game.game[this->current_miss]->scen[this->current_scen]->info.ID;
        // note pour plus tard, une scene peu être composé de plusieur background
        // donc il faut boucler.
        SCEN *sceneOpts = this->optionParser.opts[optionScenID];

        for (auto bg : sceneOpts->background->images) {
            background *tmpbg = new background();
            tmpbg->img = this->getShape(bg->ID);
            this->layers.push_back(tmpbg);
        }
        uint8_t paltID = sceneOpts->background->palette->ID;
        uint8_t forPalTID = sceneOpts->foreground->palette->ID;

        this->sprites.clear();
        this->zones.clear();
        for (auto sprite : this->gameFlowParser.game.game[this->current_miss]->scen[this->current_scen]->sprt) {
            uint8_t sprtId = sprite->info.ID;
            // le clck dans sprite semble indiquer qu'il faut jouer l'animation après avoir cliquer et donc executer le
            // efect à la fin de l'animation.
            if (sceneOpts->foreground->sprites.count(sprtId) > 0) {
                uint8_t optsprtId = sceneOpts->foreground->sprites[sprtId]->sprite.SHP_ID;
                animatedSprites *sprt = new animatedSprites();

                if (sceneOpts->foreground->sprites[sprtId]->zone != nullptr) {
                    sprt->rect = new sprtRect();
                    sprt->rect->x1 = sceneOpts->foreground->sprites[sprtId]->zone->X1;
                    sprt->rect->y1 = sceneOpts->foreground->sprites[sprtId]->zone->Y1;
                    sprt->rect->x2 = sceneOpts->foreground->sprites[sprtId]->zone->X2;
                    sprt->rect->y2 = sceneOpts->foreground->sprites[sprtId]->zone->Y2;
                    SCZone *z = new SCZone();
                    z->id = sprtId;
                    z->position.x = sprt->rect->x1;
                    z->position.y = sprt->rect->y1;
                    z->dimension.x = sprt->rect->x2 - sprt->rect->x1;
                    z->dimension.y = sprt->rect->y2 - sprt->rect->y1;
                    z->label = sceneOpts->foreground->sprites[sprtId]->label;
                    z->onclick = std::bind(&SCGameFlow::clicked, this, std::placeholders::_1);
                    z->quad = nullptr;
                    this->zones.push_back(z);
                }
                if (sceneOpts->foreground->sprites[sprtId]->quad != nullptr) {
                    sprt->quad = new std::vector<Point2D *>();

                    Point2D *p;

                    p = new Point2D();
                    p->x = sceneOpts->foreground->sprites[sprtId]->quad->xa1;
                    p->y = sceneOpts->foreground->sprites[sprtId]->quad->ya1;
                    sprt->quad->push_back(p);

                    p = new Point2D();
                    p->x = sceneOpts->foreground->sprites[sprtId]->quad->xa2;
                    p->y = sceneOpts->foreground->sprites[sprtId]->quad->ya2;
                    sprt->quad->push_back(p);

                    p = new Point2D();
                    p->x = sceneOpts->foreground->sprites[sprtId]->quad->xb1;
                    p->y = sceneOpts->foreground->sprites[sprtId]->quad->yb1;
                    sprt->quad->push_back(p);

                    p = new Point2D();
                    p->x = sceneOpts->foreground->sprites[sprtId]->quad->xb2;
                    p->y = sceneOpts->foreground->sprites[sprtId]->quad->yb2;
                    sprt->quad->push_back(p);

                    SCZone *z = new SCZone();
                    z->id = sprtId;
                    z->quad = sprt->quad;
                    z->label = sceneOpts->foreground->sprites[sprtId]->label;
                    z->onclick = std::bind(&SCGameFlow::clicked, this, std::placeholders::_1);
                    this->zones.push_back(z);
                }

                sprt->id = sprite->info.ID;
                sprt->shapid = optsprtId;
                sprt->unkown = sprite->info.UNKOWN;
                sprt->efect = sprite->efct;
                sprt->img = this->getShape(optsprtId);
                sprt->frameCounter = 0;
                if (sceneOpts->foreground->sprites[sprtId]->CLCK == 1) {
                    sprt->cliked = true;
                    sprt->frameCounter = 1;
                }
                if (sceneOpts->foreground->sprites[sprtId]->SEQU != nullptr) {
                    sprt->frames = sceneOpts->foreground->sprites[sprtId]->SEQU;
                    sprt->frameCounter = 0;
                }
                this->sprites[sprtId] = sprt;
            } else {
                printf("%d, ID Sprite not found !!\n", sprtId);
            }
        }

        this->rawPalette = this->optPals.GetEntry(paltID)->data;
        this->forPalette = this->optPals.GetEntry(forPalTID)->data;
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
RSImageSet *SCGameFlow::getShape(uint8_t shpid) {
    PakEntry *shapeEntry = this->optShps.GetEntry(shpid);
    PakArchive subPAK;

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
void SCGameFlow::RunFrame(void) {
    
    if (this->cutsenes.size() > 0) {
        SCShot *c = this->cutsenes.front();
        this->cutsenes.pop();
        Game.AddActivity(c);
        return;
    }
    if (this->convs.size() > 0) {
        SCConvPlayer *c = this->convs.front();
        this->convs.pop();
        Game.AddActivity(c);
        return;
    }
    if (this->fly_mission.size() > 0) {
        SCStrike *c = this->fly_mission.front();
        this->fly_mission.pop();
        Game.AddActivity(c);
        return;
    }
    if (this->next_miss>0) {
        this->current_miss = this->next_miss;
        this->next_miss = 0;
        this->current_scen = 0;
        this->createMiss();
        return;
    }
    
    int fpsupdate = 0;
    fpsupdate = (SDL_GetTicks() / 10) - this->fps > 12;
    if (fpsupdate) {
        this->fps = (SDL_GetTicks() / 10);
    }
    CheckKeyboard();
    VGA.Activate();
    VGA.Clear();
    ByteStream paletteReader;
    paletteReader.Set((this->rawPalette));
    this->palette.ReadPatch(&paletteReader);
    VGA.SetPalette(&this->palette);
    for (auto layer : this->layers) {
        VGA.DrawShape(layer->img->GetShape(layer->img->sequence[layer->frameCounter]));
        if (layer->img->sequence.size() > 1) {
            layer->frameCounter = (uint8_t)(layer->frameCounter + fpsupdate) % layer->img->sequence.size();
        }
    }

    paletteReader.Set((this->forPalette));
    this->palette.ReadPatch(&paletteReader);
    VGA.SetPalette(&this->palette);
    if (this->currentSpriteId > 0 && this->sprites.size() > 0 && this->sprites[this->currentSpriteId]->cliked) {
        if (this->sprites[this->currentSpriteId]->frameCounter <
            this->sprites[this->currentSpriteId]->img->sequence.size() - 1) {
            if (fpsupdate) {
                this->sprites[this->currentSpriteId]->frameCounter++;
            }
        } else {
            this->sprites[this->currentSpriteId]->cliked = false;
            this->currentSpriteId = 0;
            this->runEffect();
        }
    }
    for (const auto &sprit : this->sprites) {
        VGA.DrawShape(sprit.second->img->GetShape(sprit.second->img->sequence[0]));
        if (sprit.second->img->GetNumImages() > 1 && sprit.second->frames != nullptr) {
            VGA.DrawShape(sprit.second->img->GetShape(sprit.second->frames->at(sprit.second->frameCounter)));
            sprit.second->frameCounter =
                (sprit.second->frameCounter + fpsupdate) % static_cast<uint8_t>(sprit.second->frames->size());

        } else if (sprit.second->img->sequence.size() > 1 && sprit.second->frames == nullptr &&
                   sprit.second->cliked == false) {

            if (sprit.second->frameCounter >= sprit.second->img->sequence.size()) {
                sprit.second->frameCounter = 1;
            }
            VGA.DrawShape(sprit.second->img->GetShape(sprit.second->img->sequence[sprit.second->frameCounter]));
            sprit.second->frameCounter += fpsupdate;
        } else if (sprit.second->img->sequence.size() > 1 && sprit.second->frames == nullptr &&
                   sprit.second->cliked == true) {
            VGA.DrawShape(sprit.second->img->GetShape(sprit.second->img->sequence[sprit.second->frameCounter]));
        }
    }
    this->CheckZones();

    for (int f = 0; f < this->zones.size(); f++) {
        this->zones.at(f)->Draw();
    }

    VGA.VSync();

    this->RenderMenu();
    VGA.SwithBuffers();
    VGA.Activate();
    VGA.Clear();
    Mouse.Draw();
    VGA.VSync();
    VGA.SwithBuffers();
}
/**
 * Draw the menu for the game flow.
 *
 * The menu is drawn using ImGui.
 *
 * The menu contains the following elements:
 * - A menu with the following items:
 *   - "Load Miss" which will open a window to select a mission to load.
 *   - "Info" which will open a window with informations about the current miss.
 *   - "Quit" which will stop the current activity.
 * - A text with the current miss number and the current scene number.
 *
 * If the "Load Miss" menu is selected, a window will open with a combo box
 * containing the list of all the missions in the game. When a mission is selected,
 * the current miss number will be updated and the createMiss function will be
 * called.
 *
 * If the "Info" menu is selected, a window will open with informations about the
 * current miss, including the number of scenes, the number of layers, and the
 * number of actors. If the miss has an efect, it will be displayed in a tree view.
 * For each actor, the frame number, the shape id, and the unknown value will be
 * displayed. If the actor has an efect, it will be displayed in a tree view.
 * If the actor has a quad selection area or a rect selection area, it will be
 * displayed.
 *
 * If the "Quit" menu is selected, the current activity will be stopped.
 *
 * The menu is rendered using ImGui::Render and ImGui_ImplOpenGL2_RenderDrawData.
 */
void SCGameFlow::RenderMenu() {
    ImGui::SetMouseCursor(ImGuiMouseCursor_None);
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    static bool show_scene_window = false;
    static bool quit_gameflow = false;
    static bool show_load_miss = false;
    static int miss_selected = 0;
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("GameFlow")) {
            ImGui::MenuItem("Load Miss", NULL, &show_load_miss);
            ImGui::MenuItem("Info", NULL, &show_scene_window);
            ImGui::MenuItem("Quit", NULL, &quit_gameflow);
            ImGui::EndMenu();
        }
        ImGui::Text("Current Miss %d, Current Scen %d", this->current_miss, this->current_scen);
        ImGui::EndMainMenuBar();
    }
    
    if (show_load_miss) {
        ImGui::Begin("Load Miss");
            static ImGuiComboFlags flags = 0;
            static char ** miss = new char * [this->gameFlowParser.game.game.size()];
            for (int i = 0; i < this->gameFlowParser.game.game.size(); i++) {
                miss[i] = new char[10];
                sprintf(miss[i], "%d", i);
            }
            if (ImGui::BeginCombo("List des miss", miss[miss_selected], flags)) {
                for (int i = 0; i < this->gameFlowParser.game.game.size(); i++) {
                    const bool is_selected = (miss_selected == i);
                    if (ImGui::Selectable(std::to_string(i).c_str(), is_selected))
                        miss_selected = i;

                    // Set the initial focus when opening the combo (scrolling +
                    // keyboard navigation focus)
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            if (ImGui::Button("Load Mission")) {
                this->current_miss = miss_selected;
                this->current_scen = 0;
                this->currentSpriteId = 0;
                this->efect = nullptr;
                this->createMiss();
            }
        ImGui::End();
    }
    if (show_scene_window) {
        ImGui::Begin("Infos");
        ImGui::Text("Current Miss %d, Current Scen %d", this->current_miss, this->current_scen);
        ImGui::Text("Nb Miss %d", this->gameFlowParser.game.game.size());
        ImGui::Text("Nb Layers %d", this->layers.size());
        ImGui::Text("Nb Scenes %d", this->gameFlowParser.game.game[this->current_miss]->scen.size());
        if (this->gameFlowParser.game.game[this->current_miss]->efct != nullptr) {
            ImGui::Text("Miss has efct");
            if (ImGui::TreeNode("Miss Efect")) {
                for (auto effect : *this->gameFlowParser.game.game[this->current_miss]->efct) {
                    ImGui::Text("OPC: %03d\tVAL: %03d", effect->opcode, effect->value);
                }
                ImGui::TreePop();
            }
        }
        ImGui::Text("Nb Actor %d", this->sprites.size());
        if (ImGui::TreeNode("Actors")) {
            int i = 0;
            for (auto sprite : this->sprites) {
                if (ImGui::TreeNode((void *)(intptr_t)i, "Actor %d, Frame %d, %d %d", sprite.first,
                                    sprite.second->frameCounter, sprite.second->shapid, sprite.second->unkown)) {
                    if (sprite.second->frames != nullptr) {
                        ImGui::Text("Frames %d", sprite.second->frames->size());
                    }
                    if (sprite.second->quad != nullptr) {
                        ImGui::Text("Quad selection area");
                    }
                    if (sprite.second->rect != nullptr) {
                        ImGui::Text("Rect selection area");
                    }
                    if (sprite.second->efect->size() > 0) {
                        if (ImGui::TreeNode("Efect")) {
                            for (auto efct: *sprite.second->efect) {
                                ImGui::Text("OPC: %03d\tVAL: %03d", efct->opcode,
                                            efct->value);
                            }
                            ImGui::TreePop();
                        }
                    }
                    ImGui::TreePop();
                }
                i++;
            }
            ImGui::TreePop();
        }

        ImGui::Text("Nb Zones %d", this->zones.size());
        if (this->zones.size() > 1) {
            if (ImGui::TreeNode("Zones")) {
                for (auto zone : this->zones) {
                    if (ImGui::TreeNode((void *)(intptr_t)zone->id, "Zone %d", zone->id)) {
                        ImGui::Text(zone->label->c_str());
                        ImGui::TreePop();
                    }
                }
                ImGui::TreePop();
            }
        }
        ImGui::End();
    }
    if (quit_gameflow) {
        Game.StopTopActivity();
    }
    ImGui::Render();
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
}