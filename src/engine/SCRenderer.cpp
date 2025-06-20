//
//  gfx.cpp
//  iff
//
//  Created by Fabien Sanglard on 12/21/2013.
//  Copyright (c) 2013 Fabien Sanglard. All rights reserved.
//

#include "SCRenderer.h"
#include "../realspace/AssetManager.h"
#include "../realspace/RSArea.h"
#include "../realspace/RSEntity.h"
#include "../realspace/RSPalette.h"

#include "RSVGA.h"
#include "Texture.h"
#include <SDL_opengl_glext.h>
#include <SDL_opengl.h>




extern SCRenderer Renderer;

SCRenderer::SCRenderer() : initialized(false) {}

SCRenderer::~SCRenderer() {}

Camera *SCRenderer::getCamera(void) { return &this->camera; }

VGAPalette *SCRenderer::getPalette(void) { return &this->palette; }

void SCRenderer::setPlayerPosition(Point3D *position) { camera.SetPosition(position); }

void SCRenderer::init(int width, int height, AssetManager *amana) {
    this->assets = amana;

    this->counter = 0;

    RSPalette palette;
    palette.initFromFileData(this->assets->GetFileData("PALETTE.IFF"));
    this->palette = *palette.GetColorPalette();

    this->width = width;
    this->height = height;

    glClearColor(0.0f, 0.5f, 1.0f, 1.0f); // Black Background
    // glClearDepth(1.0f);								// Depth Buffer Setup
    glDisable(GL_DEPTH_TEST); // Disable Depth Testing

    camera.SetPersective(45.0f, this->width / (float)this->height, 1.0f, BLOCK_WIDTH * BLOCK_PER_MAP_SIDE * 4);

    light.SetWithCoo(300, 300, 300);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 800, 600, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    initialized = true;
}

void SCRenderer::setClearColor(uint8_t red, uint8_t green, uint8_t blue) {
    if (!initialized)
        return;

    glClearColor(red / 255.0f, green / 255.0f, blue / 255.0f, 1.0f);
}

void SCRenderer::clear(void) {

    if (!initialized)
        return;
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    
}

