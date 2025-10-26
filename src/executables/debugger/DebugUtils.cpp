#include "DebugUtils.h"
#include <imgui.h>
#include <imgui_impl_opengl2.h>
#include <imgui_impl_sdl2.h>

GLuint renderFrameBuffer(FrameBuffer *fb, VGAPalette *palette) { 
    if (fb == nullptr) {
        return 0;
    }
    Texel* tex = fb->getTexture(palette);
    GLuint glTex = 0;
    glGenTextures(1, &glTex);
    glBindTexture(GL_TEXTURE_2D, glTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fb->width, fb->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex);
    delete tex;
    return glTex;
}

GLuint renderShape(RLEShape *shape, VGAPalette *palette) { 
    if (shape == nullptr || palette == nullptr) {
        return 0;
    }
    FrameBuffer *fb = new FrameBuffer(320, 200);
    fb->fillWithColor(223);
    fb->drawShape(shape);
    GLuint glTex = renderFrameBuffer(fb, palette);
    delete fb;
    return glTex;
}

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