/*
 * test_sound_spatialization.c - Unit tests for sound spatialization
 * Tests converting 3D positions to stereo pan/volume
 */

#include "../unity/unity.h"
#include <math.h>
#include <string.h>

// Vector math helpers
typedef float vec3_t[3];
#define Vec3Copy(a,b)		((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2])
#define Vec3Subtract(a,b,c)	((c)[0]=(a)[0]-(b)[0],(c)[1]=(a)[1]-(b)[1],(c)[2]=(a)[2]-(b)[2])
#define DotProduct(x,y)		((x)[0]*(y)[0]+(x)[1]*(y)[1]+(x)[2]*(y)[2])

// Constants
#define SOUND_FULLVOLUME	80.0f

// Mock implementation of Q_rint (round to nearest integer)
static int Q_rint(float x) {
	return (int)(x + 0.5f);
}

// Mock implementation of VectorNormalizef
static float VectorNormalizef(vec3_t in, vec3_t out) {
	float length = sqrtf(in[0]*in[0] + in[1]*in[1] + in[2]*in[2]);
	
	if (length == 0.0f) {
		Vec3Copy(in, out);
		return 0.0f;
	}
	
	float ilength = 1.0f / length;
	out[0] = in[0] * ilength;
	out[1] = in[1] * ilength;
	out[2] = in[2] * ilength;
	
	return length;
}

// Mock implementation of sound spatialization
// Converts 3D position to stereo left/right volume
static void Sound_SpatializeOrigin(vec3_t origin, vec3_t listenerPos, vec3_t listenerRight, 
                                    float masterVol, float distMult, int *leftVol, int *rightVol) {
	float		dot, dist;
	float		leftScale, rightScale, scale;
	vec3_t		sourceVec;

	// Calculate stereo separation and distance attenuation
	Vec3Subtract(origin, listenerPos, sourceVec);

	dist = VectorNormalizef(sourceVec, sourceVec) - SOUND_FULLVOLUME;
	if (dist < 0)
		dist = 0;			// close enough to be at full volume
	dist *= distMult;		// different attenuation levels
	
	dot = DotProduct(listenerRight, sourceVec);

	if (!distMult) {
		// No attenuation = no spatialization
		rightScale = 1.0f;
		leftScale = 1.0f;
	}
	else {
		rightScale = 0.5f * (1.0f + dot);
		leftScale = 0.5f * (1.0f - dot);
	}

	// Add in distance effect
	scale = (1.0f - dist) * rightScale;
	*rightVol = Q_rint(masterVol * scale);
	if (*rightVol < 0)
		*rightVol = 0;

	scale = (1.0f - dist) * leftScale;
	*leftVol = Q_rint(masterVol * scale);
	if (*leftVol < 0)
		*leftVol = 0;
}

// ============================================================================
// TESTS: Basic Spatialization
// ============================================================================

// Test: sound directly in front (no pan)
void test_Sound_DirectlyInFront(void) {
	vec3_t origin = {100.0f, 0.0f, 0.0f};
	vec3_t listenerPos = {0.0f, 0.0f, 0.0f};
	vec3_t listenerRight = {0.0f, 1.0f, 0.0f};
	int leftVol = 0, rightVol = 0;
	
	Sound_SpatializeOrigin(origin, listenerPos, listenerRight, 255.0f, 0.002f, &leftVol, &rightVol);
	
	// Sound in front should be equal in both ears
	TEST_ASSERT_EQUAL(leftVol, rightVol);
	TEST_ASSERT_GREATER_THAN(0, leftVol);
}

// Test: sound directly to the right
void test_Sound_DirectlyRight(void) {
	vec3_t origin = {0.0f, 100.0f, 0.0f};
	vec3_t listenerPos = {0.0f, 0.0f, 0.0f};
	vec3_t listenerRight = {0.0f, 1.0f, 0.0f};
	int leftVol = 0, rightVol = 0;
	
	Sound_SpatializeOrigin(origin, listenerPos, listenerRight, 255.0f, 0.002f, &leftVol, &rightVol);
	
	// Sound to the right should be louder in right ear
	TEST_ASSERT_GREATER_THAN(leftVol, rightVol);
}

