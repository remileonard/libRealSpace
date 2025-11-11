#include "DebugAnimationPlayer.h"
#include "DebugUtils.h"
#include <imgui.h>
#include <imgui_impl_opengl2.h>
#include <imgui_impl_sdl2.h>
#include <nfd.hpp>
#define EMPTY_VALUE -9999
RSImageSet *loadImage(PakArchive *pak, int entry_id) {
    if (pak == nullptr || entry_id < 0 || entry_id >= pak->GetNumEntries()) {
        return nullptr;
    }
    PakEntry *entry = pak->GetEntry(entry_id);
    if (entry == nullptr) {
        return nullptr;
    }
    RSImageSet *imgset = new RSImageSet();
    PakArchive *pk = new PakArchive();
    pk->InitFromRAM("toto", entry->data, entry->size);
    imgset->InitFromPakArchive(pk, 0);
    if (imgset->GetNumImages() == 0) {
        delete imgset;
        imgset = new RSImageSet();
        imgset->InitFromPakEntry(entry);
    }
    return imgset;
}

DebugAnimationPlayer::DebugAnimationPlayer() : SCAnimationPlayer() { this->auto_stop = false; }
DebugAnimationPlayer::~DebugAnimationPlayer() {}
void DebugAnimationPlayer::renderMenu() {
    ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);

    static bool show_editor = false;
    static int miss_selected = 0;
    if (ImGui::BeginMenu("MidGames Animations")) {
        ImGui::MenuItem("Edit Animation", NULL, &show_editor);
        if (ImGui::MenuItem("Load", nullptr, false)) {
            // Initialiser NFD si pas déjà fait
            static bool nfd_initialized = false;
            if (!nfd_initialized) {
                nfdresult_t result = NFD::Init();
                if (result != NFD_OKAY) {
                    printf("NFD Init failed: %s\n", NFD::GetError());
                    return;
                }
                nfd_initialized = true;
            }
            NFD::UniquePath outPath;
            nfdfilteritem_t filters[1] = { { "Animation Files", "iff" } };
            nfdresult_t result = NFD::OpenDialog(outPath, filters, 1);
            
            if (result == NFD_OKAY) {
                this->loadShot(outPath.get());
                printf("Animation loaded from %s\n", outPath.get());
            } else if (result == NFD_CANCEL) {
                printf("User cancelled load dialog\n");
            } else {
                printf("Error: %s\n", NFD::GetError());
            }
        }
        if (ImGui::MenuItem("Save", nullptr, false)) {
            NFD::UniquePath savePath;
            nfdfilteritem_t filters[1] = { { "Animation Files", "iff" } };
            nfdresult_t result = NFD::SaveDialog(savePath, filters, 1, nullptr, "animation.iff");
            
            if (result == NFD_OKAY) {
                SCAnimationArchive archive;
                if (archive.SaveToFile(savePath.get(), this->midgames_shots[1])) {
                    printf("Animation saved to %s\n", savePath.get());
                } else {
                    printf("Failed to save animation\n");
                }
            } else if (result == NFD_CANCEL) {
                printf("User cancelled save dialog\n");
            } else {
                printf("Error: %s\n", NFD::GetError());
            }
        }
        ImGui::EndMenu();
    }
    ImGui::Text("Animation Player frame: %d, fps: %d, shot: %d", this->fps_counter, this->fps, this->shot_counter);
    if (show_editor) {
        if (ImGui::Begin("Shot Editor", &show_editor,
                         ImGuiWindowFlags_None | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse)) {
            this->showEditor();
            ImGui::End();
        }
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
                this->stay_on_current_shot = false;
                if (this->shot_counter < 0) {
                    this->shot_counter = this->midgames_shots[1].size() - 1;
                }
                this->fps = 1;
                this->fps_counter = 0;
                for (auto bg : this->midgames_shots[1][this->shot_counter]->background) {
                    bg->current_position = {bg->position_start.x, bg->position_start.y};
                }
                for (auto sprt: this->midgames_shots[1][this->shot_counter]->sprites) {
                    sprt->current_frame = 1;
                    sprt->current_position = {sprt->position_start.x, sprt->position_start.y};
                }
                for (auto bg : this->midgames_shots[1][this->shot_counter]->foreground) {
                    bg->current_position = {bg->position_start.x, bg->position_start.y};
                }
                this->midgames_shots[1][this->shot_counter]->sound_played = false;
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
            if (ImGui::Button("stop music")) {
                this->Mixer.stopMusic();
            }
            ImGui::SameLine();
            if (ImGui::Button("Next Frame")) {
                this->fps++;
                this->fps_counter++;
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
                this->stay_on_current_shot = false;
                if (this->shot_counter > this->midgames_shots[1].size() - 1) {
                    this->shot_counter = 0;
                }
                this->fps = 1;
                this->fps_counter = 0;
                for (auto bg : this->midgames_shots[1][this->shot_counter]->background) {
                    bg->current_position = {bg->position_start.x, bg->position_start.y};
                }
                for (auto sprt: this->midgames_shots[1][this->shot_counter]->sprites) {
                    sprt->current_frame = 1;
                    sprt->current_position = {sprt->position_start.x, sprt->position_start.y};
                }
                for (auto bg : this->midgames_shots[1][this->shot_counter]->foreground) {
                    bg->current_position = {bg->position_start.x, bg->position_start.y};
                }
                this->midgames_shots[1][this->shot_counter]->sound_played = false;
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
                    if (ImGui::TreeNode(("Background " + std::to_string(i + 1)).c_str())) {
                        ImGui::Text("Background %d:", i + 1);
                        ImGui::Text("PAK Archive: %s", bg->pak != nullptr ? bg->pak->GetName() : "None");
                        ImGui::Text("Entry ID: %d", bg->pak_entry_id);
                        ImGui::Text("Sub frame id: %d", bg->shapeid);
                        ImGui::Text("Pos Start: (%d,%d)", bg->position_start.x, bg->position_start.y);
                        ImGui::Text("Pos End: (%d,%d)", bg->position_end.x, bg->position_end.y);
                        ImGui::Text("Velocity: (%d,%d)", bg->velocity.x, bg->velocity.y);
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
                                ImGui::Image((void *)(uintptr_t)glTex, ImVec2(320, 200));
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
                    if (ImGui::TreeNode(("Sprite " + std::to_string(i + 1)).c_str())) {
                        ImGui::Text("Sprite %d:", i + 1);
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
                                GLuint glTex =
                                    renderShape(sprt->image->GetShape(frame_to_show),
                                                sprt->pal != nullptr ? sprt->pal->GetColorPalette() : &this->palette);
                                s_CurrentFrameGLTex.push_back(glTex); // Garder la texture pour la supprimer plus tard
                                ImGui::Image((void *)(uintptr_t)glTex, ImVec2(320, 200));
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
        for (auto midarchive : this->mid) {
            if (ImGui::Selectable(midarchive->GetName(), midarchive == this->current_mid)) {
                this->current_mid = midarchive;
            }
        }
        ImGui::EndCombo();
    }
    if (this->current_mid != nullptr) {
        ImGui::Text("Archive: %s, NumEntries: %d", this->current_mid->GetName(), this->current_mid->GetNumEntries());
        if (ImGui::BeginCombo("Entries", this->current_entry_index >= 0
                                             ? ("Entry " + std::to_string(this->current_entry_index)).c_str()
                                             : "None")) {
            for (int i = 0; i < this->current_mid->GetNumEntries(); i++) {
                PakEntry *entry = this->current_mid->GetEntry(i);
                if (entry == nullptr) {
                    continue;
                }
                if (entry->size == 0) {
                    continue;
                }
                int16_t *shortP = (int16_t *)entry->data;
                int16_t offset = *shortP;
                shortP = (int16_t *)(entry->data + 2);
                int16_t numColors = *shortP;
                if (numColors + offset == 256) {
                    continue;
                }
                if (ImGui::Selectable(("Entry " + std::to_string(i)).c_str(), i == this->current_entry_index)) {
                    this->current_entry_index = i;
                }
            }
            ImGui::EndCombo();
        }
    }
}
void DebugAnimationPlayer::midgamePaletteChoser() {
    if (ImGui::BeginCombo("MID Palette", this->current_palette_mid ? this->current_palette_mid->GetName() : "None")) {
        if (ImGui::Selectable("OptPal", this->current_palette_mid == &this->optPals)) {
            this->current_palette_mid = &this->optPals;
        }
        if (ImGui::Selectable("MidGames", this->current_palette_mid == &this->midgames)) {
            this->current_palette_mid = &this->midgames;
        }
        for (auto midarchive : this->mid) {
            if (ImGui::Selectable(midarchive->GetName(), midarchive == this->current_palette_mid)) {
                this->current_palette_mid = midarchive;
            }
        }
        ImGui::EndCombo();
    }
    if (this->current_palette_mid != nullptr) {
        ImGui::Text("Archive: %s, NumEntries: %d", this->current_palette_mid->GetName(),
                    this->current_palette_mid->GetNumEntries());
        if (ImGui::BeginCombo("Palettes", this->current_palette_entry_index >= 0
                                              ? ("Pal " + std::to_string(this->current_palette_entry_index)).c_str()
                                              : "None")) {
            for (int i = 0; i < this->current_palette_mid->GetNumEntries(); i++) {
                PakEntry *entry = this->current_palette_mid->GetEntry(i);
                if (entry == nullptr) {
                    continue;
                }
                if (entry->size == 0) {
                    continue;
                }
                int16_t *shortP = (int16_t *)entry->data;
                int16_t offset = *shortP;
                shortP = (int16_t *)(entry->data + 2);
                int16_t numColors = *shortP;
                if (numColors + offset <= 0 || numColors + offset > 256) {
                    continue;
                }
                if (ImGui::Selectable(("Pal " + std::to_string(i)).c_str(), i == this->current_palette_entry_index)) {
                    this->current_palette_entry_index = i;
                }
            }
            ImGui::EndCombo();
        }
    }
}

void DebugAnimationPlayer::resetEditorSelection() {
    this->current_mid = nullptr;
    this->current_entry_index = -1;
    this->current_palette_mid = nullptr;
    this->current_palette_entry_index = -1;
    this->current_midvoc = nullptr;
    this->current_midvoc_entry_index = -1;
}

void DebugAnimationPlayer::selectEditorElement(MIDGAME_SHOT_BG *bg, MIDGAME_SHOT_SPRITE *sprite, MIDGAME_SHOT_BG *fg,
                                               MIDGAME_SHOT_CHARACTER *character, MIDGAME_SHOT *shotPtr) {
    this->p_bg_editor = bg;
    this->p_sprite_editor = sprite;
    this->p_foreground_editor = fg;
    this->p_character_editor = character;
    this->p_shot_editor = shotPtr;
    resetEditorSelection();
}

MIDGAME_SHOT *DebugAnimationPlayer::createEmptyShot() {
    MIDGAME_SHOT *newShot = new MIDGAME_SHOT();
    newShot->nbframe = 30;
    newShot->music = -1;
    newShot->sound = nullptr;
    newShot->sound_time_code = 0;
    return newShot;
}

float DebugAnimationPlayer::calculateSectionHeight(size_t elementCount, float sectionHeaderHeight, float elementHeight,
                                                   float elementSpacing, float emptyMessageHeight) {
    return sectionHeaderHeight + (elementCount ? elementCount * (elementHeight + elementSpacing) : emptyMessageHeight);
}

std::string DebugAnimationPlayer::formatElementPosition(int startX, int startY, int endX, int endY) {
    char buffer[48];
    sprintf(buffer, "(%d,%d)→(%d,%d)", startX, startY, endX, endY);
    return std::string(buffer);
}

void DebugAnimationPlayer::renderInsertShotButton(size_t shotIndex, float &xOffset, float timelineHeight,
                                                  ImVec2 timelinePos, float insertButtonWidth, bool isBeforeShot) {
    ImVec2 insertButtonPos(timelinePos.x + xOffset, timelinePos.y - 35.0f);
    ImGui::SetCursorScreenPos(ImVec2(insertButtonPos.x, insertButtonPos.y + timelineHeight / 2 - 15));

    int buttonId = isBeforeShot ? 1000 + static_cast<int>(shotIndex) : 2000 + static_cast<int>(shotIndex);
    ImGui::PushID(buttonId);

    if (ImGui::Button("+", ImVec2(insertButtonWidth, 30))) {
        MIDGAME_SHOT *newShot = createEmptyShot();
        size_t insertPos = isBeforeShot ? shotIndex : shotIndex + 1;
        this->midgames_shots[1].insert(this->midgames_shots[1].begin() + insertPos, newShot);
        this->shot_counter = static_cast<int>(insertPos);
        selectEditorElement(nullptr, nullptr, nullptr, nullptr, newShot);
    }

    ImGui::PopID();
    insertButtonPos = {timelinePos.x + xOffset, timelinePos.y + 5.0f};
    ImGui::SetCursorScreenPos(ImVec2(insertButtonPos.x, insertButtonPos.y + timelineHeight / 2 - 15));

    buttonId = isBeforeShot ? 4000 + static_cast<int>(shotIndex) : 3000 + static_cast<int>(shotIndex);
    ImGui::PushID(buttonId);

    if (ImGui::Button("D", ImVec2(insertButtonWidth, 30))) {
        MIDGAME_SHOT *shot = this->midgames_shots[1][shotIndex];
        MIDGAME_SHOT *newShot = new MIDGAME_SHOT();
        // Copy the basic shot properties
        newShot->nbframe = shot->nbframe;
        newShot->music = shot->music;
        newShot->sound = shot->sound; // Note: this is a shallow copy
        newShot->sound_time_code = shot->sound_time_code;

        // Copy backgrounds
        for (auto *bg : shot->background) {
            MIDGAME_SHOT_BG *newBg = new MIDGAME_SHOT_BG();
            newBg->pak = bg->pak;
            newBg->pak_entry_id = bg->pak_entry_id;
            newBg->pak_palette = bg->pak_palette;
            newBg->palette = bg->palette;
            newBg->shapeid = bg->shapeid;
            newBg->position_start = bg->position_start;
            newBg->position_end = bg->position_end;
            newBg->velocity = bg->velocity;
            newBg->use_external_palette = bg->use_external_palette;

            // Load image
            if (bg->image) {
                newBg->image = loadImage(bg->pak, bg->pak_entry_id);
                if (bg->pal && newBg->image) {
                    newBg->pal = newBg->image->palettes.size() > 0 ? newBg->image->palettes[0] : nullptr;
                }
            } else {
                newBg->image = nullptr;
                newBg->pal = nullptr;
            }

            newShot->background.push_back(newBg);
        }

        // Copy sprites
        for (auto *sprite : shot->sprites) {
            MIDGAME_SHOT_SPRITE *newSprite = new MIDGAME_SHOT_SPRITE();
            newSprite->pak = sprite->pak;
            newSprite->pak_entry_id = sprite->pak_entry_id;
            newSprite->pak_palette = sprite->pak_palette;
            newSprite->palette = sprite->palette;
            newSprite->shapeid = sprite->shapeid;
            newSprite->position_start = sprite->position_start;
            newSprite->position_end = sprite->position_end;
            newSprite->velocity = sprite->velocity;
            newSprite->keep_first_frame = sprite->keep_first_frame;
            newSprite->use_external_palette = sprite->use_external_palette;

            // Load image
            if (sprite->image) {
                newSprite->image = loadImage(sprite->pak, sprite->pak_entry_id);
                if (sprite->pal && newSprite->image) {
                    newSprite->pal = newSprite->image->palettes.size() > 0 ? newSprite->image->palettes[0] : nullptr;
                }
            } else {
                newSprite->image = nullptr;
                newSprite->pal = nullptr;
            }

            newShot->sprites.push_back(newSprite);
        }

        // Copy foregrounds
        for (auto *fg : shot->foreground) {
            MIDGAME_SHOT_BG *newFg = new MIDGAME_SHOT_BG();
            newFg->pak = fg->pak;
            newFg->pak_entry_id = fg->pak_entry_id;
            newFg->pak_palette = fg->pak_palette;
            newFg->palette = fg->palette;
            newFg->shapeid = fg->shapeid;
            newFg->position_start = fg->position_start;
            newFg->position_end = fg->position_end;
            newFg->velocity = fg->velocity;
            newFg->use_external_palette = fg->use_external_palette;

            // Load image
            if (fg->image) {
                newFg->image = loadImage(fg->pak, fg->pak_entry_id);
                if (fg->pal && newFg->image) {
                    newFg->pal = newFg->image->palettes.size() > 0 ? newFg->image->palettes[0] : nullptr;
                }
            } else {
                newFg->image = nullptr;
                newFg->pal = nullptr;
            }

            newShot->foreground.push_back(newFg);
        }

        // Copy characters
        for (auto *character : shot->characters) {
            MIDGAME_SHOT_CHARACTER *newChar = new MIDGAME_SHOT_CHARACTER();
            newChar->position_start = character->position_start;
            newChar->position_end = character->position_end;
            newChar->velocity = character->velocity;
            newChar->cloth_id = character->cloth_id;
            newChar->expression_id = character->expression_id;
            newChar->head_id = character->head_id;
            newChar->character_name = character->character_name;
            newChar->palette = character->palette;

            // Share the image pointer since characters use ConvAssetManager
            newChar->image = character->image;

            newShot->characters.push_back(newChar);
        }
        size_t insertPos = isBeforeShot ? shotIndex : shotIndex + 1;
        this->midgames_shots[1].insert(this->midgames_shots[1].begin() + insertPos, newShot);
        this->shot_counter = static_cast<int>(insertPos);
        selectEditorElement(nullptr, nullptr, nullptr, nullptr, newShot);
    }

    ImGui::PopID();

    insertButtonPos = {timelinePos.x + xOffset, timelinePos.y + 40.0f};
    ImGui::SetCursorScreenPos(ImVec2(insertButtonPos.x, insertButtonPos.y + timelineHeight / 2 - 15));
    buttonId = isBeforeShot ? 5000 + static_cast<int>(shotIndex) : 6000 + static_cast<int>(shotIndex);
    ImGui::PushID(buttonId);
    if (ImGui::Button(">", ImVec2(insertButtonWidth, 30))) {
        if (shotIndex + 1 < this->midgames_shots[1].size()) {
            // Swap this shot with the next one
            std::swap(this->midgames_shots[1][shotIndex], this->midgames_shots[1][shotIndex + 1]);

            // Update shot_counter if needed to keep track of the same shot
            if (this->shot_counter == static_cast<int>(shotIndex)) {
                this->shot_counter = static_cast<int>(shotIndex + 1);
            } else if (this->shot_counter == static_cast<int>(shotIndex + 1)) {
                this->shot_counter = static_cast<int>(shotIndex);
            }

            // Update editing state if needed
            MIDGAME_SHOT *currentShot = this->midgames_shots[1][shotIndex + 1];
            if (this->p_shot_editor == currentShot) {
                selectEditorElement(nullptr, nullptr, nullptr, nullptr, currentShot);
            }
        }
    }
    ImGui::PopID();
    xOffset += insertButtonWidth + 5.0f;
}

void DebugAnimationPlayer::deleteShotAtIndex(size_t shotIndex) {
    if (this->midgames_shots[1].size() <= 1) {
        return;
    }

    MIDGAME_SHOT *shotToDelete = this->midgames_shots[1][shotIndex];

    // Libérer les backgrounds
    for (auto *bg : shotToDelete->background) {
        if (bg->image)
            delete bg->image;
        if (bg->pal)
            delete bg->pal;
        delete bg;
    }

    // Libérer les sprites
    for (auto *sprite : shotToDelete->sprites) {
        if (sprite->image)
            delete sprite->image;
        if (sprite->pal)
            delete sprite->pal;
        delete sprite;
    }

    // Libérer les foregrounds
    for (auto *fg : shotToDelete->foreground) {
        if (fg->image)
            delete fg->image;
        if (fg->pal)
            delete fg->pal;
        delete fg;
    }

    // Libérer les characters
    for (auto *character : shotToDelete->characters) {
        if (character->image)
            character->image = nullptr;
        delete character;
    }

    delete shotToDelete;

    this->midgames_shots[1].erase(this->midgames_shots[1].begin() + shotIndex);

    if (this->shot_counter >= static_cast<int>(shotIndex)) {
        this->shot_counter = (std::max)(0, this->shot_counter - 1);
    }

    if (this->p_shot_editor == shotToDelete) {
        resetEditorSelection();
        this->p_shot_editor = nullptr;
        this->p_bg_editor = nullptr;
        this->p_sprite_editor = nullptr;
        this->p_foreground_editor = nullptr;
        this->p_character_editor = nullptr;
    }
}

void DebugAnimationPlayer::addNewBackground(MIDGAME_SHOT *shot) {
    MIDGAME_SHOT_BG *newBg = new MIDGAME_SHOT_BG();
    newBg->pak = &this->optShps;
    newBg->pak_entry_id = 0;
    newBg->pak_palette = &this->optPals;
    newBg->palette = 0;
    newBg->shapeid = 0;
    newBg->position_start = {0, 0};
    newBg->position_end = {0, 0};
    newBg->velocity = {0, 0};
    newBg->image = nullptr;
    newBg->pal = nullptr;
    newBg->use_external_palette = 0;
    shot->background.push_back(newBg);
    selectEditorElement(newBg, nullptr, nullptr, nullptr, nullptr);
}

void DebugAnimationPlayer::addNewCharacter(MIDGAME_SHOT *shot) {
    MIDGAME_SHOT_CHARACTER *newChara = new MIDGAME_SHOT_CHARACTER();
    newChara->position_start = {0, 0};
    newChara->position_end = {0, 0};
    newChara->velocity = {0, 0};
    newChara->image = nullptr;
    newChara->cloth_id = 0;
    newChara->expression_id = 0;
    newChara->head_id = 0;
    newChara->character_name.clear();
    shot->characters.push_back(newChara);
    selectEditorElement(nullptr, nullptr, nullptr, newChara, nullptr);
}

void DebugAnimationPlayer::addNewSprite(MIDGAME_SHOT *shot) {
    MIDGAME_SHOT_SPRITE *newSprite = new MIDGAME_SHOT_SPRITE();
    newSprite->shapeid = 0;
    newSprite->palette = 0;
    newSprite->pak_entry_id = 0;
    newSprite->pak = &this->optShps;
    newSprite->pak_palette = &this->optPals;
    newSprite->position_start = {0, 0};
    newSprite->position_end = {0, 0};
    newSprite->velocity = {0, 0};
    newSprite->keep_first_frame = 0;
    newSprite->image = nullptr;
    newSprite->pal = nullptr;
    newSprite->use_external_palette = 0;
    shot->sprites.push_back(newSprite);
    selectEditorElement(nullptr, newSprite, nullptr, nullptr, nullptr);
}

void DebugAnimationPlayer::addNewForeground(MIDGAME_SHOT *shot) {
    MIDGAME_SHOT_BG *newFg = new MIDGAME_SHOT_BG();
    newFg->pak = &this->optShps;
    newFg->pak_entry_id = 0;
    newFg->pak_palette = &this->optPals;
    newFg->palette = 0;
    newFg->shapeid = 0;
    newFg->position_start = {0, 0};
    newFg->position_end = {0, 0};
    newFg->velocity = {0, 0};
    newFg->image = nullptr;
    newFg->pal = nullptr;
    newFg->use_external_palette = 0;
    shot->foreground.push_back(newFg);
    selectEditorElement(nullptr, nullptr, newFg, nullptr, nullptr);
}

std::string DebugAnimationPlayer::buildBackgroundLabel(MIDGAME_SHOT_BG *bg, size_t index) {
    std::string pak_name = bg->pak ? bg->pak->GetName() : "None";
    std::string filename = pak_name;
    size_t lastSlash = pak_name.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        filename = pak_name.substr(lastSlash + 1);
    }
    return "BG: " + filename + ":" + std::to_string(bg->pak_entry_id);
}

std::string DebugAnimationPlayer::buildCharacterLabel(MIDGAME_SHOT_CHARACTER *character, size_t index) {
    int count = character->image ? character->image->GetNumImages() : 0;
    std::string label = "CHR: ";
    if (!character->character_name.empty()) {
        label += " - " + character->character_name;
    }
    return label;
}

std::string DebugAnimationPlayer::buildSpriteLabel(MIDGAME_SHOT_SPRITE *bg, size_t index) {
    std::string pak_name = bg->pak ? bg->pak->GetName() : "None";
    std::string filename = pak_name;
    size_t lastSlash = pak_name.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        filename = pak_name.substr(lastSlash + 1);
    }
    return "SP: " + filename + ":" + std::to_string(bg->pak_entry_id);
}

