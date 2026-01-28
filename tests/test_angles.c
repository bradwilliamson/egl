/*
====================
test_angles.c

Unit tests for shared/m_angles.c

Covers AngleModf, LerpAngle, VecToAngles, VecToYaw — core angle utility
functions used by the matrix/rotation system and throughout the engine.
====================
*/

#include "../unity/unity.h"
#include <math.h>

#include "../shared/shared.h"

/*
=============================================================================
AngleModf tests

Wraps an angle into [0, 360) using fixed-point quantization.
Used by Matrix3_Angles to normalize output angles.
=============================================================================
*/

void test_AngleModf_zero(void) {
    float result = AngleModf(0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result);
}

void test_AngleModf_positive_in_range(void) {
    float result = AngleModf(90.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 90.0f, result);
}

void test_AngleModf_360_wraps_to_zero(void) {
    float result = AngleModf(360.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result);
}

void test_AngleModf_negative_wraps(void) {
    float result = AngleModf(-90.0f);
    /* -90 should wrap to ~270 */
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 270.0f, result);
}

void test_AngleModf_large_positive(void) {
    float result = AngleModf(720.0f + 45.0f);
    /* 765 mod 360 = 45 */
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 45.0f, result);
}

void test_AngleModf_large_negative(void) {
    float result = AngleModf(-720.0f - 30.0f);
    /* -750 mod 360 → 330 */
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 330.0f, result);
}

void test_AngleModf_180(void) {
    float result = AngleModf(180.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 180.0f, result);
}

void test_AngleModf_negative_180(void) {
    float result = AngleModf(-180.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 180.0f, result);
}

void test_AngleModf_result_always_positive(void) {
    /* Fuzz test: many values should always produce [0, 360) */
    float angles[] = {-1000.0f, -359.0f, -0.01f, 0.01f, 359.99f, 500.0f};
    for (int i = 0; i < 6; i++) {
        float result = AngleModf(angles[i]);
        TEST_ASSERT_TRUE(result >= 0.0f);
        TEST_ASSERT_TRUE(result < 360.0f);
    }
}

/*
=============================================================================
LerpAngle tests

Linear interpolation between two angles, handling the 360° wrap.
=============================================================================
*/

void test_LerpAngle_same_angle(void) {
    float result = LerpAngle(45.0f, 45.0f, 0.5f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 45.0f, result);
}

void test_LerpAngle_t0_returns_first(void) {
    float result = LerpAngle(10.0f, 90.0f, 0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, result);
}

void test_LerpAngle_t1_returns_second(void) {
    float result = LerpAngle(10.0f, 90.0f, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 90.0f, result);
}

void test_LerpAngle_midpoint(void) {
    float result = LerpAngle(0.0f, 90.0f, 0.5f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 45.0f, result);
}

void test_LerpAngle_wraps_across_zero(void) {
    /* From 350 to 10 should go the short way (through 0/360) */
    float result = LerpAngle(350.0f, 10.0f, 0.5f);
    /* Midpoint of 350→10 the short way is 0 (or equivalently 360) */
    float diff = fabsf(result);
    if (diff > 180) diff = 360.0f - diff;
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 0.0f, diff);
}

void test_LerpAngle_wraps_across_zero_reverse(void) {
    /* From 10 to 350 should go the short way (through 0/360) */
    float result = LerpAngle(10.0f, 350.0f, 0.5f);
    /* Midpoint should be ~0 / 360 */
    float diff = fabsf(result);
    if (diff > 180) diff = 360.0f - diff;
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 0.0f, diff);
}

void test_LerpAngle_quarter(void) {
    float result = LerpAngle(0.0f, 100.0f, 0.25f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 25.0f, result);
}

/*
=============================================================================
VecToAngles tests

Converts a direction vector to pitch/yaw/roll angles.
=============================================================================
*/

void test_VecToAngles_forward_x(void) {
    vec3_t vec = {1.0f, 0.0f, 0.0f};
    vec3_t angles;

    VecToAngles(vec, angles);

    /* Forward along +X: pitch=0, yaw=0 */
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 0.0f, angles[PITCH]);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 0.0f, angles[YAW]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, angles[ROLL]);
}

void test_VecToAngles_forward_y(void) {
    vec3_t vec = {0.0f, 1.0f, 0.0f};
    vec3_t angles;

    VecToAngles(vec, angles);

    /* Forward along +Y: yaw=90 */
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 90.0f, angles[YAW]);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 0.0f, angles[PITCH]);
}

void test_VecToAngles_forward_neg_x(void) {
    vec3_t vec = {-1.0f, 0.0f, 0.0f};
    vec3_t angles;

    VecToAngles(vec, angles);

    /* Forward along -X: yaw=180 */
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 180.0f, angles[YAW]);
}

