/*
====================
test_pmove_clipvelocity.c

Unit tests for PM_ClipVelocity - velocity reflection off surfaces
====================
*/

#include "../unity/unity.h"
#include <math.h>
#include <string.h>

// Forward declarations
typedef float vec_t;
typedef vec_t vec3_t[3];

#define LARGE_EPSILON 0.1f

// Vector operations
static float DotProduct(const vec3_t v1, const vec3_t v2) {
	return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

static float VectorLength(const vec3_t v) {
	return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

static void VectorCopy(const vec3_t in, vec3_t out) {
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

static void VectorNormalize(vec3_t v) {
	float length = VectorLength(v);
	if (length > 0) {
		float ilength = 1.0f / length;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}
}

static void VectorSet(vec3_t v, float x, float y, float z) {
	v[0] = x;
	v[1] = y;
	v[2] = z;
}

// PM_ClipVelocity implementation (from sv_pmove.c)
static void PM_ClipVelocity(vec3_t in, vec3_t normal, vec3_t out, float overbounce) {
	float backoff;
	float change;
	int i;
	
	backoff = DotProduct(in, normal) * overbounce;
	
	for (i = 0; i < 3; i++) {
		change = normal[i] * backoff;
		out[i] = in[i] - change;
		if ((out[i] > -LARGE_EPSILON) && (out[i] < LARGE_EPSILON))
			out[i] = 0;
	}
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

static int VectorsEqual(const vec3_t v1, const vec3_t v2, float epsilon) {
	return (fabsf(v1[0] - v2[0]) < epsilon &&
	        fabsf(v1[1] - v2[1]) < epsilon &&
	        fabsf(v1[2] - v2[2]) < epsilon);
}

static int IsParallelToSurface(const vec3_t velocity, const vec3_t normal, float epsilon) {
	// Velocity is parallel to surface if dot product with normal is ~0
	float dot = DotProduct(velocity, normal);
	return fabsf(dot) < epsilon;
}

static float AngleBetweenVectors(const vec3_t v1, const vec3_t v2) {
	float dot = DotProduct(v1, v2);
	float len1 = VectorLength(v1);
	float len2 = VectorLength(v2);
	
	if (len1 < 0.0001f || len2 < 0.0001f)
		return 0.0f;
	
	float cosAngle = dot / (len1 * len2);
	
	// Clamp to [-1, 1] to avoid acos domain errors
	if (cosAngle > 1.0f) cosAngle = 1.0f;
	if (cosAngle < -1.0f) cosAngle = -1.0f;
	
	return acosf(cosAngle) * (180.0f / 3.14159265358979323846f);
}

// ============================================================================
// TESTS: 90 Degree Collisions (Perpendicular)
// ============================================================================

void test_ClipVelocity_90Degrees_IntoWall_Overbounce1_0(void) {
	vec3_t velocity, normal, result;
	
	// Moving forward into a wall (velocity perpendicular to wall)
	VectorSet(velocity, 100.0f, 0.0f, 0.0f);
	VectorSet(normal, 1.0f, 0.0f, 0.0f);  // Wall facing left
	
	PM_ClipVelocity(velocity, normal, result, 1.0f);
	
	// Should stop completely (all velocity cancelled)
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result[0]);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result[1]);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result[2]);
}

void test_ClipVelocity_90Degrees_IntoWall_Overbounce1_001(void) {
	vec3_t velocity, normal, result;
	
	// Moving forward into a wall with slight overbounce
	VectorSet(velocity, 100.0f, 0.0f, 0.0f);
	VectorSet(normal, 1.0f, 0.0f, 0.0f);
	
	PM_ClipVelocity(velocity, normal, result, 1.001f);
	
	// Should have tiny negative velocity (bounced back slightly)
	TEST_ASSERT_TRUE(result[0] < 0.0f);
	TEST_ASSERT_FLOAT_WITHIN(1.0f, -0.1f, result[0]);  // Small bounce back
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result[1]);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result[2]);
}