// Test: sound directly to the left
void test_Sound_DirectlyLeft(void) {
	vec3_t origin = {0.0f, -100.0f, 0.0f};
	vec3_t listenerPos = {0.0f, 0.0f, 0.0f};
	vec3_t listenerRight = {0.0f, 1.0f, 0.0f};
	int leftVol = 0, rightVol = 0;
	
	Sound_SpatializeOrigin(origin, listenerPos, listenerRight, 255.0f, 0.002f, &leftVol, &rightVol);
	
	// Sound to the left should be louder in left ear
	TEST_ASSERT_GREATER_THAN(rightVol, leftVol);
}

// Test: sound directly behind (no pan)
void test_Sound_DirectlyBehind(void) {
	vec3_t origin = {-100.0f, 0.0f, 0.0f};
	vec3_t listenerPos = {0.0f, 0.0f, 0.0f};
	vec3_t listenerRight = {0.0f, 1.0f, 0.0f};
	int leftVol = 0, rightVol = 0;
	
	Sound_SpatializeOrigin(origin, listenerPos, listenerRight, 255.0f, 0.002f, &leftVol, &rightVol);
	
	// Sound behind should be equal in both ears (like front)
	TEST_ASSERT_EQUAL(leftVol, rightVol);
}

// Test: sound at 45 degrees to the right
void test_Sound_45DegreesRight(void) {
	vec3_t origin = {100.0f, 100.0f, 0.0f};
	vec3_t listenerPos = {0.0f, 0.0f, 0.0f};
	vec3_t listenerRight = {0.0f, 1.0f, 0.0f};
	int leftVol = 0, rightVol = 0;
	
	Sound_SpatializeOrigin(origin, listenerPos, listenerRight, 255.0f, 0.002f, &leftVol, &rightVol);
	
	// Sound at 45 degrees should favor right ear
	TEST_ASSERT_GREATER_THAN(leftVol, rightVol);
}

// Test: sound at 45 degrees to the left
void test_Sound_45DegreesLeft(void) {
	vec3_t origin = {100.0f, -100.0f, 0.0f};
	vec3_t listenerPos = {0.0f, 0.0f, 0.0f};
	vec3_t listenerRight = {0.0f, 1.0f, 0.0f};
	int leftVol = 0, rightVol = 0;
	
	Sound_SpatializeOrigin(origin, listenerPos, listenerRight, 255.0f, 0.002f, &leftVol, &rightVol);
	
	// Sound at 45 degrees should favor left ear
	TEST_ASSERT_GREATER_THAN(rightVol, leftVol);
}

// ============================================================================
// TESTS: Distance Attenuation
// ============================================================================

// Test: close sound (within SOUND_FULLVOLUME) at full volume
void test_Sound_CloseFullVolume(void) {
	vec3_t origin = {50.0f, 0.0f, 0.0f}; // Within 80 units
	vec3_t listenerPos = {0.0f, 0.0f, 0.0f};
	vec3_t listenerRight = {0.0f, 1.0f, 0.0f};
	int leftVol = 0, rightVol = 0;
	
	Sound_SpatializeOrigin(origin, listenerPos, listenerRight, 255.0f, 0.002f, &leftVol, &rightVol);
	
	// Sound within SOUND_FULLVOLUME should be at master volume (no distance attenuation)
	TEST_ASSERT_GREATER_THAN(100, leftVol);
	TEST_ASSERT_GREATER_THAN(100, rightVol);
}

// Test: far sound has reduced volume
void test_Sound_FarReducedVolume(void) {
	vec3_t origin = {500.0f, 0.0f, 0.0f}; // Far away
	vec3_t listenerPos = {0.0f, 0.0f, 0.0f};
	vec3_t listenerRight = {0.0f, 1.0f, 0.0f};
	int leftVol = 0, rightVol = 0;
	
	Sound_SpatializeOrigin(origin, listenerPos, listenerRight, 255.0f, 0.002f, &leftVol, &rightVol);
	
	// Sound should be attenuated
	TEST_ASSERT_LESS_THAN(200, leftVol);
	TEST_ASSERT_LESS_THAN(200, rightVol);
}

// Test: very far sound is silent
void test_Sound_VeryFarSilent(void) {
	vec3_t origin = {2000.0f, 0.0f, 0.0f}; // Very far
	vec3_t listenerPos = {0.0f, 0.0f, 0.0f};
	vec3_t listenerRight = {0.0f, 1.0f, 0.0f};
	int leftVol = 0, rightVol = 0;
	
	Sound_SpatializeOrigin(origin, listenerPos, listenerRight, 255.0f, 0.002f, &leftVol, &rightVol);
	
	// Sound should be silent or near-silent
	TEST_ASSERT_LESS_THAN(10, leftVol);
	TEST_ASSERT_LESS_THAN(10, rightVol);
}