std::string DebugAnimationPlayer::buildForegroundLabel(MIDGAME_SHOT_BG *bg, size_t index) {
    std::string pak_name = bg->pak ? bg->pak->GetName() : "None";
    std::string filename = pak_name;
    size_t lastSlash = pak_name.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        filename = pak_name.substr(lastSlash + 1);
    }
    return "FG: " + filename + ":" + std::to_string(bg->pak_entry_id);
}

void DebugAnimationPlayer::drawShotSection(const char *title, const char *emptyLabel, ImU32 color, int idOffset,
                                           size_t shotIndex, ImVec2 shotPos, float &yOffset,
                                           const std::vector<ShotSectionElement> &elements,
                                           const std::function<void()> &addElement,
                                           const std::function<void(void *)> &onSelect, float shotWidth,
                                           float textPadding, float sectionHeaderHeight, float emptyMessageHeight,
                                           float elementHeight, float elementSpacing) {
    ImDrawList *drawList = ImGui::GetWindowDrawList();

    // En-tête de section avec bouton +
    drawList->AddText(ImVec2(shotPos.x + textPadding, shotPos.y + yOffset), IM_COL32(200, 200, 200, 255), title);
    ImGui::SetCursorScreenPos(
        ImVec2(shotPos.x + textPadding + ImGui::CalcTextSize(title).x + 5, shotPos.y + yOffset - 2));
    ImGui::PushID(static_cast<int>(shotIndex) * 100 + idOffset);
    if (ImGui::SmallButton("+")) {
        addElement();
    }
    ImGui::PopID();
    yOffset += sectionHeaderHeight;

    if (elements.empty()) {
        drawList->AddText(ImVec2(shotPos.x + textPadding * 2, shotPos.y + yOffset), IM_COL32(180, 180, 180, 255),
                          emptyLabel);
        yOffset += emptyMessageHeight;
        return;
    }

    float elementWidth = shotWidth - (textPadding * 2);
    for (size_t elementIndex = 0; elementIndex < elements.size(); ++elementIndex) {
        const auto &element = elements[elementIndex];
        ImVec2 elementPos(shotPos.x + textPadding, shotPos.y + yOffset);

        drawList->AddRectFilled(elementPos, ImVec2(elementPos.x + elementWidth, elementPos.y + elementHeight), color,
                                3.0f);

        drawList->AddText(ImVec2(elementPos.x + textPadding, elementPos.y + textPadding), IM_COL32(255, 255, 255, 255),
                          element.label.c_str());

        std::string posInfo =
            formatElementPosition(static_cast<int>(element.positionStart.x), static_cast<int>(element.positionStart.y),
                                  static_cast<int>(element.positionEnd.x), static_cast<int>(element.positionEnd.y));
        drawList->AddText(ImVec2(elementPos.x + textPadding, elementPos.y + elementHeight / 2),
                          IM_COL32(220, 220, 220, 255), posInfo.c_str());

        if (ImGui::IsMouseHoveringRect(elementPos, ImVec2(elementPos.x + elementWidth, elementPos.y + elementHeight)) &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            onSelect(element.elementPtr);
        }

        yOffset += elementHeight + elementSpacing;
    }
}

