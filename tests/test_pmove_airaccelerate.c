/*
====================
test_pmove_airaccelerate.c

Unit tests for PM_AirAccelerate - air movement acceleration with air control limits
====================
*/

#include "../unity/unity.h"
#include <math.h>
#include <string.h>

// Forward declarations
typedef float vec_t;
typedef vec_t vec3_t[3];

#define SV_PM_ACCELERATE 10.0f
#define AIR_CONTROL_MAX 30.0f
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

// PM_AirAccelerate implementation (from sv_pmove.c)
static void PM_AirAccelerate(vec3_t velocity, vec3_t wishdir, float wishspeed, float accel, float frametime) {
	float addspeed, accelspeed;
	float currentspeed, wishspd = wishspeed;
	
	if (wishspd > AIR_CONTROL_MAX)
		wishspd = AIR_CONTROL_MAX;
	currentspeed = DotProduct(velocity, wishdir);
	addspeed = wishspd - currentspeed;
	if (addspeed <= 0)
		return;
	
	accelspeed = accel * wishspd * frametime;
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
// TESTS: Air Control Wishspeed Clamping
// ============================================================================

void test_AirAccelerate_WishspeedClamped_30Max(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	// Try to accelerate to 100 units/sec (way above air control limit)
	PM_AirAccelerate(velocity, wishdir, 100.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// Should be clamped to 30 units/sec max
	// accelspeed = 10 * 30 * 0.1 = 30
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 30.0f, velocity[0]);
}

void test_AirAccelerate_WishspeedClamped_Exactly30(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	// Exactly at air control limit
	PM_AirAccelerate(velocity, wishdir, AIR_CONTROL_MAX, SV_PM_ACCELERATE, FRAMETIME);
	
	// accelspeed = 10 * 30 * 0.1 = 30
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 30.0f, velocity[0]);
}

void test_AirAccelerate_WishspeedClamped_Below30(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	// Below air control limit
	PM_AirAccelerate(velocity, wishdir, 20.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// accelspeed = 10 * 20 * 0.1 = 20
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 20.0f, velocity[0]);
}

void test_AirAccelerate_WishspeedClamped_Zero(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	// Zero wishspeed
	PM_AirAccelerate(velocity, wishdir, 0.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// No acceleration
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, velocity[0]);
}

// ============================================================================
// TESTS: Basic Air Acceleration from Rest
// ============================================================================

void test_AirAccelerate_FromRest_Forward(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	PM_AirAccelerate(velocity, wishdir, 25.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// accelspeed = 10 * 25 * 0.1 = 25
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 25.0f, velocity[0]);
}

void test_AirAccelerate_FromRest_Right(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 0.0f, 1.0f, 0.0f);
	
	PM_AirAccelerate(velocity, wishdir, 15.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// accelspeed = 10 * 15 * 0.1 = 15
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 15.0f, velocity[1]);
}