// Test: same position as listener is full volume
void test_Sound_SamePosition(void) {
	vec3_t origin = {0.0f, 0.0f, 0.0f};
	vec3_t listenerPos = {0.0f, 0.0f, 0.0f};
	vec3_t listenerRight = {0.0f, 1.0f, 0.0f};
	int leftVol = 0, rightVol = 0;
	
	Sound_SpatializeOrigin(origin, listenerPos, listenerRight, 255.0f, 0.002f, &leftVol, &rightVol);
	
	// Sound at same position should be equal in both ears and audible
	TEST_ASSERT_EQUAL(leftVol, rightVol);
	TEST_ASSERT_GREATER_THAN(100, leftVol);
}

// ============================================================================
// TESTS: Distance Multipliers
// ============================================================================

// Test: higher distance multiplier attenuates more
void test_Sound_HighDistMult(void) {
	vec3_t origin = {200.0f, 0.0f, 0.0f};
	vec3_t listenerPos = {0.0f, 0.0f, 0.0f};
	vec3_t listenerRight = {0.0f, 1.0f, 0.0f};
	int leftVol1 = 0, rightVol1 = 0;
	int leftVol2 = 0, rightVol2 = 0;
	
	Sound_SpatializeOrigin(origin, listenerPos, listenerRight, 255.0f, 0.002f, &leftVol1, &rightVol1);
	Sound_SpatializeOrigin(origin, listenerPos, listenerRight, 255.0f, 0.004f, &leftVol2, &rightVol2);
	
	// Higher distMult should result in lower volume
	TEST_ASSERT_LESS_THAN(leftVol1, leftVol2);
	TEST_ASSERT_LESS_THAN(rightVol1, rightVol2);
}

// Test: zero distance multiplier (no attenuation)
void test_Sound_ZeroDistMult(void) {
	vec3_t origin = {500.0f, 0.0f, 0.0f};
	vec3_t listenerPos = {0.0f, 0.0f, 0.0f};
	vec3_t listenerRight = {0.0f, 1.0f, 0.0f};
	int leftVol = 0, rightVol = 0;
	
	Sound_SpatializeOrigin(origin, listenerPos, listenerRight, 255.0f, 0.0f, &leftVol, &rightVol);
	
	// Zero distMult means no attenuation - full volume
	TEST_ASSERT_EQUAL(255, leftVol);
	TEST_ASSERT_EQUAL(255, rightVol);
}

// Test: lower distance multiplier attenuates less
void test_Sound_LowDistMult(void) {
	vec3_t origin = {200.0f, 0.0f, 0.0f};
	vec3_t listenerPos = {0.0f, 0.0f, 0.0f};
	vec3_t listenerRight = {0.0f, 1.0f, 0.0f};
	int leftVol1 = 0, rightVol1 = 0;
	int leftVol2 = 0, rightVol2 = 0;
	
	Sound_SpatializeOrigin(origin, listenerPos, listenerRight, 255.0f, 0.001f, &leftVol1, &rightVol1);
	Sound_SpatializeOrigin(origin, listenerPos, listenerRight, 255.0f, 0.002f, &leftVol2, &rightVol2);
	
	// Lower distMult should result in higher volume
	TEST_ASSERT_GREATER_THAN(leftVol2, leftVol1);
	TEST_ASSERT_GREATER_THAN(rightVol2, rightVol1);
}

// ============================================================================
// TESTS: Master Volume
// ============================================================================

// Test: master volume affects output
void test_Sound_MasterVolume(void) {
	vec3_t origin = {100.0f, 0.0f, 0.0f};
	vec3_t listenerPos = {0.0f, 0.0f, 0.0f};
	vec3_t listenerRight = {0.0f, 1.0f, 0.0f};
	int leftVol1 = 0, rightVol1 = 0;
	int leftVol2 = 0, rightVol2 = 0;
	
	Sound_SpatializeOrigin(origin, listenerPos, listenerRight, 127.0f, 0.002f, &leftVol1, &rightVol1);
	Sound_SpatializeOrigin(origin, listenerPos, listenerRight, 255.0f, 0.002f, &leftVol2, &rightVol2);
	
	// Higher master volume should produce higher output
	TEST_ASSERT_LESS_THAN(leftVol2, leftVol1);
	TEST_ASSERT_LESS_THAN(rightVol2, rightVol1);
}