std::vector<ShotSectionElement> DebugAnimationPlayer::buildBackgroundElements(MIDGAME_SHOT *shot) {
    std::vector<ShotSectionElement> elements;
    for (size_t i = 0; i < shot->background.size(); ++i) {
        auto *bg = shot->background[i];
        elements.push_back({buildBackgroundLabel(bg, i),
                            ImVec2(static_cast<float>(bg->position_start.x), static_cast<float>(bg->position_start.y)),
                            ImVec2(static_cast<float>(bg->position_end.x), static_cast<float>(bg->position_end.y)),
                            bg});
    }
    return elements;
}

std::vector<ShotSectionElement> DebugAnimationPlayer::buildCharacterElements(MIDGAME_SHOT *shot) {
    std::vector<ShotSectionElement> elements;
    for (size_t i = 0; i < shot->characters.size(); ++i) {
        auto *character = shot->characters[i];
        elements.push_back(
            {buildCharacterLabel(character, i),
             ImVec2(static_cast<float>(character->position_start.x), static_cast<float>(character->position_start.y)),
             ImVec2(static_cast<float>(character->position_end.x), static_cast<float>(character->position_end.y)),
             character});
    }
    return elements;
}

std::vector<ShotSectionElement> DebugAnimationPlayer::buildSpriteElements(MIDGAME_SHOT *shot) {
    std::vector<ShotSectionElement> elements;
    for (size_t i = 0; i < shot->sprites.size(); ++i) {
        auto *sprite = shot->sprites[i];
        elements.push_back(
            {
                buildSpriteLabel(sprite, i),
                ImVec2(static_cast<float>(sprite->position_start.x), static_cast<float>(sprite->position_start.y)),
                ImVec2(static_cast<float>(sprite->position_end.x), static_cast<float>(sprite->position_end.y)), sprite
            }
        );
    }
    return elements;
}