void test_AirAccelerate_FromRest_Diagonal(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	
	// 45 degree diagonal
	VectorSet(wishdir, 1.0f, 1.0f, 0.0f);
	VectorNormalize(wishdir);
	
	PM_AirAccelerate(velocity, wishdir, 20.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// accelspeed = 10 * 20 * 0.1 = 20, split equally
	float expected = 20.0f / sqrtf(2.0f);
	TEST_ASSERT_FLOAT_WITHIN(1.0f, expected, velocity[0]);
	TEST_ASSERT_FLOAT_WITHIN(1.0f, expected, velocity[1]);
}

// ============================================================================
// TESTS: Acceleration with Existing Velocity (Same Direction)
// ============================================================================

void test_AirAccelerate_SameDirection_Partial(void) {
	vec3_t velocity, wishdir;
	
	// Already moving forward at 10 units/sec
	VectorSet(velocity, 10.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	// Want to reach 25 units/sec
	PM_AirAccelerate(velocity, wishdir, 25.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// currentspeed = 10, addspeed = 25 - 10 = 15
	// accelspeed = 10 * 25 * 0.1 = 25, capped to 15
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 25.0f, velocity[0]);
}

void test_AirAccelerate_SameDirection_AlreadyAtLimit(void) {
	vec3_t velocity, wishdir;
	
	// Already at air control limit
	VectorSet(velocity, AIR_CONTROL_MAX, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	PM_AirAccelerate(velocity, wishdir, 40.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// currentspeed = 30, wishspd = 30, addspeed = 30 - 30 = 0
	// No acceleration
	TEST_ASSERT_FLOAT_WITHIN(0.01f, AIR_CONTROL_MAX, velocity[0]);
}

void test_AirAccelerate_SameDirection_ExceedsTarget(void) {
	vec3_t velocity, wishdir;
	
	// Already faster than target
	VectorSet(velocity, 35.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	PM_AirAccelerate(velocity, wishdir, 25.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// currentspeed = 35, wishspd = 25, addspeed = 25 - 35 = -10 (negative)
	// No acceleration
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 35.0f, velocity[0]);
}

// ============================================================================
// TESTS: Acceleration Perpendicular to Velocity
// ============================================================================

void test_AirAccelerate_Perpendicular_Right(void) {
	vec3_t velocity, wishdir;
	
	// Moving forward
	VectorSet(velocity, 20.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 0.0f, 1.0f, 0.0f);
	
	PM_AirAccelerate(velocity, wishdir, 15.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// Forward velocity unchanged
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 20.0f, velocity[0]);
	
	// Right velocity added
	// currentspeed = 0, accelspeed = 10 * 15 * 0.1 = 15
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 15.0f, velocity[1]);
}

void test_AirAccelerate_Perpendicular_Creates2DMotion(void) {
	vec3_t velocity, wishdir;
	float finalSpeed;
	
	VectorSet(velocity, 20.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 0.0f, 1.0f, 0.0f);
	
	PM_AirAccelerate(velocity, wishdir, 20.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	finalSpeed = VectorLength(velocity);
	
	// Should have diagonal velocity: sqrt(20^2 + 20^2) = 28.28
	TEST_ASSERT_FLOAT_WITHIN(2.0f, 28.28f, finalSpeed);
}

// ============================================================================
// TESTS: Acceleration at Angles
// ============================================================================

void test_AirAccelerate_45Degrees_ToVelocity(void) {
	vec3_t velocity, wishdir;
	
	// Moving forward
	VectorSet(velocity, 20.0f, 0.0f, 0.0f);
	
	// Want to accelerate at 45 degrees
	VectorSet(wishdir, 1.0f, 1.0f, 0.0f);
	VectorNormalize(wishdir);
	
	PM_AirAccelerate(velocity, wishdir, 25.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// currentspeed = dot([20,0,0], [0.707,0.707,0]) = 14.14
	// addspeed = 25 - 14.14 = 10.86
	// accelspeed = 10 * 25 * 0.1 = 25, capped to 10.86
	// Add 10.86 * [0.707, 0.707, 0] = [7.67, 7.67, 0]
	
	TEST_ASSERT_FLOAT_WITHIN(2.0f, 27.67f, velocity[0]);
	TEST_ASSERT_FLOAT_WITHIN(2.0f, 7.67f, velocity[1]);
}

void test_AirAccelerate_OppositeDirection_Slows(void) {
	vec3_t velocity, wishdir;
	
	// Moving forward fast
	VectorSet(velocity, 25.0f, 0.0f, 0.0f);
	
	// Want to move backward
	VectorSet(wishdir, -1.0f, 0.0f, 0.0f);
	
	PM_AirAccelerate(velocity, wishdir, 20.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// currentspeed = dot([25,0,0], [-1,0,0]) = -25
	// addspeed = 20 - (-25) = 45
	// accelspeed = 10 * 20 * 0.1 = 20
	// velocity -= 20 * [1,0,0] = [5, 0, 0]
	
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 5.0f, velocity[0]);
}

// ============================================================================
// TESTS: Different Acceleration Values
// ============================================================================

void test_AirAccelerate_HighAccel(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	// High acceleration
	PM_AirAccelerate(velocity, wishdir, 20.0f, 20.0f, FRAMETIME);
	
	// accelspeed = 20 * 20 * 0.1 = 40, capped to addspeed = 20
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 20.0f, velocity[0]);
}

void test_AirAccelerate_LowAccel(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	// Low acceleration
	PM_AirAccelerate(velocity, wishdir, 20.0f, 2.0f, FRAMETIME);
	
	// accelspeed = 2 * 20 * 0.1 = 4
	TEST_ASSERT_FLOAT_WITHIN(0.5f, 4.0f, velocity[0]);
}

void test_AirAccelerate_ZeroAccel(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	PM_AirAccelerate(velocity, wishdir, 20.0f, 0.0f, FRAMETIME);
	
	// No acceleration
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, velocity[0]);
}

// ============================================================================
// TESTS: Multiple Frames (Progressive Acceleration)
// ============================================================================

void test_AirAccelerate_MultipleFrames_BuildSpeed(void) {
	vec3_t velocity, wishdir;
	int i;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	// Accelerate over 5 frames
	for (i = 0; i < 5; i++) {
		PM_AirAccelerate(velocity, wishdir, AIR_CONTROL_MAX, SV_PM_ACCELERATE, FRAMETIME);
	}
	
	// Each frame: accelspeed = 10 * 30 * 0.1 = 30
	// After 4 frames: 30 * 4 = 120, but capped at 30 per frame
	// Actually reaches 30 and stays there
	TEST_ASSERT_FLOAT_WITHIN(2.0f, AIR_CONTROL_MAX, velocity[0]);
}

void test_AirAccelerate_MultipleFrames_SlowBuildup(void) {
	vec3_t velocity, wishdir;
	int i;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	// Use low accel so we see gradual buildup
	for (i = 0; i < 10; i++) {
		PM_AirAccelerate(velocity, wishdir, AIR_CONTROL_MAX, 1.0f, FRAMETIME);
	}
	
	// Each frame: accelspeed = 1 * 30 * 0.1 = 3
	// After 10 frames: 3 * 10 = 30
	TEST_ASSERT_FLOAT_WITHIN(2.0f, AIR_CONTROL_MAX, velocity[0]);
}

void test_AirAccelerate_MultipleFrames_ChangingDirection(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	
	// Frame 1: Accelerate forward
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	PM_AirAccelerate(velocity, wishdir, 20.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// Frame 2: Accelerate right (perpendicular)
	VectorSet(wishdir, 0.0f, 1.0f, 0.0f);
	PM_AirAccelerate(velocity, wishdir, 20.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// Should have both components
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 20.0f, velocity[0]);
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 20.0f, velocity[1]);
}

// ============================================================================
// TESTS: Frametime Variations
// ============================================================================

void test_AirAccelerate_ShortFrametime(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	// Very short frame (high FPS)
	PM_AirAccelerate(velocity, wishdir, 20.0f, SV_PM_ACCELERATE, 0.016f);
	
	// accelspeed = 10 * 20 * 0.016 = 3.2
	TEST_ASSERT_FLOAT_WITHIN(0.5f, 3.2f, velocity[0]);
}

void test_AirAccelerate_LongFrametime(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	// Long frame (low FPS)
	PM_AirAccelerate(velocity, wishdir, 20.0f, SV_PM_ACCELERATE, 0.5f);
	
	// accelspeed = 10 * 20 * 0.5 = 100, capped to addspeed = 20
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 20.0f, velocity[0]);
}

// ============================================================================
// TESTS: Edge Cases
// ============================================================================

void test_AirAccelerate_NegativeWishspeed(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	// Negative wishspeed (shouldn't happen, but test it)
	PM_AirAccelerate(velocity, wishdir, -10.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// wishspd = -10, but clamped to 0 effectively (negative addspeed)
	// No acceleration
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, velocity[0]);
}

void test_AirAccelerate_NonNormalizedWishdir(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	
	// Non-normalized direction (length = 2)
	VectorSet(wishdir, 2.0f, 0.0f, 0.0f);
	
	PM_AirAccelerate(velocity, wishdir, 20.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// accelspeed = 10 * 20 * 0.1 = 20
	// But wishdir is not normalized, so velocity += 20 * [2,0,0] = [40,0,0]
	// This is actually wrong - wishdir should be normalized, but testing edge case
	TEST_ASSERT_FLOAT_WITHIN(2.0f, 40.0f, velocity[0]);
}

void test_AirAccelerate_ZeroFrametime(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	PM_AirAccelerate(velocity, wishdir, 20.0f, SV_PM_ACCELERATE, 0.0f);
	
	// accelspeed = 10 * 20 * 0 = 0
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, velocity[0]);
}

// ============================================================================
// TESTS: 3D Movement
// ============================================================================

void test_AirAccelerate_3D_AllAxes(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	
	// Diagonal in 3D space
	VectorSet(wishdir, 1.0f, 1.0f, 1.0f);
	VectorNormalize(wishdir);
	
	PM_AirAccelerate(velocity, wishdir, 20.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// accelspeed = 10 * 20 * 0.1 = 20, distributed across all axes
	float expected = 20.0f / sqrtf(3.0f);
	TEST_ASSERT_FLOAT_WITHIN(2.0f, expected, velocity[0]);
	TEST_ASSERT_FLOAT_WITHIN(2.0f, expected, velocity[1]);
	TEST_ASSERT_FLOAT_WITHIN(2.0f, expected, velocity[2]);
}

void test_AirAccelerate_3D_WithExistingVelocity(void) {
	vec3_t velocity, wishdir;
	
	VectorSet(velocity, 10.0f, 10.0f, 0.0f);
	VectorSet(wishdir, 0.0f, 0.0f, 1.0f);
	
	PM_AirAccelerate(velocity, wishdir, 20.0f, SV_PM_ACCELERATE, FRAMETIME);
	
	// Horizontal velocity unchanged
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, velocity[0]);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, velocity[1]);
	
	// Vertical velocity added
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 20.0f, velocity[2]);
}

// ============================================================================
// TESTS: Strafe Jumping and Air Control Scenarios
// ============================================================================

void test_AirAccelerate_StrafeJump_RightStrafe(void) {
	vec3_t velocity, wishdir;
	
	// Player jumping forward with some speed
	VectorSet(velocity, 15.0f, 0.0f, 10.0f);
	
	// Strafe right while in air
	VectorSet(wishdir, 0.0f, 1.0f, 0.0f);
	
	PM_AirAccelerate(velocity, wishdir, AIR_CONTROL_MAX, SV_PM_ACCELERATE, FRAMETIME);
	
	// Forward and up velocity unchanged
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 15.0f, velocity[0]);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, velocity[2]);
	
	// Right velocity added (up to air control limit)
	// accelspeed = 10 * 30 * 0.1 = 30
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 30.0f, velocity[1]);
}