void test_ClipVelocity_90Degrees_IntoFloor_Overbounce1_0(void) {
	vec3_t velocity, normal, result;
	
	// Falling straight down into floor
	VectorSet(velocity, 0.0f, 0.0f, -100.0f);
	VectorSet(normal, 0.0f, 0.0f, 1.0f);  // Floor (up normal)
	
	PM_ClipVelocity(velocity, normal, result, 1.0f);
	
	// Should stop completely
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result[0]);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result[1]);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result[2]);
}

void test_ClipVelocity_90Degrees_IntoFloor_Overbounce1_001(void) {
	vec3_t velocity, normal, result;
	
	// Falling straight down with overbounce
	VectorSet(velocity, 0.0f, 0.0f, -100.0f);
	VectorSet(normal, 0.0f, 0.0f, 1.0f);
	
	PM_ClipVelocity(velocity, normal, result, 1.001f);
	
	// Should bounce up slightly
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result[0]);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result[1]);
	TEST_ASSERT_TRUE(result[2] > 0.0f);
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 0.1f, result[2]);  // Small bounce up
}

void test_ClipVelocity_90Degrees_IntoCeiling_Overbounce1_0(void) {
	vec3_t velocity, normal, result;
	
	// Jumping into ceiling
	VectorSet(velocity, 0.0f, 0.0f, 100.0f);
	VectorSet(normal, 0.0f, 0.0f, -1.0f);  // Ceiling (down normal)
	
	PM_ClipVelocity(velocity, normal, result, 1.0f);
	
	// Should stop completely
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result[0]);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result[1]);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result[2]);
}

// ============================================================================
// TESTS: 45 Degree Collisions
// ============================================================================

void test_ClipVelocity_45Degrees_IntoWall_Overbounce1_0(void) {
	vec3_t velocity, normal, result;
	
	// Moving at 45 degrees into a wall
	VectorSet(velocity, 100.0f, 100.0f, 0.0f);
	VectorSet(normal, 1.0f, 0.0f, 0.0f);  // Wall facing left
	
	PM_ClipVelocity(velocity, normal, result, 1.0f);
	
	// Forward component should be cancelled, sideways preserved
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result[0]);
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 100.0f, result[1]);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result[2]);
	
	// Result should be parallel to wall
	TEST_ASSERT_TRUE(IsParallelToSurface(result, normal, 0.01f));
}

void test_ClipVelocity_45Degrees_IntoWall_Overbounce1_001(void) {
	vec3_t velocity, normal, result;
	
	// Moving at 45 degrees into a wall with overbounce
	VectorSet(velocity, 100.0f, 100.0f, 0.0f);
	VectorSet(normal, 1.0f, 0.0f, 0.0f);
	
	PM_ClipVelocity(velocity, normal, result, 1.001f);
	
	// Forward component should be slightly negative (bounce back)
	TEST_ASSERT_TRUE(result[0] < 0.0f);
	TEST_ASSERT_FLOAT_WITHIN(1.0f, -0.1f, result[0]);
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 100.0f, result[1]);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result[2]);
}

void test_ClipVelocity_45Degrees_IntoFloor_Overbounce1_0(void) {
	vec3_t velocity, normal, result;
	
	// Moving at 45 degrees into floor
	VectorSet(velocity, 100.0f, 0.0f, -100.0f);
	VectorSet(normal, 0.0f, 0.0f, 1.0f);  // Floor
	
	PM_ClipVelocity(velocity, normal, result, 1.0f);
	
	// Horizontal component preserved, vertical cancelled
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 100.0f, result[0]);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result[1]);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result[2]);
	
	// Result should be parallel to floor
	TEST_ASSERT_TRUE(IsParallelToSurface(result, normal, 0.01f));
}

void test_ClipVelocity_45Degrees_IntoFloor_Overbounce1_001(void) {
	vec3_t velocity, normal, result;
	
	// Moving at 45 degrees into floor with overbounce
	VectorSet(velocity, 100.0f, 0.0f, -100.0f);
	VectorSet(normal, 0.0f, 0.0f, 1.0f);
	
	PM_ClipVelocity(velocity, normal, result, 1.001f);
	
	// Horizontal preserved, vertical bounces slightly
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 100.0f, result[0]);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result[1]);
	TEST_ASSERT_TRUE(result[2] > 0.0f);
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 0.1f, result[2]);
}