void SCRenderer::createTextureInGPU(Texture *texture) {

    if (!initialized)
        return;

    glGenTextures(1, &texture->id);
    glBindTexture(GL_TEXTURE_2D, texture->id);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_ADD);

    glTexImage2D(GL_TEXTURE_2D, 0, 4, (GLsizei)texture->width, (GLsizei)texture->height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 texture->data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

void SCRenderer::uploadTextureContentToGPU(Texture *texture) {
    if (!initialized)
        return;
    glBindTexture(GL_TEXTURE_2D, texture->id);
    glTexImage2D(GL_TEXTURE_2D, 0, 4, (GLsizei)texture->width, (GLsizei)texture->height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 texture->data);
}

void SCRenderer::deleteTextureInGPU(Texture *texture) {
    if (!initialized)
        return;
    glDeleteTextures(1, &texture->id);
}

void SCRenderer::getNormal(RSEntity *object, Triangle *triangle, Vector3D *normal) {
    // Calculate the normal for this triangle
    Vector3D edge1;
    edge1 = object->vertices[triangle->ids[0]];
    edge1.Substract(&object->vertices[triangle->ids[1]]);

    Vector3D edge2;
    edge2 = object->vertices[triangle->ids[2]];
    edge2.Substract(&object->vertices[triangle->ids[1]]);

    *normal = edge1;
    *normal = normal->CrossProduct(&edge2);
    normal->Normalize();

    // All normals are supposed to point outward in modern GPU but SC triangles
    // don't have consistent winding. They can be CW or CCW (the back governal of a jet  is
    // typically one triangle that must be visible from both sides !
    // As a result, gouraud shading was probably performed in screen space.
    // How can we replicate this ?
    //        - Take the normal and compare it to the sign of the direction to the camera.
    //        - If the signs don't match: reverse the normal.
    Point3D cameraPosition = camera.GetPosition();

    Point3D *vertexOnTriangle = &object->vertices[triangle->ids[0]];

    Point3D cameraDirection;
    cameraDirection = cameraPosition;
    cameraDirection.Substract(vertexOnTriangle);
    cameraDirection.Normalize();

    if (cameraDirection.DotProduct(normal) < 0) {
        normal->Negate();
    }
}

void SCRenderer::drawParticle(Vector3D pos, float alpha) {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glPushMatrix();
    Matrix smoke_rotation;
    smoke_rotation.Clear();
    smoke_rotation.Identity();
    smoke_rotation.translateM(pos.x, pos.y, pos.z);
    glMultMatrixf((float *)smoke_rotation.v);
    glBegin(GL_QUADS);
    glColor4f(1.0f,1.0f,1.0f, alpha);
    glVertex3f(1.0f,-1.0f,-1.0f);
    glVertex3f(1.0f,1.0f,-1.0f);
    glVertex3f(-1.0f,1.0f,-1.0f);
    glVertex3f(-1.0f,-1.0f,1.0f);
    glEnd();
    glBegin(GL_QUADS);
    glColor4f(1.0f,1.0f,1.0f, alpha);
    glVertex3f(-1.0f,-1.0f,-1.0f);
    glVertex3f(-1.0f,1.0f,-1.0f);
    glVertex3f(1.0f,1.0f,1.0f);
    glVertex3f(1.0f,-1.0f,1.0f);
    glEnd();
    glPopMatrix();
    glDisable( GL_BLEND );
}
void SCRenderer::drawModel(RSEntity *object, size_t lodLevel, Vector3D position, Vector3D orientation, Vector3D ajustement) { 
    glPushMatrix();
    glTranslatef(static_cast<GLfloat>(position.x), static_cast<GLfloat>(position.y),
                static_cast<GLfloat>(position.z));
    glRotatef(orientation.x, 0, 1, 0);
    glRotatef(orientation.y, 0, 0, 1);
    glRotatef(orientation.z, 1, 0, 0);
    glTranslatef(ajustement.x, ajustement.y, ajustement.z);
    drawModel(object, 0);
    glPopMatrix(); 
}
void SCRenderer::drawModel(RSEntity *object, size_t lodLevel, Vector3D position, Vector3D orientation) {
    glPushMatrix();
    glTranslatef(static_cast<GLfloat>(position.x), static_cast<GLfloat>(position.y),
                static_cast<GLfloat>(position.z));
    glRotatef(orientation.x, 0, 1, 0);
    glRotatef(orientation.y, 0, 0, 1);
    glRotatef(orientation.z, 1, 0, 0);
    drawModel(object, 0);
    glPopMatrix();
}
void SCRenderer::drawPoint(Vector3D point, Vector3D color, Vector3D pos, Vector3D orientation) {
    glPushMatrix();
    glTranslatef(pos.x, pos.y, pos.z);
    glRotatef(orientation.x, 0, 1, 0);
    glRotatef(orientation.y, 0, 0, 1);
    glRotatef(orientation.z, 1, 0, 0);
    glPointSize(5.0f);
    glBegin(GL_POINTS);
    glColor3f(color.x, color.y, color.z);
    glVertex3f(point.x, point.y, point.z);
    glEnd();
    glPopMatrix();
}
void SCRenderer::drawModel(RSEntity *object, Vector3D position, Vector3D orientation) {
    glPushMatrix();
    Matrix rotation;
    rotation.Clear();
    rotation.Identity();
    rotation.translateM(position.x, position.y, position.z);
    rotation.rotateM(orientation.x, 0.0f, 1.0f, 0.0f);
    rotation.rotateM(orientation.y, 0.0f, 0.0f, 1.0f);
    rotation.rotateM(orientation.z, 1.0f, 0.0f, 0.0f);
    
    glMultMatrixf((float *)rotation.v);
    drawModel(object, 0);
    glPopMatrix();
}
void SCRenderer::drawLine(Vector3D start, Vector3D end, Vector3D color, Vector3D orientation) {
    glPushMatrix();
    glTranslatef(start.x, start.y, start.z);
    glRotatef(orientation.x, 0, 1, 0);
    glRotatef(orientation.y, 0, 0, 1);
    glRotatef(orientation.z, 1, 0, 0);
    glLineWidth(1.2f);
    glBegin(GL_LINES);
    glColor3f(color.x, color.y, color.z);
    glVertex3f(0.0f,0.0f,0.0f);
    glVertex3f(end.x, end.y, end.z);
    glEnd();
    glPopMatrix();
}
void SCRenderer::drawLine(Vector3D start, Vector3D end, Vector3D color, Vector3D orientation, Vector3D position) {
    glPushMatrix();
    glTranslatef(position.x, position.y, position.z);
    glRotatef(orientation.x, 0, 1, 0);
    glRotatef(orientation.y, 0, 0, 1);
    glRotatef(orientation.z, 1, 0, 0);
    glLineWidth(1.2f);
    glBegin(GL_LINES);
    glColor3f(color.x, color.y, color.z);
    glVertex3f(start.x, start.y, start.z);
    glVertex3f(end.x, end.y, end.z);
    glEnd();
    glPopMatrix();
}
void SCRenderer::drawLine(Vector3D start, Vector3D end, Vector3D color) {
    glPushMatrix();
    glTranslatef(start.x, start.y, start.z);
    glLineWidth(1.2f);
    glBegin(GL_LINES);
    glColor3f(color.x, color.y, color.z);
    glVertex3f(0.0f,0.0f,0.0f);
    glVertex3f(end.x, end.y, end.z);
    glEnd();
    glPopMatrix();
}
void SCRenderer::drawSprite(Vector3D pos, Texture *tex, float zoom) {
    size_t cpt=0;
    
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_ADD);
    glEnable(GL_BLEND);
    
    glPushMatrix();
    Matrix smoke_rotation;
    smoke_rotation.Clear();
    smoke_rotation.Identity();
    smoke_rotation.translateM(pos.x, pos.y, pos.z);
    smoke_rotation.rotateM(0.0f, 1.0f, 0.0f, 0.0f);

    glMultMatrixf((float *)smoke_rotation.v);
    if (tex->initialized == false) {
        glGenTextures(1, &tex->id);
        glBindTexture(GL_TEXTURE_2D, tex->id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        // Upload pixels into texture
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)tex->width, (GLsizei)tex->height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
            tex->data);
        
        tex->initialized = true;
    } else {
        glBindTexture(GL_TEXTURE_2D, tex->id);
    }
    float smoke_size = 4.0f * zoom + 1.0f;
    glBegin(GL_QUADS);
    glColor4f(1.0f,1.0f,1.0f,0.0f);
    glTexCoord2f (0.0, 0.0);
    glVertex3f(smoke_size,-smoke_size,-smoke_size);
    glTexCoord2f (1.0, 0.0);
    glVertex3f(smoke_size,smoke_size,-smoke_size);
    glTexCoord2f (1.0, 1.0);
    glVertex3f(-smoke_size,smoke_size,-smoke_size);
    glTexCoord2f (0.0, 1.0);
    glVertex3f(-smoke_size,-smoke_size,smoke_size);
    glEnd();
    glBegin(GL_QUADS);
    glColor4f(1.0f,1.0f,1.0f,0.0f);
    glTexCoord2f (0.0, 0.0);
    glVertex3f(-smoke_size,-smoke_size,-smoke_size);
    glTexCoord2f (1.0, 0.0);
    glVertex3f(-smoke_size,smoke_size,-smoke_size);
    glTexCoord2f (1.0, 1.0);
    glVertex3f(smoke_size,smoke_size,smoke_size);
    glTexCoord2f (0.0, 1.0);
    glVertex3f(smoke_size,-smoke_size,smoke_size);
    glEnd();
    glPopMatrix();
    glDisable( GL_BLEND );
    glDisable(GL_TEXTURE_2D);
}
void SCRenderer::drawModelWithChilds(
    RSEntity *object,
    size_t lodLevel, 
    Vector3D position,
    Vector3D orientation, 
    int wheel_index, 
    int thrust, 
    std::vector<std::tuple<Vector3D, RSEntity *>> weaps_load
) {
    if (object != nullptr) {
        glPushMatrix();
        Matrix rotation;
        rotation.Clear();
        rotation.Identity();
        rotation.translateM(position.x, position.y, position.z);
        rotation.rotateM(orientation.x, 0.0f, 1.0f, 0.0f);
        rotation.rotateM(orientation.y, 0.0f, 0.0f, 1.0f);
        rotation.rotateM(orientation.z, 1.0f, 0.0f, 0.0f);

        glMultMatrixf((float *)rotation.v);
        
        Renderer.drawModel(object, LOD_LEVEL_MAX);
        if (wheel_index) {
            if (object->chld.size() > wheel_index) {
                Renderer.drawModel(object->chld[wheel_index]->objct, LOD_LEVEL_MAX);
            }
        }
        if (thrust > 50) {
            if (object->chld.size() > 0) {
                glPushMatrix();
                Vector3D pos = {
                    (float) object->chld[0]->x,
                    (float) object->chld[0]->y,
                    (float) object->chld[0]->z
                };
                glTranslatef(pos.z/250 , pos.y /250 , pos.x /250);
                glScalef(1+thrust/100.0f,1,1);
                Renderer.drawModel(object->chld[0]->objct, LOD_LEVEL_MAX);
                glPopMatrix();
            }
        }
        for (auto weaps_it: weaps_load) {
            float decy=0.5f;
            Vector3D weaps_pos = std::get<0>(weaps_it);
            RSEntity *weaps = std::get<1>(weaps_it);
            if (weaps == nullptr) {
                continue;  
            }
            
            glPushMatrix();
            glTranslatef(weaps_pos.x, weaps_pos.y, weaps_pos.z);
            Renderer.drawModel(weaps, LOD_LEVEL_MAX);
            glPopMatrix();            
        }
        glPopMatrix();
    }
}
void SCRenderer::drawModel(RSEntity *object, size_t lodLevel) {
    if (!initialized)
        return;

    if (object->vertices.size() == 0)
        return;

    if (lodLevel >= object->NumLods()) {
        printf("Unable to render this Level Of Details (out of range): Max level is  %llu %llu\n",
               (object->NumLods() - 1), lodLevel);
        return;
    }

    float ambientLamber = 0.4f;

    Lod *lod = &object->lods[lodLevel];
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Texture pass
    if (lodLevel == 0) {
        glEnable(GL_TEXTURE_2D);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
        glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_ADD);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);
        // glAlphaFunc ( GL_ALWAYS, 1.0f ) ;
        // glEnable ( GL_ALPHA_TEST ) ;

        for (int i = 0; i < object->NumUVs(); i++) {

            uvxyEntry *textInfo = &object->uvs[i];

            // Seems we have a textureID that we don't have :( !
            if (textInfo->textureID >= object->images.size())
                continue;

            RSImage *image = object->images[textInfo->textureID];

            Texture *texture = image->GetTexture();
            Triangle *triangle = &object->triangles[textInfo->triangleID];
            float alpha = 1.0f;
            int colored = 0;
            if (triangle->property == 6) {
                alpha = 0.0f;
            }
            if (triangle->property == 7) {
                alpha = 1.0f;
                colored = 1;
            }
            if (triangle->property == 8) {
                alpha = 1.0f;
                colored = 1;
            }
            if (triangle->property == 9) {
                alpha = 0.0f;
            }

            glBindTexture(GL_TEXTURE_2D, texture->id);
            Vector3D normal;
            getNormal(object, triangle, &normal);

            glBegin(GL_TRIANGLES);
            for (int j = 0; j < 3; j++) {

                Point3D vertice = object->vertices[triangle->ids[j]];

                Vector3D lighDirection;
                lighDirection = light;
                lighDirection.Substract(&vertice);
                lighDirection.Normalize();

                float lambertianFactor = lighDirection.DotProduct(&normal);
                if (lambertianFactor < 0)
                    lambertianFactor = 0;

                lambertianFactor += ambientLamber;
                if (lambertianFactor > 1)
                    lambertianFactor = 1;

                const Texel *texel = palette.GetRGBColor(triangle->color);

                //
                // glColor4f(texel->r/255.0f, texel->g/255.0f, texel->b/255.0f,alpha);
                // glColor4f(0, 0, 0,1);
                if (colored) {
                    glColor4f(texel->r / 255.0f * lambertianFactor, texel->g / 255.0f * lambertianFactor,
                              texel->b / 255.0f * lambertianFactor, alpha);
                } else {
                    glColor4f(lambertianFactor, lambertianFactor, lambertianFactor, alpha);
                }
                glTexCoord2f(textInfo->uvs[j].u / (float)texture->width, textInfo->uvs[j].v / (float)texture->height);
                glVertex3f(object->vertices[triangle->ids[j]].x, object->vertices[triangle->ids[j]].y,
                           object->vertices[triangle->ids[j]].z);
            }
            glEnd();
        }

        for (auto quv: object->qmapuvs) {
            if (quv->textureID >= object->images.size())
                continue;

            RSImage *image = object->images[quv->textureID];

            Texture *texture = image->GetTexture();
            Quads *triangle = object->quads[quv->triangleID];
            float alpha = 1.0f;
            int colored = 0;
            if (triangle->property == 6) {
                alpha = 0.0f;
            }
            if (triangle->property == 7) {
                alpha = 1.0f;
                colored = 1;
            }
            if (triangle->property == 8) {
                alpha = 1.0f;
                colored = 1;
            }
            if (triangle->property == 9) {
                alpha = 0.0f;
            }

            glBindTexture(GL_TEXTURE_2D, texture->id);
            Vector3D normal;
            Triangle *tri = new Triangle();
            tri->ids[0] = triangle->ids[0];
            tri->ids[1] = triangle->ids[1];
            tri->ids[2] = triangle->ids[2];

            getNormal(object, tri, &normal);

            glBegin(GL_QUADS);
            for (int j = 0; j < 4; j++) {

                Point3D vertice = object->vertices[triangle->ids[j]];

                Vector3D lighDirection;
                lighDirection = light;
                lighDirection.Substract(&vertice);
                lighDirection.Normalize();

                float lambertianFactor = lighDirection.DotProduct(&normal);
                if (lambertianFactor < 0)
                    lambertianFactor = 0;

                lambertianFactor += ambientLamber;
                if (lambertianFactor > 1)
                    lambertianFactor = 1;

                const Texel *texel = palette.GetRGBColor(triangle->color);

                //
                // glColor4f(texel->r/255.0f, texel->g/255.0f, texel->b/255.0f,alpha);
                // glColor4f(0, 0, 0,1);
                if (colored) {
                    glColor4f(texel->r / 255.0f * lambertianFactor, texel->g / 255.0f * lambertianFactor,
                              texel->b / 255.0f * lambertianFactor, alpha);
                } else {
                    glColor4f(lambertianFactor, lambertianFactor, lambertianFactor, alpha);
                }
                glTexCoord2f(quv->uvs[j].u / (float)texture->width, quv->uvs[j].v / (float)texture->height);
                glVertex3f(object->vertices[triangle->ids[j]].x, object->vertices[triangle->ids[j]].y,
                           object->vertices[triangle->ids[j]].z);
            }
            glEnd();
        }
        glDisable(GL_BLEND);
        glDisable(GL_TEXTURE_2D);
    }

    // Pass 3: Let's draw the transparent stuff render RSEntity::SC_TRANSPARENT)
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
#ifndef _WIN32
    glBlendEquation(GL_ADD);
