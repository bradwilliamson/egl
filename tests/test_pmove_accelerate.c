/*
====================
test_pmove_accelerate.c

Unit tests for PM_Accelerate - velocity acceleration in wishdir direction
====================
*/

#include "../unity/unity.h"
#include <math.h>
#include <string.h>

// Forward declarations
typedef float vec_t;
typedef vec_t vec3_t[3];

#define SV_PM_MAXSPEED 300.0f
#define SV_PM_ACCELERATE 10.0f
#define FRAMETIME 0.1f  // 100ms frame (10 FPS for testing)

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

static void VectorSet(vec3_t v, float x, float y, float z) {
	v[0] = x;
	v[1] = y;
	v[2] = z;
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

static void VectorScale(const vec3_t in, float scale, vec3_t out) {
	out[0] = in[0] * scale;
	out[1] = in[1] * scale;
	out[2] = in[2] * scale;
}

// PM_Accelerate implementation (from sv_pmove.c)
static void PM_Accelerate(vec3_t velocity, vec3_t wishdir, float wishspeed, float accel, float frametime) {
	float currentspeed, addspeed, accelspeed;
	
	currentspeed = DotProduct(velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0)
		return;
	
	accelspeed = accel * frametime * wishspeed;
	if (accelspeed > addspeed)
		accelspeed = addspeed;
	
	velocity[0] += accelspeed * wishdir[0];
	velocity[1] += accelspeed * wishdir[1];
	velocity[2] += accelspeed * wishdir[2];
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

static int VectorsEqual(const vec3_t v1, const vec3_t v2, float epsilon) {
	return (fabsf(v1[0] - v2[0]) < epsilon &&
	        fabsf(v1[1] - v2[1]) < epsilon &&
	        fabsf(v1[2] - v2[2]) < epsilon);
}

// ============================================================================
// TESTS: Basic Acceleration from Rest
// ============================================================================

void test_Accelerate_FromRest_Forward(void) {
	vec3_t velocity, wishdir;
	
	// Starting from rest
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	
	// Wish to move forward
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	VectorNormalize(wishdir);
	
	PM_Accelerate(velocity, wishdir, 100.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// Should accelerate forward
	// accelspeed = 10 * 0.1 * 100 = 100
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 100.0f, velocity[0]);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, velocity[1]);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, velocity[2]);
}

void test_Accelerate_FromRest_Right(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 0.0f, 1.0f, 0.0f);
	
	PM_Accelerate(velocity, wishdir, 100.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, velocity[0]);
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 100.0f, velocity[1]);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, velocity[2]);
}

void test_Accelerate_FromRest_Diagonal(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	
	// 45 degree diagonal
	VectorSet(wishdir, 1.0f, 1.0f, 0.0f);
	VectorNormalize(wishdir);
	
	PM_Accelerate(velocity, wishdir, 100.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// Should accelerate diagonally
	// accelspeed = 100, split equally in X and Y
	float expected = 100.0f / sqrtf(2.0f);
	TEST_ASSERT_FLOAT_WITHIN(1.0f, expected, velocity[0]);
	TEST_ASSERT_FLOAT_WITHIN(1.0f, expected, velocity[1]);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, velocity[2]);
}

void test_Accelerate_FromRest_Vertical(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 0.0f, 0.0f, 1.0f);
	
	PM_Accelerate(velocity, wishdir, 100.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, velocity[0]);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, velocity[1]);
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 100.0f, velocity[2]);
}

// ============================================================================
// TESTS: Acceleration with Existing Velocity (Same Direction)
// ============================================================================

void test_Accelerate_SameDirection_Partial(void) {
	vec3_t velocity, wishdir;
	
	// Already moving forward at 50 units/sec
	VectorSet(velocity, 50.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	// Want to reach 200 units/sec
	PM_Accelerate(velocity, wishdir, 200.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// accelspeed = 10 * 0.1 * 200 = 200
	// addspeed = 200 - 50 = 150, so we add min(200, 150) = 150
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 200.0f, velocity[0]);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, velocity[1]);
}