// ============================================================================
// TESTS: Diagonal Wall (45 degree angle)
// ============================================================================

void test_ClipVelocity_45Degrees_DiagonalWall_Overbounce1_0(void) {
	vec3_t velocity, normal, result;
	
	// Moving perpendicular to diagonal wall
	VectorSet(velocity, 100.0f, 0.0f, 0.0f);
	
	// Diagonal wall (45 degrees)
	VectorSet(normal, 0.707107f, 0.707107f, 0.0f);  // Normalized (1,1,0)
	
	PM_ClipVelocity(velocity, normal, result, 1.0f);
	
	// Should slide along the wall
	TEST_ASSERT_TRUE(IsParallelToSurface(result, normal, 0.01f));
	
	// Should have velocity in Y direction (sliding along wall)
	TEST_ASSERT_TRUE(fabsf(result[1]) > 1.0f);
}

void test_ClipVelocity_45Degrees_DiagonalWall_Overbounce1_001(void) {
	vec3_t velocity, normal, result;
	
	// Moving perpendicular to diagonal wall with overbounce
	VectorSet(velocity, 100.0f, 0.0f, 0.0f);
	VectorSet(normal, 0.707107f, 0.707107f, 0.0f);
	
	PM_ClipVelocity(velocity, normal, result, 1.001f);
	
	// Should still be mostly parallel (overbounce adds tiny perpendicular component)
	float dot = fabsf(DotProduct(result, normal));
	TEST_ASSERT_TRUE(dot < 1.0f);  // Nearly parallel
}

// ============================================================================
// TESTS: Parallel to Surface (Should Pass Through Unchanged)
// ============================================================================

void test_ClipVelocity_ParallelToWall_NoChange(void) {
	vec3_t velocity, normal, result;
	
	// Moving parallel to wall (should not be clipped)
	VectorSet(velocity, 0.0f, 100.0f, 0.0f);
	VectorSet(normal, 1.0f, 0.0f, 0.0f);  // Wall facing left
	
	PM_ClipVelocity(velocity, normal, result, 1.0f);
	
	// Velocity should be unchanged
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result[0]);
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 100.0f, result[1]);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result[2]);
}

void test_ClipVelocity_ParallelToFloor_NoChange(void) {
	vec3_t velocity, normal, result;
	
	// Moving parallel to floor
	VectorSet(velocity, 100.0f, 100.0f, 0.0f);
	VectorSet(normal, 0.0f, 0.0f, 1.0f);  // Floor
	
	PM_ClipVelocity(velocity, normal, result, 1.0f);
	
	// Velocity should be unchanged
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 100.0f, result[0]);
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 100.0f, result[1]);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result[2]);
}

// ============================================================================
// TESTS: Grazing Angles
// ============================================================================

void test_ClipVelocity_GrazingAngle_10Degrees(void) {
	vec3_t velocity, normal, result;
	float angle;
	
	// Nearly parallel (10 degree angle)
	VectorSet(velocity, 100.0f, 17.6f, 0.0f);  // tan(10°) ≈ 0.176
	VectorSet(normal, 1.0f, 0.0f, 0.0f);
	
	PM_ClipVelocity(velocity, normal, result, 1.0f);
	
	// Most velocity should be preserved (sliding along surface)
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result[0]);
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 17.6f, result[1]);
	
	// Result must be parallel to surface
	TEST_ASSERT_TRUE(IsParallelToSurface(result, normal, 0.01f));
}

void test_ClipVelocity_GrazingAngle_80Degrees(void) {
	vec3_t velocity, normal, result;
	
	// Steep angle (80 degrees from parallel = 10 from perpendicular)
	VectorSet(velocity, 100.0f, 567.1f, 0.0f);  // tan(80°) ≈ 5.671
	VectorSet(normal, 1.0f, 0.0f, 0.0f);
	
	PM_ClipVelocity(velocity, normal, result, 1.0f);
	
	// Forward component cancelled, large sideways component preserved
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result[0]);
	TEST_ASSERT_FLOAT_WITHIN(10.0f, 567.1f, result[1]);
	
	// Result must be parallel to surface
	TEST_ASSERT_TRUE(IsParallelToSurface(result, normal, 0.01f));
}