#else
    typedef void(APIENTRY * PFNGLBLENDEQUATIONPROC)(GLenum mode);
    PFNGLBLENDEQUATIONPROC glBlendEquation = NULL;
    glBlendEquation = (PFNGLBLENDEQUATIONPROC)wglGetProcAddress("glBlendEquation");
    glBlendEquation(GL_FUNC_ADD);
#endif
    for (int i = 0; i < lod->numTriangles; i++) {
        uint16_t triangleID = lod->triangleIDs[i];
        if (object->attrs.size() > 0) {
            if (object->attrs[triangleID] == nullptr) {
                continue;
            }
            if (object->attrs[triangleID]->type == 'Q') {
                continue;
            }
            if (object->attrs[triangleID]->type == 'L') {
                continue;
            }
            triangleID = object->attrs[triangleID]->id;
        }
        if (triangleID >= object->triangles.size()) {
            continue;
        }
        Triangle *triangle = &object->triangles[triangleID];

        if (triangle->property != RSEntity::SC_TRANSPARENT)
            continue;

        Vector3D normal;
        getNormal(object, triangle, &normal);

        glBegin(GL_TRIANGLES);
        for (int j = 0; j < 3; j++) {

            Point3D vertice = object->vertices[triangle->ids[j]];

            Vector3D sunDirection;
            sunDirection = light;
            sunDirection.Substract(&vertice);
            sunDirection.Normalize();

            float lambertianFactor = sunDirection.DotProduct(&normal);
            if (lambertianFactor < 0)
                lambertianFactor = 0;

            lambertianFactor = 0.2f;

            // int8_t gouraud = 255 * lambertianFactor;

            // gouraud = 255;

            glColor4f(lambertianFactor, lambertianFactor, lambertianFactor, 1);

            glVertex3f(vertice.x, vertice.y, vertice.z);
        }
        glEnd();
    }

    if (object->quads.size() > 0) {
        for (int i = 0; i < lod->numTriangles; i++) {
            uint16_t triangleID = lod->triangleIDs[i];
            if (object->attrs.size() > 0) {
                if (object->attrs[triangleID]->type == 'T') {
                    continue;
                }
                if (object->attrs[triangleID]->type == 'L') {
                    continue;
                }
                triangleID = object->attrs[triangleID]->id;
            }
            if (triangleID >= object->triangles.size()) {
                continue;
            }
            Quads *triangle = object->quads[triangleID];
    
            if (triangle->property != RSEntity::SC_TRANSPARENT)
                continue;
    
            Vector3D normal;
            Triangle *tri = new Triangle();
            tri->ids[0] = triangle->ids[0];
            tri->ids[1] = triangle->ids[1];
            tri->ids[2] = triangle->ids[2];
    
            getNormal(object, tri, &normal);
    
            glBegin(GL_QUADS);
            for (int j = 0; j < 4; j++) {
    
                Point3D vertice = object->vertices[triangle->ids[j]];
    
                Vector3D sunDirection;
                sunDirection = light;
                sunDirection.Substract(&vertice);
                sunDirection.Normalize();
    
                float lambertianFactor = sunDirection.DotProduct(&normal);
                if (lambertianFactor < 0)
                    lambertianFactor = 0;
    
                lambertianFactor = 0.2f;
    
                // int8_t gouraud = 255 * lambertianFactor;
    
                // gouraud = 255;
    
                glColor4f(lambertianFactor, lambertianFactor, lambertianFactor, 1);
    
                glVertex3f(vertice.x, vertice.y, vertice.z);
            }
            glEnd();
        }    
    }
    
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Pass 1, draw color
    for (int i = 0; i < lod->numTriangles; i++) {
        // for(int i = 60 ; i < 62 ; i++){  //Debug purpose only back governal of F-16 is 60-62
        
        uint16_t triangleID = lod->triangleIDs[i];
        if (object->attrs.size() > 0) {
            if (object->attrs[triangleID] == nullptr) {
                continue;
            }
            if (object->attrs[triangleID]->type == 'Q') {
                continue;
            }
            if (object->attrs[triangleID]->type == 'L') {
                continue;
            }
            triangleID = object->attrs[triangleID]->id;
        }
        if (triangleID >= object->triangles.size()) {
            continue;
        }
        Triangle *triangle = &object->triangles[triangleID];

        if (triangle->property == RSEntity::SC_TRANSPARENT)
            continue;
        if (triangle->property == 6) {
            continue;
        }
        if (triangle->property == 7) {
            continue;
        }
        if (triangle->property == 8) {
            continue;
        }
        if (triangle->property == 9) {
            continue;
        }
        Vector3D normal;
        getNormal(object, triangle, &normal);

        glBegin(GL_TRIANGLES);
        for (int j = 0; j < 3; j++) {

            Point3D vertice = object->vertices[triangle->ids[j]];

            Vector3D lighDirection;
            lighDirection = light;
            lighDirection.Substract(&vertice);
            lighDirection.Normalize();

            float lambertianFactor = lighDirection.DotProduct(&normal);
            if (lambertianFactor < 0)
                lambertianFactor = 0;

            lambertianFactor += ambientLamber;
            if (lambertianFactor > 1)
                lambertianFactor = 1;

            const Texel *texel = palette.GetRGBColor(triangle->color);

            glColor4f(texel->r / 255.0f * lambertianFactor, texel->g / 255.0f * lambertianFactor,
                      texel->b / 255.0f * lambertianFactor, texel->a);
            // glColor4f(texel->r/255.0f, texel->g/255.0f, texel->b/255.0f,1);

            glVertex3f(object->vertices[triangle->ids[j]].x, object->vertices[triangle->ids[j]].y,
                       object->vertices[triangle->ids[j]].z);
        }
        glEnd();
    }
    if (object->quads.size() > 0) {
        for (int i = 0; i < lod->numTriangles; i++) {
            uint16_t triangleID = lod->triangleIDs[i];
            if (object->attrs.size() > 0) {
                if (object->attrs[triangleID]->type == 'T') {
                    continue;
                }
                if (object->attrs[triangleID]->type == 'L') {
                    continue;
                }
                triangleID = object->attrs[triangleID]->id;
            }
            if (triangleID >= object->quads.size()) {
                continue;
            }
            Quads *triangle = object->quads[triangleID];
    
            if (triangle->property == RSEntity::SC_TRANSPARENT)
                continue;
            if (triangle->property == 6) {
                continue;
            }
            if (triangle->property == 7) {
                continue;
            }
            if (triangle->property == 8) {
                continue;
            }
            if (triangle->property == 9) {
                continue;
            }
            Vector3D normal;
            Triangle *tri = new Triangle();
            tri->ids[0] = triangle->ids[0];
            tri->ids[1] = triangle->ids[1];
            tri->ids[2] = triangle->ids[2];
    
            getNormal(object, tri, &normal);
    
            glBegin(GL_QUADS);
            for (int j = 0; j < 4; j++) {
    
                Vector3D vertice = object->vertices[triangle->ids[j]];
    
                Vector3D lighDirection;
                lighDirection = light;
                lighDirection.Substract(&vertice);
                lighDirection.Normalize();
    
                float lambertianFactor = lighDirection.DotProduct(&normal);
                if (lambertianFactor < 0)
                    lambertianFactor = 0;
    
                lambertianFactor += ambientLamber;
                if (lambertianFactor > 1)
                    lambertianFactor = 1;
    
                const Texel *texel = palette.GetRGBColor(triangle->color);
    
                glColor4f(texel->r / 255.0f * lambertianFactor, texel->g / 255.0f * lambertianFactor,
                          texel->b / 255.0f * lambertianFactor, texel->a);
    
                glVertex3f(vertice.x, vertice.y,
                    vertice.z);
            }
            glEnd();
        }
    }
    

    glDisable(GL_BLEND);
}