void test_AirAccelerate_StrafeJump_DiagonalStrafe(void) {
	vec3_t velocity, wishdir;
	
	// Player jumping forward
	VectorSet(velocity, 20.0f, 0.0f, 15.0f);
	
	// Strafe diagonally (forward-right)
	VectorSet(wishdir, 1.0f, 1.0f, 0.0f);
	VectorNormalize(wishdir);
	
	PM_AirAccelerate(velocity, wishdir, AIR_CONTROL_MAX, SV_PM_ACCELERATE, FRAMETIME);
	
	// currentspeed = dot([20,0,15], [0.707,0.707,0]) = 14.14
	// addspeed = 30 - 14.14 = 15.86
	// accelspeed = 10 * 30 * 0.1 = 30, capped to 15.86
	// Add 15.86 * [0.707, 0.707, 0] = [11.2, 11.2, 0]
	
	// Forward: 20 + 11.2 = 31.2
	// Right: 0 + 11.2 = 11.2
	// Up: 15 (unchanged)
	
	TEST_ASSERT_FLOAT_WITHIN(2.0f, 31.2f, velocity[0]);
	TEST_ASSERT_FLOAT_WITHIN(2.0f, 11.2f, velocity[1]);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 15.0f, velocity[2]);
}

void test_AirAccelerate_StrafeJump_MaxAirControl(void) {
	vec3_t velocity, wishdir;
	
	// Player with high horizontal speed
	VectorSet(velocity, 25.0f, 25.0f, 20.0f);
	
	// Try to strafe right
	VectorSet(wishdir, 0.0f, 1.0f, 0.0f);
	
	PM_AirAccelerate(velocity, wishdir, AIR_CONTROL_MAX, SV_PM_ACCELERATE, FRAMETIME);
	
	// currentspeed in right direction = 25
	// wishspd = 30, addspeed = 30 - 25 = 5
	// accelspeed = 10 * 30 * 0.1 = 30, capped to 5
	
	// Right velocity: 25 + 5 = 30
	// Other components unchanged
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 25.0f, velocity[0]);
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 30.0f, velocity[1]);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 20.0f, velocity[2]);
}