// ============================================================================
// TESTS: Velocity Conservation
// ============================================================================

void test_ClipVelocity_EnergyLoss_Overbounce1_0(void) {
	vec3_t velocity, normal, result;
	float initialSpeed, finalSpeed;
	
	// 45 degree collision
	VectorSet(velocity, 100.0f, 100.0f, 0.0f);
	VectorSet(normal, 1.0f, 0.0f, 0.0f);
	
	initialSpeed = VectorLength(velocity);
	
	PM_ClipVelocity(velocity, normal, result, 1.0f);
	
	finalSpeed = VectorLength(result);
	
	// With overbounce = 1.0, energy should be lost (inelastic collision)
	TEST_ASSERT_TRUE(finalSpeed < initialSpeed);
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 100.0f, finalSpeed);  // Should be ~100 (Y component)
}

void test_ClipVelocity_EnergyIncrease_Overbounce1_001(void) {
	vec3_t velocity, normal, result;
	float initialSpeed, finalSpeed;
	
	// 45 degree collision with overbounce
	VectorSet(velocity, 100.0f, 100.0f, 0.0f);
	VectorSet(normal, 1.0f, 0.0f, 0.0f);
	
	initialSpeed = VectorLength(velocity);
	
	PM_ClipVelocity(velocity, normal, result, 1.001f);
	
	finalSpeed = VectorLength(result);
	
	// With overbounce > 1.0, should have slightly more energy
	TEST_ASSERT_TRUE(finalSpeed >= 100.0f);
}

// ============================================================================
// TESTS: Angled Surfaces (Slopes)
// ============================================================================

void test_ClipVelocity_30DegreeSlope_Downward(void) {
	vec3_t velocity, normal, result;
	
	// Moving down a 30 degree slope
	VectorSet(velocity, 0.0f, 0.0f, -100.0f);
	
	// 30 degree slope normal
	float angle = 30.0f * 3.14159265f / 180.0f;
	VectorSet(normal, 0.0f, sinf(angle), cosf(angle));
	VectorNormalize(normal);
	
	PM_ClipVelocity(velocity, normal, result, 1.0f);
	
	// Should slide down the slope (forward and down components)
	TEST_ASSERT_TRUE(result[1] != 0.0f || result[2] != 0.0f);
	TEST_ASSERT_TRUE(IsParallelToSurface(result, normal, 0.01f));
}

void test_ClipVelocity_45DegreeSlope_Forward(void) {
	vec3_t velocity, normal, result;
	
	// Moving forward on a 45 degree slope
	VectorSet(velocity, 100.0f, 0.0f, 0.0f);
	
	// 45 degree slope normal (pointing up and back)
	VectorSet(normal, -0.707107f, 0.0f, 0.707107f);
	
	PM_ClipVelocity(velocity, normal, result, 1.0f);
	
	// Should slide along slope
	TEST_ASSERT_TRUE(IsParallelToSurface(result, normal, 0.01f));
	
	// Should have both forward and down components
	TEST_ASSERT_TRUE(fabsf(result[0]) > 1.0f);
	TEST_ASSERT_TRUE(fabsf(result[2]) > 1.0f);
}

// ============================================================================
// TESTS: Edge Cases
// ============================================================================

void test_ClipVelocity_ZeroVelocity(void) {
	vec3_t velocity, normal, result;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	VectorSet(normal, 1.0f, 0.0f, 0.0f);
	
	PM_ClipVelocity(velocity, normal, result, 1.0f);
	
	// Should remain zero
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result[0]);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result[1]);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result[2]);
}

void test_ClipVelocity_SmallVelocity(void) {
	vec3_t velocity, normal, result;
	
	// Very small velocity (should be clamped to zero by LARGE_EPSILON)
	VectorSet(velocity, 0.05f, 0.0f, 0.0f);
	VectorSet(normal, 1.0f, 0.0f, 0.0f);
	
	PM_ClipVelocity(velocity, normal, result, 1.0f);
	
	// Should be clamped to zero
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result[0]);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result[1]);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result[2]);
}