void SCRenderer::setLight(Point3D *l) { this->light = *l; }

void SCRenderer::prepare(RSEntity *object) {

    for (size_t i = 0; i < object->NumImages(); i++) {
        object->images[i]->SyncTexture();
    }

    object->prepared = true;
}

void SCRenderer::displayModel(RSEntity *object, size_t lodLevel) {

    if (!initialized)
        return;

    if (object->IsPrepared())
        prepare(object);

    glMatrixMode(GL_PROJECTION);
    Matrix *projectionMatrix = camera.GetProjectionMatrix();
    glLoadMatrixf(projectionMatrix->ToGL());

    running = true;
    float counter = 0;
    while (running) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        Matrix *modelViewMatrix;

        light.x = 20.0f * cosf(counter);
        light.y = 10.0f;
        light.z = 20.0f * sinf(counter);
        counter += 0.02f;

        // camera.SetPosition(position);

        modelViewMatrix = camera.GetViewMatrix();
        glLoadMatrixf(modelViewMatrix->ToGL());

        drawModel(object, lodLevel);

        // Render light

        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glPointSize(6);

        glBegin(GL_POINTS);
        glColor4f(1, 1, 0, 1);
        glVertex3f(light.x, light.y, light.z);
        glEnd();
    }
}

#define TEX_ZERO (0 / 64.0f)
#define TEX_ONE (64 / 64.0f)

// What is this offset ? It is used to get rid of the red delimitations
// in the 64x64 textures.
#define OFFSET (2 / 64.0f)
float textTrianCoo64[2][3][2] = {

    {{TEX_ZERO, TEX_ZERO + OFFSET},
     {TEX_ONE - 2 * OFFSET, TEX_ONE - OFFSET},
     {TEX_ZERO, TEX_ONE - OFFSET}}, // LOWER_TRIANGE

    {{TEX_ZERO + 2 * OFFSET, TEX_ZERO + OFFSET},
     {TEX_ONE, TEX_ZERO + OFFSET},
     {TEX_ONE, TEX_ONE - OFFSET}} // UPPER_TRIANGE
};

float textTrianCoo[2][3][2] = {

    {{TEX_ZERO, TEX_ZERO}, {TEX_ONE, TEX_ONE}, {TEX_ZERO, TEX_ONE}}, // LOWER_TRIANGE

    {{TEX_ZERO, TEX_ZERO}, {TEX_ONE, TEX_ZERO}, {TEX_ONE, TEX_ONE}} // UPPER_TRIANGE
};

