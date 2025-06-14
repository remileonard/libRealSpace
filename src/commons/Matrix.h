//
//  Matrix.h
//  libRealSpace
//
//  Created by Fabien Sanglard on 12/30/2013.
//  Copyright (c) 2013 Fabien Sanglard. All rights reserved.
//

#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include <stdlib.h>
#include <stdio.h>

#ifdef __cplusplus
}
#endif
#define _USE_MATH_DEFINES
#include <math.h>

#include "Maths.h"

#define DEG_TO_RAD (2.0f*M_PI/360.0f)
class Vector3DHomogeneous {
public:
    float x;
    float y;
    float z;
    float w;
};
class Matrix{
    
public:
    
    void Clear(void);
    void Identity(void);
    
    void Multiply(Matrix* other);
    
    void Print(void);
    
    float* ToGL(void);
    
    void Transpose(void);
    
    float v[4][4];
    
    void SetTranslation(float x, float y, float z);
    void SetRotationX(float angle);
    void SetRotationY(float angle);
    void SetRotationZ(float angle);
    void translateM(float x, float y, float z);
    void rotateM(float angle, float x, float y, float z);
    void Multiply(Vector3DHomogeneous other);
    Vector3DHomogeneous multiplyMatrixVector(Vector3DHomogeneous v);
private:
};
class Vector3D{
    
public:
    inline Vector3D(void){
        this->x = 0.0f;
        this->y = 0.0f;
        this->z = 0.0f;
    };
    inline Vector3D(float x, float y, float z) {
        this->x = x;
        this->y = y;
        this->z = z;
    };    
    inline void SetWithCoo(float x, float y, float z){
        this->x = x;
        this->y = y;
        this->z = z;
    };
    
    inline void Clear(void){
        SetWithCoo(0, 0,0);
    };

    inline void Negate(void){
        Scale(-1);
    };
    
    inline void Add(Vector3D* other){
        this->x += other->x;
        this->y += other->y;
        this->z += other->z;
    };

    inline void Substract(Vector3D* other){
        this->x -= other->x;
        this->y -= other->y;
        this->z -= other->z;
    };
    

    inline void Scale(float factor){
        this->x *= factor;
        this->y *= factor;
        this->z *= factor;
    };

    
    inline Vector3D CrossProduct(const Vector3D* other){
        Vector3D result;

        result.x = this->y * other->z - this->z * other->y;                // X
        result.y = this->z * other->x - this->x * other->z;                // Y
        result.z = this->x * other->y - this->y * other->x;                // Z
        
        return result;
    };

    static Vector3D Cross(const Vector3D& v1, const Vector3D& v2) {
        return Vector3D{
            v1.y * v2.z - v1.z * v2.y,
            v1.z * v2.x - v1.x * v2.z,
            v1.x * v2.y - v1.y * v2.x
        };
    };

    inline void Normalize(void){
        float ilength;
        ilength = DotProduct(this);
        float invSqrtLength = InvSqrt(ilength);
        Scale(invSqrtLength);
    };
    inline float Length() {
        return sqrtf(this->x * this->x + this->y * this->y + this->z * this->z);
    };
    inline float Distance(Vector3D* other) {
        return sqrtf((this->x - other->x) * (this->x - other->x) + (this->y - other->y) * (this->y - other->y) + (this->z - other->z) * (this->z - other->z));
    };
    
    inline float DotProduct(Vector3D* other){
        
        float acc = 0;
        acc += this->x * other->x;
        acc += this->y * other->y;
        acc += this->z * other->z;
        
        return acc;
    };

    float Norm() {
        return sqrtf(this->x * this->x + this->y * this->y + this->z * this->z);
    };

    inline Vector3D limit(float max_value) {
        if (this->Norm() > max_value) {
            this->Scale(max_value / this->Norm());
        }
        return *this;
    };
    inline Vector3D operator+=(const Vector3D& other) {
        this->x = this->x + other.x;
        this->y = this->y + other.y;
        this->z = this-> z + other.z;
        return *this;
    };
    inline Vector3D applyRotation(Matrix matrix) {
        Vector3D result;
        result.x = matrix.v[0][0] * this->x + matrix.v[0][1] * this->y + matrix.v[0][2] * this->z;
        result.y = matrix.v[1][0] * this->x + matrix.v[1][1] * this->y + matrix.v[1][2] * this->z;
        result.z = matrix.v[2][0] * this->x + matrix.v[2][1] * this->y + matrix.v[2][2] * this->z;
        return result;
    };
    inline Vector3D operator+(const Vector3D& other) {
        return Vector3D(this->x + other.x, this->y + other.y, this->z + other.z);
    };
    inline Vector3D operator-(const Vector3D& other) {
        return Vector3D(this->x - other.x, this->y - other.y, this->z - other.z);
    };
    inline Vector3D operator-() {
        return Vector3D(-this->x, -this->y, -this->z);
    };
    inline Vector3D operator*(const float& factor) {
        return Vector3D(this->x * factor, this->y * factor, this->z * factor);
    };
    int operator==(const Vector3D& other) {
        return this->x == other.x && this->y == other.y && this->z == other.z;
    };
    float x;
    float y;
    float z;
    
private:
};

#define Point3D Vector3D