// Test: zero master volume produces silence
void test_Sound_ZeroMasterVolume(void) {
	vec3_t origin = {100.0f, 0.0f, 0.0f};
	vec3_t listenerPos = {0.0f, 0.0f, 0.0f};
	vec3_t listenerRight = {0.0f, 1.0f, 0.0f};
	int leftVol = 0, rightVol = 0;
	
	Sound_SpatializeOrigin(origin, listenerPos, listenerRight, 0.0f, 0.002f, &leftVol, &rightVol);
	
	TEST_ASSERT_EQUAL(0, leftVol);
	TEST_ASSERT_EQUAL(0, rightVol);
}

// ============================================================================
// TESTS: 3D Positioning
// ============================================================================

// Test: sound above listener
void test_Sound_Above(void) {
	vec3_t origin = {100.0f, 0.0f, 100.0f};
	vec3_t listenerPos = {0.0f, 0.0f, 0.0f};
	vec3_t listenerRight = {0.0f, 1.0f, 0.0f};
	int leftVol = 0, rightVol = 0;
	
	Sound_SpatializeOrigin(origin, listenerPos, listenerRight, 255.0f, 0.002f, &leftVol, &rightVol);
	
	// Sound above (Z axis) should be centered
	TEST_ASSERT_EQUAL(leftVol, rightVol);
}

// Test: sound below listener
void test_Sound_Below(void) {
	vec3_t origin = {100.0f, 0.0f, -100.0f};
	vec3_t listenerPos = {0.0f, 0.0f, 0.0f};
	vec3_t listenerRight = {0.0f, 1.0f, 0.0f};
	int leftVol = 0, rightVol = 0;
	
	Sound_SpatializeOrigin(origin, listenerPos, listenerRight, 255.0f, 0.002f, &leftVol, &rightVol);
	
	// Sound below (Z axis) should be centered
	TEST_ASSERT_EQUAL(leftVol, rightVol);
}

// Test: diagonal position combines distance and pan
void test_Sound_Diagonal(void) {
	vec3_t origin = {100.0f, 100.0f, 100.0f};
	vec3_t listenerPos = {0.0f, 0.0f, 0.0f};
	vec3_t listenerRight = {0.0f, 1.0f, 0.0f};
	int leftVol = 0, rightVol = 0;
	
	Sound_SpatializeOrigin(origin, listenerPos, listenerRight, 255.0f, 0.002f, &leftVol, &rightVol);
	
	// Diagonal should favor right ear and have some attenuation
	TEST_ASSERT_GREATER_THAN(leftVol, rightVol);
	TEST_ASSERT_GREATER_THAN(0, leftVol);
	TEST_ASSERT_GREATER_THAN(0, rightVol);
}

// ============================================================================
// TESTS: Listener Orientation
// ============================================================================

// Test: rotated listener (facing right)
void test_Sound_RotatedListener(void) {
	vec3_t origin = {100.0f, 0.0f, 0.0f};
	vec3_t listenerPos = {0.0f, 0.0f, 0.0f};
	vec3_t listenerRight = {1.0f, 0.0f, 0.0f}; // Rotated 90 degrees
	int leftVol = 0, rightVol = 0;
	
	Sound_SpatializeOrigin(origin, listenerPos, listenerRight, 255.0f, 0.002f, &leftVol, &rightVol);
	
	// With listener facing right, sound in front is now to the right
	TEST_ASSERT_GREATER_THAN(leftVol, rightVol);
}

// Test: listener moved to sound position
void test_Sound_ListenerMoved(void) {
	vec3_t origin = {100.0f, 100.0f, 0.0f};
	vec3_t listenerPos = {100.0f, 100.0f, 0.0f};
	vec3_t listenerRight = {0.0f, 1.0f, 0.0f};
	int leftVol = 0, rightVol = 0;
	
	Sound_SpatializeOrigin(origin, listenerPos, listenerRight, 255.0f, 0.002f, &leftVol, &rightVol);
	
	// Listener at sound position should be audible and equal
	TEST_ASSERT_EQUAL(leftVol, rightVol);
	TEST_ASSERT_GREATER_THAN(100, leftVol);
}

