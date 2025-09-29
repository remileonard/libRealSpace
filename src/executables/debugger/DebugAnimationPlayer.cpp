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
    ImGui::Text("Animation Player frame: %d, fps: %d, shot: %d", this->fps_counter, this->fps, this->shot_counter);
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
                PakEntry *entry = this->current_mid->GetEntry(i);
                if (ImGui::Selectable(
                    ("Entry " + std::to_string(i)+ " " + std::to_string(entry->type)).c_str(),
                    entry == this->current_entry
                )) {
                    // do something when selected
                    this->current_entry = entry;
                }
            }
            ImGui::EndCombo();
        }

    }
}