std::vector<ShotSectionElement> DebugAnimationPlayer::buildForegroundElements(MIDGAME_SHOT *shot) {
    std::vector<ShotSectionElement> elements;
    for (size_t i = 0; i < shot->foreground.size(); ++i) {
        auto *fg = shot->foreground[i];
        elements.push_back(
            {
                buildForegroundLabel(fg, i),
                ImVec2(static_cast<float>(fg->position_start.x), static_cast<float>(fg->position_start.y)),
                ImVec2(static_cast<float>(fg->position_end.x), static_cast<float>(fg->position_end.y)),
                fg
            }
        );
    }
    return elements;
}

void DebugAnimationPlayer::showEditor() {
    ImGui::Text("Éditeur de Shots - Timeline");

    if (this->midgames_shots[1].empty()) {
        ImGui::Text("Aucun shot disponible dans l'animation actuelle.");
        return;
    }

    const float shotWidth = 180.0f;
    const float elementHeight = 40.0f;
    const float timelineHeight = 350.0f;
    const float shotSpacing = 15.0f;
    const float elementSpacing = 5.0f;
    const float sectionHeaderHeight = 20.0f;
    const float emptyMessageHeight = 20.0f;
    const float buttonAreaHeight = 30.0f;
    const float textPadding = 5.0f;
    const float shotTitleHeight = 25.0f;
    const float insertButtonWidth = 30.0f;
    RSScreen::getInstance().force_keyboard_capture = true;
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize(ImGui::GetWindowWidth() - 20, timelineHeight);

    drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
                            IM_COL32(40, 40, 50, 255));

    ImGui::BeginChild("TimelineScroll", ImVec2(0, timelineHeight), true, ImGuiWindowFlags_HorizontalScrollbar);

    ImVec2 timelinePos = ImGui::GetCursorScreenPos();
    float xOffset = 10.0f;

    drawList->PushClipRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x - 15, canvasPos.y + canvasSize.y - 15), true);

    for (size_t shotIndex = 0; shotIndex < this->midgames_shots[1].size(); ++shotIndex) {
        MIDGAME_SHOT *shot = this->midgames_shots[1][shotIndex];

        if (shotIndex == 0) {
            renderInsertShotButton(shotIndex, xOffset, timelineHeight, timelinePos, insertButtonWidth, true);
        }

        float shotHeight = shotTitleHeight + buttonAreaHeight;
        shotHeight += calculateSectionHeight(shot->background.size(), sectionHeaderHeight, elementHeight,
                                             elementSpacing, emptyMessageHeight);
        shotHeight += calculateSectionHeight(shot->characters.size(), sectionHeaderHeight, elementHeight,
                                             elementSpacing, emptyMessageHeight);
        shotHeight += calculateSectionHeight(shot->sprites.size(), sectionHeaderHeight, elementHeight, elementSpacing,
                                             emptyMessageHeight);
        shotHeight += calculateSectionHeight(shot->foreground.size(), sectionHeaderHeight, elementHeight,
                                             elementSpacing, emptyMessageHeight);

        ImVec2 shotPos(timelinePos.x + xOffset, timelinePos.y + 5.0f);

        ImU32 shotColor = (shotIndex == this->shot_counter) ? IM_COL32(80, 140, 200, 255) : IM_COL32(70, 70, 80, 255);
        drawList->AddRectFilled(shotPos, ImVec2(shotPos.x + shotWidth, shotPos.y + shotHeight), shotColor, 4.0f);

        float yOffset = 8.0f;
        char shotTitle[64];
        sprintf(shotTitle, "Shot %zu (%d frames)", shotIndex + 1, shot->nbframe);
        drawList->AddText(ImVec2(shotPos.x + textPadding, shotPos.y + yOffset), IM_COL32(255, 255, 255, 255),
                          shotTitle);
        yOffset += shotTitleHeight;

        // Backgrounds
        auto bgElements = buildBackgroundElements(shot);
        drawShotSection(
            "Backgrounds:", "Aucun background", IM_COL32(60, 120, 180, 255), 0, shotIndex, shotPos, yOffset, bgElements,
            [&]() { addNewBackground(shot); },
            [&](void *ptr) {
                selectEditorElement(static_cast<MIDGAME_SHOT_BG *>(ptr), nullptr, nullptr, nullptr, nullptr);
            },
            shotWidth, textPadding, sectionHeaderHeight, emptyMessageHeight, elementHeight, elementSpacing);

        // Characters
        auto characterElements = buildCharacterElements(shot);
        drawShotSection(
            "Characters:", "Aucun character", IM_COL32(180, 60, 120, 255), 1, shotIndex, shotPos, yOffset,
            characterElements, [&]() { addNewCharacter(shot); },
            [&](void *ptr) {
                selectEditorElement(nullptr, nullptr, nullptr, static_cast<MIDGAME_SHOT_CHARACTER *>(ptr), nullptr);
            },
            shotWidth, textPadding, sectionHeaderHeight, emptyMessageHeight, elementHeight, elementSpacing);

        // Sprites
        auto spriteElements = buildSpriteElements(shot);
        drawShotSection(
            "Sprites:", "Aucun sprite", IM_COL32(180, 120, 60, 255), 2, shotIndex, shotPos, yOffset, spriteElements,
            [&]() { addNewSprite(shot); },
            [&](void *ptr) {
                selectEditorElement(nullptr, static_cast<MIDGAME_SHOT_SPRITE *>(ptr), nullptr, nullptr, nullptr);
            },
            shotWidth, textPadding, sectionHeaderHeight, emptyMessageHeight, elementHeight, elementSpacing);

        // Foregrounds
        auto foregroundElements = buildForegroundElements(shot);
        drawShotSection(
            "Foregrounds:", "Pas de foreground", IM_COL32(220, 120, 180, 255), 3, shotIndex, shotPos, yOffset,
            foregroundElements, [&]() { addNewForeground(shot); },
            [&](void *ptr) {
                selectEditorElement(nullptr, nullptr, static_cast<MIDGAME_SHOT_BG *>(ptr), nullptr, nullptr);
            },
            shotWidth, textPadding, sectionHeaderHeight, emptyMessageHeight, elementHeight, elementSpacing);

        ImGui::SetCursorScreenPos(ImVec2(shotPos.x + textPadding, shotPos.y + shotHeight - buttonAreaHeight + 5.0f));
        ImGui::PushID(static_cast<int>(shotIndex));
        if (ImGui::Button("Éditer", ImVec2((shotWidth - textPadding * 4) / 3, 20))) {
            this->shot_counter = static_cast<int>(shotIndex);
            selectEditorElement(nullptr, nullptr, nullptr, nullptr, shot);
        }
        ImGui::SameLine(0, textPadding);
        if (ImGui::Button("Lire", ImVec2((shotWidth - textPadding * 4) / 3, 20))) {
            this->shot_counter = static_cast<int>(shotIndex);
            this->fps_counter = 0;
            this->stay_on_current_shot = true;
            this->pause = false;
        }
        ImGui::SameLine(0, textPadding);
        if (ImGui::Button("X", ImVec2((shotWidth - textPadding * 4) / 3, 20))) {
            deleteShotAtIndex(shotIndex);
        }
        ImGui::PopID();

        xOffset += shotWidth + shotSpacing;
        renderInsertShotButton(shotIndex, xOffset, timelineHeight, timelinePos, insertButtonWidth, false);
    }

    drawList->PopClipRect();
    ImGui::EndChild();

    ImGui::Separator();
    if (this->p_bg_editor != nullptr) {
        editMidGameShotBG(this->p_bg_editor);
    } else if (this->p_sprite_editor != nullptr) {
        editMidGameShotSprite(this->p_sprite_editor);
    } else if (this->p_foreground_editor != nullptr) {
        editMidGameShotBG(this->p_foreground_editor);
    } else if (this->p_character_editor != nullptr) {
        editMidGameShotCharacter(this->p_character_editor);
    } else if (this->p_shot_editor != nullptr) {
        editMidGameShot(this->p_shot_editor);
    } else {
        ImGui::Text("Sélectionnez un élément à éditer");
    }
}
void DebugAnimationPlayer::editMidGameShotCharacter(MIDGAME_SHOT_CHARACTER *chara) {
    static std::vector<GLuint> s_PrevFrameGLTex;
    static std::vector<GLuint> s_CurrentFrameGLTex;
    static int palette_id = chara->palette;
    // Détruire les textures de la frame précédente (elles ont été rendues)
    if (!s_PrevFrameGLTex.empty()) {
        for (GLuint id : s_PrevFrameGLTex) {
            glDeleteTextures(1, &id);
        }
        s_PrevFrameGLTex.clear();
    }
    // Préparer la liste pour cette frame
    s_PrevFrameGLTex.swap(s_CurrentFrameGLTex); // s_CurrentFrameGLTex devient vide

    if (!chara)
        return;
    // ID de forme

    if (ImGui::BeginCombo("Character", chara->character_name.empty() ? "None" : chara->character_name.c_str())) {
        for (const auto &[name, image] : ConvAssetManager::getInstance().faces) {
            if (ImGui::Selectable(name.c_str(), chara->character_name == name)) {
                chara->character_name = name;
                chara->image = image->appearances;
                chara->cloth_id = 0;
                chara->expression_id = 0;
            }
        }
        ImGui::EndCombo();
    }
    if (chara->image) {
        ImGui::Text("NumImages: %d", chara->image->GetNumImages());
    } else {
        ImGui::Text("No image loaded");
    }
    if (ImGui::BeginCombo("Palette", std::to_string(chara->palette).c_str())) {
        for (int i = 0; i < ConvAssetManager::getInstance().convPals.GetNumEntries(); i++) {
            PakEntry *entry = ConvAssetManager::getInstance().convPals.GetEntry(i);
            if (entry == nullptr) {
                continue;
            }
            if (entry->size == 0) {
                continue;
            }
            if (ImGui::Selectable(("Pal " + std::to_string(i)).c_str(), i == chara->palette)) {
                chara->palette = i;
                palette_id = i;
            }
        }
        ImGui::EndCombo();
    }

    if (ImGui::InputInt("ID de forme", &palette_id)) {
        chara->palette = palette_id;
    }
    ImGui::Separator();
    ImGui::Text("Reglages des paramètres du character");
    int head_id = chara->head_id;
    if (ImGui::InputInt("ID head", &head_id)) {
        if (head_id < 0)
            head_id = 0;
        chara->head_id = head_id;
    }
    int cloth_id = chara->cloth_id;
    if (ImGui::InputInt("ID Vetements", &cloth_id)) {
        if (cloth_id < 0)
            cloth_id = 0;
        chara->cloth_id = cloth_id;
    }
    int expression_id = chara->expression_id;
    if (ImGui::InputInt("ID Expression", &expression_id)) {
        if (expression_id < 0)
            expression_id = 0;
        chara->expression_id = expression_id;
    }
    ImGui::Separator();
    ImGui::Text("Preview du character");
    if (chara->image) {
        // Rendu de l'image
        int imgIndex = 0;
        if (chara->expression_id >= 0 && chara->expression_id < chara->image->GetNumImages()) {
            imgIndex = chara->expression_id;
        }
        FrameBuffer *testFb = new FrameBuffer(200, 200);
        testFb->fillWithColor(255);
        testFb->drawShape(chara->image->GetShape(1));
        GLuint glTex = renderFrameBuffer(testFb, &this->palette);
        s_CurrentFrameGLTex.push_back(glTex); // Garder la trace pour la déstruction plus tard
        ImGui::Image((void *)(intptr_t)glTex, ImVec2(100, 100));
    } else {
        ImGui::Text("Aucune image chargée");
    }
    // Position de départ
    ImGui::Text("Position de départ");
    int start_x = chara->position_start.x;
    int start_y = chara->position_start.y;
    if (ImGui::InputInt("X##start", &start_x) || ImGui::InputInt("Y##start", &start_y)) {
        chara->position_start.x = start_x;
        chara->position_start.y = start_y;
    }

    // Position finale
    ImGui::Text("Position finale");
    int end_x = chara->position_end.x;
    int end_y = chara->position_end.y;
    if (ImGui::InputInt("X##end", &end_x) || ImGui::InputInt("Y##end", &end_y)) {
        chara->position_end.x = end_x;
        chara->position_end.y = end_y;
    }

    // Vitesse
    ImGui::Text("Vitesse");
    int velocity_x = chara->velocity.x;
    int velocity_y = chara->velocity.y;
    if (ImGui::InputInt("X##velocity", &velocity_x) || ImGui::InputInt("Y##velocity", &velocity_y)) {
        chara->velocity.x = velocity_x;
        chara->velocity.y = velocity_y;
    }
    if (ImGui::Button("Supprimer le character")) {
        for (auto &shot : this->midgames_shots[1]) {
            auto it = std::find(shot->characters.begin(), shot->characters.end(), chara);
            if (it != shot->characters.end()) {
                // Supprimer du vecteur AVANT de libérer la mémoire
                shot->characters.erase(it);
                
                // Maintenant on peut libérer la mémoire
                if (chara->image) {
                    chara->image = nullptr; // Ne pas delete, géré par ConvAssetManager
                }
                delete chara;
                
                // Réinitialiser le pointeur d'édition
                this->p_character_editor = nullptr;
                break;
            }
        }
    }
}
void DebugAnimationPlayer::editMidGameShot(MIDGAME_SHOT *shot) {
    if (!shot)
        return;
    ImGui::Text("Édition du shot (%d frames)", shot->nbframe);
    int nbframe = shot->nbframe;
    if (ImGui::InputInt("Nombre de frames", &nbframe)) {
        if (nbframe < 1)
            nbframe = 1;
        shot->nbframe = nbframe;
    }
    ImGui::Separator();
    ImGui::Text("Music");
    static int music_id = shot->music;
    if (ImGui::InputInt("ID Musique", &music_id)) {
        if (music_id < -1)
            music_id = -1;
        shot->music = music_id;
    }
    ImGui::Separator();
    ImGui::Text("Sound Effects");
    if (this->current_midvoc == nullptr) {
        this->current_midvoc = shot->sound_pak;
    } else {
        shot->sound_pak = this->current_midvoc;
    }
    if (this->current_midvoc_entry_index == -1) {
        this->current_midvoc_entry_index = shot->sound_pak_entry_id;
    } else {
        shot->sound_pak_entry_id = this->current_midvoc_entry_index;
    }
    this->midvocChooser();
    if (ImGui::Button("Ajouter un effet sonore")) {
        if (shot->sound_pak != nullptr) {
            PakEntry *soundEntry = shot->sound_pak->GetEntry(shot->sound_pak_entry_id);
            if (soundEntry == nullptr || soundEntry->size == 0) {
                ImGui::OpenPopup("ErrorSoundEffect");
            } else {
                shot->sound = new MIDGAME_SOUND();
                shot->sound->data = soundEntry->data;
                shot->sound->size = soundEntry->size;
            }
        }
    }
    if (ImGui::Button("Supprimer l'effet sonore")) {
        if (shot->sound != nullptr) {
            shot->sound->data = nullptr;
            shot->sound->size = 0;
            shot->sound = nullptr;
            shot->sound_pak = nullptr;
            shot->sound_pak_entry_id = -1;
            delete shot->sound;
            shot->sound = nullptr;
            this->current_midvoc = nullptr;
            this->current_midvoc_entry_index = -1;
        }
    }
    if (ImGui::Button("Play Sound Effect")) {
        if (shot->sound == nullptr) {
            ImGui::OpenPopup("ErrorSoundEffect");
        } else {
            if (shot->sound->data && shot->sound->size > 0) {
                RSMixer::getInstance().playSoundVoc(shot->sound->data, shot->sound->size);
            }
        }
    }
}
void initIntHelper(int *value, int defaultValue) {
    if (*value == EMPTY_VALUE || *value != defaultValue) {
        *value = defaultValue;
    }
}
void DebugAnimationPlayer::editMidGameShotBG(MIDGAME_SHOT_BG *bg) {
    static std::vector<GLuint> s_PrevFrameGLTex;
    static std::vector<GLuint> s_CurrentFrameGLTex;

    static int shapeid = EMPTY_VALUE;
    static int palette_id = EMPTY_VALUE;
    static int start_x = EMPTY_VALUE, start_y = EMPTY_VALUE;
    static int end_x = EMPTY_VALUE, end_y = EMPTY_VALUE;
    static int velocity_x = EMPTY_VALUE, velocity_y = EMPTY_VALUE;

    static MIDGAME_SHOT_BG *s_LastEditedBG = nullptr;

    if (s_LastEditedBG != bg) {
        // Réinitialiser toutes les valeurs statiques
        shapeid = bg->pak_entry_id;
        palette_id = bg->palette;
        start_x = bg->position_start.x;
        start_y = bg->position_start.y;
        end_x = bg->position_end.x;
        end_y = bg->position_end.y;
        velocity_x = bg->velocity.x;
        velocity_y = bg->velocity.y;
        
        // Mettre à jour le pointeur
        s_LastEditedBG = bg;
    }



    // Détruire les textures de la frame précédente (elles ont été rendues)
    if (!s_PrevFrameGLTex.empty()) {
        for (GLuint id : s_PrevFrameGLTex) {
            glDeleteTextures(1, &id);
        }
        s_PrevFrameGLTex.clear();
    }
    // Préparer la liste pour cette frame
    s_PrevFrameGLTex.swap(s_CurrentFrameGLTex); // s_CurrentFrameGLTex devient vide

    if (!bg)
        return;
    // ID de forme

    if (bg->pak == &this->optShps) {
        ImGui::Text("Formes dans l'archive OPTSHPS");
    } else if (bg->pak == &this->midgames) {
        ImGui::Text("Formes dans l'archive MIDGAME_SHPS");
    } else {
        for (auto &midarchive : this->mid) {
            if (bg->pak == midarchive) {
                ImGui::Text("Formes dans l'archive %s", midarchive->GetName());
                break;
            }
        }
    }
    if (this->current_mid == nullptr) {
        this->current_mid = bg->pak;
    } else {
        bg->pak = this->current_mid;
    }
    if (this->current_entry_index == -1) {
        this->current_entry_index = bg->pak_entry_id;
    } else {
        bg->pak_entry_id = this->current_entry_index;
    }
    this->current_entry_index = bg->pak_entry_id;
    this->midgameChoser();
    
    if (ImGui::InputInt("ID de forme", &shapeid)) {
        this->current_entry_index = shapeid;
    }
    if (ImGui::Button("Charger l'image")) {
        if (bg->image) {
            delete bg->image;
            bg->image = nullptr;
        }
        bg->image = loadImage(bg->pak, bg->pak_entry_id);
        if (bg->image->palettes.size() > 0) {
            bg->pal = bg->image->palettes[0];
        } else {
            if (bg->pal) {
                bg->pal = nullptr;
            }
        }
    }
    // ID de palette
    if (bg->pak_palette == nullptr) {
        bg->pak_palette = &this->optPals;
    }
    if (this->current_palette_mid == nullptr) {
        this->current_palette_mid = bg->pak_palette;
    } else {
        bg->pak_palette = this->current_palette_mid;
    }
    if (this->current_palette_entry_index == -1) {
        this->current_palette_entry_index = bg->palette;
    } else {
        bg->palette = this->current_palette_entry_index;
    }
    this->current_palette_entry_index = bg->palette;
    
    midgamePaletteChoser();
    if (ImGui::InputInt("ID de palette", &palette_id)) {
        if (palette_id >= 0 && palette_id <= 255) {
            this->current_palette_entry_index = static_cast<uint8_t>(palette_id);
        }
    }
    if (ImGui::Checkbox("Use external palette", &bg->use_external_palette)) {
        bg->use_external_palette = bg->use_external_palette ? 1 : 0;
    }
    ImGui::Separator();
    ImGui::Text("Current position: (%d, %d)", bg->current_position.x, bg->current_position.y);
    // Position de départ
    ImGui::Text("Position de départ");
    
    if (ImGui::InputInt("X##start", &start_x) || ImGui::InputInt("Y##start", &start_y)) {
        bg->position_start.x = start_x;
        bg->position_start.y = start_y;
        bg->current_position.x = start_x;
        bg->current_position.y = start_y;
    }

    // Position finale
    ImGui::Text("Position finale");

    if (ImGui::InputInt("X##end", &end_x) || ImGui::InputInt("Y##end", &end_y)) {
        bg->position_end.x = end_x;
        bg->position_end.y = end_y;
    }

    // Vitesse
    ImGui::Text("Vitesse");

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
            if (preview_frame < 0)
                preview_frame = 0;
            if (preview_frame >= bg->image->GetNumImages())
                preview_frame = bg->image->GetNumImages() - 1;
        }

        if (preview_frame >= 0 && preview_frame < bg->image->GetNumImages()) {
            GLuint glTex =
                renderShape(bg->image->GetShape(preview_frame), bg->pal ? bg->pal->GetColorPalette() : &this->palette);
            ImGui::Image((void *)(uintptr_t)glTex, ImVec2(320, 200));
            // Note: Ne pas oublier de nettoyer la texture dans le code principal après utilisation
            bg->shapeid = preview_frame;
            s_CurrentFrameGLTex.push_back(glTex);
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
    if (ImGui::Button("Remove BG")) {
        bool found = false;
        for (auto &shot : this->midgames_shots[1]) {
            auto it = std::find(shot->background.begin(), shot->background.end(), bg);
            if (it != shot->background.end()) {
                shot->background.erase(it);
                found = true;
            }
            
            // Chercher aussi dans les foregrounds
            if (!found) {
                auto itFg = std::find(shot->foreground.begin(), shot->foreground.end(), bg);
                if (itFg != shot->foreground.end()) {
                    shot->foreground.erase(itFg);
                    found = true;
                }
            }
            
            if (found) {
                // Libérer la mémoire
                if (bg->image) {
                    delete bg->image;
                    bg->image = nullptr;
                }
                if (bg->pal) {
                    delete bg->pal;
                    bg->pal = nullptr;
                }
                delete bg;
                
                // Réinitialiser les pointeurs d'édition
                this->p_bg_editor = nullptr;
                this->p_foreground_editor = nullptr;
                break;
            }
        }
    }
}