// ============================================================================
// TESTS: Edge Cases
// ============================================================================

// Test: negative volumes are clamped to zero
void test_Sound_NegativeVolumeClamped(void) {
	vec3_t origin = {5000.0f, 0.0f, 0.0f}; // Extremely far
	vec3_t listenerPos = {0.0f, 0.0f, 0.0f};
	vec3_t listenerRight = {0.0f, 1.0f, 0.0f};
	int leftVol = 0, rightVol = 0;
	
	Sound_SpatializeOrigin(origin, listenerPos, listenerRight, 255.0f, 0.01f, &leftVol, &rightVol);
	
	// Even with extreme distance, volumes should not be negative
	TEST_ASSERT_TRUE(leftVol >= 0);
	TEST_ASSERT_TRUE(rightVol >= 0);
}

// Test: symmetry (left and right are symmetric)
void test_Sound_Symmetry(void) {
	vec3_t originRight = {100.0f, 100.0f, 0.0f};
	vec3_t originLeft = {100.0f, -100.0f, 0.0f};
	vec3_t listenerPos = {0.0f, 0.0f, 0.0f};
	vec3_t listenerRight = {0.0f, 1.0f, 0.0f};
	int leftVolR = 0, rightVolR = 0;
	int leftVolL = 0, rightVolL = 0;
	
	Sound_SpatializeOrigin(originRight, listenerPos, listenerRight, 255.0f, 0.002f, &leftVolR, &rightVolR);
	Sound_SpatializeOrigin(originLeft, listenerPos, listenerRight, 255.0f, 0.002f, &leftVolL, &rightVolL);
	
	// Left and right should be symmetric
	TEST_ASSERT_EQUAL(leftVolR, rightVolL);
	TEST_ASSERT_EQUAL(rightVolR, leftVolL);
}

// Test: pan range is bounded (0-255)
void test_Sound_PanBounded(void) {
	vec3_t origin = {100.0f, 1000.0f, 0.0f}; // Far to the right
	vec3_t listenerPos = {0.0f, 0.0f, 0.0f};
	vec3_t listenerRight = {0.0f, 1.0f, 0.0f};
	int leftVol = 0, rightVol = 0;
	
	Sound_SpatializeOrigin(origin, listenerPos, listenerRight, 255.0f, 0.002f, &leftVol, &rightVol);
	
	// Volumes should be within valid range
	TEST_ASSERT_TRUE(leftVol <= 255);
	TEST_ASSERT_TRUE(rightVol <= 255);
	TEST_ASSERT_TRUE(leftVol >= 0);
	TEST_ASSERT_TRUE(rightVol >= 0);
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main(void) {
	UNITY_BEGIN();

	// Basic spatialization tests
	RUN_TEST(test_Sound_DirectlyInFront);
	RUN_TEST(test_Sound_DirectlyRight);
	RUN_TEST(test_Sound_DirectlyLeft);
	RUN_TEST(test_Sound_DirectlyBehind);
	RUN_TEST(test_Sound_45DegreesRight);
	RUN_TEST(test_Sound_45DegreesLeft);

	// Distance attenuation tests
	RUN_TEST(test_Sound_CloseFullVolume);
	RUN_TEST(test_Sound_FarReducedVolume);
	RUN_TEST(test_Sound_VeryFarSilent);
	RUN_TEST(test_Sound_SamePosition);

	// Distance multiplier tests
	RUN_TEST(test_Sound_HighDistMult);
	RUN_TEST(test_Sound_ZeroDistMult);
	RUN_TEST(test_Sound_LowDistMult);

	// Master volume tests
	RUN_TEST(test_Sound_MasterVolume);
	RUN_TEST(test_Sound_ZeroMasterVolume);

	// 3D positioning tests
	RUN_TEST(test_Sound_Above);
	RUN_TEST(test_Sound_Below);
	RUN_TEST(test_Sound_Diagonal);

	// Listener orientation tests
	RUN_TEST(test_Sound_RotatedListener);
	RUN_TEST(test_Sound_ListenerMoved);

	// Edge case tests
	RUN_TEST(test_Sound_NegativeVolumeClamped);
	RUN_TEST(test_Sound_Symmetry);
	RUN_TEST(test_Sound_PanBounded);

	return UNITY_END();
}