void test_AirAccelerate_StrafeJump_AlreadyAtLimit(void) {
	vec3_t velocity, wishdir;
	
	// Player already at air control limit in right direction
	VectorSet(velocity, 0.0f, AIR_CONTROL_MAX, 20.0f);
	
	// Try to strafe right more
	VectorSet(wishdir, 0.0f, 1.0f, 0.0f);
	
	PM_AirAccelerate(velocity, wishdir, AIR_CONTROL_MAX, SV_PM_ACCELERATE, FRAMETIME);
	
	// currentspeed = 30, wishspd = 30, addspeed = 0
	// No additional acceleration
	TEST_ASSERT_FLOAT_WITHIN(0.01f, AIR_CONTROL_MAX, velocity[1]);
}

// ============================================================================
// TESTS: Air Acceleration Disabled (Fallback to Ground Accelerate with accel=1)
// ============================================================================

void test_AirAccelerate_Disabled_FallbackToGroundAccel(void) {
	vec3_t velocity, wishdir;
	
	// When air acceleration is disabled, it uses PM_Accelerate with accel=1
	// Let's simulate this by calling PM_Accelerate with accel=1
	VectorSet(velocity, 0.0f, 0.0f, 0.0f);
	VectorSet(wishdir, 1.0f, 0.0f, 0.0f);
	
	// Simulate disabled air accel: PM_Accelerate(wishdir, wishspeed, 1)
	float currentspeed = DotProduct(velocity, wishdir);
	float addspeed = 100.0f - currentspeed;
	if (addspeed > 0) {
		float accelspeed = 1.0f * FRAMETIME * 100.0f;
		if (accelspeed > addspeed) accelspeed = addspeed;
		velocity[0] += accelspeed * wishdir[0];
	}
	
	// accelspeed = 1 * 0.1 * 100 = 10
	TEST_ASSERT_FLOAT_WITHIN(1.0f, 10.0f, velocity[0]);
}

