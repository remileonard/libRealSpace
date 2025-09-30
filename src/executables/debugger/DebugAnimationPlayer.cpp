#include "DebugAnimationPlayer.h"
#include "DebugUtils.h"
#include <imgui.h>
#include <imgui_impl_opengl2.h>
#include <imgui_impl_sdl2.h>


void displayPalette(VGAPalette *palette) {
    for (int i = 0; i < 256; i++) {
        if (i % 12 == 0 && i != 0) {
            ImGui::NewLine();
        } else if (i != 0) {
            ImGui::SameLine();
        }
        ImGui::ColorButton(("##" + std::to_string(i)).c_str(), ImVec4(
            palette->colors[i].r / 255.0f,
            palette->colors[i].g / 255.0f,
            palette->colors[i].b / 255.0f,
            1.0f
        ), ImGuiColorEditFlags_NoTooltip, ImVec2(20,20));
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Color %d: R:%d G:%d B:%d", i, palette->colors[i].r, palette->colors[i].g, palette->colors[i].b);
        }
    }
}
DebugAnimationPlayer::DebugAnimationPlayer(): SCAnimationPlayer() {
    this->auto_stop = false;
}
DebugAnimationPlayer::~DebugAnimationPlayer() {
}
void DebugAnimationPlayer::renderMenu() {
    ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
    
    static bool show_editor = false;
    static int miss_selected = 0;
    if (ImGui::BeginMenu("MidGames Animations")) {
        ImGui::MenuItem("Edit Animation", NULL, &show_editor);
        ImGui::EndMenu();
    }
    ImGui::Text("Animation Player frame: %d, fps: %d, shot: %d", this->fps_counter, this->fps, this->shot_counter);
    if (show_editor) {
        this->showEditor();
    }
}
void DebugAnimationPlayer::renderUI() {
    static std::vector<GLuint> s_PrevFrameGLTex;
    static std::vector<GLuint> s_CurrentFrameGLTex;
    if (!s_PrevFrameGLTex.empty()) {
        for (GLuint id : s_PrevFrameGLTex) {
            glDeleteTextures(1, &id);
        }
        s_PrevFrameGLTex.clear();
    }
    // Préparer la liste pour cette frame
    s_PrevFrameGLTex.swap(s_CurrentFrameGLTex); // s_CurrentFrameGLTex devient vide

    if (ImGui::BeginTabBar("Animations")) {
        if (ImGui::BeginTabItem("MidGames Data")) {
            if (ImGui::Button("Previous Shot")) {
                this->shot_counter--;
                if (this->shot_counter < 0) {
                    this->shot_counter = this->midgames_shots[1].size() - 1;
                }
                this->fps = 1;
                this->fps_counter = 0;
            }
            ImGui::SameLine();
            if (ImGui::Button("Previous Frame")) {
                this->fps--;
                if (this->fps < 1) {
                    this->fps = 1;
                }
                this->fps_counter--;
                if (this->fps_counter < 0) {
                    this->fps_counter = 0;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button(pause ? "Resume" : "Pause")) {
                this->pause = !this->pause;
            }
            ImGui::SameLine();
            if (ImGui::Button("Next Frame")) {
                this->fps++;
                if (this->fps > this->midgames_shots[1][this->shot_counter]->nbframe) {
                    this->fps = this->midgames_shots[1][this->shot_counter]->nbframe;
                }
                this->fps_counter++;
                if (this->fps_counter > this->midgames_shots[1][this->shot_counter]->nbframe) {
                    this->fps_counter = this->midgames_shots[1][this->shot_counter]->nbframe;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Next Shot")) {
                this->shot_counter++;
                if (this->shot_counter > this->midgames_shots[1].size() - 1) {
                    this->shot_counter = 0;
                }
                this->fps = 1;
                this->fps_counter = 0;
            }
            ImGui::Separator();
            ImGui::Text("Total shots: %d", this->midgames_shots[1].size());
            if (this->midgames_shots[1].size() > 0 && this->shot_counter < this->midgames_shots[1].size()) {
                MIDGAME_SHOT *shot = this->midgames_shots[1][this->shot_counter];
                ImGui::Text("Shot %d/%d", this->shot_counter + 1, this->midgames_shots[1].size());
                ImGui::Text("Number of frames: %d", shot->nbframe);
                ImGui::Text("Music ID: %d", shot->music);
                if (shot->sound != nullptr) {
                    ImGui::Text("Sound available to play at frame %d", shot->sound_time_code);
                } else {
                    ImGui::Text("No sound available");
                }
                ImGui::Text("Backgrounds: %d", shot->background.size());
                for (size_t i = 0; i < shot->background.size(); i++) {
                    MIDGAME_SHOT_BG *bg = shot->background[i];
                    if (ImGui::TreeNode(("Background " + std::to_string(i+1)).c_str())) {
                        ImGui::Text("Background %d:", i+1);
                        ImGui::Text("ShapeID: %d",bg->shapeid);
                        ImGui::Text("Pos Start: (%d,%d)",bg->position_start.x, bg->position_start.y);
                        ImGui::Text("Pos End: (%d,%d)", bg->position_end.x, bg->position_end.y);
                        ImGui::Text("Velocity: (%d,%d)",bg->velocity.x, bg->velocity.y);
                        ImGui::Text("PaletteID: %d, HasPal: %s", bg->palette, bg->pal != nullptr ? "Yes" : "No");
                        if (bg->pal != nullptr) {
                            ImGui::Text("Palette Colors:");
                            displayPalette(bg->pal->GetColorPalette());
                        }
                        if (bg->image != nullptr) {
                            ImGui::Text("Image has %d images", bg->image->GetNumImages());
                            if (this->fps_counter < shot->nbframe) {
                                int frame_to_show = this->fps_counter % bg->image->GetNumImages();
                                ImGui::Text("Showing frame %d", frame_to_show);
                                GLuint glTex = renderShape(bg->image->GetShape(frame_to_show), &this->palette);
                                s_CurrentFrameGLTex.push_back(glTex); // Garder la texture pour la supprimer plus tard
                                ImGui::Image((void*)(uintptr_t)glTex, ImVec2(320,200));
                            } else {
                                ImGui::Text("No frame to show at current fps counter");
                            }
                        } else {
                            ImGui::Text("No image associated");
                        }
                        ImGui::TreePop();
                    }   
                    
                }
                ImGui::Text("Sprites: %d", shot->sprites.size());
                for (size_t i = 0; i < shot->sprites.size(); i++) {
                    MIDGAME_SHOT_SPRITE *sprt = shot->sprites[i];
                    if (ImGui::TreeNode(("Sprite " + std::to_string(i+1)).c_str())) {
                        ImGui::Text("Sprite %d:", i+1);
                        ImGui::Text("NumImages: %d", sprt->image->GetNumImages());
                        ImGui::Text("Pos Start: (%d,%d)", sprt->position_start.x, sprt->position_start.y);
                        ImGui::Text("Pos End: (%d,%d)", sprt->position_end.x, sprt->position_end.y);
                        ImGui::Text("Velocity: (%d,%d)", sprt->velocity.x, sprt->velocity.y);
                        ImGui::Text("Keep first frame: %s", sprt->keep_first_frame ? "Yes" : "No");
                        ImGui::Text("HasPal: %s", sprt->pal != nullptr ? "Yes" : "No");
                        if (sprt->pal != nullptr) {
                            ImGui::Text("Palette Colors:");
                            displayPalette(sprt->pal->GetColorPalette());
                        }
                        if (sprt->image != nullptr) {
                            if (this->fps_counter < shot->nbframe) {
                                int frame_to_show = this->fps_counter % sprt->image->GetNumImages();
                                ImGui::Text("Showing frame %d", frame_to_show);
                                GLuint glTex = renderShape(sprt->image->GetShape(frame_to_show), sprt->pal != nullptr ? sprt->pal->GetColorPalette() : &this->palette);
                                s_CurrentFrameGLTex.push_back(glTex); // Garder la texture pour la supprimer plus tard
                                ImGui::Image((void*)(uintptr_t)glTex, ImVec2(320,200));
                            } else {
                                ImGui::Text("No frame to show at current fps counter");
                            }
                        } else {
                            ImGui::Text("No image associated");
                        }
                        ImGui::TreePop();
                    }
                }
            }
            ImGui::Separator();
            ImGui::Text("Current color palette");
            
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("VGA Pallette")) {
            if (ImGui::Button("Reset to original palette")) {
                this->palette.CopyFromOtherPalette(&this->original_palette, false);
                VGA.setPalette(&this->palette);
            }
            ImGui::Separator();
            ImGui::Text("Original color palette");
            displayPalette(&this->original_palette);
            ImGui::Separator();
            ImGui::Text("Current color palette");
            displayPalette(&this->palette);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Editor")) {
            if (ImGui::Button("New Shot")) {
                
            }
            ImGui::SameLine();
            if (ImGui::Button("Save Shot")) {
                
            }
            ImGui::SameLine();
            if (ImGui::Button("Delete Shot")) {
                
            }
            if (ImGui::Button("Add Background")) {
                
            }
            ImGui::SameLine();
            if (ImGui::Button("Add Sprite")) {
                
            }
            ImGui::SameLine();
            if (ImGui::Button("Add Foreground")) {
                
            }
            ImGui::SameLine();
            if (ImGui::Button("Add Character")) {

            }
            if (ImGui::Button("Add Sound")) {
                
            }ImGui::SameLine();
            if (ImGui::Button("Add Text")) {

            }
            
            ImGui::EndTabItem();
        }
        this->midgameChoser();
        ImGui::Separator();
        ImGui::EndTabBar();
    }
}
void DebugAnimationPlayer::midgameChoser() {
    if (ImGui::BeginCombo("MID Archive", this->current_mid ? this->current_mid->GetName() : "None")) {
        if (ImGui::Selectable("OptSHPs", this->current_mid == &this->optShps)) {
            this->current_mid = &this->optShps;
        }
        if (ImGui::Selectable("MidGames", this->current_mid == &this->midgames)) {
            this->current_mid = &this->midgames;
        }
        for (auto midarchive: this->mid) {
            if (ImGui::Selectable(midarchive->GetName(), midarchive == this->current_mid)) {
                this->current_mid = midarchive;
            }
        }
        ImGui::EndCombo();
    }
    if (this->current_mid != nullptr) {
        ImGui::Text("Archive: %s, NumEntries: %d", this->current_mid->GetName(), this->current_mid->GetNumEntries());
        if (ImGui::BeginCombo("Entries", "Select an entry")) {
            for (int i = 0; i < this->current_mid->GetNumEntries(); i++) {
                PakEntry* entry = this->current_mid->GetEntry(i);
                if (ImGui::Selectable(
                    ("Entry " + std::to_string(i)+ " " + std::to_string(entry->type)).c_str(),
                    i == this->current_entry_index
                )) {
                    // do something when selected
                    this->current_entry_index = i;
                }
            }
            ImGui::EndCombo();
        }

    }
}

void DebugAnimationPlayer::showEditor() {
    bool window_open = true;
    static ImVec2 lastWinSize(0, 0);
    bool isResizing = false;
    if (ImGui::Begin("Shot Editor", &window_open, ImGuiWindowFlags_None | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse)) {
        ImVec2 currentWinSize = ImGui::GetWindowSize();
        if (currentWinSize.x != lastWinSize.x || currentWinSize.y != lastWinSize.y) {
            isResizing = true;
            lastWinSize = currentWinSize;
        }

        if (!isResizing) {
            ImGui::Text("Éditeur de Shots - Timeline");

            if (this->midgames_shots[1].empty()) {
                ImGui::Text("Aucun shot disponible dans l'animation actuelle.");
                ImGui::End();
                return;
            }

            const float shotWidth = 150.0f;
            const float elementHeight = 40.0f;
            const float timelineHeight = 350.0f;
            const float shotSpacing = 15.0f;
            const float elementSpacing = 5.0f;
            const float sectionHeaderHeight = 20.0f;
            const float emptyMessageHeight = 20.0f;
            const float buttonAreaHeight = 30.0f;
            const float textPadding = 5.0f;
            const float shotTitleHeight = 25.0f;

            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 canvasPos = ImGui::GetCursorScreenPos();
            ImVec2 canvasSize = ImVec2(ImGui::GetWindowWidth() - 20, timelineHeight);

            // Dessiner le fond de la timeline
            drawList->AddRectFilled(canvasPos, 
                                ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), 
                                IM_COL32(40, 40, 50, 255));

        
            // Zone scrollable pour la timeline
            ImGui::BeginChild("TimelineScroll", ImVec2(0, timelineHeight), true, ImGuiWindowFlags_HorizontalScrollbar);
            
            ImVec2 timelinePos = ImGui::GetCursorScreenPos();
            float xOffset = 10.0f;

            drawList->PushClipRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x - 15, canvasPos.y + canvasSize.y - 15), true);
            // Dessiner chaque shot
            for (size_t shotIndex = 0; shotIndex < this->midgames_shots[1].size(); shotIndex++) {
                MIDGAME_SHOT* shot = this->midgames_shots[1][shotIndex];
                
                // Calcul précis de la hauteur nécessaire pour ce shot
                float shotHeight = shotTitleHeight; // Commencer avec la hauteur du titre
                
                // Hauteur section backgrounds
                shotHeight += sectionHeaderHeight; // En-tête "Backgrounds:"
                if (shot->background.empty()) {
                    shotHeight += emptyMessageHeight; // "Aucun background"
                } else {
                    shotHeight += shot->background.size() * (elementHeight + elementSpacing);
                }
                
                // Hauteur section sprites
                shotHeight += sectionHeaderHeight; // En-tête "Sprites:"
                if (shot->sprites.empty()) {
                    shotHeight += emptyMessageHeight; // "Aucun sprite"
                } else {
                    shotHeight += shot->sprites.size() * (elementHeight + elementSpacing);
                }
                
                // Hauteur section foregrounds
                shotHeight += sectionHeaderHeight;
                if (shot->foreground.empty()) {
                    shotHeight += emptyMessageHeight;
                } else {
                    shotHeight += shot->foreground.size() * (elementHeight + elementSpacing);
                }
                
                // Espace pour les boutons
                shotHeight += buttonAreaHeight;
                
                // Position du shot actuel
                ImVec2 shotPos = ImVec2(timelinePos.x + xOffset, timelinePos.y + 5.0f);
                
                // Bordure et fond du shot
                ImU32 shotColor = (shotIndex == this->shot_counter) ? 
                                IM_COL32(80, 140, 200, 255) : IM_COL32(70, 70, 80, 255);
                drawList->AddRectFilled(
                    shotPos, 
                    ImVec2(shotPos.x + shotWidth, shotPos.y + shotHeight), 
                    shotColor, 4.0f);
                
                float yOffset = 8.0f;
                // Titre du shot
                char shotTitle[64];
                sprintf(shotTitle, "Shot %zu (%d frames)", shotIndex + 1, shot->nbframe);
                drawList->AddText(
                    ImVec2(shotPos.x + textPadding, shotPos.y + yOffset), 
                    IM_COL32(255, 255, 255, 255), 
                    shotTitle);
                
                yOffset += shotTitleHeight;
                
                // En-tête pour les backgrounds
                drawList->AddText(
                    ImVec2(shotPos.x + textPadding, shotPos.y + yOffset), 
                    IM_COL32(200, 200, 200, 255), 
                    "Backgrounds:");
                yOffset += sectionHeaderHeight;
                
                // Dessiner les backgrounds
                if (shot->background.empty()) {
                    drawList->AddText(
                        ImVec2(shotPos.x + textPadding * 2, shotPos.y + yOffset), 
                        IM_COL32(180, 180, 180, 255), 
                        "Aucun background");
                    yOffset += emptyMessageHeight;
                } else {
                    for (size_t bgIndex = 0; bgIndex < shot->background.size(); bgIndex++) {
                        MIDGAME_SHOT_BG* bg = shot->background[bgIndex];
                        ImVec2 elementPos = ImVec2(shotPos.x + textPadding, shotPos.y + yOffset);
                        
                        // S'assurer que l'élément rentre dans la largeur du shot
                        float elementWidth = shotWidth - (textPadding * 2);
                        
                        drawList->AddRectFilled(
                            elementPos, 
                            ImVec2(elementPos.x + elementWidth, elementPos.y + elementHeight), 
                            IM_COL32(60, 120, 180, 255), 3.0f);
                        
                        // Label pour le background - vérifie qu'il ne déborde pas
                        char bgLabel[32];
                        sprintf(bgLabel, "BG %zu (Shape %d)", bgIndex + 1, bg->pak_entry_id);
                        drawList->AddText(
                            ImVec2(elementPos.x + textPadding, elementPos.y + textPadding), 
                            IM_COL32(255, 255, 255, 255), 
                            bgLabel);
                        
                        // Info de position - en plus petit et tronqué au besoin
                        char posInfo[48];
                        sprintf(posInfo, "(%d,%d)→(%d,%d)", bg->position_start.x, bg->position_start.y, 
                            bg->position_end.x, bg->position_end.y);
                        drawList->AddText(
                            ImVec2(elementPos.x + textPadding, elementPos.y + elementHeight/2), 
                            IM_COL32(220, 220, 220, 255), 
                            posInfo);
                        if (ImGui::IsMouseHoveringRect(elementPos, ImVec2(elementPos.x + elementWidth, elementPos.y + elementHeight))) {
                            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                                this->p_bg_editor = bg;
                                this->p_sprite_editor = nullptr;
                                this->p_foreground_editor = nullptr;
                            }
                        }
                        yOffset += elementHeight + elementSpacing;
                    }
                }
                
                // En-tête pour les sprites
                drawList->AddText(
                    ImVec2(shotPos.x + textPadding, shotPos.y + yOffset), 
                    IM_COL32(200, 200, 200, 255), 
                    "Sprites:");
                yOffset += sectionHeaderHeight;
                
                // Dessiner les sprites
                if (shot->sprites.empty()) {
                    drawList->AddText(
                        ImVec2(shotPos.x + textPadding * 2, shotPos.y + yOffset), 
                        IM_COL32(180, 180, 180, 255), 
                        "Aucun sprite");
                    yOffset += emptyMessageHeight;
                } else {
                    for (size_t spriteIndex = 0; spriteIndex < shot->sprites.size(); spriteIndex++) {
                        MIDGAME_SHOT_SPRITE* sprite = shot->sprites[spriteIndex];
                        ImVec2 elementPos = ImVec2(shotPos.x + textPadding, shotPos.y + yOffset);
                        
                        // S'assurer que l'élément rentre dans la largeur du shot
                        float elementWidth = shotWidth - (textPadding * 2);
                        
                        drawList->AddRectFilled(
                            elementPos, 
                            ImVec2(elementPos.x + elementWidth, elementPos.y + elementHeight), 
                            IM_COL32(180, 120, 60, 255), 3.0f);
                        
                        // Limiter le texte pour qu'il tienne dans la boîte
                        char spriteLabel[48];
                        sprintf(spriteLabel, "Sprite %zu (%d img)", 
                            spriteIndex + 1, 
                            sprite->image ? sprite->image->GetNumImages() : 0);
                        drawList->AddText(
                            ImVec2(elementPos.x + textPadding, elementPos.y + textPadding), 
                            IM_COL32(255, 255, 255, 255), 
                            spriteLabel);
                        
                        // Info de position en plus petit
                        char posInfo[48];
                        sprintf(posInfo, "(%d,%d)→(%d,%d)", sprite->position_start.x, sprite->position_start.y, 
                            sprite->position_end.x, sprite->position_end.y);
                        drawList->AddText(
                            ImVec2(elementPos.x + textPadding, elementPos.y + elementHeight/2), 
                            IM_COL32(220, 220, 220, 255), 
                            posInfo);
                        if (ImGui::IsMouseHoveringRect(elementPos, ImVec2(elementPos.x + elementWidth, elementPos.y + elementHeight))) {
                            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                                this->p_bg_editor = nullptr;
                                this->p_sprite_editor = sprite;
                                this->p_foreground_editor = nullptr;
                            }
                        }
                        yOffset += elementHeight + elementSpacing;
                    }
                }
                
                // Section des foregrounds
                drawList->AddText(
                    ImVec2(shotPos.x + textPadding, shotPos.y + yOffset), 
                    IM_COL32(200, 200, 200, 255), 
                    "Foregrounds:");
                yOffset += sectionHeaderHeight;
                if (shot->foreground.empty()) {
                    drawList->AddText(
                        ImVec2(shotPos.x + textPadding * 2, shotPos.y + yOffset), 
                        IM_COL32(180, 180, 180, 255), 
                        "Pas de foreground");
                    yOffset += emptyMessageHeight;
                } else {
                    for (size_t bgIndex = 0; bgIndex < shot->foreground.size(); bgIndex++) {
                        MIDGAME_SHOT_BG* bg = shot->foreground[bgIndex];
                        ImVec2 elementPos = ImVec2(shotPos.x + textPadding, shotPos.y + yOffset);
                        
                        // S'assurer que l'élément rentre dans la largeur du shot
                        float elementWidth = shotWidth - (textPadding * 2);
                        
                        drawList->AddRectFilled(
                            elementPos, 
                            ImVec2(elementPos.x + elementWidth, elementPos.y + elementHeight), 
                            IM_COL32(220, 120, 180, 255), 3.0f);
                        
                        // Label pour le background - vérifie qu'il ne déborde pas
                        char bgLabel[32];
                        sprintf(bgLabel, "FG %zu (Shape %d)", bgIndex + 1, bg->pak_entry_id);
                        drawList->AddText(
                            ImVec2(elementPos.x + textPadding, elementPos.y + textPadding), 
                            IM_COL32(255, 255, 255, 255), 
                            bgLabel);
                        
                        // Info de position - en plus petit et tronqué au besoin
                        char posInfo[48];
                        sprintf(posInfo, "(%d,%d)→(%d,%d)", bg->position_start.x, bg->position_start.y, 
                            bg->position_end.x, bg->position_end.y);
                        drawList->AddText(
                            ImVec2(elementPos.x + textPadding, elementPos.y + elementHeight/2), 
                            IM_COL32(220, 220, 220, 255), 
                            posInfo);
                        if (ImGui::IsMouseHoveringRect(elementPos, ImVec2(elementPos.x + elementWidth, elementPos.y + elementHeight))) {
                            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                                this->p_bg_editor = nullptr;
                                this->p_sprite_editor = nullptr;
                                this->p_foreground_editor = bg;
                            }
                        }
                        yOffset += elementHeight + elementSpacing;
                    }
                }   
                
                // Ajouter des boutons interactifs sous chaque shot
                // S'assurer qu'ils sont positionnés correctement
                ImGui::SetCursorScreenPos(ImVec2(shotPos.x + textPadding, shotPos.y + shotHeight - buttonAreaHeight + 5.0f));
                ImGui::PushID(static_cast<int>(shotIndex));
                
                if (ImGui::Button("Éditer", ImVec2((shotWidth - textPadding*3)/2, 20))) {
                    this->shot_counter = shotIndex;
                }
                ImGui::SameLine(0, textPadding);
                if (ImGui::Button("Lire", ImVec2((shotWidth - textPadding*3)/2, 20))) {
                    this->shot_counter = shotIndex;
                    this->fps_counter = 0;
                    this->pause = false;
                }
                ImGui::PopID();
                
                // Passer au shot suivant
                xOffset += shotWidth + shotSpacing;
            }
            drawList->PopClipRect();
            ImGui::EndChild();

            // Légende
            ImGui::Separator();
            if (this->p_bg_editor != nullptr) {
                editMidGameShotBG(this->p_bg_editor);
            } else if (this->p_sprite_editor != nullptr) {
                editMidGameShotSprite(this->p_sprite_editor);
            } else if (this->p_foreground_editor != nullptr) {
                editMidGameShotBG(this->p_foreground_editor);
            } else {
                ImGui::Text("Sélectionnez un élément à éditer");
            }
        }
        ImGui::End();
    }
}