void test_Accelerate_SameDirection_Capped(void) {
	vec3_t velocity, wishdir;
	
	// Already moving at 180 units/sec
	VectorSet(velocity, 180.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	// Want to reach 200 units/sec
	PM_Accelerate(velocity, wishdir, 200.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// accelspeed = 10 * 0.1 * 200 = 200
	// addspeed = 200 - 180 = 20, so we add min(200, 20) = 20
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 200.0f, velocity[0]);
}

void test_Accelerate_SameDirection_AlreadyFast(void) {
	vec3_t velocity, wishdir;
	
	// Already at target speed
	VectorSet(velocity, 200.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	PM_Accelerate(velocity, wishdir, 200.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// Should not change (addspeed <= 0)
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 200.0f, velocity[0]);
}

void test_Accelerate_SameDirection_ExceedsTarget(void) {
	vec3_t velocity, wishdir;
	
	// Already faster than target
	VectorSet(velocity, 250.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	PM_Accelerate(velocity, wishdir, 200.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// Should not change (addspeed <= 0)
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 250.0f, velocity[0]);
}

// ============================================================================
// TESTS: Acceleration Perpendicular to Velocity
// ============================================================================

void test_Accelerate_Perpendicular_Right(void) {
	vec3_t velocity, wishdir;
	float initialX;
	
	// Moving forward
	VectorSet(velocity, 100.0f, 0.0f, 0.0f);
	initialX = velocity[0];
	
	// Want to accelerate right
	VectorSet(wishdir, 0.0f, 1.0f, 0.0f);
	
	PM_Accelerate(velocity, wishdir, 100.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// Forward velocity unchanged (perpendicular)
	TEST_ASSERT_FLOAT_WITHIN(0.01f, initialX, velocity[0]);
	
	// Right velocity added
	// currentspeed in wishdir = 0, accelspeed = 10 * 0.1 * 100 = 100
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 100.0f, velocity[1]);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, velocity[2]);
}

void test_Accelerate_Perpendicular_Creates2DMotion(void) {
	vec3_t velocity, wishdir;
	float finalSpeed;
	
	VectorSet(velocity, 100.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 0.0f, 1.0f, 0.0f);
	
	PM_Accelerate(velocity, wishdir, 100.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// Should now have diagonal velocity
	finalSpeed = VectorLength(velocity);
	
	// Pythagorean: sqrt(100^2 + 100^2) = 141.42
	TEST_ASSERT_FLOAT_WITHIN(2.0f, 141.42f, finalSpeed);
}

// ============================================================================
// TESTS: Acceleration at Angles
// ============================================================================

void test_Accelerate_45Degrees_ToVelocity(void) {
	vec3_t velocity, wishdir;
	
	// Moving forward
	VectorSet(velocity, 100.0f, 0.0f, 0.0f);
	
	// Want to accelerate at 45 degrees
	VectorSet(wishdir, 1.0f, 1.0f, 0.0f);
	VectorNormalize(wishdir);
	
	PM_Accelerate(velocity, wishdir, 150.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// currentspeed = dot([100,0,0], [0.707,0.707,0]) = 70.7
	// addspeed = 150 - 70.7 = 79.3
	// accelspeed = 10 * 0.1 * 150 = 150, use min(150, 79.3) = 79.3
	// Add 79.3 * [0.707, 0.707, 0] = [56.1, 56.1, 0]
	
	TEST_ASSERT_FLOAT_WITHIN(2.0f, 156.0f, velocity[0]);
	TEST_ASSERT_FLOAT_WITHIN(2.0f, 56.0f, velocity[1]);
}

void test_Accelerate_OppositeDirection_Slows(void) {
	vec3_t velocity, wishdir;
	
	// Moving forward fast
	VectorSet(velocity, 200.0f, 0.0f, 0.0f);
	
	// Want to move backward
	VectorSet(wishdir, -1.0f, 0.0f, 0.0f);
	
	PM_Accelerate(velocity, wishdir, 100.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// currentspeed = dot([200,0,0], [-1,0,0]) = -200
	// addspeed = 100 - (-200) = 300
	// accelspeed = 10 * 0.1 * 100 = 100
	// velocity -= 100 * [1,0,0] = [100, 0, 0]
	
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 100.0f, velocity[0]);
}

// ============================================================================
// TESTS: Different Wishspeed Values
// ============================================================================

void test_Accelerate_LowWishspeed(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	// Low target speed
	PM_Accelerate(velocity, wishdir, 10.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// accelspeed = 10 * 0.1 * 10 = 10
	TEST_ASSERT_FLOAT_WITHIN(0.5f, 10.0f, velocity[0]);
}

void test_Accelerate_HighWishspeed(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	// High target speed
	PM_Accelerate(velocity, wishdir, 500.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// accelspeed = 10 * 0.1 * 500 = 500
	TEST_ASSERT_FLOAT_WITHIN(2.0f, 500.0f, velocity[0]);
}

void test_Accelerate_MaxspeedWishspeed(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	// Exactly at max speed
	PM_Accelerate(velocity, wishdir, SV_PM_MAXSPEED, SV_PM_ACCELERATE, FRAMETIME);
	
	// accelspeed = 10 * 0.1 * 300 = 300
	TEST_ASSERT_FLOAT_WITHIN(2.0f, 300.0f, velocity[0]);
}

// ============================================================================
// TESTS: Different Acceleration Values
// ============================================================================

void test_Accelerate_HighAccel(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	// High acceleration (e.g., water acceleration)
	PM_Accelerate(velocity, wishdir, 100.0f, 50.0f, FRAMETIME);
	
	// accelspeed = 50 * 0.1 * 100 = 500, capped at addspeed = 100
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 100.0f, velocity[0]);
}

void test_Accelerate_LowAccel(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	// Low acceleration (slow response)
	PM_Accelerate(velocity, wishdir, 100.0f, 1.0f, FRAMETIME);
	
	// accelspeed = 1 * 0.1 * 100 = 10
	TEST_ASSERT_FLOAT_WITHIN(0.5f, 10.0f, velocity[0]);
}

void test_Accelerate_ZeroAccel(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	PM_Accelerate(velocity, wishdir, 100.0f, 0.0f, FRAMETIME);
	
	// No acceleration
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, velocity[0]);
}

// ============================================================================
// TESTS: Multiple Frames (Progressive Acceleration)
// ============================================================================

void test_Accelerate_MultipleFrames_BuildSpeed(void) {
	vec3_t velocity, wishdir;
	int i;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	// Accelerate over 5 frames
	for (i = 0; i < 5; i++) {
		PM_Accelerate(velocity, wishdir, 300.0f, SV_PM_ACCELERATE, FRAMETIME);
	}
	
	// Each frame: accelspeed = 10 * 0.1 * 300 = 300
	// Frame 1: 0 + 300 = 300
	// Already at target, no more acceleration
	TEST_ASSERT_FLOAT_WITHIN(2.0f, 300.0f, velocity[0]);
}

void test_Accelerate_MultipleFrames_SlowBuildup(void) {
	vec3_t velocity, wishdir;
	int i;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	// Use low accel so we see gradual buildup
	for (i = 0; i < 10; i++) {
		PM_Accelerate(velocity, wishdir, 300.0f, 3.0f, FRAMETIME);
	}
	
	// Each frame: accelspeed = 3 * 0.1 * 300 = 90
	// After 4 frames: 90 * 4 = 360, but capped at 300
	TEST_ASSERT_FLOAT_WITHIN(2.0f, 300.0f, velocity[0]);
}

void test_Accelerate_MultipleFrames_ChangingDirection(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	
	// Frame 1: Accelerate forward
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	PM_Accelerate(velocity, wishdir, 100.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 100.0f, velocity[0]);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, velocity[1]);
	
	// Frame 2: Accelerate right (perpendicular)
	VectorSet(wishdir, 0.0f, 1.0f, 0.0f);
	PM_Accelerate(velocity, wishdir, 100.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// Should have both components
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 100.0f, velocity[0]);
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 100.0f, velocity[1]);
}

// ============================================================================
// TESTS: Maxspeed Clamping (External to PM_Accelerate)
// ============================================================================

void test_Accelerate_ExceedsMaxspeed_NotClamped(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	// PM_Accelerate itself doesn't clamp - that happens in calling code
	// Try to accelerate to 500 (above maxspeed)
	PM_Accelerate(velocity, wishdir, 500.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// accelspeed = 10 * 0.1 * 500 = 500
	TEST_ASSERT_FLOAT_WITHIN(2.0f, 500.0f, velocity[0]);
}

void test_Accelerate_PreClampedWishspeed(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	// In actual game code, wishspeed would be clamped to maxspeed before calling
	float clampedWishspeed = SV_PM_MAXSPEED;
	PM_Accelerate(velocity, wishdir, clampedWishspeed, SV_PM_ACCELERATE, FRAMETIME);
	
	TEST_ASSERT_FLOAT_WITHIN(2.0f, 300.0f, velocity[0]);
}

void test_Accelerate_Maxspeed_MultipleFrames(void) {
	vec3_t velocity, wishdir;
	int i;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	// Accelerate multiple frames with clamped wishspeed
	for (i = 0; i < 5; i++) {
		// Simulate external clamping
		float wishspeed = 500.0f;
		if (wishspeed > SV_PM_MAXSPEED)
			wishspeed = SV_PM_MAXSPEED;
		
		PM_Accelerate(velocity, wishdir, wishspeed, SV_PM_ACCELERATE, FRAMETIME);
	}
	
	// Should reach maxspeed and stop
	TEST_ASSERT_FLOAT_WITHIN(2.0f, SV_PM_MAXSPEED, velocity[0]);
	TEST_ASSERT_TRUE(velocity[0] <= SV_PM_MAXSPEED + 1.0f);
}

// ============================================================================
// TESTS: Frametime Variations
// ============================================================================

void test_Accelerate_ShortFrametime(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	// Very short frame (high FPS)
	PM_Accelerate(velocity, wishdir, 100.0f, SV_PM_ACCELERATE, 0.016f);
	
	// accelspeed = 10 * 0.016 * 100 = 16
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 16.0f, velocity[0]);
}

void test_Accelerate_LongFrametime(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	// Long frame (low FPS)
	PM_Accelerate(velocity, wishdir, 100.0f, SV_PM_ACCELERATE, 0.5f);
	
	// accelspeed = 10 * 0.5 * 100 = 500, capped at addspeed = 100
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 100.0f, velocity[0]);
}

// ============================================================================
// TESTS: Edge Cases
// ============================================================================

void test_Accelerate_ZeroWishspeed(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 100.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	PM_Accelerate(velocity, wishdir, 0.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// addspeed = 0 - 100 = -100 (negative), no acceleration
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, velocity[0]);
}

void test_Accelerate_NegativeWishspeed(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 100.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	// Negative wishspeed (shouldn't happen, but test it)
	PM_Accelerate(velocity, wishdir, -50.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// addspeed = -50 - 100 = -150 (negative), no acceleration
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, velocity[0]);
}

void test_Accelerate_NonNormalizedWishdir(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	
	// Non-normalized direction (length = 2)
	VectorSet(wishdir, 2.0f, 0.0f, 0.0f);
	
	PM_Accelerate(velocity, wishdir, 100.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// accelspeed = 10 * 0.1 * 100 = 100
	// But wishdir is not normalized, so velocity += 100 * [2,0,0] = [200,0,0]
	TEST_ASSERT_FLOAT_WITHIN(2.0f, 200.0f, velocity[0]);
}

void test_Accelerate_ZeroFrametime(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	PM_Accelerate(velocity, wishdir, 100.0f, SV_PM_ACCELERATE, 0.0f);
	
	// accelspeed = 10 * 0 * 100 = 0
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, velocity[0]);
}

// ============================================================================
// TESTS: 3D Movement
// ============================================================================

void test_Accelerate_3D_AllAxes(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	
	// Diagonal in 3D space
	VectorSet(wishdir, 1.0f, 1.0f, 1.0f);
	VectorNormalize(wishdir);
	
	PM_Accelerate(velocity, wishdir, 100.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// accelspeed = 100, distributed across all axes
	float expected = 100.0f / sqrtf(3.0f);
	TEST_ASSERT_FLOAT_WITHIN(2.0f, expected, velocity[0]);
	TEST_ASSERT_FLOAT_WITHIN(2.0f, expected, velocity[1]);
	TEST_ASSERT_FLOAT_WITHIN(2.0f, expected, velocity[2]);
}

void test_Accelerate_3D_WithExistingVelocity(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 50.0f, 50.0f, 0.0f);
	VectorSet(wishdir, 0.0f, 0.0f, 1.0f);
	
	PM_Accelerate(velocity, wishdir, 100.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// Perpendicular, so horizontal velocity unchanged
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 50.0f, velocity[0]);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 50.0f, velocity[1]);
	
	// Vertical velocity added
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 100.0f, velocity[2]);
}

