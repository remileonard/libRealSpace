#include <gtest/gtest.h>
#include "commons/Matrix.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class MatrixTest : public ::testing::Test {
protected:
    const float epsilon = 1e-4f;
};

// ============================================================================
// Tests Vector3D
// ============================================================================

TEST_F(MatrixTest, Vector3D_Constructor_And_Setters) {
    Vector3D v1;
    EXPECT_FLOAT_EQ(0.0f, v1.x);
    EXPECT_FLOAT_EQ(0.0f, v1.y);
    EXPECT_FLOAT_EQ(0.0f, v1.z);

    Vector3D v2(1.0f, 2.0f, 3.0f);
    EXPECT_FLOAT_EQ(1.0f, v2.x);
    EXPECT_FLOAT_EQ(2.0f, v2.y);
    EXPECT_FLOAT_EQ(3.0f, v2.z);

    v1.SetWithCoo(4.0f, 5.0f, 6.0f);
    EXPECT_FLOAT_EQ(4.0f, v1.x);
    EXPECT_FLOAT_EQ(5.0f, v1.y);
    EXPECT_FLOAT_EQ(6.0f, v1.z);
}

TEST_F(MatrixTest, Vector3D_Clear) {
    Vector3D v(1.0f, 2.0f, 3.0f);
    v.Clear();
    EXPECT_FLOAT_EQ(0.0f, v.x);
    EXPECT_FLOAT_EQ(0.0f, v.y);
    EXPECT_FLOAT_EQ(0.0f, v.z);
}

TEST_F(MatrixTest, Vector3D_Negate) {
    Vector3D v(1.0f, -2.0f, 3.0f);
    v.Negate();
    EXPECT_FLOAT_EQ(-1.0f, v.x);
    EXPECT_FLOAT_EQ(2.0f, v.y);
    EXPECT_FLOAT_EQ(-3.0f, v.z);
}

TEST_F(MatrixTest, Vector3D_Add) {
    Vector3D v1(1.0f, 2.0f, 3.0f);
    Vector3D v2(4.0f, 5.0f, 6.0f);
    v1.Add(&v2);
    EXPECT_FLOAT_EQ(5.0f, v1.x);
    EXPECT_FLOAT_EQ(7.0f, v1.y);
    EXPECT_FLOAT_EQ(9.0f, v1.z);
}

TEST_F(MatrixTest, Vector3D_Substract) {
    Vector3D v1(4.0f, 5.0f, 6.0f);
    Vector3D v2(1.0f, 2.0f, 3.0f);
    v1.Substract(&v2);
    EXPECT_FLOAT_EQ(3.0f, v1.x);
    EXPECT_FLOAT_EQ(3.0f, v1.y);
    EXPECT_FLOAT_EQ(3.0f, v1.z);
}

TEST_F(MatrixTest, Vector3D_Scale) {
    Vector3D v(1.0f, 2.0f, 3.0f);
    v.Scale(2.0f);
    EXPECT_FLOAT_EQ(2.0f, v.x);
    EXPECT_FLOAT_EQ(4.0f, v.y);
    EXPECT_FLOAT_EQ(6.0f, v.z);
}

TEST_F(MatrixTest, Vector3D_CrossProduct) {
    Vector3D v1(1.0f, 0.0f, 0.0f); // X axis
    Vector3D v2(0.0f, 1.0f, 0.0f); // Y axis
    
    // Z axis expected (Right-hand rule)
    Vector3D res = v1.CrossProduct(&v2); 
    EXPECT_FLOAT_EQ(0.0f, res.x);
    EXPECT_FLOAT_EQ(0.0f, res.y);
    EXPECT_FLOAT_EQ(1.0f, res.z);

    // Test Static Cross
    Vector3D resStatic = Vector3D::Cross(v1, v2);
    EXPECT_EQ(res, resStatic);
}

TEST_F(MatrixTest, Vector3D_DotProduct) {
    Vector3D v1(1.0f, 2.0f, 3.0f);
    Vector3D v2(4.0f, 5.0f, 6.0f);
    // 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32
    float dot = v1.DotProduct(&v2);
    EXPECT_FLOAT_EQ(32.0f, dot);
}

TEST_F(MatrixTest, Vector3D_Length_Norm) {
    Vector3D v(3.0f, 0.0f, 4.0f);
    // Sqrt(9 + 0 + 16) = 5
    EXPECT_FLOAT_EQ(5.0f, v.Length());
    EXPECT_FLOAT_EQ(5.0f, v.Norm());
}

TEST_F(MatrixTest, Vector3D_Normalize) {
    Vector3D v(3.0f, 0.0f, 4.0f);
    v.Normalize();
    EXPECT_FLOAT_EQ(0.6f, v.x);
    EXPECT_FLOAT_EQ(0.0f, v.y);
    EXPECT_FLOAT_EQ(0.8f, v.z);
    EXPECT_NEAR(1.0f, v.Length(), epsilon);
}

TEST_F(MatrixTest, Vector3D_Distance) {
    Vector3D v1(1.0f, 1.0f, 1.0f);
    Vector3D v2(4.0f, 5.0f, 1.0f); // Different by (3, 4, 0)
    // Distance should be 5
    EXPECT_FLOAT_EQ(5.0f, v1.Distance(&v2));
}