#define LOWER_TRIANGE 0
#define UPPER_TRIANGE 1

void SCRenderer::renderTexturedTriangle(MapVertex *tri0, MapVertex *tri1, MapVertex *tri2, RSArea *area,
                                        int triangleType, RSImage *image) {

    int mainColor = 0;
    if (tri0->type != tri1->type || tri0->type != tri2->type) {
        mainColor = 1;
        if (tri1->type > tri0->type)
            if (tri1->type > tri2->type)
                glColor4fv(tri1->color);
            else
                glColor4fv(tri2->color);
        else if (tri0->type > tri2->type)
            glColor4fv(tri0->color);
        else
            glColor4fv(tri2->color);
    }

    if (image->width == 64) {
        glTexCoord2fv(textTrianCoo64[triangleType][0]);
        if (!mainColor) {
            glColor4fv(tri0->color);
        }
        glVertex3f(tri0->v.x, tri0->v.y, tri0->v.z);

        if (!mainColor) {
            glColor4fv(tri1->color);
        }
        glTexCoord2fv(textTrianCoo64[triangleType][1]);
        glVertex3f(tri1->v.x, tri1->v.y, tri1->v.z);

        if (!mainColor) {
            glColor4fv(tri2->color);
        }
        glTexCoord2fv(textTrianCoo64[triangleType][2]);
        glVertex3f(tri2->v.x, tri2->v.y, tri2->v.z);
    } else {
        glTexCoord2fv(textTrianCoo[triangleType][0]);
        if (!mainColor) {
            glColor4fv(tri0->color);
        }
        glVertex3f(tri0->v.x, tri0->v.y, tri0->v.z);

        glTexCoord2fv(textTrianCoo[triangleType][1]);
        if (!mainColor) {
            glColor4fv(tri1->color);
        }
        glVertex3f(tri1->v.x, tri1->v.y, tri1->v.z);

        glTexCoord2fv(textTrianCoo[triangleType][2]);
        if (!mainColor) {
            glColor4fv(tri2->color);
        }
        glVertex3f(tri2->v.x, tri2->v.y, tri2->v.z);
    }
}

bool SCRenderer::isTextured(MapVertex *tri0, MapVertex *tri1, MapVertex *tri2) {
    return
        // tri0->type != tri1->type ||
        // tri0->type != tri2->type ||
        tri0->upperImageID == 0xFF || tri0->lowerImageID == 0xFF;
}
void SCRenderer::renderColoredTriangle(MapVertex *tri0, MapVertex *tri1, MapVertex *tri2) {

    if (tri0->type != tri1->type || tri0->type != tri2->type

    ) {

        if (tri1->type > tri0->type)
            if (tri1->type > tri2->type)
                glColor4fv(tri1->color);
            else
                glColor4fv(tri2->color);
        else if (tri0->type > tri2->type)
            glColor4fv(tri0->color);
        else
            glColor4fv(tri2->color);

        glVertex3f(tri0->v.x, tri0->v.y, tri0->v.z);

        glVertex3f(tri1->v.x, tri1->v.y, tri1->v.z);

        glVertex3f(tri2->v.x, tri2->v.y, tri2->v.z);
    } else {
        glColor4fv(tri0->color);
        glVertex3f(tri0->v.x, tri0->v.y, tri0->v.z);

        glColor4fv(tri1->color);
        glVertex3f(tri1->v.x, tri1->v.y, tri1->v.z);

        glColor4fv(tri2->color);
        glVertex3f(tri2->v.x, tri2->v.y, tri2->v.z);
    }
}

void SCRenderer::renderQuad(MapVertex *currentVertex, MapVertex *rightVertex, MapVertex *bottomRightVertex,
                            MapVertex *bottomVertex, RSArea *area, bool renderTexture) {

    if (!renderTexture) {
        if (currentVertex->lowerImageID == 0xFF) {
            // Render lower triangle
            renderColoredTriangle(currentVertex, bottomRightVertex, bottomVertex);
        }
        if (currentVertex->upperImageID == 0xFF) {
            // Render Upper triangles
            renderColoredTriangle(currentVertex, rightVertex, bottomRightVertex);
        }
    } else {

        if (currentVertex->lowerImageID != 0xFF) {
            VertexVector &vcache = textureSortedVertex[currentVertex->lowerImageID];
            VertexCache v = {currentVertex, bottomRightVertex, bottomVertex};
            v.lv1 = currentVertex;
            v.lv2 = bottomRightVertex;
            v.lv3 = bottomVertex;
            v.uv1 = v.uv2 = v.uv3 = NULL;
            vcache.push_back(v);
        }
        if (currentVertex->upperImageID != 0xFF) {
            VertexVector &vcache = textureSortedVertex[currentVertex->upperImageID];
            VertexCache v;
            v.uv1 = currentVertex;
            v.uv2 = rightVertex;
            v.uv3 = bottomRightVertex;
            v.lv1 = v.lv2 = v.lv3 = NULL;
            vcache.push_back(v);
        }
    }
}

