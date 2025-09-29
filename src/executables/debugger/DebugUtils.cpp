#include "DebugUtils.h"

GLuint renderShape(RLEShape *shape, VGAPalette *palette) { 
    if (shape == nullptr || palette == nullptr) {
        return 0;
    }
    FrameBuffer *fb = new FrameBuffer(320, 200);
    fb->fillWithColor(223);
    fb->drawShape(shape);
    Texel* tex = fb->getTexture(palette);
    GLuint glTex = 0;
    glGenTextures(1, &glTex);
    glBindTexture(GL_TEXTURE_2D, glTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 320, 200, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex);
    delete fb;
    delete tex;
    return glTex;
}