void DebugAnimationPlayer::editMidGameShotSprite(MIDGAME_SHOT_SPRITE *sprite) {
    static std::vector<GLuint> s_PrevFrameGLTex;
    static std::vector<GLuint> s_CurrentFrameGLTex;
    // Détruire les textures de la frame précédente (elles ont été rendues)
    if (!s_PrevFrameGLTex.empty()) {
        for (GLuint id : s_PrevFrameGLTex) {
            glDeleteTextures(1, &id);
        }
        s_PrevFrameGLTex.clear();
    }
    // Préparer la liste pour cette frame
    s_PrevFrameGLTex.swap(s_CurrentFrameGLTex); // s_CurrentFrameGLTex devient vide
    if (!sprite)
        return;
    if (sprite->pak == &this->optShps) {
        ImGui::Text("Formes dans l'archive OPTSHPS");
    } else if (sprite->pak == &this->midgames) {
        ImGui::Text("Formes dans l'archive MIDGAME_SHPS");
    } else {
        for (auto &midarchive : this->mid) {
            if (sprite->pak == midarchive) {
                ImGui::Text("Formes dans l'archive %s", midarchive->GetName());
                break;
            }
        }
    }
    if (this->current_mid == nullptr) {
        this->current_mid = sprite->pak;
    } else {
        sprite->pak = this->current_mid;
    }
    if (this->current_entry_index == -1) {
        this->current_entry_index = sprite->pak_entry_id;
    } else {
        sprite->pak_entry_id = this->current_entry_index;
    }
    this->current_entry_index = sprite->pak_entry_id;
    sprite->shapeid = this->current_entry_index;
    this->midgameChoser();
    // ID de forme
    int shapeid = sprite->shapeid;
    if (ImGui::InputInt("ID de forme", &shapeid)) {
        sprite->shapeid = shapeid;
    }
    if (ImGui::Button("Charger l'image")) {
        if (sprite->image) {
            delete sprite->image;
            sprite->image = nullptr;
        }
        sprite->image = loadImage(sprite->pak, sprite->pak_entry_id);
        if (sprite->image->palettes.size() > 0) {
            sprite->pal = sprite->image->palettes[0];
        } else {
            if (sprite->pal) {
                sprite->pal = nullptr;
            }
        }
    }
    if (sprite->pak_palette == nullptr) {
        sprite->pak_palette = &this->optPals;
    }
    if (this->current_palette_mid == nullptr) {
        this->current_palette_mid = sprite->pak_palette;
    } else {
        sprite->pak_palette = this->current_palette_mid;
    }
    if (this->current_palette_entry_index == -1) {
        this->current_palette_entry_index = sprite->palette;
    } else {
        sprite->palette = this->current_palette_entry_index;
    }
    this->current_palette_entry_index = sprite->palette;
    int palette_id = sprite->palette;
    midgamePaletteChoser();
    if (ImGui::InputInt("ID de palette", &palette_id)) {
        if (palette_id >= 0 && palette_id <= 255) {
            this->current_palette_entry_index = static_cast<uint8_t>(palette_id);
        }
    }
    if (ImGui::Checkbox("Use external palette", &sprite->use_external_palette)) {
        sprite->use_external_palette = sprite->use_external_palette ? 1 : 0;
    }
    // Option de conserver la première frame
    bool keep_first = sprite->keep_first_frame != 0;
    if (ImGui::Checkbox("Conserver la première frame", &keep_first)) {
        sprite->keep_first_frame = keep_first ? 1 : 0;
    }
    bool repeate = sprite->repeat_animation != 0;
    if (ImGui::Checkbox("Repeate animation", &repeate)) {
        sprite->repeat_animation = repeate ? 1 : 0;
    }
    // Position de départ
    ImGui::Text("Position de départ");
    int start_x = sprite->position_start.x;
    int start_y = sprite->position_start.y;
    if (ImGui::InputInt("X##start", &start_x) || ImGui::InputInt("Y##start", &start_y)) {
        sprite->position_start.x = start_x;
        sprite->position_start.y = start_y;
        sprite->current_position.x = start_x;
        sprite->current_position.y = start_y;
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
            if (preview_frame < 0)
                preview_frame = 0;
            if (preview_frame >= sprite->image->GetNumImages())
                preview_frame = sprite->image->GetNumImages() - 1;
        }

        if (preview_frame >= 0 && preview_frame < sprite->image->GetNumImages()) {
            GLuint glTex = renderShape(sprite->image->GetShape(preview_frame),
                                       sprite->pal ? sprite->pal->GetColorPalette() : &this->palette);
            ImGui::Image((void *)(uintptr_t)glTex, ImVec2(320, 200));
            s_CurrentFrameGLTex.push_back(glTex);
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

    if (ImGui::Button("Supprimer le sprite")) {
        // Trouver et supprimer le sprite de son shot parent
        for (auto &shot : this->midgames_shots[1]) {
            auto it = std::find(shot->sprites.begin(), shot->sprites.end(), sprite);
            if (it != shot->sprites.end()) {
                // Supprimer du vecteur AVANT de libérer la mémoire
                shot->sprites.erase(it);
                
                // Maintenant on peut libérer la mémoire
                if (sprite->image) {
                    delete sprite->image;
                    sprite->image = nullptr;
                }
                if (sprite->pal) {
                    delete sprite->pal;
                    sprite->pal = nullptr;
                }
                delete sprite;
                
                // Réinitialiser le pointeur d'édition
                this->p_sprite_editor = nullptr;
                break;
            }
        }
    }
}

void DebugAnimationPlayer::midvocChooser() {
    if (ImGui::BeginCombo("MIDVOC Archive",
                          this->current_midvoc != nullptr ? this->current_midvoc->GetName() : "None")) {
        for (auto &midvocarchive : this->midvoc) {
            if (ImGui::Selectable(midvocarchive->GetName(), midvocarchive == this->current_midvoc)) {
                this->current_midvoc = midvocarchive;
            }
        }
        ImGui::EndCombo();
    }
    if (this->current_midvoc != nullptr) {
        ImGui::Text("Num Entries: %d", this->current_midvoc->GetNumEntries());
        if (ImGui::InputInt("Entry Index", &this->current_midvoc_entry_index)) {
            if (this->current_midvoc_entry_index < 0) {
                this->current_midvoc_entry_index = 0;
            }
            if (this->current_midvoc_entry_index >= this->current_midvoc->GetNumEntries()) {
                this->current_midvoc_entry_index = this->current_midvoc->GetNumEntries() - 1;
            }
        }
        if (ImGui::BeginCombo("Sound Pak",
                              this->current_midvoc_entry_index >= 0 &&
                                      this->current_midvoc_entry_index < this->current_midvoc->GetNumEntries()
                                  ? std::to_string(this->current_midvoc_entry_index).c_str()
                                  : "None")) {
            for (int i = 0; i < this->current_midvoc->GetNumEntries(); i++) {
                PakEntry *entry = this->current_midvoc->GetEntry(i);
                if (entry == nullptr) {
                    continue;
                }
                if (ImGui::Selectable(std::to_string(i).c_str(), i == this->current_midvoc_entry_index)) {
                    this->current_midvoc_entry_index = i;
                }
            }
            ImGui::EndCombo();
        }
    }
}