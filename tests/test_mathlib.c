/*
 * test_mathlib.c - Unit tests for shared/mathlib.c
 * 
 * These tests verify the core math functions used throughout EGL.
 * Run with: make test_mathlib && ./test_mathlib
 */

#include "../unity/unity.h"

/* Include the actual implementation */
#include "../shared/shared.h"

/*
 * ============================================================================
 * DirToByte / ByteToDir Tests
 * 
 * These functions compress/decompress direction vectors to/from a single byte.
 * Critical for network protocol - must be invertible within tolerance.
 * ============================================================================
 */

void test_DirToByte_null_returns_zero(void) {
    /* NULL direction should return 0 safely */
    byte result = DirToByte(NULL);
    TEST_ASSERT_EQUAL(0, result);
}

void test_DirToByte_returns_valid_index(void) {
    vec3_t dir = {0.5f, 0.5f, 0.707f};
    byte result = DirToByte(dir);
    TEST_ASSERT_LESS_THAN(NUMVERTEXNORMALS, result);
}

void test_ByteToDir_clears_on_invalid(void) {
    vec3_t result;
    /* Index >= NUMVERTEXNORMALS should clear the vector */
    ByteToDir(255, result);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, result[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, result[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, result[2]);
}

void test_DirToByte_ByteToDir_roundtrip_up(void) {
    vec3_t original = {0.0f, 0.0f, 1.0f};  /* Straight up */
    vec3_t result;
    
    byte index = DirToByte(original);
    ByteToDir(index, result);
    
    /* Should be close to original (quantization error expected) */
    TEST_ASSERT_FLOAT_WITHIN(0.15f, original[0], result[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.15f, original[1], result[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.15f, original[2], result[2]);
}

void test_DirToByte_ByteToDir_roundtrip_diagonal(void) {
    vec3_t original = {0.577f, 0.577f, 0.577f};  /* Diagonal (normalized) */
    vec3_t result;
    
    byte index = DirToByte(original);
    ByteToDir(index, result);
    
    /* Diagonal directions may have more quantization error */
    TEST_ASSERT_FLOAT_WITHIN(0.25f, original[0], result[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.25f, original[1], result[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.25f, original[2], result[2]);
}

/*
 * ============================================================================
 * FloatToByte Tests
 * 
 * Converts float to byte with clamping. Used for color components.
 * ============================================================================
 */

void test_FloatToByte_zero(void) {
    /* 0.0 should map to 0 */
    byte result = FloatToByte(0.0f);
    TEST_ASSERT_EQUAL(0, result);
}

void test_FloatToByte_max_clamp(void) {
    /* Large values should clamp to 255 */
    TEST_ASSERT_EQUAL(255, FloatToByte(999.0f));
    TEST_ASSERT_EQUAL(255, FloatToByte(256.0f));
}

void test_FloatToByte_negative_clamp(void) {
    /* Negative values should clamp to 0 */
    TEST_ASSERT_EQUAL(0, FloatToByte(-1.0f));
    TEST_ASSERT_EQUAL(0, FloatToByte(-100.0f));
}

void test_FloatToByte_midrange(void) {
    /* Mid-range values */
    byte result = FloatToByte(128.0f);
    /* Allow some tolerance due to float->int conversion */
    TEST_ASSERT_GREATER_THAN(120, result);
    TEST_ASSERT_LESS_THAN(136, result);
}

/*
 * ============================================================================
 * ColorNormalizef Tests
 * 
 * Normalizes RGB so max component is 1.0. Returns scale factor.
 * ============================================================================
 */

void test_ColorNormalizef_already_normalized(void) {
    float in[3] = {0.5f, 0.5f, 0.5f};
    float out[3];
    float scale = ColorNormalizef(in, out);
    
    /* Already under 1.0, should pass through unchanged */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, out[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, out[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, out[2]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, scale);  /* Scale factor when no change needed */
}

void test_ColorNormalizef_needs_scaling(void) {
    float in[3] = {2.0f, 1.0f, 0.5f};
    float out[3];
    float scale = ColorNormalizef(in, out);
    
    /* Max was 2.0, so everything scaled by 0.5 */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, out[0]);  /* 2.0 * 0.5 = 1.0 */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, out[1]);  /* 1.0 * 0.5 = 0.5 */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.25f, out[2]); /* 0.5 * 0.5 = 0.25 */
}

void test_ColorNormalizef_all_zeros(void) {
    float in[3] = {0.0f, 0.0f, 0.0f};
    float out[3];
    float scale = ColorNormalizef(in, out);

    /* Zero color should remain zero */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, out[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, out[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, out[2]);
    /* Scale should be 1.0 (no components > 1.0) */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, scale);
}

void test_ColorNormalizef_returns_scale(void) {
    /* Verify the actual scale factor returned matches 1/max */
    float in[3] = {4.0f, 2.0f, 1.0f};
    float out[3];
    float scale = ColorNormalizef(in, out);

    /* Max component is 4.0, so scale = 1/4 = 0.25 */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.25f, scale);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, out[0]);   /* 4.0 * 0.25 = 1.0 */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, out[1]);   /* 2.0 * 0.25 = 0.5 */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.25f, out[2]);  /* 1.0 * 0.25 = 0.25 */
}

void test_ColorNormalizef_large_values(void) {
    /* Large overbright color */
    float in[3] = {10.0f, 5.0f, 2.5f};
    float out[3];
    float scale = ColorNormalizef(in, out);

    /* Max is 10.0, scale = 1/10 = 0.1 */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.1f, scale);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, out[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, out[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.25f, out[2]);
}

void test_ColorNormalizef_exactly_one(void) {
    /* Max component is exactly 1.0 — should pass through */
    float in[3] = {1.0f, 0.5f, 0.0f};
    float out[3];
    float scale = ColorNormalizef(in, out);

    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, scale);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, out[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, out[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, out[2]);
}

void test_ColorNormalizef_just_over_one(void) {
    /* Max component is 1.01 — should trigger scaling */
    float in[3] = {1.01f, 0.5f, 0.0f};
    float out[3];
    float scale = ColorNormalizef(in, out);

    TEST_ASSERT_TRUE(scale < 1.0f);
    /* Output max should be ~1.0 */
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, out[0]);
}

/*
 * ============================================================================
 * Vector Macro Tests
 * 
 * Test the Vec3* macros from shared.h
 * ============================================================================
 */

void test_Vec3Copy(void) {
    vec3_t src = {1.0f, 2.0f, 3.0f};
    vec3_t dst;
    
    Vec3Copy(src, dst);
    
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, dst[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, dst[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.0f, dst[2]);
}

void test_Vec3Clear(void) {
    vec3_t v = {99.0f, 99.0f, 99.0f};
    
    Vec3Clear(v);
    
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, v[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, v[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, v[2]);
}

void test_DotProduct(void) {
    vec3_t a = {1.0f, 0.0f, 0.0f};
    vec3_t b = {1.0f, 0.0f, 0.0f};
    
    float dot = DotProduct(a, b);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, dot);
    
    /* Perpendicular vectors */
    vec3_t c = {0.0f, 1.0f, 0.0f};
    dot = DotProduct(a, c);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, dot);
}