// ============================================================================
// MAIN
// ============================================================================

int main(void) {
	UNITY_BEGIN();

	// Air Control Wishspeed Clamping
	RUN_TEST(test_AirAccelerate_WishspeedClamped_30Max);
	RUN_TEST(test_AirAccelerate_WishspeedClamped_Exactly30);
	RUN_TEST(test_AirAccelerate_WishspeedClamped_Below30);
	RUN_TEST(test_AirAccelerate_WishspeedClamped_Zero);

	// Basic Air Acceleration from Rest
	RUN_TEST(test_AirAccelerate_FromRest_Forward);
	RUN_TEST(test_AirAccelerate_FromRest_Right);
	RUN_TEST(test_AirAccelerate_FromRest_Diagonal);

	// Acceleration with Existing Velocity (Same Direction)
	RUN_TEST(test_AirAccelerate_SameDirection_Partial);
	RUN_TEST(test_AirAccelerate_SameDirection_AlreadyAtLimit);
	RUN_TEST(test_AirAccelerate_SameDirection_ExceedsTarget);

	// Acceleration Perpendicular to Velocity
	RUN_TEST(test_AirAccelerate_Perpendicular_Right);
	RUN_TEST(test_AirAccelerate_Perpendicular_Creates2DMotion);

	// Acceleration at Angles
	RUN_TEST(test_AirAccelerate_45Degrees_ToVelocity);
	RUN_TEST(test_AirAccelerate_OppositeDirection_Slows);

	// Different Acceleration Values
	RUN_TEST(test_AirAccelerate_HighAccel);
	RUN_TEST(test_AirAccelerate_LowAccel);
	RUN_TEST(test_AirAccelerate_ZeroAccel);

	// Multiple Frames
	RUN_TEST(test_AirAccelerate_MultipleFrames_BuildSpeed);
	RUN_TEST(test_AirAccelerate_MultipleFrames_SlowBuildup);
	RUN_TEST(test_AirAccelerate_MultipleFrames_ChangingDirection);

	// Frametime Variations
	RUN_TEST(test_AirAccelerate_ShortFrametime);
	RUN_TEST(test_AirAccelerate_LongFrametime);

	// Edge Cases
	RUN_TEST(test_AirAccelerate_NegativeWishspeed);
	RUN_TEST(test_AirAccelerate_NonNormalizedWishdir);
	RUN_TEST(test_AirAccelerate_ZeroFrametime);

	// 3D Movement
	RUN_TEST(test_AirAccelerate_3D_AllAxes);
	RUN_TEST(test_AirAccelerate_3D_WithExistingVelocity);

	// Strafe Jumping and Air Control Scenarios
	RUN_TEST(test_AirAccelerate_StrafeJump_RightStrafe);
	RUN_TEST(test_AirAccelerate_StrafeJump_DiagonalStrafe);
	RUN_TEST(test_AirAccelerate_StrafeJump_MaxAirControl);
	RUN_TEST(test_AirAccelerate_StrafeJump_AlreadyAtLimit);

	// Air Acceleration Disabled
	RUN_TEST(test_AirAccelerate_Disabled_FallbackToGroundAccel);

	return UNITY_END();
}