void SCRenderer::renderBlock(RSArea *area, int LOD, int i, bool renderTexture) {

    AreaBlock *block = area->GetAreaBlockByID(LOD, i);
    if (block == nullptr) {
        return;
    }
    uint32_t sideSize = block->sideSize;

    // printf("Rendering block %d at x %f,z %f\n", i, block->vertice[0].v.x, block->vertice[0].v.z);
    for (size_t x = 0; x < sideSize - 1; x++) {
        for (size_t y = 0; y < sideSize - 1; y++) {

            MapVertex *currentVertex = &block->vertice[x + y * sideSize];
            MapVertex *rightVertex = &block->vertice[(x + 1) + y * sideSize];
            MapVertex *bottomRightVertex = &block->vertice[(x + 1) + (y + 1) * sideSize];
            MapVertex *bottomVertex = &block->vertice[x + (y + 1) * sideSize];

            renderQuad(currentVertex, rightVertex, bottomRightVertex, bottomVertex, area, renderTexture);
        }
    }

    // Inter-block right side
    if (i % BLOCK_PER_MAP_SIDE != 17) {
        AreaBlock *currentBlock = area->GetAreaBlockByID(LOD, static_cast<size_t>(i));
        AreaBlock *rightBlock = area->GetAreaBlockByID(LOD, static_cast<size_t>(i + 1));

        for (uint32_t y = 0; y < sideSize - 1; y++) {
            MapVertex *currentVertex = currentBlock->GetVertice(currentBlock->sideSize - 1, y);
            MapVertex *rightVertex = rightBlock->GetVertice(0, y);
            MapVertex *bottomRightVertex = rightBlock->GetVertice(0, y + 1);
            MapVertex *bottomVertex = currentBlock->GetVertice(currentBlock->sideSize - 1, y + 1);

            renderQuad(currentVertex, rightVertex, bottomRightVertex, bottomVertex, area, renderTexture);
        }
    }

    // Inter-block bottom side
    if (i / BLOCK_PER_MAP_SIDE != 17) {

        AreaBlock *currentBlock = area->GetAreaBlockByID(LOD, i);
        AreaBlock *bottomBlock = area->GetAreaBlockByID(LOD, i + BLOCK_PER_MAP_SIDE);

        for (uint32_t x = 0; x < sideSize - 1; x++) {
            MapVertex *currentVertex = currentBlock->GetVertice(x, currentBlock->sideSize - 1);
            MapVertex *rightVertex = currentBlock->GetVertice(x + 1, currentBlock->sideSize - 1);
            MapVertex *bottomRightVertex = bottomBlock->GetVertice(x + 1, 0);
            MapVertex *bottomVertex = bottomBlock->GetVertice(x, 0);

            renderQuad(currentVertex, rightVertex, bottomRightVertex, bottomVertex, area, renderTexture);
        }
    }

    // Inter bottom-right quad
    if (i % BLOCK_PER_MAP_SIDE != 17 && i / BLOCK_PER_MAP_SIDE != 17) {

        AreaBlock *currentBlock = area->GetAreaBlockByID(LOD, i);
        AreaBlock *rightBlock = area->GetAreaBlockByID(LOD, i + 1);
        AreaBlock *rightBottonBlock = area->GetAreaBlockByID(LOD, i + 1 + BLOCK_PER_MAP_SIDE);
        AreaBlock *bottomBlock = area->GetAreaBlockByID(LOD, i + BLOCK_PER_MAP_SIDE);

        MapVertex *currentVertex = currentBlock->GetVertice(currentBlock->sideSize - 1, currentBlock->sideSize - 1);
        MapVertex *rightVertex = rightBlock->GetVertice(0, currentBlock->sideSize - 1);
        MapVertex *bottomRightVertex = rightBottonBlock->GetVertice(0, 0);
        MapVertex *bottomVertex = bottomBlock->GetVertice(currentBlock->sideSize - 1, 0);

        renderQuad(currentVertex, rightVertex, bottomRightVertex, bottomVertex, area, renderTexture);
    }
}
void SCRenderer::renderWorldSkyAndGround() {
    static const float max_int = (BLOCK_WIDTH * BLOCK_PER_MAP_SIDE)*2;
    static const float max_height = 60000.0f;
    static const float min_height = -2000.0f;

    GLfloat skycolor[2][3] = {
        {150.0f / 255.0f, 150.0f / 255.0f, 250.0f / 255.0f},
        {100.0f / 255.0f, 100.0f / 255.0f, 200.0f / 255.0f},
    };

    float ground_min{-1000.1f};
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.4f,0.0f);
    glVertex3f(-max_int, ground_min, -max_int);
    glVertex3f(max_int, ground_min, -max_int);
    glVertex3f(max_int, ground_min, max_int);
    glVertex3f(-max_int, ground_min, max_int);
    glEnd();
    /**/
    glBegin(GL_QUADS);
    // sol
    glColor3f(skycolor[0][0], skycolor[0][1], skycolor[0][2]);
    glVertex3f(-max_int, min_height, -max_int);
    glColor3f(skycolor[0][0], skycolor[0][1], skycolor[0][2]);
    glVertex3f(max_int, min_height, -max_int);
    glColor3f(skycolor[0][0], skycolor[0][1], skycolor[0][2]);
    glVertex3f(max_int, min_height, max_int);
    glColor3f(skycolor[0][0], skycolor[0][1], skycolor[0][2]);
    glVertex3f(-max_int, min_height, max_int);

    // ciel
    glColor3f(skycolor[1][0], skycolor[1][1], skycolor[1][2]);
    glVertex3f(-max_int, max_height, -max_int);
    glColor3f(skycolor[1][0], skycolor[1][1], skycolor[1][2]);
    glVertex3f(max_int, max_height, -max_int);
    glColor3f(skycolor[1][0], skycolor[1][1], skycolor[1][2]);
    glVertex3f(max_int, max_height, max_int);
    glColor3f(skycolor[1][0], skycolor[1][1], skycolor[1][2]);
    glVertex3f(-max_int, max_height, max_int);

    glColor3f(skycolor[0][0], skycolor[0][1], skycolor[0][2]);
    glVertex3f(-max_int, min_height, max_int);
    glColor3f(skycolor[0][0], skycolor[0][1], skycolor[0][2]);
    glVertex3f(max_int, min_height, max_int);
    glColor3f(skycolor[1][0], skycolor[1][1], skycolor[1][2]);
    glVertex3f(max_int, max_height, max_int);
    glColor3f(skycolor[1][0], skycolor[1][1], skycolor[1][2]);
    glVertex3f(-max_int, max_height, max_int);

    glColor3f(skycolor[0][0], skycolor[0][1], skycolor[0][2]);
    glVertex3f(-max_int, min_height, -max_int);
    glColor3f(skycolor[0][0], skycolor[0][1], skycolor[0][2]);
    glVertex3f(max_int, min_height, -max_int);
    glColor3f(skycolor[1][0], skycolor[1][1], skycolor[1][2]);
    glVertex3f(max_int, max_height, -max_int);
    glColor3f(skycolor[1][0], skycolor[1][1], skycolor[1][2]);
    glVertex3f(-max_int, max_height, -max_int);

    glColor3f(skycolor[0][0], skycolor[0][1], skycolor[0][2]);
    glVertex3f(-max_int, min_height, -max_int);
    glColor3f(skycolor[0][0], skycolor[0][1], skycolor[0][2]);
    glVertex3f(-max_int, min_height, max_int);
    glColor3f(skycolor[1][0], skycolor[1][1], skycolor[1][2]);
    glVertex3f(-max_int, max_height, max_int);
    glColor3f(skycolor[1][0], skycolor[1][1], skycolor[1][2]);
    glVertex3f(-max_int, max_height, -max_int);

    glColor3f(skycolor[0][0], skycolor[0][1], skycolor[0][2]);
    glVertex3f(max_int, min_height, -max_int);
    glColor3f(skycolor[0][0], skycolor[0][1], skycolor[0][2]);
    glVertex3f(max_int, min_height, max_int);
    glColor3f(skycolor[1][0], skycolor[1][1], skycolor[1][2]);
    glVertex3f(max_int, max_height, max_int);
    glColor3f(skycolor[1][0], skycolor[1][1], skycolor[1][2]);
    glVertex3f(max_int, max_height, -max_int);

    glEnd();
}