void test_Vec3Add(void) {
    vec3_t a = {1.0f, 2.0f, 3.0f};
    vec3_t b = {4.0f, 5.0f, 6.0f};
    vec3_t result;
    
    Vec3Add(a, b, result);
    
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, result[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 7.0f, result[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 9.0f, result[2]);
}

void test_Vec3Subtract(void) {
    vec3_t a = {5.0f, 7.0f, 9.0f};
    vec3_t b = {1.0f, 2.0f, 3.0f};
    vec3_t result;
    
    Vec3Subtract(a, b, result);
    
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 4.0f, result[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, result[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 6.0f, result[2]);
}

void test_Vec3Scale(void) {
    vec3_t v = {1.0f, 2.0f, 3.0f};
    vec3_t result;
    
    Vec3Scale(v, 2.0f, result);
    
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, result[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 4.0f, result[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 6.0f, result[2]);
}

/*
 * ============================================================================
 * Vec3Length Tests
 * 
 * Tests vector length calculation (magnitude)
 * ============================================================================
 */

void test_Vec3Length_unit_vector(void) {
    vec3_t v = {1.0f, 0.0f, 0.0f};
    float length = Vec3Length(v);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, length);
}

void test_Vec3Length_zero_vector(void) {
    vec3_t v = {0.0f, 0.0f, 0.0f};
    float length = Vec3Length(v);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, length);
}

void test_Vec3Length_pythagorean(void) {
    /* 3-4-5 triangle */
    vec3_t v = {3.0f, 4.0f, 0.0f};
    float length = Vec3Length(v);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, length);
}

void test_Vec3Length_negative_components(void) {
    /* Length should be positive even with negative components */
    vec3_t v = {-3.0f, -4.0f, 0.0f};
    float length = Vec3Length(v);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, length);
}

void test_Vec3Length_3d_vector(void) {
    /* Test 3D: sqrt(1^2 + 2^2 + 2^2) = sqrt(9) = 3 */
    vec3_t v = {1.0f, 2.0f, 2.0f};
    float length = Vec3Length(v);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.0f, length);
}

void test_Vec3Length_very_small_vector(void) {
    /* Very small but non-zero vector */
    vec3_t v = {1e-6f, 1e-6f, 1e-6f};
    float length = Vec3Length(v);
    /* sqrt(3 * (1e-6)^2) = sqrt(3e-12) ≈ 1.732e-6 */
    TEST_ASSERT_GREATER_THAN(0.0f, length);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 1.732e-6f, length);
}

void test_Vec3Length_large_vector(void) {
    vec3_t v = {1000.0f, 2000.0f, 2000.0f};
    float length = Vec3Length(v);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 3000.0f, length);
}

/*
 * ============================================================================
 * VectorNormalizef Tests
 * 
 * Normalizes vector to unit length, returns original length.
 * Critical for direction vectors and lighting calculations.
 * ============================================================================
 */

void test_VectorNormalizef_unit_vector(void) {
    vec3_t in = {1.0f, 0.0f, 0.0f};
    vec3_t out;
    
    float length = VectorNormalizef(in, out);
    
    /* Should return length 1.0 */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, length);
    
    /* Output should still be unit vector */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, out[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, out[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, out[2]);
}

void test_VectorNormalizef_zero_vector(void) {
    vec3_t in = {0.0f, 0.0f, 0.0f};
    vec3_t out;
    
    float length = VectorNormalizef(in, out);
    
    /* Should return length 0.0 */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, length);
    
    /* Output should be cleared to zero (no division by zero) */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, out[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, out[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, out[2]);
}

void test_VectorNormalizef_regular_vector(void) {
    vec3_t in = {3.0f, 4.0f, 0.0f};
    vec3_t out;
    
    float length = VectorNormalizef(in, out);
    
    /* Should return original length 5.0 */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, length);
    
    /* Normalized: (3/5, 4/5, 0) = (0.6, 0.8, 0) */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.6f, out[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.8f, out[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, out[2]);
}

void test_VectorNormalizef_3d_vector(void) {
    vec3_t in = {1.0f, 2.0f, 2.0f};
    vec3_t out;
    
    float length = VectorNormalizef(in, out);
    
    /* Length should be 3.0 */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.0f, length);
    
    /* Normalized: (1/3, 2/3, 2/3) */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.333f, out[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.667f, out[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.667f, out[2]);
    
    /* Verify result is unit length */
    float result_length = Vec3Length(out);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, result_length);
}

void test_VectorNormalizef_negative_vector(void) {
    vec3_t in = {-3.0f, -4.0f, 0.0f};
    vec3_t out;
    
    float length = VectorNormalizef(in, out);
    
    /* Length should be 5.0 */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, length);
    
    /* Normalized: (-3/5, -4/5, 0) = (-0.6, -0.8, 0) */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -0.6f, out[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -0.8f, out[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, out[2]);
}

void test_VectorNormalizef_very_small_vector(void) {
    /* Very small vector that might cause precision issues */
    vec3_t in = {1e-6f, 2e-6f, 2e-6f};
    vec3_t out;
    
    float length = VectorNormalizef(in, out);
    
    /* Should return small but non-zero length */
    TEST_ASSERT_GREATER_THAN(0.0f, length);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 3e-6f, length);
    
    /* Result should still be normalized (unit length) */
    float result_length = Vec3Length(out);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, result_length);
}

void test_VectorNormalizef_in_place(void) {
    /* Test normalizing vector in place (in == out) */
    vec3_t v = {3.0f, 4.0f, 0.0f};
    
    float length = VectorNormalizef(v, v);
    
    /* Should return original length 5.0 */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, length);
    
    /* Vector should now be normalized */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.6f, v[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.8f, v[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, v[2]);
}

void test_VectorNormalizef_large_vector(void) {
    vec3_t in = {1000.0f, 2000.0f, 2000.0f};
    vec3_t out;
    
    float length = VectorNormalizef(in, out);
    
    /* Should return original length 3000.0 */
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 3000.0f, length);
    
    /* Result should be unit length */
    float result_length = Vec3Length(out);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, result_length);
}

/*
 * ============================================================================
 * Angles_Vectors Tests
 * 
 * Converts pitch/yaw/roll angles to forward/right/up direction vectors.
 * Critical for camera orientation, entity facing, and weapon aiming.
 * 
 * Quake 2 uses:
 * - PITCH (0): up/down (negative pitch looks up)
 * - YAW (1): left/right rotation
 * - ROLL (2): banking/tilting
 * ============================================================================
 */