void test_VecToAngles_straight_up(void) {
    vec3_t vec = {0.0f, 0.0f, 1.0f};
    vec3_t angles;

    VecToAngles(vec, angles);

    /* Looking straight up: pitch = -90 (Q2 convention) */
    TEST_ASSERT_FLOAT_WITHIN(0.5f, -90.0f, angles[PITCH]);
}

void test_VecToAngles_straight_down(void) {
    vec3_t vec = {0.0f, 0.0f, -1.0f};
    vec3_t angles;

    VecToAngles(vec, angles);

    /* Looking straight down: pitch = -270 or equivalently 90 after wrap
       Q2 convention: pitch = -(atan2 result), and atan2(-1,0) = -90° → pitch = -270 */
    /* The exact value depends on the implementation wrapping */
    float pitch = angles[PITCH];
    /* Normalize to check it's a downward pitch */
    TEST_ASSERT_TRUE(fabsf(pitch - (-270.0f)) < 0.5f || fabsf(pitch - 90.0f) < 0.5f);
}

void test_VecToAngles_roll_always_zero(void) {
    /* VecToAngles always sets roll = 0 */
    vec3_t vec = {1.0f, 1.0f, 1.0f};
    vec3_t angles;

    VecToAngles(vec, angles);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, angles[ROLL]);
}

void test_VecToAngles_diagonal_xy(void) {
    vec3_t vec = {1.0f, 1.0f, 0.0f};
    vec3_t angles;

    VecToAngles(vec, angles);

    /* 45° yaw, 0° pitch */
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 45.0f, angles[YAW]);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 0.0f, angles[PITCH]);
}

/*
=============================================================================
VecToYaw tests

Extracts only the yaw angle from a direction vector.
=============================================================================
*/

void test_VecToYaw_forward_x(void) {
    vec3_t vec = {1.0f, 0.0f, 0.0f};
    float yaw = VecToYaw(vec);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 0.0f, yaw);
}

void test_VecToYaw_forward_y(void) {
    vec3_t vec = {0.0f, 1.0f, 0.0f};
    float yaw = VecToYaw(vec);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 90.0f, yaw);
}

void test_VecToYaw_forward_neg_y(void) {
    vec3_t vec = {0.0f, -1.0f, 0.0f};
    float yaw = VecToYaw(vec);
    /* VecToYaw returns -90 for -Y (no wrapping to positive range) */
    TEST_ASSERT_FLOAT_WITHIN(0.5f, -90.0f, yaw);
}

void test_VecToYaw_zero_vector(void) {
    vec3_t vec = {0.0f, 0.0f, 0.0f};
    float yaw = VecToYaw(vec);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, yaw);
}

void test_VecToYaw_diagonal(void) {
    vec3_t vec = {1.0f, 1.0f, 0.0f};
    float yaw = VecToYaw(vec);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 45.0f, yaw);
}

int main(void)
{
    UNITY_BEGIN();

    /* AngleModf */
    RUN_TEST(test_AngleModf_zero);
    RUN_TEST(test_AngleModf_positive_in_range);
    RUN_TEST(test_AngleModf_360_wraps_to_zero);
    RUN_TEST(test_AngleModf_negative_wraps);
    RUN_TEST(test_AngleModf_large_positive);
    RUN_TEST(test_AngleModf_large_negative);
    RUN_TEST(test_AngleModf_180);
    RUN_TEST(test_AngleModf_negative_180);
    RUN_TEST(test_AngleModf_result_always_positive);

    /* LerpAngle */
    RUN_TEST(test_LerpAngle_same_angle);
    RUN_TEST(test_LerpAngle_t0_returns_first);
    RUN_TEST(test_LerpAngle_t1_returns_second);
    RUN_TEST(test_LerpAngle_midpoint);
    RUN_TEST(test_LerpAngle_wraps_across_zero);
    RUN_TEST(test_LerpAngle_wraps_across_zero_reverse);
    RUN_TEST(test_LerpAngle_quarter);

    /* VecToAngles */
    RUN_TEST(test_VecToAngles_forward_x);
    RUN_TEST(test_VecToAngles_forward_y);
    RUN_TEST(test_VecToAngles_forward_neg_x);
    RUN_TEST(test_VecToAngles_straight_up);
    RUN_TEST(test_VecToAngles_straight_down);
    RUN_TEST(test_VecToAngles_roll_always_zero);
    RUN_TEST(test_VecToAngles_diagonal_xy);

    /* VecToYaw */
    RUN_TEST(test_VecToYaw_forward_x);
    RUN_TEST(test_VecToYaw_forward_y);
    RUN_TEST(test_VecToYaw_forward_neg_y);
    RUN_TEST(test_VecToYaw_zero_vector);
    RUN_TEST(test_VecToYaw_diagonal);

    return UNITY_END();
}