void SCRenderer::renderWorldSolid(RSArea *area, int LOD, int verticesPerBlock) {
    GLfloat fogColor[4] = {0.8f, 0.8f, 0.8f, 1.0f};
    float model_view_mat[4][4];
    Matrix *projectionMatrix = camera.GetProjectionMatrix();
    glViewport(0,0,this->width,this->height);			// Reset The Current Viewport
	
    const float verticalOffset = 0.45f; // Move the horizon to the top third of the screen
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(0.0f, verticalOffset, 0.0f); // Apply the vertical offset
    glMultMatrixf(projectionMatrix->ToGL()); // Apply the camera's projection matrix
        

    glDisable(GL_CULL_FACE);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *)model_view_mat);
    glFogi(GL_FOG_MODE, GL_EXP2);      // Fog Mode
    glFogfv(GL_FOG_COLOR, fogColor);   // Set Fog Color
    glFogf(GL_FOG_DENSITY, 0.0000015f); // How Dense Will The Fog Be
    glHint(GL_FOG_HINT, GL_DONT_CARE); // Fog Hint Value
    glFogf(GL_FOG_START, 5000.0f);     // Fog Start Depth
    glFogf(GL_FOG_END, 16000.0f);      // Fog End Depth
    glEnable(GL_FOG);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);

    Matrix *modelViewMatrix = camera.GetViewMatrix();
    glLoadMatrixf(modelViewMatrix->ToGL());

    this->renderWorldSkyAndGround();
    /**/
    textureSortedVertex.clear();
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glBegin(GL_TRIANGLES);
    Vector3D pos = camera.GetPosition();
    int centerX = BLOCK_WIDTH * BLOCK_PER_MAP_SIDE_DIV_2;
    int centerY = BLOCK_WIDTH * BLOCK_PER_MAP_SIDE_DIV_2;
    int blocX = (int)(pos.x + centerX) / BLOCK_WIDTH;
    int blocY = (int)(pos.z + centerY) / BLOCK_WIDTH;


    int block_id = blocY * BLOCK_PER_MAP_SIDE + blocX;
    std::map<uint8_t, Point2D> blockid_offset;
    int index = 0;
    int distance = 1;
    for (int y = -distance; y <= distance; y++) {
        for (int x = -distance; x <= distance; x++) {
            blockid_offset[index] = {x, y};
            index++;
        }
    }
    std::vector<int> blockid_rendered;
    blockid_rendered.clear();
    blockid_rendered.shrink_to_fit();

    for (int i=0; i<blockid_offset.size()-1; i++) {
        int final_block_id = (blocY + blockid_offset[i].y) * BLOCK_PER_MAP_SIDE + (blocX+blockid_offset[i].x);
        if (final_block_id<0)
            continue;
        blockid_rendered.push_back(final_block_id);
        renderBlock(area, LOD, final_block_id, false);
    }
    
    for (int i = 0; i < BLOCKS_PER_MAP; i++) {
        if (std::find(blockid_rendered.begin(), blockid_rendered.end(), i) != blockid_rendered.end())
            continue;
        renderBlock(area, LOD+1, i, false);
    }
    glEnd();
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    /*for (int i = 0; i < BLOCKS_PER_MAP; i++) {
        RenderBlock(area, LOD+1, i, true);
    }*/
    blockid_rendered.clear();
    blockid_rendered.shrink_to_fit();
    for (int i=0; i<blockid_offset.size()-1; i++) {
        int final_block_id = (blocY + blockid_offset[i].y) * BLOCK_PER_MAP_SIDE + (blocX+blockid_offset[i].x);
        if (final_block_id<0)
            continue;
        blockid_rendered.push_back(final_block_id);
        renderBlock(area, LOD, final_block_id, true);
    }
    for (int i = 0; i < BLOCKS_PER_MAP; i++) {
        if (std::find(blockid_rendered.begin(), blockid_rendered.end(), i) != blockid_rendered.end())
            continue;
        renderBlock(area, LOD+1, i, true);
    }
    for (auto const &x : textureSortedVertex) {
        RSImage *image = NULL;
        image = area->GetImageByID(x.first);
        if (image == NULL) {
            printf("This should never happen: Put a break point here.\n");
            return;
        }
        glBindTexture(GL_TEXTURE_2D, image->GetTexture()->GetTextureID());

        glBegin(GL_TRIANGLES);
        for (int i = 0; i < x.second.size(); i++) {
            VertexCache v = x.second.at(i);
            if (v.lv1 != NULL && v.lv1->lowerImageID == x.first) {
                renderTexturedTriangle(v.lv1, v.lv2, v.lv3, area, LOWER_TRIANGE, image);
            }
            if (v.uv1 != NULL && v.uv1->upperImageID == x.first) {
                renderTexturedTriangle(v.uv1, v.uv2, v.uv3, area, UPPER_TRIANGE, image);
            }
        }
        glEnd();
    }
    glDisable(GL_TEXTURE_2D);
    renderMapOverlay(area);
    glDisable(GL_FOG);
    /**/
}