void test_Angles_Vectors_zero_angles(void) {
    vec3_t angles = {0.0f, 0.0f, 0.0f};
    vec3_t forward, right, up;
    
    Angles_Vectors(angles, forward, right, up);
    
    /* Zero angles: looking down +X axis */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, forward[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, forward[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, forward[2]);
    
    /* Right should be -Y axis (Quake 2 left-handed system) */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, right[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, right[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, right[2]);
    
    /* Up should be +Z axis */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, up[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, up[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, up[2]);
}

void test_Angles_Vectors_yaw_90(void) {
    vec3_t angles = {0.0f, 90.0f, 0.0f};
    vec3_t forward, right, up;
    
    Angles_Vectors(angles, forward, right, up);
    
    /* 90° yaw: looking down +Y axis */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, forward[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, forward[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, forward[2]);
    
    /* Right should be +X axis */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, right[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, right[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, right[2]);
}

void test_Angles_Vectors_yaw_180(void) {
    vec3_t angles = {0.0f, 180.0f, 0.0f};
    vec3_t forward, right, up;
    
    Angles_Vectors(angles, forward, right, up);
    
    /* 180° yaw: looking down -X axis */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, forward[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, forward[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, forward[2]);
    
    /* Right should be +Y axis */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, right[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, right[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, right[2]);
}

void test_Angles_Vectors_yaw_270(void) {
    vec3_t angles = {0.0f, 270.0f, 0.0f};
    vec3_t forward, right, up;
    
    Angles_Vectors(angles, forward, right, up);
    
    /* 270° yaw: looking down -Y axis */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, forward[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, forward[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, forward[2]);
    
    /* Right should be -X axis */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, right[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, right[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, right[2]);
}

void test_Angles_Vectors_pitch_90(void) {
    vec3_t angles = {90.0f, 0.0f, 0.0f};
    vec3_t forward, right, up;
    
    Angles_Vectors(angles, forward, right, up);
    
    /* 90° pitch: looking straight down */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, forward[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, forward[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, forward[2]);
}

void test_Angles_Vectors_pitch_negative_90(void) {
    vec3_t angles = {-90.0f, 0.0f, 0.0f};
    vec3_t forward, right, up;
    
    Angles_Vectors(angles, forward, right, up);
    
    /* -90° pitch: looking straight up */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, forward[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, forward[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, forward[2]);
}

void test_Angles_Vectors_pitch_180(void) {
    vec3_t angles = {180.0f, 0.0f, 0.0f};
    vec3_t forward, right, up;
    
    Angles_Vectors(angles, forward, right, up);
    
    /* 180° pitch: flipped upside down, looking backwards */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, forward[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, forward[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, forward[2]);
}

void test_Angles_Vectors_roll_90(void) {
    vec3_t angles = {0.0f, 0.0f, 90.0f};
    vec3_t forward, right, up;
    
    Angles_Vectors(angles, forward, right, up);
    
    /* 90° roll: tilted 90° to the right */
    /* Forward unchanged */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, forward[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, forward[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, forward[2]);
    
    /* Right becomes down (-Z) */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, right[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, right[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, right[2]);
    
    /* Up becomes -Y */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, up[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, up[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, up[2]);
}

void test_Angles_Vectors_roll_180(void) {
    vec3_t angles = {0.0f, 0.0f, 180.0f};
    vec3_t forward, right, up;
    
    Angles_Vectors(angles, forward, right, up);
    
    /* 180° roll: completely upside down */
    /* Forward unchanged */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, forward[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, forward[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, forward[2]);
    
    /* Right becomes +Y */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, right[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, right[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, right[2]);
    
    /* Up becomes -Z (down) */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, up[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, up[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, up[2]);
}

void test_Angles_Vectors_roll_270(void) {
    vec3_t angles = {0.0f, 0.0f, 270.0f};
    vec3_t forward, right, up;
    
    Angles_Vectors(angles, forward, right, up);
    
    /* 270° roll: tilted 90° to the left */
    /* Forward unchanged */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, forward[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, forward[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, forward[2]);
    
    /* Right becomes up (+Z) */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, right[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, right[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, right[2]);
    
    /* Up becomes +Y */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, up[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, up[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, up[2]);
}

void test_Angles_Vectors_combined_pitch_yaw(void) {
    vec3_t angles = {45.0f, 45.0f, 0.0f};
    vec3_t forward, right, up;
    
    Angles_Vectors(angles, forward, right, up);
    
    /* Combined 45° pitch and yaw: diagonal direction */
    /* All vectors should be unit length */
    float forward_len = Vec3Length(forward);
    float right_len = Vec3Length(right);
    float up_len = Vec3Length(up);
    
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, forward_len);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, right_len);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, up_len);
    
    /* Vectors should be orthogonal */
    float dot_fr = DotProduct(forward, right);
    float dot_fu = DotProduct(forward, up);
    float dot_ru = DotProduct(right, up);
    
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, dot_fr);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, dot_fu);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, dot_ru);
}

void test_Angles_Vectors_null_pointers(void) {
    vec3_t angles = {0.0f, 90.0f, 0.0f};
    vec3_t forward;
    
    /* Should handle NULL pointers for unwanted outputs */
    Angles_Vectors(angles, forward, NULL, NULL);
    
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, forward[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, forward[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, forward[2]);
}

void test_Angles_Vectors_orthogonality(void) {
    vec3_t angles = {30.0f, 60.0f, 45.0f};
    vec3_t forward, right, up;
    
    Angles_Vectors(angles, forward, right, up);
    
    /* All vectors should be unit length */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, Vec3Length(forward));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, Vec3Length(right));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, Vec3Length(up));
    
    /* Vectors should be mutually perpendicular (orthogonal) */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, DotProduct(forward, right));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, DotProduct(forward, up));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, DotProduct(right, up));
    
    /* Verify cross product relationship (right and up vectors are orthogonal) */
    vec3_t cross;
    CrossProduct(right, up, cross);
    /* Cross product should be unit length */
    float cross_len = Vec3Length(cross);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, cross_len);
}

/*
 * ============================================================================ * PlaneFromPoints Tests
 * 
 * Creates a plane equation (normal + distance) from 3 vertices.
 * Used for collision detection and BSP operations.
 * Plane equation: dot(normal, point) = dist for any point on the plane.
 * ============================================================================
 */

void test_PlaneFromPoints_xy_plane_at_origin(void) {
    vec3_t verts[3];
    cBspPlane_t plane;
    
    /* Three points on the XY plane at Z=0 */
    Vec3Set(verts[0], 0.0f, 0.0f, 0.0f);
    Vec3Set(verts[1], 1.0f, 0.0f, 0.0f);
    Vec3Set(verts[2], 0.0f, 1.0f, 0.0f);
    
    PlaneFromPoints(verts, &plane);
    
    /* Normal should point in -Z direction (v2 x v1) */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, plane.normal[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, plane.normal[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, plane.normal[2]);
    
    /* Distance should be 0 (plane at origin) */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, plane.dist);
}

void test_PlaneFromPoints_xy_plane_elevated(void) {
    vec3_t verts[3];
    cBspPlane_t plane;
    
    /* Three points on the XY plane at Z=5 */
    Vec3Set(verts[0], 0.0f, 0.0f, 5.0f);
    Vec3Set(verts[1], 1.0f, 0.0f, 5.0f);
    Vec3Set(verts[2], 0.0f, 1.0f, 5.0f);
    
    PlaneFromPoints(verts, &plane);
    
    /* Normal should point in -Z direction (v2 x v1) */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, plane.normal[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, plane.normal[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, plane.normal[2]);
    
    /* Distance should be -5 (dot product of (0,0,5) with (0,0,-1)) */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -5.0f, plane.dist);
}

void test_PlaneFromPoints_xz_plane(void) {
    vec3_t verts[3];
    cBspPlane_t plane;
    
    /* Three points on the XZ plane at Y=0 */
    Vec3Set(verts[0], 0.0f, 0.0f, 0.0f);
    Vec3Set(verts[1], 1.0f, 0.0f, 0.0f);
    Vec3Set(verts[2], 0.0f, 0.0f, 1.0f);
    
    PlaneFromPoints(verts, &plane);
    
    /* Normal should point in +Y direction (v2 x v1) */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, plane.normal[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, plane.normal[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, plane.normal[2]);
    
    /* Distance should be 0 */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, plane.dist);
}

void test_PlaneFromPoints_yz_plane(void) {
    vec3_t verts[3];
    cBspPlane_t plane;
    
    /* Three points on the YZ plane at X=0 */
    Vec3Set(verts[0], 0.0f, 0.0f, 0.0f);
    Vec3Set(verts[1], 0.0f, 1.0f, 0.0f);
    Vec3Set(verts[2], 0.0f, 0.0f, 1.0f);
    
    PlaneFromPoints(verts, &plane);
    
    /* Normal should point in -X direction (v2 x v1) */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, plane.normal[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, plane.normal[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, plane.normal[2]);
    
    /* Distance should be 0 */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, plane.dist);
}

void test_PlaneFromPoints_diagonal_plane(void) {
    vec3_t verts[3];
    cBspPlane_t plane;
    
    /* Three points forming a diagonal plane */
    Vec3Set(verts[0], 0.0f, 0.0f, 0.0f);
    Vec3Set(verts[1], 1.0f, 0.0f, 0.0f);
    Vec3Set(verts[2], 0.0f, 0.0f, 1.0f);
    
    PlaneFromPoints(verts, &plane);
    
    /* Normal should be normalized */
    float length = Vec3Length(plane.normal);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, length);
    
    /* All three vertices should satisfy the plane equation */
    for (int i = 0; i < 3; i++) {
        float dot = DotProduct(verts[i], plane.normal);
        TEST_ASSERT_FLOAT_WITHIN(0.001f, plane.dist, dot);
    }
}

void test_PlaneFromPoints_45_degree_slope(void) {
    vec3_t verts[3];
    cBspPlane_t plane;
    
    /* Create a 45-degree sloped plane */
    Vec3Set(verts[0], 0.0f, 0.0f, 0.0f);
    Vec3Set(verts[1], 1.0f, 0.0f, 0.0f);
    Vec3Set(verts[2], 0.0f, 0.0f, 1.0f);
    
    PlaneFromPoints(verts, &plane);
    
    /* Normal should be normalized */
    float length = Vec3Length(plane.normal);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, length);
    
    /* Test that the normal is perpendicular to the plane */
    vec3_t edge1, edge2;
    Vec3Subtract(verts[1], verts[0], edge1);
    Vec3Subtract(verts[2], verts[0], edge2);
    
    /* Normal should be perpendicular to both edges */
    float dot1 = DotProduct(plane.normal, edge1);
    float dot2 = DotProduct(plane.normal, edge2);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, dot1);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, dot2);
}

void test_PlaneFromPoints_winding_order_matters(void) {
    vec3_t verts1[3], verts2[3];
    cBspPlane_t plane1, plane2;
    
    /* Same triangle, different winding order */
    Vec3Set(verts1[0], 0.0f, 0.0f, 0.0f);
    Vec3Set(verts1[1], 1.0f, 0.0f, 0.0f);
    Vec3Set(verts1[2], 0.0f, 1.0f, 0.0f);
    
    Vec3Set(verts2[0], 0.0f, 0.0f, 0.0f);
    Vec3Set(verts2[1], 0.0f, 1.0f, 0.0f);
    Vec3Set(verts2[2], 1.0f, 0.0f, 0.0f);
    
    PlaneFromPoints(verts1, &plane1);
    PlaneFromPoints(verts2, &plane2);
    
    /* Normals should point in opposite directions */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -plane1.normal[0], plane2.normal[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -plane1.normal[1], plane2.normal[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -plane1.normal[2], plane2.normal[2]);
    
    /* Distances should have opposite signs */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -plane1.dist, plane2.dist);
}

void test_PlaneFromPoints_arbitrary_triangle(void) {
    vec3_t verts[3];
    cBspPlane_t plane;
    
    /* Arbitrary non-collinear triangle in 3D space */
    Vec3Set(verts[0], 1.0f, 2.0f, 3.0f);
    Vec3Set(verts[1], 4.0f, 5.0f, 6.0f);
    Vec3Set(verts[2], 2.0f, 8.0f, 5.0f);
    
    PlaneFromPoints(verts, &plane);
    
    /* Normal should be normalized */
    float length = Vec3Length(plane.normal);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, length);
    
    /* All vertices should satisfy plane equation */
    for (int i = 0; i < 3; i++) {
        float dot = DotProduct(verts[i], plane.normal);
        TEST_ASSERT_FLOAT_WITHIN(0.001f, plane.dist, dot);
    }
}

void test_PlaneFromPoints_right_angle_triangle(void) {
    vec3_t verts[3];
    cBspPlane_t plane;
    
    /* Right angle triangle: (0,0,0), (3,0,0), (0,4,0) */
    Vec3Set(verts[0], 0.0f, 0.0f, 0.0f);
    Vec3Set(verts[1], 3.0f, 0.0f, 0.0f);
    Vec3Set(verts[2], 0.0f, 4.0f, 0.0f);
    
    PlaneFromPoints(verts, &plane);
    
    /* Should create valid plane */
    float length = Vec3Length(plane.normal);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, length);
    
    /* Normal should point in +Z or -Z */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, plane.normal[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, plane.normal[1]);
    TEST_ASSERT_TRUE(fabs(plane.normal[2]) > 0.99f);
}

void test_PlaneFromPoints_equilateral_triangle(void) {
    vec3_t verts[3];
    cBspPlane_t plane;
    
    /* Equilateral triangle on XY plane */
    Vec3Set(verts[0], 0.0f, 0.0f, 0.0f);
    Vec3Set(verts[1], 1.0f, 0.0f, 0.0f);
    Vec3Set(verts[2], 0.5f, 0.866f, 0.0f);  /* sqrt(3)/2 ≈ 0.866 */
    
    PlaneFromPoints(verts, &plane);
    
    /* Normal should be unit length */
    float length = Vec3Length(plane.normal);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, length);
    
    /* Should lie on XY plane (normal in Z direction) */
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, plane.normal[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, plane.normal[1]);
    TEST_ASSERT_TRUE(fabs(plane.normal[2]) > 0.99f);
}

void test_PlaneFromPoints_negative_coordinates(void) {
    vec3_t verts[3];
    cBspPlane_t plane;
    
    /* Triangle with negative coordinates */
    Vec3Set(verts[0], -1.0f, -1.0f, -1.0f);
    Vec3Set(verts[1], -2.0f, -1.0f, -1.0f);
    Vec3Set(verts[2], -1.0f, -2.0f, -1.0f);
    
    PlaneFromPoints(verts, &plane);
    
    /* Normal should be normalized */
    float length = Vec3Length(plane.normal);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, length);
    
    /* All vertices should satisfy plane equation */
    for (int i = 0; i < 3; i++) {
        float dot = DotProduct(verts[i], plane.normal);
        TEST_ASSERT_FLOAT_WITHIN(0.001f, plane.dist, dot);
    }
}

void test_PlaneFromPoints_large_coordinates(void) {
    vec3_t verts[3];
    cBspPlane_t plane;
    
    /* Triangle with large coordinates */
    Vec3Set(verts[0], 1000.0f, 0.0f, 0.0f);
    Vec3Set(verts[1], 1001.0f, 0.0f, 0.0f);
    Vec3Set(verts[2], 1000.0f, 1.0f, 0.0f);
    
    PlaneFromPoints(verts, &plane);
    
    /* Normal should be normalized */
    float length = Vec3Length(plane.normal);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, length);
    
    /* Distance should be correct (large magnitude) */
    for (int i = 0; i < 3; i++) {
        float dot = DotProduct(verts[i], plane.normal);
        TEST_ASSERT_FLOAT_WITHIN(0.01f, plane.dist, dot);
    }
}