void test_ClipVelocity_HighSpeed(void) {
	vec3_t velocity, normal, result;
	
	// Very high speed
	VectorSet(velocity, 10000.0f, 0.0f, 0.0f);
	VectorSet(normal, 1.0f, 0.0f, 0.0f);
	
	PM_ClipVelocity(velocity, normal, result, 1.0f);
	
	// Should still clip correctly
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 0.0f, result[0]);
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 0.0f, result[1]);
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 0.0f, result[2]);
}

void test_ClipVelocity_Overbounce_Zero(void) {
	vec3_t velocity, normal, result;
	
	// Overbounce = 0 means no reflection at all
	VectorSet(velocity, 100.0f, 0.0f, 0.0f);
	VectorSet(normal, 1.0f, 0.0f, 0.0f);
	
	PM_ClipVelocity(velocity, normal, result, 0.0f);
	
	// Velocity should pass through unchanged
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 100.0f, result[0]);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result[1]);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result[2]);
}

void test_ClipVelocity_Overbounce_High(void) {
	vec3_t velocity, normal, result;
	
	// Very high overbounce (2.0 = perfect elastic collision)
	VectorSet(velocity, 100.0f, 0.0f, 0.0f);
	VectorSet(normal, 1.0f, 0.0f, 0.0f);
	
	PM_ClipVelocity(velocity, normal, result, 2.0f);
	
	// Should bounce back fully
	TEST_ASSERT_FLOAT_WITHIN(1.0f, -100.0f, result[0]);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result[1]);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result[2]);
}

// ============================================================================
// TESTS: Multiple Axis Collisions
// ============================================================================

void test_ClipVelocity_3DMovement_FloorCollision(void) {
	vec3_t velocity, normal, result;
	
	// Moving in all 3 axes, hitting floor
	VectorSet(velocity, 50.0f, 50.0f, -100.0f);
	VectorSet(normal, 0.0f, 0.0f, 1.0f);  // Floor
	
	PM_ClipVelocity(velocity, normal, result, 1.0f);
	
	// X and Y preserved, Z cancelled
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 50.0f, result[0]);
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 50.0f, result[1]);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result[2]);
	
	// Result parallel to floor
	TEST_ASSERT_TRUE(IsParallelToSurface(result, normal, 0.01f));
}

void test_ClipVelocity_3DMovement_CornerCollision(void) {
	vec3_t velocity, normal, result;
	
	// Moving toward a corner (45-45-45)
	VectorSet(velocity, 100.0f, 100.0f, 100.0f);
	
	// Diagonal normal (corner)
	VectorSet(normal, 0.577350f, 0.577350f, 0.577350f);  // Normalized (1,1,1)
	
	PM_ClipVelocity(velocity, normal, result, 1.0f);
	
	// Should be deflected but remain parallel to surface
	TEST_ASSERT_TRUE(IsParallelToSurface(result, normal, 0.01f));
}

// ============================================================================
// TESTS: Verify Parallel Output
// ============================================================================

void test_ClipVelocity_OutputAlwaysParallel_RandomAngles(void) {
	vec3_t velocity, normal, result;
	int i;
	
	// Test 10 random angles
	float angles[] = {0, 15, 30, 45, 60, 75, 85, 89, 89.9f, 90};
	
	for (i = 0; i < 10; i++) {
		float radians = angles[i] * 3.14159265f / 180.0f;
		
		// Velocity at various angles to wall
		VectorSet(velocity, 100.0f * cosf(radians), 100.0f * sinf(radians), 0.0f);
		VectorSet(normal, 1.0f, 0.0f, 0.0f);
		
		PM_ClipVelocity(velocity, normal, result, 1.0f);
		
		// Result must be parallel to surface (perpendicular component removed)
		if (VectorLength(result) > 0.1f) {  // Ignore near-zero velocities
			TEST_ASSERT_TRUE(IsParallelToSurface(result, normal, 0.1f));
		}
	}
}

