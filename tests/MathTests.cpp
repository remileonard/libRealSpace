#include <gtest/gtest.h>
#include "commons/Maths.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class MathsTest : public ::testing::Test {
protected:
    // Marge d'erreur pour les comparaisons flottantes
    const float epsilon = 1e-4f;
};

// ============================================================================
// Tests Point2D (Coordonnées entières)
// ============================================================================

TEST_F(MathsTest, Point2D_Rotate_ReturnsCorrectCoordinates) {
    Point2D p = {10, 0};
    
    // Rotation de 90 degrés (PI/2)
    // x = 10 * cos(90) - 0 * sin(90) = 0
    // y = 10 * sin(90) + 0 * cos(90) = 10
    Point2D rotated = p.rotate((float)(M_PI / 2.0));
    
    EXPECT_EQ(0, rotated.x);
    EXPECT_EQ(10, rotated.y);
}

TEST_F(MathsTest, Point2D_Rotate_TruncatesValuesCorrectly) {
    Point2D p = {10, 0};
    
    // Rotation de 45 degrés
    // cos(45) ~= 0.707, sin(45) ~= 0.707
    // x = 7.07 -> 7 (cast int)
    // y = 7.07 -> 7 (cast int)
    Point2D rotated = p.rotate((float)(M_PI / 4.0));
    
    EXPECT_EQ(7, rotated.x);
    EXPECT_EQ(7, rotated.y);
}

TEST_F(MathsTest, Point2D_RotateAroundPoint_ReturnsCorrectCoordinates) {
    Point2D center = {10, 10};
    Point2D p = {20, 10}; // À 10 unités à droite du centre
    
    // Rotation de 90 degrés autour du centre
    // Devrait se retrouver à 10 unités "au-dessus" (y positif vers le bas ?) ou "en dessous" selon le repère.
    // Formule : newx = (20-10)*cos(90) - (10-10)*sin(90) + 10 = 0 - 0 + 10 = 10
    // Formule : newy = (20-10)*sin(90) + (10-10)*cos(90) + 10 = 10 + 0 + 10 = 20
    Point2D rotated = p.rotateAroundPoint(center, (float)(M_PI / 2.0));
    
    EXPECT_EQ(10, rotated.x);
    EXPECT_EQ(20, rotated.y);
}

// ============================================================================
// Tests Point2Df (Coordonnées flottantes)
// ============================================================================

TEST_F(MathsTest, Point2Df_Constructor_SetsValuesCorrectly) {
    Point2Df p(1.5f, 2.5f);
    
    EXPECT_FLOAT_EQ(1.5f, p.x);
    EXPECT_FLOAT_EQ(2.5f, p.y);
}

TEST_F(MathsTest, Point2Df_OperatorEquality_ReturnsTrueForSameValues) {
    Point2Df p1(1.0f, 2.0f);
    Point2Df p2(1.0f, 2.0f);
    
    EXPECT_TRUE(p1 == p2);
}

TEST_F(MathsTest, Point2Df_OperatorEquality_ReturnsFalseForDifferentValues) {
    Point2Df p1(1.0f, 2.0f);
    Point2Df p2(1.0f, 2.1f);
    
    EXPECT_FALSE(p1 == p2);
}

// ============================================================================
// Tests Vector2D
// ============================================================================

TEST_F(MathsTest, Vector2D_Constructor_SetsValuesCorrectly) {
    Vector2D v(3.0f, 4.0f);
    
    EXPECT_FLOAT_EQ(3.0f, v.x);
    EXPECT_FLOAT_EQ(4.0f, v.y);
}

TEST_F(MathsTest, Vector2D_DefaultConstructor_SetsToZero) {
    Vector2D v;
    
    EXPECT_FLOAT_EQ(0.0f, v.x);
    EXPECT_FLOAT_EQ(0.0f, v.y);
}

TEST_F(MathsTest, Vector2D_Length_ReturnsCorrectMagnitude) {
    Vector2D v(3.0f, 4.0f);
    // sqrt(3^2 + 4^2) = 5
    EXPECT_FLOAT_EQ(5.0f, v.Length());
}

TEST_F(MathsTest, Vector2D_Normalize_UpdatesVectorCorrectly) {
    Vector2D v(3.0f, 4.0f);
    v.Normalize();
    
    EXPECT_FLOAT_EQ(0.6f, v.x); // 3 / 5
    EXPECT_FLOAT_EQ(0.8f, v.y); // 4 / 5
    EXPECT_FLOAT_EQ(1.0f, v.Length());
}

TEST_F(MathsTest, Vector2D_Normalize_ZeroLength_DoesNothing) {
    Vector2D v(0.0f, 0.0f);
    v.Normalize();
    
    EXPECT_FLOAT_EQ(0.0f, v.x);
    EXPECT_FLOAT_EQ(0.0f, v.y);
}

TEST_F(MathsTest, Vector2D_RotateAroundPoint_ReturnsCorrectVector) {
    Vector2D center(0.0f, 0.0f);
    Vector2D v(10.0f, 0.0f);
    
    // Rotation 90 degrés
    Vector2D rotated = v.rotateAroundPoint(center, (float)(M_PI / 2.0));
    
    EXPECT_NEAR(0.0f, rotated.x, epsilon);
    EXPECT_NEAR(10.0f, rotated.y, epsilon);
}

// ============================================================================
// Tests Fonctions Globales (Nécessite que leur implémentation soit liée)
// ============================================================================

TEST_F(MathsTest, DegreeToRad_ReturnsCorrectValue) {
    float deg = 180.0f;
    float rad = degreeToRad(deg);
    EXPECT_FLOAT_EQ((float)M_PI, rad);
}

TEST_F(MathsTest, RadToDegree_ReturnsCorrectValue) {
    float rad = (float)M_PI;
    float deg = radToDegree(rad);
    EXPECT_FLOAT_EQ(180.0f, deg);
}

TEST_F(MathsTest, TenthOfDegreeToRad_ReturnsCorrectValue) {
    // 900 dixièmes de degré = 90 degrés = PI/2
    float tenths = 900.0f;
    float rad = tenthOfDegreeToRad(tenths);
    EXPECT_FLOAT_EQ((float)(M_PI / 2.0), rad);
}

TEST_F(MathsTest, InvSqrt_ReturnsCorrectApprox) {
    // 1 / sqrt(4) = 0.5
    float val = 4.0f;
    float res = InvSqrt(val);
    
    // InvSqrt est souvent une approximation, on utilise une marge plus large si c'est la version Quake3
    EXPECT_NEAR(0.5f, res, 1e-3f);
}