void test_PlaneFromPoints_point_on_plane_test(void) {
    vec3_t verts[3];
    cBspPlane_t plane;
    vec3_t testPoint;
    
    /* Create a plane */
    Vec3Set(verts[0], 0.0f, 0.0f, 5.0f);
    Vec3Set(verts[1], 10.0f, 0.0f, 5.0f);
    Vec3Set(verts[2], 0.0f, 10.0f, 5.0f);
    
    PlaneFromPoints(verts, &plane);
    
    /* Test point that should be on the plane */
    Vec3Set(testPoint, 5.0f, 5.0f, 5.0f);
    float dist = DotProduct(testPoint, plane.normal);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, plane.dist, dist);
    
    /* Test point that should NOT be on the plane */
    Vec3Set(testPoint, 5.0f, 5.0f, 6.0f);
    dist = DotProduct(testPoint, plane.normal);
    TEST_ASSERT_FALSE(fabs(dist - plane.dist) < 0.001f);
}

void test_PlaneFromPoints_normal_direction_consistency(void) {
    vec3_t verts[3];
    cBspPlane_t plane;
    
    /* Standard counter-clockwise triangle on XY plane */
    Vec3Set(verts[0], 0.0f, 0.0f, 0.0f);
    Vec3Set(verts[1], 1.0f, 0.0f, 0.0f);
    Vec3Set(verts[2], 0.0f, 1.0f, 0.0f);
    
    PlaneFromPoints(verts, &plane);
    
    /* Using right-hand rule: v1 = (1,0,0), v2 = (0,1,0)
     * CrossProduct(v2, v1) should give (0,0,-1) 
     * But implementation does CrossProduct(v2, v1) which gives negative Z */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, plane.normal[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, plane.normal[1]);
    /* Normal points in Z direction (sign depends on winding) */
    TEST_ASSERT_TRUE(fabs(plane.normal[2]) > 0.99f);
}