void SCRenderer::renderObjects(RSArea *area, size_t blockID) {
    float y = 0;
    for (auto object : area->objects) {

        glPushMatrix();

        glTranslatef(object.position.x, object.position.y, object.position.z);
        if (object.entity != nullptr) {
            drawModel(object.entity, BLOCK_LOD_MAX);
        } else {
            printf("OBJECT [%s] NOT FOUND\n", object.name);
            glBegin(GL_POINTS);
            glColor3f(0, 1, 0);
            glVertex3d(0, 0, 0);
            glEnd();
        }

        glPopMatrix();
    }
}
void SCRenderer::renderWorldToTexture(RSArea *area) {
    

    Vector3D pos = camera.GetPosition();
    int centerX = BLOCK_WIDTH * BLOCK_PER_MAP_SIDE_DIV_2;
    int centerY = BLOCK_WIDTH * BLOCK_PER_MAP_SIDE_DIV_2;
    int blocX = (int)(pos.x + centerX) / BLOCK_WIDTH;
    int blocY = (int)(pos.z + centerY) / BLOCK_WIDTH;

    
    int block_id = blocY * BLOCK_PER_MAP_SIDE + blocX;
    
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    this->renderWorldSkyAndGround();
    /**/
    textureSortedVertex.clear();
    // Render your scene here
    glBegin(GL_TRIANGLES);
    renderBlock(area, 0, block_id, false);
    glEnd();
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    renderBlock(area, 0, block_id, true);
    for (auto const &x : textureSortedVertex) {
        RSImage *image = NULL;
        image = area->GetImageByID(x.first);
        if (image == NULL) {
            printf("This should never happen: Put a break point here.\n");
            return;
        }
        glBindTexture(GL_TEXTURE_2D, image->GetTexture()->GetTextureID());

        glBegin(GL_TRIANGLES);
        for (int i = 0; i < x.second.size(); i++) {
            VertexCache v = x.second.at(i);
            if (v.lv1 != NULL && v.lv1->lowerImageID == x.first) {
                renderTexturedTriangle(v.lv1, v.lv2, v.lv3, area, LOWER_TRIANGE, image);
            }
            if (v.uv1 != NULL && v.uv1->upperImageID == x.first) {
                renderTexturedTriangle(v.uv1, v.uv2, v.uv3, area, UPPER_TRIANGE, image);
            }
        }
        glEnd();
    }
    glDisable(GL_TEXTURE_2D);

    
    
}
void SCRenderer::initRenderToTexture() {
    glViewport(0, 0, 800, 600);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
void SCRenderer::getRenderToTexture() {
    glBindTexture(GL_TEXTURE_2D, texture);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, 800, 600, 0);
}
void SCRenderer::initRenderCameraView(){ 
    const float verticalOffset = 0.45f;
    Matrix *projectionMatrix = camera.GetProjectionMatrix();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(0.0f, verticalOffset, 0.0f);
    glMultMatrixf(projectionMatrix->ToGL());
    glMatrixMode(GL_MODELVIEW);
    Matrix *modelViewMatrix = camera.GetViewMatrix();
    glLoadMatrixf(modelViewMatrix->ToGL());
}
void SCRenderer::renderMissionObjects(RSMission *mission) {

    float y = 0;
    for (auto object : mission->mission_data.parts) {
        if (object == mission->getPlayerCoord()) {
            continue;
        }
        glPushMatrix();

        glTranslatef(static_cast<GLfloat>(object->position.x), static_cast<GLfloat>(object->position.y),
                     static_cast<GLfloat>(object->position.z));

        float model_view_mat[4][4];
        glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *)model_view_mat);
        glRotatef((360.0f - (float)object->azymuth + 90.0f), 0, 1, 0);
        glRotatef((float)object->pitch, 0, 0, 1);
        glRotatef(-(float)object->roll, 1, 0, 0);
        if (object->entity != NULL) {
            drawModel(object->entity, BLOCK_LOD_MAX);
        } else {
            printf("OBJECT [%s] NOT FOUND\n", object->member_name.c_str());
            glBegin(GL_POINTS);
            glColor3f(0, 1, 0);
            glVertex3d(0, 0, 0);
            glEnd();
        }
        glPopMatrix();
    }
}
void SCRenderer::renderLineCube(Vector3D position, int32_t size) {
    glPushMatrix();
    glTranslatef(position.x, position.y, position.z);
    glScalef((float)size, (float)size, (float)size);
    glBegin(GL_LINES);
    // Front face
    glColor3f(1.0f, 0.0f, 0.0f); // Red
    glVertex3f(-0.5f, -0.5f,  0.5f);
    glVertex3f( 0.5f, -0.5f,  0.5f);
    glVertex3f( 0.5f, -0.5f,  0.5f);
    glVertex3f( 0.5f,  0.5f,  0.5f);
    glVertex3f( 0.5f,  0.5f,  0.5f);
    glVertex3f(-0.5f,  0.5f,  0.5f);
    glVertex3f(-0.5f,  0.5f,  0.5f);
    glVertex3f(-0.5f, -0.5f,  0.5f);

    // Back face
    glColor3f(0.0f, 1.0f, 0.0f); // Green
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f(-0.5f,  0.5f, -0.5f);
    glVertex3f(-0.5f,  0.5f, -0.5f);
    glVertex3f( 0.5f,  0.5f, -0.5f);
    glVertex3f( 0.5f,  0.5f, -0.5f);
    glVertex3f( 0.5f, -0.5f, -0.5f);
    glVertex3f( 0.5f, -0.5f, -0.5f);
    glVertex3f(-0.5f, -0.5f, -0.5f);

    // Edges
    glColor3f(0.0f, 0.0f, 1.0f); // Blue
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f(-0.5f, -0.5f,  0.5f);
    glVertex3f(-0.5f,  0.5f, -0.5f);
    glVertex3f(-0.5f,  0.5f,  0.5f);
    glVertex3f( 0.5f, -0.5f, -0.5f);
    glVertex3f( 0.5f, -0.5f,  0.5f);
    glVertex3f( 0.5f,  0.5f, -0.5f);
    glVertex3f( 0.5f,  0.5f,  0.5f);

    glEnd();
    glPopMatrix();
}
void SCRenderer::renderBBox(Vector3D position, Point3D min, Point3D max) {
    glPushMatrix();
    glTranslatef(position.x, position.y, position.z);
    glScalef(max.x - min.x, max.y - min.y, max.z - min.z);
    glBegin(GL_LINES);
    // Front face
    glColor3f(1.0f, 0.0f, 0.0f); // Red
    glVertex3f(-0.5f, -0.5f,  0.5f);
    glVertex3f( 0.5f, -0.5f,  0.5f);
    glVertex3f( 0.5f, -0.5f,  0.5f);
    glVertex3f( 0.5f,  0.5f,  0.5f);
    glVertex3f( 0.5f,  0.5f,  0.5f);
    glVertex3f(-0.5f,  0.5f,  0.5f);
    glVertex3f(-0.5f,  0.5f,  0.5f);
    glVertex3f(-0.5f, -0.5f,  0.5f);

    // Back face
    glColor3f(0.0f, 1.0f, 0.0f); // Green
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f(-0.5f,  0.5f, -0.5f);
    glVertex3f(-0.5f,  0.5f, -0.5f);
    glVertex3f( 0.5f,  0.5f, -0.5f);
    glVertex3f( 0.5f,  0.5f, -0.5f);
    glVertex3f( 0.5f, -0.5f, -0.5f);
    glVertex3f( 0.5f, -0.5f, -0.5f);
    glVertex3f(-0.5f, -0.5f, -0.5f);

    // Edges
    glColor3f(0.0f, 0.0f, 1.0f); // Blue
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f(-0.5f, -0.5f,  0.5f);
    glVertex3f(-0.5f,  0.5f, -0.5f);
    glVertex3f(-0.5f,  0.5f,  0.5f);
    glVertex3f( 0.5f, -0.5f, -0.5f);
    glVertex3f( 0.5f, -0.5f,  0.5f);
    glVertex3f( 0.5f,  0.5f, -0.5f);
    glVertex3f( 0.5f,  0.5f,  0.5f);

    glEnd();
    glPopMatrix();
}
void SCRenderer::renderMapOverlay(RSArea *area) {

    // glDepthFunc(GL_LESS);
    glDisable(GL_DEPTH_TEST);
    for (int i = 0; i < area->objectOverlay.size(); i++) {
        AoVPoints *v = area->objectOverlay[i].vertices;
        for (int j = 0; j < area->objectOverlay[i].nbTriangles; j++) {
            AoVPoints v1, v2, v3;
            v1 = area->objectOverlay[i].vertices[area->objectOverlay[i].trianles[j].verticesIdx[0]];
            v2 = area->objectOverlay[i].vertices[area->objectOverlay[i].trianles[j].verticesIdx[1]];
            v3 = area->objectOverlay[i].vertices[area->objectOverlay[i].trianles[j].verticesIdx[2]];
            glBegin(GL_TRIANGLES);
            const Texel *texel = palette.GetRGBColor(area->objectOverlay[i].trianles[j].color);
            glColor4f(texel->r / 255.0f, texel->g / 255.0f, texel->b / 255.0f, 1);
            glVertex3f((GLfloat)v1.x, (GLfloat)v1.y, (GLfloat)-v1.z);
            glVertex3f((GLfloat)v2.x, (GLfloat)v2.y, (GLfloat)-v2.z);
            glVertex3f((GLfloat)v3.x, (GLfloat)v3.y, (GLfloat)-v3.z);
            glEnd();
        }
    }
    glEnable(GL_DEPTH_TEST);
}
void SCRenderer::renderWorldByID(RSArea *area, int LOD, int verticesPerBlock, int blockId) {
    textureSortedVertex.clear();
    printf("X:%f,Y:%f", area->GetAreaBlockByID(LOD, blockId)->vertice[0].v.x,
           area->GetAreaBlockByID(LOD, blockId)->vertice[0].v.z);
    glPushMatrix();
    glScalef(1 / 100.0f, 1 / 100.0f, 1 / 100.0f);
    glTranslatef(-area->GetAreaBlockByID(LOD, blockId)->vertice[0].v.x - 12500, 0,
                 -area->GetAreaBlockByID(LOD, blockId)->vertice[0].v.z - 12500);
    //
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glBegin(GL_TRIANGLES);
    renderBlock(area, LOD, blockId, false);
    glEnd();

    renderBlock(area, LOD, blockId, true);
    glEnable(GL_TEXTURE_2D);
    for (auto const &x : textureSortedVertex) {

        RSImage *image = NULL;

        image = area->GetImageByID(x.first);
        if (image == NULL) {
            printf("This should never happen: Put a break point here.\n");
            return;
        }
        glBindTexture(GL_TEXTURE_2D, image->GetTexture()->GetTextureID());

        glBegin(GL_TRIANGLES);
        for (int i = 0; i < x.second.size(); i++) {
            VertexCache v = x.second.at(i);
            if (v.lv1 != NULL && v.lv1->lowerImageID == x.first) {
                renderTexturedTriangle(v.lv1, v.lv2, v.lv3, area, LOWER_TRIANGE, image);
            }
            if (v.uv1 != NULL && v.uv1->upperImageID == x.first) {
                renderTexturedTriangle(v.uv1, v.uv2, v.uv3, area, UPPER_TRIANGE, image);
            }
        }
        glEnd();
    }
    glDisable(GL_TEXTURE_2D);
    renderObjects(area, blockId);
    glPopMatrix();
}