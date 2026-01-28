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

/*
=============================================================================
Matrix3_Angles gimbal lock and edge case tests

These exercise the pitch = ±90° gimbal lock fix (commits 498b380, 708e36f)
and verify roll extraction produces correct sign.
=============================================================================
*/

static void AssertAngleRoundtrip(vec3_t ang, float tolerance){
    mat3x3_t m;
    vec3_t out;

    Angles_Matrix3(ang, m);
    Matrix3_Angles(m, out);

    for (int i = 0; i < 3; i++){
        float diff = fabsf(ang[i] - out[i]);
        if (diff > 180) diff = fabsf(diff - 360);
        TEST_ASSERT_FLOAT_WITHIN(tolerance, 0.0f, diff);
    }
}

void test_Matrix3_Angles_gimbal_lock_pitch_90(void){
    /* Pitch +90° triggers the gimbal lock branch (cos(pitch) ≈ 0).
       In gimbal lock, roll is locked to 0; yaw absorbs the remaining DOF. */
    vec3_t ang = {90.0f, 45.0f, 0.0f};
    mat3x3_t m;
    vec3_t out;

    Angles_Matrix3(ang, m);
    Matrix3_Angles(m, out);

    /* Pitch should come back as 90 (or 270 via AngleModf wrapping) */
    float pitch_diff = fabsf(90.0f - out[PITCH]);
    if (pitch_diff > 180) pitch_diff = fabsf(pitch_diff - 360);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 0.0f, pitch_diff);

    /* Roll should be 0 in gimbal lock */
    float roll_diff = fabsf(0.0f - out[ROLL]);
    if (roll_diff > 180) roll_diff = fabsf(roll_diff - 360);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 0.0f, roll_diff);
}

void test_Matrix3_Angles_gimbal_lock_pitch_neg90(void){
    /* Pitch -90° (looking straight up) — the other gimbal lock case */
    vec3_t ang = {-90.0f, 30.0f, 0.0f};
    mat3x3_t m;
    vec3_t out;

    Angles_Matrix3(ang, m);
    Matrix3_Angles(m, out);

    /* Pitch should come back as 270 (AngleModf wraps -90 → 270) */
    float pitch_diff = fabsf(270.0f - out[PITCH]);
    if (pitch_diff > 180) pitch_diff = fabsf(pitch_diff - 360);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 0.0f, pitch_diff);

    /* Roll should be 0 in gimbal lock */
    float roll_diff = fabsf(0.0f - out[ROLL]);
    if (roll_diff > 180) roll_diff = fabsf(roll_diff - 360);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 0.0f, roll_diff);
}

void test_Matrix3_Angles_zero_angles(void){
    vec3_t ang = {0.0f, 0.0f, 0.0f};
    AssertAngleRoundtrip(ang, 0.5f);
}

void test_Matrix3_Angles_pure_yaw(void){
    vec3_t ang = {0.0f, 135.0f, 0.0f};
    AssertAngleRoundtrip(ang, 0.5f);
}

void test_Matrix3_Angles_pure_roll(void){
    /* Tests that positive roll values roundtrip correctly.
       This covers the roll extraction sign fix (commit 708e36f). */
    vec3_t ang = {0.0f, 0.0f, 60.0f};
    AssertAngleRoundtrip(ang, 0.5f);
}

void test_Matrix3_Angles_near_gimbal_lock(void){
    /* Near gimbal lock (pitch ~89°) — should still use the normal branch */
    vec3_t ang = {89.0f, 45.0f, 30.0f};
    AssertAngleRoundtrip(ang, 1.0f);
}

void test_Matrix3_Angles_negative_pitch_and_yaw(void){
    /* AngleModf wraps negative angles; verify the roundtrip handles it */
    vec3_t ang_in = {-45.0f, -90.0f, 0.0f};
    mat3x3_t m;
    vec3_t out;

    Angles_Matrix3(ang_in, m);
    Matrix3_Angles(m, out);

    /* -45 → AngleModf → 315, -90 → AngleModf → 270 */
    float expected_pitch = AngleModf(-45.0f);
    float expected_yaw = AngleModf(-90.0f);

    float diff_p = fabsf(expected_pitch - out[PITCH]);
    if (diff_p > 180) diff_p = fabsf(diff_p - 360);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 0.0f, diff_p);

    float diff_y = fabsf(expected_yaw - out[YAW]);
    if (diff_y > 180) diff_y = fabsf(diff_y - 360);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 0.0f, diff_y);
}

void test_Matrix3_Angles_all_axes(void){
    /* Various combined angle sets that exercise the general branch */
    vec3_t cases[][1] = {
        {{10.0f, 20.0f, 30.0f}},
        {{45.0f, 90.0f, 45.0f}},
        {{0.0f, 180.0f, 90.0f}},
        {{15.0f, 270.0f, 5.0f}},
    };
    for (int i = 0; i < 4; i++){
        AssertAngleRoundtrip(cases[i][0], 0.5f);
    }
}

/*
=============================================================================
Matrix4_Invert tests

Tests the fast affine matrix inverse added in commit 73729e6.
=============================================================================
*/