void DebugAnimationPlayer::editMidGameShotBG(MIDGAME_SHOT_BG *bg) {
    if (!bg) return;
    // ID de forme

    if (bg->pak == &this->optShps) {
        ImGui::Text("Formes dans l'archive OPTSHPS");
    } else if (bg->pak == &this->midgames) {
        ImGui::Text("Formes dans l'archive MIDGAME_SHPS");
    } else {
        for (auto &midarchive: this->mid) {
            if (bg->pak == midarchive) {
                ImGui::Text("Formes dans l'archive %s", midarchive->GetName());
                break;
            }
        }
    }
    this->current_mid = bg->pak;
    this->current_entry_index = bg->pak_entry_id;
    this->midgameChoser();
    int shapeid = bg->pak_entry_id;
    if (ImGui::InputInt("ID de forme", &shapeid)) {
        bg->pak_entry_id = shapeid;
    }
    
    // ID de palette
    int palette_id = bg->palette;
    if (ImGui::InputInt("ID de palette", &palette_id)) {
        if (palette_id >= 0 && palette_id <= 255) {
            bg->palette = static_cast<uint8_t>(palette_id);
        }
    }
    
    // Position de départ
    ImGui::Text("Position de départ");
    int start_x = bg->position_start.x;
    int start_y = bg->position_start.y;
    if (ImGui::InputInt("X##start", &start_x) || ImGui::InputInt("Y##start", &start_y)) {
        bg->position_start.x = start_x;
        bg->position_start.y = start_y;
    }
    
    // Position finale
    ImGui::Text("Position finale");
    int end_x = bg->position_end.x;
    int end_y = bg->position_end.y;
    if (ImGui::InputInt("X##end", &end_x) || ImGui::InputInt("Y##end", &end_y)) {
        bg->position_end.x = end_x;
        bg->position_end.y = end_y;
    }
    
    // Vitesse
    ImGui::Text("Vitesse");
    int velocity_x = bg->velocity.x;
    int velocity_y = bg->velocity.y;
    if (ImGui::InputInt("X##velocity", &velocity_x) || ImGui::InputInt("Y##velocity", &velocity_y)) {
        bg->velocity.x = velocity_x;
        bg->velocity.y = velocity_y;
    }
    
    // Affichage de l'aperçu de l'image si disponible
    if (bg->image) {
        ImGui::Separator();
        ImGui::Text("Aperçu de l'image");
        ImGui::Text("Nombre d'images: %d", bg->image->GetNumImages());
        
        static int preview_frame = bg->shapeid;
        if (ImGui::SliderInt("Frame", &preview_frame, 0, bg->image->GetNumImages() - 1)) {
            if (preview_frame < 0) preview_frame = 0;
            if (preview_frame >= bg->image->GetNumImages()) preview_frame = bg->image->GetNumImages() - 1;
        }
        
        if (preview_frame >= 0 && preview_frame < bg->image->GetNumImages()) {
            GLuint glTex = renderShape(bg->image->GetShape(preview_frame), bg->pal ? bg->pal->GetColorPalette() : &this->palette);
            ImGui::Image((void*)(uintptr_t)glTex, ImVec2(320, 200));
            // Note: Ne pas oublier de nettoyer la texture dans le code principal après utilisation
        }
    } else {
        ImGui::Text("Aucune image associée");
    }
    
    // Affichage de la palette si disponible
    if (bg->pal) {
        ImGui::Separator();
        ImGui::Text("Palette associée");
        displayPalette(bg->pal->GetColorPalette());
    } else {
        ImGui::Text("Utilisation de la palette par défaut");
    }
}