TEST_F(MatrixTest, Vector3D_Operators) {
    Vector3D v1(1.0f, 2.0f, 3.0f);
    Vector3D v2(4.0f, 5.0f, 6.0f);

    Vector3D sum = v1 + v2;
    EXPECT_FLOAT_EQ(5.0f, sum.x);
    
    Vector3D sub = v2 - v1;
    EXPECT_FLOAT_EQ(3.0f, sub.x);

    Vector3D neg = -v1;
    EXPECT_FLOAT_EQ(-1.0f, neg.x);

    Vector3D scaled = v1 * 2.0f;
    EXPECT_FLOAT_EQ(2.0f, scaled.x);

    v1 += v2;
    EXPECT_FLOAT_EQ(5.0f, v1.x);

    EXPECT_TRUE(v1 == sum); // v1 became (5,7,9) via +=
}

// ============================================================================
// Tests Matrix
// ============================================================================

TEST_F(MatrixTest, Matrix_Clear) {
    Matrix m;
    // Fill with junk first to be sure
    for(int i=0; i<4; i++) for(int j=0; j<4; j++) m.v[i][j] = 99.9f;
    
    m.Clear(); // Should be zero matrix? Or identity? 
    // Looking at common engines, Clear usually zeros.
    // If your implementation zeros it out:
    EXPECT_FLOAT_EQ(0.0f, m.v[0][0]);
    EXPECT_FLOAT_EQ(0.0f, m.v[3][3]);
}

TEST_F(MatrixTest, Matrix_Identity) {
    Matrix m;
    m.Identity();
    
    EXPECT_FLOAT_EQ(1.0f, m.v[0][0]);
    EXPECT_FLOAT_EQ(0.0f, m.v[0][1]);
    EXPECT_FLOAT_EQ(1.0f, m.v[1][1]);
    EXPECT_FLOAT_EQ(1.0f, m.v[2][2]);
    EXPECT_FLOAT_EQ(1.0f, m.v[3][3]);
}

TEST_F(MatrixTest, Matrix_SetTranslation) {
    Matrix m;
    m.Identity();
    m.SetTranslation(10.0f, 20.0f, 30.0f);
    
    // [1 0 0 0]
    // [0 1 0 0]
    // [0 0 1 0]
    // [x y z 1]
    
    EXPECT_FLOAT_EQ(1.0f, m.v[0][0]);
    EXPECT_FLOAT_EQ(10.0f, m.v[3][0]); 
    EXPECT_FLOAT_EQ(20.0f, m.v[3][1]);
    EXPECT_FLOAT_EQ(30.0f, m.v[3][2]);
    EXPECT_FLOAT_EQ(1.0f, m.v[3][3]);
}

TEST_F(MatrixTest, Matrix_Transpose) {
    Matrix m;
    m.Identity();
    m.v[0][3] = 10.0f; // Translation X
    
    m.Transpose();
    
    EXPECT_FLOAT_EQ(10.0f, m.v[3][0]); // Moved to bottom-left
    EXPECT_FLOAT_EQ(0.0f, m.v[0][3]);
}

TEST_F(MatrixTest, Matrix_MultiplyMatrixVector) {
    Matrix m;
    m.Identity();
    // Translate by (10, 0, 0)
    m.SetTranslation(10.0f, 0.0f, 0.0f);
    
    Vector3DHomogeneous vec = {0.0f, 0.0f, 0.0f, 1.0f}; // Origin point
    
    Vector3DHomogeneous res = m.multiplyMatrixVector(vec);
    
    EXPECT_FLOAT_EQ(10.0f, res.x);
    EXPECT_FLOAT_EQ(0.0f, res.y);
    EXPECT_FLOAT_EQ(0.0f, res.z);
    EXPECT_FLOAT_EQ(1.0f, res.w);
}

TEST_F(MatrixTest, Vector3D_ApplyRotation_Matrix) {
    Matrix m;
    m.Identity();
    // Rotation 90 degrees around Z
    // x -> y, y -> -x
    m.SetRotationZ(90.0f*(M_PI/180.0f));
    
    Vector3D v(1.0f, 0.0f, 0.0f);
    Vector3D res = v.transformPoint(m);
    
    // cos(90)=0, sin(90)=1
    // New X = 1*0 - 0*1 = 0
    // New Y = 1*1 + 0*0 = 1
    EXPECT_NEAR(0.0f, res.x, epsilon);
    EXPECT_NEAR(1.0f, res.y, epsilon);
    EXPECT_NEAR(0.0f, res.z, epsilon);
}

TEST_F(MatrixTest, Vector3D_TransformPoint) {
    Matrix m;
    m.Identity();
    m.SetTranslation(5.0f, 5.0f, 5.0f);
    
    Vector3D p(1.0f, 1.0f, 1.0f);
    Vector3D res = p.transformPoint(m);
    
    // 1+5 = 6
    EXPECT_FLOAT_EQ(6.0f, res.x);
    EXPECT_FLOAT_EQ(6.0f, res.y);
    EXPECT_FLOAT_EQ(6.0f, res.z);
}