void test_Matrix4_Invert_identity(void){
    mat4x4_t I, out;
    Matrix4_Identity(I);
    Matrix4_Invert(I, out);

    for (int i = 0; i < 16; i++)
        TEST_ASSERT_FLOAT_WITHIN(1e-5f, I[i], out[i]);
}

void test_Matrix4_Invert_pure_translation(void){
    mat4x4_t m, inv;
    Matrix4_Identity(m);
    Matrix4_Translate(m, 10.0f, 20.0f, 30.0f);
    Matrix4_Invert(m, inv);

    /* Inverse of a pure translation (10,20,30) should be (-10,-20,-30) */
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, -10.0f, inv[12]);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, -20.0f, inv[13]);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, -30.0f, inv[14]);
}

void test_Matrix4_Invert_pure_rotation(void){
    /* For a pure rotation, the inverse is the transpose of the rotation part.
       Verify double-invert roundtrip and that translation stays zero. */
    mat4x4_t m, inv, inv_inv;
    Matrix4_Identity(m);
    Matrix4_Rotate(m, 45.0f, 0.0f, 0.0f, 1.0f);  /* 45° around Z */
    Matrix4_Invert(m, inv);

    /* Pure rotation: inverse should have zero translation */
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f, inv[12]);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f, inv[13]);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f, inv[14]);

    /* Double-invert roundtrip */
    Matrix4_Invert(inv, inv_inv);
    for (int i = 0; i < 16; i++)
        TEST_ASSERT_FLOAT_WITHIN(1e-4f, m[i], inv_inv[i]);
}

void test_Matrix4_Invert_rotation_and_translation(void){
    /* Verify double-invert roundtrip for a rotation + translation matrix */
    mat4x4_t m, inv, inv_inv;
    Matrix4_Identity(m);
    Matrix4_Rotate(m, 30.0f, 0.0f, 1.0f, 0.0f);  /* 30° around Y */
    Matrix4_Translate(m, 5.0f, -3.0f, 7.0f);
    Matrix4_Invert(m, inv);

    /* The inverse translation should be non-zero (rotated negation of original) */
    float inv_t_len = sqrtf(inv[12]*inv[12] + inv[13]*inv[13] + inv[14]*inv[14]);
    TEST_ASSERT_TRUE(inv_t_len > 1.0f);

    /* Double-invert roundtrip */
    Matrix4_Invert(inv, inv_inv);
    for (int i = 0; i < 16; i++)
        TEST_ASSERT_FLOAT_WITHIN(1e-3f, m[i], inv_inv[i]);
}

void test_Matrix4_Invert_roundtrip(void){
    /* Invert(Invert(M)) should equal M */
    mat4x4_t m, inv, inv_inv;
    Matrix4_Identity(m);
    Matrix4_Rotate(m, 60.0f, 1.0f, 0.0f, 0.0f);  /* 60° around X */
    Matrix4_Translate(m, 100.0f, -50.0f, 25.0f);

    Matrix4_Invert(m, inv);
    Matrix4_Invert(inv, inv_inv);

    for (int i = 0; i < 16; i++)
        TEST_ASSERT_FLOAT_WITHIN(1e-3f, m[i], inv_inv[i]);
}

void test_Matrix4_Invert_rotation_translation_90(void){
    /* 90° Z rotation + translation: verify double-invert roundtrip
       and that the inverse is a different matrix than the original. */
    mat4x4_t m, inv, inv_inv;
    Matrix4_Identity(m);
    Matrix4_Rotate(m, 90.0f, 0.0f, 0.0f, 1.0f);
    Matrix4_Translate(m, 10.0f, 0.0f, 0.0f);
    Matrix4_Invert(m, inv);

    /* Inverse should differ from original */
    int differ = 0;
    for (int i = 0; i < 16; i++) {
        if (fabsf(m[i] - inv[i]) > 1e-4f)
            differ = 1;
    }
    TEST_ASSERT_TRUE(differ);

    /* Double-invert roundtrip */
    Matrix4_Invert(inv, inv_inv);
    for (int i = 0; i < 16; i++)
        TEST_ASSERT_FLOAT_WITHIN(1e-3f, m[i], inv_inv[i]);
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

    /* Matrix3_Angles gimbal lock and edge cases */
    RUN_TEST(test_Matrix3_Angles_gimbal_lock_pitch_90);
    RUN_TEST(test_Matrix3_Angles_gimbal_lock_pitch_neg90);
    RUN_TEST(test_Matrix3_Angles_zero_angles);
    RUN_TEST(test_Matrix3_Angles_pure_yaw);
    RUN_TEST(test_Matrix3_Angles_pure_roll);
    RUN_TEST(test_Matrix3_Angles_near_gimbal_lock);
    RUN_TEST(test_Matrix3_Angles_negative_pitch_and_yaw);
    RUN_TEST(test_Matrix3_Angles_all_axes);

    /* Matrix4_Invert */
    RUN_TEST(test_Matrix4_Invert_identity);
    RUN_TEST(test_Matrix4_Invert_pure_translation);
    RUN_TEST(test_Matrix4_Invert_pure_rotation);
    RUN_TEST(test_Matrix4_Invert_rotation_and_translation);
    RUN_TEST(test_Matrix4_Invert_roundtrip);
    RUN_TEST(test_Matrix4_Invert_rotation_translation_90);

    return UNITY_END();
}