void DebugAnimationPlayer::editMidGameShotSprite(MIDGAME_SHOT_SPRITE *sprite) {
    if (!sprite) return;
    
    // ID de forme
    int shapeid = sprite->shapeid;
    if (ImGui::InputInt("ID de forme", &shapeid)) {
        sprite->shapeid = shapeid;
    }
    
    // ID de palette
    int palette_id = sprite->palette;
    if (ImGui::InputInt("ID de palette", &palette_id)) {
        if (palette_id >= 0 && palette_id <= 255) {
            sprite->palette = static_cast<uint8_t>(palette_id);
        }
    }
    
    // Option de conserver la première frame
    bool keep_first = sprite->keep_first_frame != 0;
    if (ImGui::Checkbox("Conserver la première frame", &keep_first)) {
        sprite->keep_first_frame = keep_first ? 1 : 0;
    }
    
    // Position de départ
    ImGui::Text("Position de départ");
    int start_x = sprite->position_start.x;
    int start_y = sprite->position_start.y;
    if (ImGui::InputInt("X##start", &start_x) || ImGui::InputInt("Y##start", &start_y)) {
        sprite->position_start.x = start_x;
        sprite->position_start.y = start_y;
    }
    
    // Position finale
    ImGui::Text("Position finale");
    int end_x = sprite->position_end.x;
    int end_y = sprite->position_end.y;
    if (ImGui::InputInt("X##end", &end_x) || ImGui::InputInt("Y##end", &end_y)) {
        sprite->position_end.x = end_x;
        sprite->position_end.y = end_y;
    }
    
    // Vitesse
    ImGui::Text("Vitesse");
    int velocity_x = sprite->velocity.x;
    int velocity_y = sprite->velocity.y;
    if (ImGui::InputInt("X##velocity", &velocity_x) || ImGui::InputInt("Y##velocity", &velocity_y)) {
        sprite->velocity.x = velocity_x;
        sprite->velocity.y = velocity_y;
    }
    
    // Affichage de l'aperçu de l'image si disponible
    if (sprite->image) {
        ImGui::Separator();
        ImGui::Text("Aperçu de l'image");
        ImGui::Text("Nombre d'images: %d", sprite->image->GetNumImages());
        
        static int preview_frame = 0;
        if (ImGui::SliderInt("Frame", &preview_frame, 0, sprite->image->GetNumImages() - 1)) {
            if (preview_frame < 0) preview_frame = 0;
            if (preview_frame >= sprite->image->GetNumImages()) preview_frame = sprite->image->GetNumImages() - 1;
        }
        
        if (preview_frame >= 0 && preview_frame < sprite->image->GetNumImages()) {
            GLuint glTex = renderShape(sprite->image->GetShape(preview_frame), sprite->pal ? sprite->pal->GetColorPalette() : &this->palette);
            ImGui::Image((void*)(uintptr_t)glTex, ImVec2(320, 200));
            // Note: Ne pas oublier de nettoyer la texture dans le code principal après utilisation
        }
    } else {
        ImGui::Text("Aucune image associée");
    }
    
    // Affichage de la palette si disponible
    if (sprite->pal) {
        ImGui::Separator();
        ImGui::Text("Palette associée");
        displayPalette(sprite->pal->GetColorPalette());
    } else {
        ImGui::Text("Utilisation de la palette par défaut");
    }
    
    // Bouton pour appliquer les modifications
    if (ImGui::Button("Appliquer les modifications", ImVec2(200, 30))) {
        // Les modifications ont déjà été appliquées en temps réel
        ImGui::CloseCurrentPopup();
    }
}
