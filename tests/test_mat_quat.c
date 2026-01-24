/*
====================
test_mat_quat.c

Unit tests for Matrix3/4 and Quaternion helpers
====================
*/

#include "../unity/unity.h"
#include <math.h>

#include "../shared/shared.h"

static void VecSet3(float v[3], float a, float b, float c){ v[0]=a; v[1]=b; v[2]=c; }

void test_Matrix3_Multiply_identity(void){
    mat3x3_t I;
    Matrix3_Identity(I);
    mat3x3_t A = { {1,2,3},{4,5,6},{7,8,9} };
    mat3x3_t R;
    Matrix3_Multiply(I, A, R);
    TEST_ASSERT_TRUE(Matrix3_Compare(A, R));
}

void test_Matrix3_Angles_roundtrip(void){
    vec3_t ang = {30.0f, 45.0f, 60.0f};
    mat3x3_t m;
    vec3_t out;

    Angles_Matrix3(ang, m);
    Matrix3_Angles(m, out);

    // Angles may wrap; compare modulo 360
    for (int i=0;i<3;i++){
        float diff = fabsf(ang[i] - out[i]);
        if (diff > 180) diff = fabsf(diff - 360);
        TEST_ASSERT_TRUE(diff < 0.5f);
    }
}

void test_Matrix4_Multiply_identity(void){
    mat4x4_t I, A, R;
    Matrix4_Identity(I);
    Matrix4_Copy(I, A);
    A[3] = 5.0f; // modify one element
    Matrix4_Multiply(I, A, R);
    for (int i=0;i<16;i++) TEST_ASSERT_FLOAT_WITHIN(1e-6f, A[i], R[i]);
}

void test_Quat_Multiply_and_inverse(void){
    quat_t a = {0.0f, 0.707f, 0.0f, 0.707f};
    quat_t b = {0.0f, 0.0f, 0.707f, 0.707f};
    quat_t res;
    Quat_Multiply(a, b, res);
    // ensure normalized
    float len = res[0]*res[0]+res[1]*res[1]+res[2]*res[2]+res[3]*res[3];
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 1.0f, len);
}

void test_Quat_Lerp_edgecases(void){
    quat_t a = {0.0f, 0.0f, 0.0f, 1.0f};
    quat_t b = {0.0f, 1.0f, 0.0f, 0.0f};
    quat_t out;
    Quat_Lerp(a, b, 0.0f, out);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, a[0], out[0]);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, a[1], out[1]);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, a[2], out[2]);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, a[3], out[3]);

    Quat_Lerp(a, b, 1.0f, out);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, b[0], out[0]);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, b[1], out[1]);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, b[2], out[2]);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, b[3], out[3]);
}

static void AssertMat3Within(float delta, mat3x3_t expected, mat3x3_t actual)
{
    for (int r = 0; r < 3; r++){
        for (int c = 0; c < 3; c++){
            TEST_ASSERT_FLOAT_WITHIN(delta, expected[r][c], actual[r][c]);
        }
    }
}

void test_Matrix3_Quat_roundtrip(void){
    vec3_t ang = {10.0f, 20.0f, 30.0f};
    mat3x3_t m, out;
    quat_t q;

    Angles_Matrix3(ang, m);
    Matrix3_Quat(m, q);
    Quat_Matrix3(q, out);

    AssertMat3Within(1e-4f, m, out);
}

void test_Quat_Normalize(void){
    quat_t q = {0.0f, 0.0f, 0.0f, 2.0f};
    float len2 = Quat_Normalize(q);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 4.0f, len2);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, q[0]);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, q[1]);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, q[2]);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 1.0f, q[3]);
}

void test_Quat_Multiply_unit(void){
    quat_t q = {0.0f, 0.70710678f, 0.0f, 0.70710678f};
    quat_t inv, res;

    Quat_Inverse(q, inv);
    Quat_Multiply(q, inv, res);

    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 0.0f, res[0]);
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 0.0f, res[1]);
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 0.0f, res[2]);
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 1.0f, res[3]);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_Matrix3_Multiply_identity);
    RUN_TEST(test_Matrix3_Angles_roundtrip);
    RUN_TEST(test_Matrix4_Multiply_identity);
    RUN_TEST(test_Quat_Multiply_and_inverse);
    RUN_TEST(test_Quat_Lerp_edgecases);
    RUN_TEST(test_Matrix3_Quat_roundtrip);
    RUN_TEST(test_Quat_Normalize);
    RUN_TEST(test_Quat_Multiply_unit);

    return UNITY_END();
}