// ============================================================================
// TESTS: Real-World Scenarios
// ============================================================================

void test_Accelerate_Scenario_Strafing(void) {
	vec3_t velocity, wishdir;
	
	// Player running forward
	VectorSet(velocity, 250.0f, 0.0f, 0.0f);
	
	// Player tries to strafe right while maintaining forward speed
	VectorSet(wishdir, 1.0f, 1.0f, 0.0f);
	VectorNormalize(wishdir);
	
	PM_Accelerate(velocity, wishdir, 300.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// Should add rightward component
	TEST_ASSERT_TRUE(velocity[1] > 0.0f);
	
	// Forward should increase slightly
	TEST_ASSERT_TRUE(velocity[0] > 250.0f);
}

void test_Accelerate_Scenario_QuickTurn(void) {
	vec3_t velocity, wishdir;
	
	// Moving right fast
	VectorSet(velocity, 0.0f, 200.0f, 0.0f);
	
	// Try to accelerate forward (perpendicular)
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	PM_Accelerate(velocity, wishdir, 200.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// Should gain forward velocity
	TEST_ASSERT_FLOAT_WITHIN(5.0f, 200.0f, velocity[0]);
	
	// Right velocity unchanged
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 200.0f, velocity[1]);
}

void test_Accelerate_Scenario_Swimming(void) {
	vec3_t velocity, wishdir;
	
	// Swimming up at angle
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	
	VectorSet(wishdir, 1.0f, 0.0f, 1.0f);
	VectorNormalize(wishdir);
	
	// Water acceleration is typically same as ground
	PM_Accelerate(velocity, wishdir, 100.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// Should have both forward and upward components
	float expected = 100.0f / sqrtf(2.0f);
	TEST_ASSERT_FLOAT_WITHIN(2.0f, expected, velocity[0]);
	TEST_ASSERT_FLOAT_WITHIN(2.0f, expected, velocity[2]);
}

// ============================================================================
// MAIN
// ============================================================================

int main(void) {
	UNITY_BEGIN();

	// Basic Acceleration from Rest
	RUN_TEST(test_Accelerate_FromRest_Forward);
	RUN_TEST(test_Accelerate_FromRest_Right);
	RUN_TEST(test_Accelerate_FromRest_Diagonal);
	RUN_TEST(test_Accelerate_FromRest_Vertical);

	// Acceleration with Existing Velocity (Same Direction)
	RUN_TEST(test_Accelerate_SameDirection_Partial);
	RUN_TEST(test_Accelerate_SameDirection_Capped);
	RUN_TEST(test_Accelerate_SameDirection_AlreadyFast);
	RUN_TEST(test_Accelerate_SameDirection_ExceedsTarget);

	// Acceleration Perpendicular to Velocity
	RUN_TEST(test_Accelerate_Perpendicular_Right);
	RUN_TEST(test_Accelerate_Perpendicular_Creates2DMotion);

	// Acceleration at Angles
	RUN_TEST(test_Accelerate_45Degrees_ToVelocity);
	RUN_TEST(test_Accelerate_OppositeDirection_Slows);

	// Different Wishspeed Values
	RUN_TEST(test_Accelerate_LowWishspeed);
	RUN_TEST(test_Accelerate_HighWishspeed);
	RUN_TEST(test_Accelerate_MaxspeedWishspeed);

	// Different Acceleration Values
	RUN_TEST(test_Accelerate_HighAccel);
	RUN_TEST(test_Accelerate_LowAccel);
	RUN_TEST(test_Accelerate_ZeroAccel);

	// Multiple Frames
	RUN_TEST(test_Accelerate_MultipleFrames_BuildSpeed);
	RUN_TEST(test_Accelerate_MultipleFrames_SlowBuildup);
	RUN_TEST(test_Accelerate_MultipleFrames_ChangingDirection);

	// Maxspeed Clamping
	RUN_TEST(test_Accelerate_ExceedsMaxspeed_NotClamped);
	RUN_TEST(test_Accelerate_PreClampedWishspeed);
	RUN_TEST(test_Accelerate_Maxspeed_MultipleFrames);

	// Frametime Variations
	RUN_TEST(test_Accelerate_ShortFrametime);
	RUN_TEST(test_Accelerate_LongFrametime);

	// Edge Cases
	RUN_TEST(test_Accelerate_ZeroWishspeed);
	RUN_TEST(test_Accelerate_NegativeWishspeed);
	RUN_TEST(test_Accelerate_NonNormalizedWishdir);
	RUN_TEST(test_Accelerate_ZeroFrametime);

	// 3D Movement
	RUN_TEST(test_Accelerate_3D_AllAxes);
	RUN_TEST(test_Accelerate_3D_WithExistingVelocity);

	// Real-World Scenarios
	RUN_TEST(test_Accelerate_Scenario_Strafing);
	RUN_TEST(test_Accelerate_Scenario_QuickTurn);
	RUN_TEST(test_Accelerate_Scenario_Swimming);

	return UNITY_END();
}