void test_ClipVelocity_OutputAlwaysParallel_VariousNormals(void) {
	vec3_t velocity, normal, result;
	
	// Fixed velocity, various surface normals
	VectorSet(velocity, 100.0f, 100.0f, -50.0f);
	
	// Test 1: Vertical wall
	VectorSet(normal, 1.0f, 0.0f, 0.0f);
	PM_ClipVelocity(velocity, normal, result, 1.0f);
	TEST_ASSERT_TRUE(IsParallelToSurface(result, normal, 0.01f));
	
	// Test 2: Floor
	VectorSet(normal, 0.0f, 0.0f, 1.0f);
	PM_ClipVelocity(velocity, normal, result, 1.0f);
	TEST_ASSERT_TRUE(IsParallelToSurface(result, normal, 0.01f));
	
	// Test 3: Diagonal surface
	VectorSet(normal, 0.707107f, 0.707107f, 0.0f);
	PM_ClipVelocity(velocity, normal, result, 1.0f);
	TEST_ASSERT_TRUE(IsParallelToSurface(result, normal, 0.1f));
	
	// Test 4: Sloped surface
	VectorSet(normal, 0.0f, 0.707107f, 0.707107f);
	PM_ClipVelocity(velocity, normal, result, 1.0f);
	TEST_ASSERT_TRUE(IsParallelToSurface(result, normal, 0.1f));
}

// ============================================================================
// MAIN
// ============================================================================

int main(void) {
	UNITY_BEGIN();

	// 90 Degree Collisions
	RUN_TEST(test_ClipVelocity_90Degrees_IntoWall_Overbounce1_0);
	RUN_TEST(test_ClipVelocity_90Degrees_IntoWall_Overbounce1_001);
	RUN_TEST(test_ClipVelocity_90Degrees_IntoFloor_Overbounce1_0);
	RUN_TEST(test_ClipVelocity_90Degrees_IntoFloor_Overbounce1_001);
	RUN_TEST(test_ClipVelocity_90Degrees_IntoCeiling_Overbounce1_0);

	// 45 Degree Collisions
	RUN_TEST(test_ClipVelocity_45Degrees_IntoWall_Overbounce1_0);
	RUN_TEST(test_ClipVelocity_45Degrees_IntoWall_Overbounce1_001);
	RUN_TEST(test_ClipVelocity_45Degrees_IntoFloor_Overbounce1_0);
	RUN_TEST(test_ClipVelocity_45Degrees_IntoFloor_Overbounce1_001);
	RUN_TEST(test_ClipVelocity_45Degrees_DiagonalWall_Overbounce1_0);
	RUN_TEST(test_ClipVelocity_45Degrees_DiagonalWall_Overbounce1_001);

	// Parallel to Surface
	RUN_TEST(test_ClipVelocity_ParallelToWall_NoChange);
	RUN_TEST(test_ClipVelocity_ParallelToFloor_NoChange);

	// Grazing Angles
	RUN_TEST(test_ClipVelocity_GrazingAngle_10Degrees);
	RUN_TEST(test_ClipVelocity_GrazingAngle_80Degrees);

	// Velocity Conservation
	RUN_TEST(test_ClipVelocity_EnergyLoss_Overbounce1_0);
	RUN_TEST(test_ClipVelocity_EnergyIncrease_Overbounce1_001);

	// Angled Surfaces
	RUN_TEST(test_ClipVelocity_30DegreeSlope_Downward);
	RUN_TEST(test_ClipVelocity_45DegreeSlope_Forward);

	// Edge Cases
	RUN_TEST(test_ClipVelocity_ZeroVelocity);
	RUN_TEST(test_ClipVelocity_SmallVelocity);
	RUN_TEST(test_ClipVelocity_HighSpeed);
	RUN_TEST(test_ClipVelocity_Overbounce_Zero);
	RUN_TEST(test_ClipVelocity_Overbounce_High);

	// Multiple Axis
	RUN_TEST(test_ClipVelocity_3DMovement_FloorCollision);
	RUN_TEST(test_ClipVelocity_3DMovement_CornerCollision);

	// Verify Parallel Output
	RUN_TEST(test_ClipVelocity_OutputAlwaysParallel_RandomAngles);
	RUN_TEST(test_ClipVelocity_OutputAlwaysParallel_VariousNormals);

	return UNITY_END();
}
