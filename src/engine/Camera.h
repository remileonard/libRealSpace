//
//  Camera.h
//  libRealSpace
//
//  Created by Fabien Sanglard on 12/30/2013.
//  Copyright (c) 2013 Fabien Sanglard. All rights reserved.
//

#pragma once
#include <math.h>

#include "../commons/Maths.h"
#include "../commons/Matrix.h"
#include "../commons/Quaternion.h"


class Camera{
private:
    
    void CalcViewMatrix(void);
    
    float fovy;
    float aspect;
    float zNear;
    float zFar;
    
    Point3D position;
    Quaternion orientation;
    
    
    Matrix mproj;
    Matrix mview;

public:
    
    //Update parameters and recalculate perspective
    void setPersective(float fovy, float aspect, float zNear, float zFar);
    
    //Update orientation and Matrix view
    void lookAt(Point3D* lookAt);
    void lookAt(Point3D *lookAt, Vector3D *up);
    //Update position and Matrix view
    void SetPosition(Point3D* position);
    
    //Update orientation and Matrix view
    void rotate(float pitch, float yaw, float roll);
    void resetRotate();
    void setOrientation(Quaternion* orientation);
    
    //Various Getters
    Point3D getPosition(void);
    Quaternion getOrientation(void);
    Matrix* getProjectionMatrix(void);
    Matrix* getViewMatrix(void);
    Vector3D getForward(void);
    Vector3D getUp(void);
    Vector3D getRight(void);
    
    
    //Moves
    void moveForward(void);
    void moveBackward(void);
    void moveStrafLeft(void);
    void moveStrafRight(void);

};