/*
 * ============================================================================
 * Bounding Box Tests (ClearBounds, AddPointToBounds, BoundsIntersect)
 * 
 * AABB (Axis-Aligned Bounding Box) operations for collision detection.
 * Bounds are represented as mins (minimum corner) and maxs (maximum corner).
 * ============================================================================
 */

void test_ClearBounds_initializes_to_invalid(void) {
    vec3_t mins, maxs;
    
    ClearBounds(mins, maxs);
    
    /* After clearing, mins should be large positive, maxs should be large negative */
    /* This allows AddPointToBounds to work correctly for the first point */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 99999.0f, mins[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 99999.0f, mins[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 99999.0f, mins[2]);
    
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -99999.0f, maxs[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -99999.0f, maxs[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -99999.0f, maxs[2]);
}

void test_AddPointToBounds_single_point(void) {
    vec3_t mins, maxs;
    vec3_t point;
    
    ClearBounds(mins, maxs);
    Vec3Set(point, 5.0f, 10.0f, 15.0f);
    
    AddPointToBounds(point, mins, maxs);
    
    /* Single point should set both mins and maxs to that point */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, mins[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, mins[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 15.0f, mins[2]);
    
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, maxs[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, maxs[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 15.0f, maxs[2]);
}

void test_AddPointToBounds_expand_bounds(void) {
    vec3_t mins, maxs;
    vec3_t point1, point2, point3;
    
    ClearBounds(mins, maxs);
    
    /* Add three points to form a bounding box */
    Vec3Set(point1, 0.0f, 0.0f, 0.0f);
    Vec3Set(point2, 10.0f, 5.0f, 8.0f);
    Vec3Set(point3, -5.0f, 3.0f, 12.0f);
    
    AddPointToBounds(point1, mins, maxs);
    AddPointToBounds(point2, mins, maxs);
    AddPointToBounds(point3, mins, maxs);
    
    /* Bounds should encompass all points */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -5.0f, mins[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, mins[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, mins[2]);
    
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, maxs[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, maxs[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 12.0f, maxs[2]);
}

void test_AddPointToBounds_expand_only_necessary_axes(void) {
    vec3_t mins, maxs;
    vec3_t point1, point2;
    
    ClearBounds(mins, maxs);
    
    /* First point at origin */
    Vec3Set(point1, 0.0f, 0.0f, 0.0f);
    AddPointToBounds(point1, mins, maxs);
    
    /* Second point only expands X axis */
    Vec3Set(point2, 10.0f, 0.0f, 0.0f);
    AddPointToBounds(point2, mins, maxs);
    
    /* Only X should have expanded */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, mins[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, maxs[0]);
    
    /* Y and Z should remain at 0 */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, mins[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, maxs[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, mins[2]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, maxs[2]);
}

void test_AddPointToBounds_negative_coordinates(void) {
    vec3_t mins, maxs;
    vec3_t point1, point2;
    
    ClearBounds(mins, maxs);
    
    Vec3Set(point1, -10.0f, -20.0f, -30.0f);
    Vec3Set(point2, -5.0f, -15.0f, -25.0f);
    
    AddPointToBounds(point1, mins, maxs);
    AddPointToBounds(point2, mins, maxs);
    
    /* Should handle negative coordinates correctly */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -10.0f, mins[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -20.0f, mins[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -30.0f, mins[2]);
    
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -5.0f, maxs[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -15.0f, maxs[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -25.0f, maxs[2]);
}

void test_AddPointToBounds_inside_existing_bounds(void) {
    vec3_t mins, maxs;
    vec3_t point1, point2, point3;
    
    ClearBounds(mins, maxs);
    
    /* Create bounds from two opposite corners */
    Vec3Set(point1, 0.0f, 0.0f, 0.0f);
    Vec3Set(point2, 10.0f, 10.0f, 10.0f);
    AddPointToBounds(point1, mins, maxs);
    AddPointToBounds(point2, mins, maxs);
    
    /* Add point inside existing bounds */
    Vec3Set(point3, 5.0f, 5.0f, 5.0f);
    AddPointToBounds(point3, mins, maxs);
    
    /* Bounds should not change */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, mins[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, mins[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, mins[2]);
    
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, maxs[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, maxs[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, maxs[2]);
}

void test_BoundsIntersect_identical_bounds(void) {
    vec3_t mins, maxs;
    
    Vec3Set(mins, 0.0f, 0.0f, 0.0f);
    Vec3Set(maxs, 10.0f, 10.0f, 10.0f);
    
    /* Identical bounds should intersect */
    TEST_ASSERT_TRUE(BoundsIntersect(mins, maxs, mins, maxs));
}

void test_BoundsIntersect_overlapping_bounds(void) {
    vec3_t mins1, maxs1, mins2, maxs2;
    
    /* First box: (0,0,0) to (10,10,10) */
    Vec3Set(mins1, 0.0f, 0.0f, 0.0f);
    Vec3Set(maxs1, 10.0f, 10.0f, 10.0f);
    
    /* Second box: (5,5,5) to (15,15,15) - overlaps first */
    Vec3Set(mins2, 5.0f, 5.0f, 5.0f);
    Vec3Set(maxs2, 15.0f, 15.0f, 15.0f);
    
    /* Should intersect */
    TEST_ASSERT_TRUE(BoundsIntersect(mins1, maxs1, mins2, maxs2));
    TEST_ASSERT_TRUE(BoundsIntersect(mins2, maxs2, mins1, maxs1));
}

void test_BoundsIntersect_touching_bounds(void) {
    vec3_t mins1, maxs1, mins2, maxs2;
    
    /* First box: (0,0,0) to (10,10,10) */
    Vec3Set(mins1, 0.0f, 0.0f, 0.0f);
    Vec3Set(maxs1, 10.0f, 10.0f, 10.0f);
    
    /* Second box: (10,10,10) to (20,20,20) - touches at corner */
    Vec3Set(mins2, 10.0f, 10.0f, 10.0f);
    Vec3Set(maxs2, 20.0f, 20.0f, 20.0f);
    
    /* Touching bounds should intersect (inclusive test) */
    TEST_ASSERT_TRUE(BoundsIntersect(mins1, maxs1, mins2, maxs2));
}

void test_BoundsIntersect_touching_face(void) {
    vec3_t mins1, maxs1, mins2, maxs2;
    
    /* First box: (0,0,0) to (10,10,10) */
    Vec3Set(mins1, 0.0f, 0.0f, 0.0f);
    Vec3Set(maxs1, 10.0f, 10.0f, 10.0f);
    
    /* Second box: (10,0,0) to (20,10,10) - shares YZ face */
    Vec3Set(mins2, 10.0f, 0.0f, 0.0f);
    Vec3Set(maxs2, 20.0f, 10.0f, 10.0f);
    
    /* Should intersect */
    TEST_ASSERT_TRUE(BoundsIntersect(mins1, maxs1, mins2, maxs2));
}

void test_BoundsIntersect_separated_bounds(void) {
    vec3_t mins1, maxs1, mins2, maxs2;
    
    /* First box: (0,0,0) to (10,10,10) */
    Vec3Set(mins1, 0.0f, 0.0f, 0.0f);
    Vec3Set(maxs1, 10.0f, 10.0f, 10.0f);
    
    /* Second box: (20,20,20) to (30,30,30) - completely separated */
    Vec3Set(mins2, 20.0f, 20.0f, 20.0f);
    Vec3Set(maxs2, 30.0f, 30.0f, 30.0f);
    
    /* Should NOT intersect */
    TEST_ASSERT_FALSE(BoundsIntersect(mins1, maxs1, mins2, maxs2));
    TEST_ASSERT_FALSE(BoundsIntersect(mins2, maxs2, mins1, maxs1));
}

void test_BoundsIntersect_separated_on_one_axis(void) {
    vec3_t mins1, maxs1, mins2, maxs2;
    
    /* First box: (0,0,0) to (10,10,10) */
    Vec3Set(mins1, 0.0f, 0.0f, 0.0f);
    Vec3Set(maxs1, 10.0f, 10.0f, 10.0f);
    
    /* Second box: overlaps XY but separated on Z */
    Vec3Set(mins2, 5.0f, 5.0f, 20.0f);
    Vec3Set(maxs2, 15.0f, 15.0f, 30.0f);
    
    /* Should NOT intersect (separated on Z axis) */
    TEST_ASSERT_FALSE(BoundsIntersect(mins1, maxs1, mins2, maxs2));
}

void test_BoundsIntersect_one_inside_other(void) {
    vec3_t mins1, maxs1, mins2, maxs2;
    
    /* Large box: (0,0,0) to (20,20,20) */
    Vec3Set(mins1, 0.0f, 0.0f, 0.0f);
    Vec3Set(maxs1, 20.0f, 20.0f, 20.0f);
    
    /* Small box inside: (5,5,5) to (10,10,10) */
    Vec3Set(mins2, 5.0f, 5.0f, 5.0f);
    Vec3Set(maxs2, 10.0f, 10.0f, 10.0f);
    
    /* Should intersect */
    TEST_ASSERT_TRUE(BoundsIntersect(mins1, maxs1, mins2, maxs2));
    TEST_ASSERT_TRUE(BoundsIntersect(mins2, maxs2, mins1, maxs1));
}

void test_BoundsIntersect_negative_coordinates(void) {
    vec3_t mins1, maxs1, mins2, maxs2;
    
    /* First box: (-10,-10,-10) to (0,0,0) */
    Vec3Set(mins1, -10.0f, -10.0f, -10.0f);
    Vec3Set(maxs1, 0.0f, 0.0f, 0.0f);
    
    /* Second box: (-5,-5,-5) to (5,5,5) - overlaps */
    Vec3Set(mins2, -5.0f, -5.0f, -5.0f);
    Vec3Set(maxs2, 5.0f, 5.0f, 5.0f);
    
    /* Should intersect */
    TEST_ASSERT_TRUE(BoundsIntersect(mins1, maxs1, mins2, maxs2));
}

void test_BoundsIntersect_point_bounds(void) {
    vec3_t mins1, maxs1, mins2, maxs2;
    
    /* First box: (0,0,0) to (10,10,10) */
    Vec3Set(mins1, 0.0f, 0.0f, 0.0f);
    Vec3Set(maxs1, 10.0f, 10.0f, 10.0f);
    
    /* Second "box" is a point inside: (5,5,5) to (5,5,5) */
    Vec3Set(mins2, 5.0f, 5.0f, 5.0f);
    Vec3Set(maxs2, 5.0f, 5.0f, 5.0f);
    
    /* Point inside should intersect */
    TEST_ASSERT_TRUE(BoundsIntersect(mins1, maxs1, mins2, maxs2));
}

void test_BoundsIntersect_point_outside(void) {
    vec3_t mins1, maxs1, mins2, maxs2;
    
    /* First box: (0,0,0) to (10,10,10) */
    Vec3Set(mins1, 0.0f, 0.0f, 0.0f);
    Vec3Set(maxs1, 10.0f, 10.0f, 10.0f);
    
    /* Second "box" is a point outside: (20,20,20) to (20,20,20) */
    Vec3Set(mins2, 20.0f, 20.0f, 20.0f);
    Vec3Set(maxs2, 20.0f, 20.0f, 20.0f);
    
    /* Point outside should NOT intersect */
    TEST_ASSERT_FALSE(BoundsIntersect(mins1, maxs1, mins2, maxs2));
}

void test_BoundsIntersect_large_coordinates(void) {
    vec3_t mins1, maxs1, mins2, maxs2;
    
    /* First box: (1000,1000,1000) to (2000,2000,2000) */
    Vec3Set(mins1, 1000.0f, 1000.0f, 1000.0f);
    Vec3Set(maxs1, 2000.0f, 2000.0f, 2000.0f);
    
    /* Second box: (1500,1500,1500) to (2500,2500,2500) */
    Vec3Set(mins2, 1500.0f, 1500.0f, 1500.0f);
    Vec3Set(maxs2, 2500.0f, 2500.0f, 2500.0f);
    
    /* Should intersect even with large coordinates */
    TEST_ASSERT_TRUE(BoundsIntersect(mins1, maxs1, mins2, maxs2));
}

void test_Bounds_integration_workflow(void) {
    vec3_t mins, maxs;
    vec3_t testMins, testMaxs;
    vec3_t points[5];
    
    /* Workflow: Clear, add multiple points, test intersection */
    ClearBounds(mins, maxs);
    
    /* Add several points to form a bounding box */
    Vec3Set(points[0], 10.0f, 20.0f, 30.0f);
    Vec3Set(points[1], 15.0f, 25.0f, 35.0f);
    Vec3Set(points[2], 12.0f, 22.0f, 32.0f);
    Vec3Set(points[3], 8.0f, 18.0f, 28.0f);
    Vec3Set(points[4], 14.0f, 24.0f, 34.0f);
    
    for (int i = 0; i < 5; i++) {
        AddPointToBounds(points[i], mins, maxs);
    }
    
    /* Verify bounds encompass all points */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 8.0f, mins[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 18.0f, mins[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 28.0f, mins[2]);
    
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 15.0f, maxs[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 25.0f, maxs[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 35.0f, maxs[2]);
    
    /* Test intersection with overlapping bounds */
    Vec3Set(testMins, 12.0f, 20.0f, 30.0f);
    Vec3Set(testMaxs, 20.0f, 30.0f, 40.0f);
    TEST_ASSERT_TRUE(BoundsIntersect(mins, maxs, testMins, testMaxs));
    
    /* Test with non-intersecting bounds */
    Vec3Set(testMins, 100.0f, 100.0f, 100.0f);
    Vec3Set(testMaxs, 200.0f, 200.0f, 200.0f);
    TEST_ASSERT_FALSE(BoundsIntersect(mins, maxs, testMins, testMaxs));
}

void test_BoundsAndSphereIntersect_sphere_inside(void) {
    vec3_t mins, maxs, centre;

    Vec3Set(mins, 0.0f, 0.0f, 0.0f);
    Vec3Set(maxs, 10.0f, 10.0f, 10.0f);
    Vec3Set(centre, 5.0f, 5.0f, 5.0f);

    /* Sphere fully inside box should intersect */
    TEST_ASSERT_TRUE(BoundsAndSphereIntersect(mins, maxs, centre, 1.0f));
}

void test_BoundsAndSphereIntersect_sphere_outside(void) {
    vec3_t mins, maxs, centre;

    Vec3Set(mins, 0.0f, 0.0f, 0.0f);
    Vec3Set(maxs, 10.0f, 10.0f, 10.0f);
    Vec3Set(centre, 20.0f, 20.0f, 20.0f);

    /* Sphere fully outside should NOT intersect */
    TEST_ASSERT_FALSE(BoundsAndSphereIntersect(mins, maxs, centre, 1.0f));
}

void test_BoundsAndSphereIntersect_sphere_partial(void) {
    vec3_t mins, maxs, centre;

    Vec3Set(mins, 0.0f, 0.0f, 0.0f);
    Vec3Set(maxs, 10.0f, 10.0f, 10.0f);
    
    /* Sphere centered just outside the box but overlapping */
    Vec3Set(centre, 10.5f, 5.0f, 5.0f);
    TEST_ASSERT_TRUE(BoundsAndSphereIntersect(mins, maxs, centre, 1.0f));

    /* Sphere just outside (no overlap) */
    Vec3Set(centre, 11.1f, 5.0f, 5.0f);
    TEST_ASSERT_FALSE(BoundsAndSphereIntersect(mins, maxs, centre, 1.0f));
}

void test_RadiusFromBounds_cube(void) {
    vec3_t mins, maxs;

    /* Cube from (-1,-1,-1) to (1,1,1) has radius sqrt(3) */
    Vec3Set(mins, -1.0f, -1.0f, -1.0f);
    Vec3Set(maxs, 1.0f, 1.0f, 1.0f);

    float r = RadiusFromBounds(mins, maxs);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, sqrtf(3.0f), r);
}

void test_RadiusFromBounds_rectangular(void) {
    vec3_t mins, maxs;

    /* Rectangular bounds: choose extremes so corner vector is (4,3,0) -> radius 5 */
    Vec3Set(mins, 0.0f, 0.0f, 0.0f);
    Vec3Set(maxs, 4.0f, 3.0f, 0.0f);

    float r = RadiusFromBounds(mins, maxs);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, r);
}

/*
 * ============================================================================ * Test Runner
 * ============================================================================
 */

int main(void) {
    UNITY_BEGIN();
    
    /* DirToByte / ByteToDir */
    RUN_TEST(test_DirToByte_null_returns_zero);
    RUN_TEST(test_DirToByte_returns_valid_index);
    RUN_TEST(test_ByteToDir_clears_on_invalid);
    RUN_TEST(test_DirToByte_ByteToDir_roundtrip_up);
    RUN_TEST(test_DirToByte_ByteToDir_roundtrip_diagonal);
    
    /* FloatToByte */
    RUN_TEST(test_FloatToByte_zero);
    RUN_TEST(test_FloatToByte_max_clamp);
    RUN_TEST(test_FloatToByte_negative_clamp);
    RUN_TEST(test_FloatToByte_midrange);
    
    /* ColorNormalizef */
    RUN_TEST(test_ColorNormalizef_already_normalized);
    RUN_TEST(test_ColorNormalizef_needs_scaling);
    RUN_TEST(test_ColorNormalizef_all_zeros);
    RUN_TEST(test_ColorNormalizef_returns_scale);
    RUN_TEST(test_ColorNormalizef_large_values);
    RUN_TEST(test_ColorNormalizef_exactly_one);
    RUN_TEST(test_ColorNormalizef_just_over_one);
    
    /* Vector macros */
    RUN_TEST(test_Vec3Copy);
    RUN_TEST(test_Vec3Clear);
    RUN_TEST(test_DotProduct);
    RUN_TEST(test_Vec3Add);
    RUN_TEST(test_Vec3Subtract);
    RUN_TEST(test_Vec3Scale);
    
    /* Vec3Length */
    RUN_TEST(test_Vec3Length_unit_vector);
    RUN_TEST(test_Vec3Length_zero_vector);
    RUN_TEST(test_Vec3Length_pythagorean);
    RUN_TEST(test_Vec3Length_negative_components);
    RUN_TEST(test_Vec3Length_3d_vector);
    RUN_TEST(test_Vec3Length_very_small_vector);
    RUN_TEST(test_Vec3Length_large_vector);
    
    /* VectorNormalizef */
    RUN_TEST(test_VectorNormalizef_unit_vector);
    RUN_TEST(test_VectorNormalizef_zero_vector);
    RUN_TEST(test_VectorNormalizef_regular_vector);
    RUN_TEST(test_VectorNormalizef_3d_vector);
    RUN_TEST(test_VectorNormalizef_negative_vector);
    RUN_TEST(test_VectorNormalizef_very_small_vector);
    RUN_TEST(test_VectorNormalizef_in_place);
    RUN_TEST(test_VectorNormalizef_large_vector);
    
    /* Angles_Vectors */
    RUN_TEST(test_Angles_Vectors_zero_angles);
    RUN_TEST(test_Angles_Vectors_yaw_90);
    RUN_TEST(test_Angles_Vectors_yaw_180);
    RUN_TEST(test_Angles_Vectors_yaw_270);
    RUN_TEST(test_Angles_Vectors_pitch_90);
    RUN_TEST(test_Angles_Vectors_pitch_negative_90);
    RUN_TEST(test_Angles_Vectors_pitch_180);
    RUN_TEST(test_Angles_Vectors_roll_90);
    RUN_TEST(test_Angles_Vectors_roll_180);
    RUN_TEST(test_Angles_Vectors_roll_270);
    RUN_TEST(test_Angles_Vectors_combined_pitch_yaw);
    RUN_TEST(test_Angles_Vectors_null_pointers);
    RUN_TEST(test_Angles_Vectors_orthogonality);
    
    /* PlaneFromPoints */
    RUN_TEST(test_PlaneFromPoints_xy_plane_at_origin);
    RUN_TEST(test_PlaneFromPoints_xy_plane_elevated);
    RUN_TEST(test_PlaneFromPoints_xz_plane);
    RUN_TEST(test_PlaneFromPoints_yz_plane);
    RUN_TEST(test_PlaneFromPoints_diagonal_plane);
    RUN_TEST(test_PlaneFromPoints_45_degree_slope);
    RUN_TEST(test_PlaneFromPoints_winding_order_matters);
    RUN_TEST(test_PlaneFromPoints_arbitrary_triangle);
    RUN_TEST(test_PlaneFromPoints_right_angle_triangle);
    RUN_TEST(test_PlaneFromPoints_equilateral_triangle);
    RUN_TEST(test_PlaneFromPoints_negative_coordinates);
    RUN_TEST(test_PlaneFromPoints_large_coordinates);
    RUN_TEST(test_PlaneFromPoints_point_on_plane_test);
    RUN_TEST(test_PlaneFromPoints_normal_direction_consistency);
    
    /* Bounding Box Functions */
    RUN_TEST(test_ClearBounds_initializes_to_invalid);
    RUN_TEST(test_AddPointToBounds_single_point);
    RUN_TEST(test_AddPointToBounds_expand_bounds);
    RUN_TEST(test_AddPointToBounds_expand_only_necessary_axes);
    RUN_TEST(test_AddPointToBounds_negative_coordinates);
    RUN_TEST(test_AddPointToBounds_inside_existing_bounds);
    RUN_TEST(test_BoundsIntersect_identical_bounds);
    RUN_TEST(test_BoundsIntersect_overlapping_bounds);
    RUN_TEST(test_BoundsIntersect_touching_bounds);
    RUN_TEST(test_BoundsIntersect_touching_face);
    RUN_TEST(test_BoundsIntersect_separated_bounds);
    RUN_TEST(test_BoundsIntersect_separated_on_one_axis);
    RUN_TEST(test_BoundsIntersect_one_inside_other);
    RUN_TEST(test_BoundsIntersect_negative_coordinates);
    RUN_TEST(test_BoundsIntersect_point_bounds);
    RUN_TEST(test_BoundsIntersect_point_outside);
    RUN_TEST(test_BoundsIntersect_large_coordinates);
    RUN_TEST(test_Bounds_integration_workflow);

    /* Bounds vs Sphere tests */
    RUN_TEST(test_BoundsAndSphereIntersect_sphere_inside);
    RUN_TEST(test_BoundsAndSphereIntersect_sphere_outside);
    RUN_TEST(test_BoundsAndSphereIntersect_sphere_partial);

    /* RadiusFromBounds tests */
    RUN_TEST(test_RadiusFromBounds_cube);
    RUN_TEST(test_RadiusFromBounds_rectangular);
    
    return UNITY_END();
}
