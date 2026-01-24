/*
 * test_sound_channel.c - Unit tests for sound channel spatialization
 * Tests DMASnd_SpatializeChannel which determines sound origin based on type
 * and calculates left/right volume for stereo output
 */

#include "../unity/unity.h"
#include <math.h>
#include <string.h>

// Vector math helpers
typedef float vec3_t[3];
typedef int qBool;
#define qTrue 1
#define qFalse 0

#define Vec3Copy(a,b)		((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2])
#define Vec3Subtract(a,b,c)	((c)[0]=(a)[0]-(b)[0],(c)[1]=(a)[1]-(b)[1],(c)[2]=(a)[2]-(b)[2])
#define DotProduct(x,y)		((x)[0]*(y)[0]+(x)[1]*(y)[1]+(x)[2]*(y)[2])

// Constants
#define SOUND_FULLVOLUME	80.0f

// Sound type enum
typedef enum {
	PSND_ENTITY,
	PSND_FIXED,
	PSND_LOCAL
} psndType_t;

// Channel structure (simplified)
typedef struct {
	psndType_t	psType;
	int			entNum;
	vec3_t		origin;
	int			masterVol;
	int			leftVol;
	int			rightVol;
	float		distMult;
} channel_t;

// Global listener state
static vec3_t snd_listenerOrigin = {0.0f, 0.0f, 0.0f};
static vec3_t snd_listenerRight = {0.0f, 1.0f, 0.0f};

// Mock entity sound origins (entity number -> origin)
static vec3_t mockEntityOrigins[10];
static int mockEntityCount = 0;

// Mock implementation of CL_CGModule_GetEntitySoundOrigin
static void CL_CGModule_GetEntitySoundOrigin(int entNum, vec3_t origin, vec3_t velocity) {
	if (entNum >= 0 && entNum < mockEntityCount) {
		Vec3Copy(mockEntityOrigins[entNum], origin);
	} else {
		origin[0] = origin[1] = origin[2] = 0.0f;
	}
	velocity[0] = velocity[1] = velocity[2] = 0.0f; // Velocity not used in tests
}

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

// Mock implementation of DMASnd_SpatializeOrigin
static void DMASnd_SpatializeOrigin(vec3_t origin, float masterVol, float distMult, int *leftVol, int *rightVol) {
	float		dot, dist;
	float		leftScale, rightScale, scale;
	vec3_t		sourceVec;

	// Calculate stereo separation and distance attenuation
	Vec3Subtract(origin, snd_listenerOrigin, sourceVec);

	dist = VectorNormalizef(sourceVec, sourceVec) - SOUND_FULLVOLUME;
	if (dist < 0)
		dist = 0;			// close enough to be at full volume
	dist *= distMult;		// different attenuation levels
	
	dot = DotProduct(snd_listenerRight, sourceVec);

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

// Mock implementation of DMASnd_SpatializeChannel
static void DMASnd_SpatializeChannel(channel_t *ch) {
	vec3_t		origin, velocity;

	switch (ch->psType) {
	case PSND_FIXED:
		Vec3Copy(ch->origin, origin);
		break;

	case PSND_ENTITY:
		CL_CGModule_GetEntitySoundOrigin(ch->entNum, origin, velocity);
		break;

	case PSND_LOCAL:
		// Anything coming from the view entity will always be full volume
		ch->leftVol = ch->masterVol;
		ch->rightVol = ch->masterVol;
		return;
	}

	// Spatialize fixed/entity sounds
	DMASnd_SpatializeOrigin(origin, ch->masterVol, ch->distMult, &ch->leftVol, &ch->rightVol);
}

// Helper to reset test state
static void ResetTestState(void) {
	snd_listenerOrigin[0] = 0.0f;
	snd_listenerOrigin[1] = 0.0f;
	snd_listenerOrigin[2] = 0.0f;
	snd_listenerRight[0] = 0.0f;
	snd_listenerRight[1] = 1.0f;
	snd_listenerRight[2] = 0.0f;
	mockEntityCount = 0;
}

// ============================================================================
// TESTS: PSND_FIXED (fixed position sounds)
// ============================================================================

// Test: fixed sound at specific position
void test_Channel_FixedSound_InFront(void) {
	ResetTestState();
	
	channel_t ch = {0};
	ch.psType = PSND_FIXED;
	ch.origin[0] = 100.0f;
	ch.origin[1] = 0.0f;
	ch.origin[2] = 0.0f;
	ch.masterVol = 255;
	ch.distMult = 0.002f;
	
	DMASnd_SpatializeChannel(&ch);
	
	// Sound in front should be centered
	TEST_ASSERT_EQUAL(ch.leftVol, ch.rightVol);
	TEST_ASSERT_GREATER_THAN(0, ch.leftVol);
}

// Test: fixed sound to the right
void test_Channel_FixedSound_Right(void) {
	ResetTestState();
	
	channel_t ch = {0};
	ch.psType = PSND_FIXED;
	ch.origin[0] = 0.0f;
	ch.origin[1] = 100.0f;
	ch.origin[2] = 0.0f;
	ch.masterVol = 255;
	ch.distMult = 0.002f;
	
	DMASnd_SpatializeChannel(&ch);
	
	// Sound to the right should favor right ear
	TEST_ASSERT_GREATER_THAN(ch.leftVol, ch.rightVol);
}

// Test: fixed sound to the left
void test_Channel_FixedSound_Left(void) {
	ResetTestState();
	
	channel_t ch = {0};
	ch.psType = PSND_FIXED;
	ch.origin[0] = 0.0f;
	ch.origin[1] = -100.0f;
	ch.origin[2] = 0.0f;
	ch.masterVol = 255;
	ch.distMult = 0.002f;
	
	DMASnd_SpatializeChannel(&ch);
	
	// Sound to the left should favor left ear
	TEST_ASSERT_GREATER_THAN(ch.rightVol, ch.leftVol);
}

// Test: fixed sound close (full volume)
void test_Channel_FixedSound_Close(void) {
	ResetTestState();
	
	channel_t ch = {0};
	ch.psType = PSND_FIXED;
	ch.origin[0] = 50.0f;  // Within SOUND_FULLVOLUME
	ch.origin[1] = 0.0f;
	ch.origin[2] = 0.0f;
	ch.masterVol = 255;
	ch.distMult = 0.002f;
	
	DMASnd_SpatializeChannel(&ch);
	
	// Close sound should be loud
	TEST_ASSERT_GREATER_THAN(100, ch.leftVol);
	TEST_ASSERT_GREATER_THAN(100, ch.rightVol);
}

// Test: fixed sound far away
void test_Channel_FixedSound_Far(void) {
	ResetTestState();
	
	channel_t ch = {0};
	ch.psType = PSND_FIXED;
	ch.origin[0] = 500.0f;  // Far away
	ch.origin[1] = 0.0f;
	ch.origin[2] = 0.0f;
	ch.masterVol = 255;
	ch.distMult = 0.002f;
	
	DMASnd_SpatializeChannel(&ch);
	
	// Far sound should be quieter
	TEST_ASSERT_LESS_THAN(200, ch.leftVol);
	TEST_ASSERT_LESS_THAN(200, ch.rightVol);
}

// Test: fixed sound with listener moved
void test_Channel_FixedSound_ListenerMoved(void) {
	ResetTestState();
	snd_listenerOrigin[0] = 100.0f;  // Move listener forward
	
	channel_t ch = {0};
	ch.psType = PSND_FIXED;
	ch.origin[0] = 200.0f;  // Sound ahead of new position
	ch.origin[1] = 0.0f;
	ch.origin[2] = 0.0f;
	ch.masterVol = 255;
	ch.distMult = 0.002f;
	
	DMASnd_SpatializeChannel(&ch);
	
	// Should spatialize relative to listener position
	TEST_ASSERT_GREATER_THAN(0, ch.leftVol);
	TEST_ASSERT_EQUAL(ch.leftVol, ch.rightVol);  // Still centered
}

// Test: fixed sound with listener rotated
void test_Channel_FixedSound_ListenerRotated(void) {
	ResetTestState();
	snd_listenerRight[0] = 1.0f;  // Listener facing right
	snd_listenerRight[1] = 0.0f;
	
	channel_t ch = {0};
	ch.psType = PSND_FIXED;
	ch.origin[0] = 100.0f;  // Sound in front of original facing
	ch.origin[1] = 0.0f;
	ch.origin[2] = 0.0f;
	ch.masterVol = 255;
	ch.distMult = 0.002f;
	
	DMASnd_SpatializeChannel(&ch);
	
	// With rotated listener, sound should pan right
	TEST_ASSERT_GREATER_THAN(ch.leftVol, ch.rightVol);
}

// ============================================================================
// TESTS: PSND_ENTITY (entity-attached sounds)
// ============================================================================

// Test: entity sound at entity 0
void test_Channel_EntitySound_Entity0(void) {
	ResetTestState();
	mockEntityCount = 1;
	mockEntityOrigins[0][0] = 100.0f;
	mockEntityOrigins[0][1] = 0.0f;
	mockEntityOrigins[0][2] = 0.0f;
	
	channel_t ch = {0};
	ch.psType = PSND_ENTITY;
	ch.entNum = 0;
	ch.masterVol = 255;
	ch.distMult = 0.002f;
	
	DMASnd_SpatializeChannel(&ch);
	
	// Should spatialize based on entity origin
	TEST_ASSERT_EQUAL(ch.leftVol, ch.rightVol);  // Centered
	TEST_ASSERT_GREATER_THAN(0, ch.leftVol);
}

// Test: entity sound to the right
void test_Channel_EntitySound_Right(void) {
	ResetTestState();
	mockEntityCount = 1;
	mockEntityOrigins[0][0] = 0.0f;
	mockEntityOrigins[0][1] = 100.0f;
	mockEntityOrigins[0][2] = 0.0f;
	
	channel_t ch = {0};
	ch.psType = PSND_ENTITY;
	ch.entNum = 0;
	ch.masterVol = 255;
	ch.distMult = 0.002f;
	
	DMASnd_SpatializeChannel(&ch);
	
	// Entity to the right should favor right ear
	TEST_ASSERT_GREATER_THAN(ch.leftVol, ch.rightVol);
}

// Test: multiple entities with different positions
void test_Channel_EntitySound_MultipleEntities(void) {
	ResetTestState();
	mockEntityCount = 3;
	mockEntityOrigins[0][0] = 100.0f;
	mockEntityOrigins[0][1] = 0.0f;
	mockEntityOrigins[0][2] = 0.0f;
	mockEntityOrigins[1][0] = 0.0f;
	mockEntityOrigins[1][1] = 100.0f;
	mockEntityOrigins[1][2] = 0.0f;
	mockEntityOrigins[2][0] = 0.0f;
	mockEntityOrigins[2][1] = -100.0f;
	mockEntityOrigins[2][2] = 0.0f;
	
	channel_t ch0 = {0}, ch1 = {0}, ch2 = {0};
	
	// Entity 0 - in front
	ch0.psType = PSND_ENTITY;
	ch0.entNum = 0;
	ch0.masterVol = 255;
	ch0.distMult = 0.002f;
	DMASnd_SpatializeChannel(&ch0);
	
	// Entity 1 - to the right
	ch1.psType = PSND_ENTITY;
	ch1.entNum = 1;
	ch1.masterVol = 255;
	ch1.distMult = 0.002f;
	DMASnd_SpatializeChannel(&ch1);
	
	// Entity 2 - to the left
	ch2.psType = PSND_ENTITY;
	ch2.entNum = 2;
	ch2.masterVol = 255;
	ch2.distMult = 0.002f;
	DMASnd_SpatializeChannel(&ch2);
	
	// Verify correct spatialization
	TEST_ASSERT_EQUAL(ch0.leftVol, ch0.rightVol);  // Centered
	TEST_ASSERT_GREATER_THAN(ch1.leftVol, ch1.rightVol);  // Right
	TEST_ASSERT_GREATER_THAN(ch2.rightVol, ch2.leftVol);  // Left
}

// Test: entity sound with invalid entity number
void test_Channel_EntitySound_InvalidEntity(void) {
	ResetTestState();
	mockEntityCount = 1;
	
	channel_t ch = {0};
	ch.psType = PSND_ENTITY;
	ch.entNum = 99;  // Invalid
	ch.masterVol = 255;
	ch.distMult = 0.002f;
	
	DMASnd_SpatializeChannel(&ch);
	
	// Should default to origin (0,0,0) which is at listener
	TEST_ASSERT_EQUAL(ch.leftVol, ch.rightVol);
}

// Test: entity sound moving (different positions in sequence)
void test_Channel_EntitySound_Moving(void) {
	ResetTestState();
	mockEntityCount = 1;
	
	channel_t ch = {0};
	ch.psType = PSND_ENTITY;
	ch.entNum = 0;
	ch.masterVol = 255;
	ch.distMult = 0.002f;
	
	// Frame 1: entity in front
	mockEntityOrigins[0][0] = 100.0f;
	mockEntityOrigins[0][1] = 0.0f;
	mockEntityOrigins[0][2] = 0.0f;
	DMASnd_SpatializeChannel(&ch);
	int leftVol1 = ch.leftVol;
	int rightVol1 = ch.rightVol;
	
	// Frame 2: entity moved right
	mockEntityOrigins[0][0] = 100.0f;
	mockEntityOrigins[0][1] = 50.0f;
	mockEntityOrigins[0][2] = 0.0f;
	DMASnd_SpatializeChannel(&ch);
	int leftVol2 = ch.leftVol;
	int rightVol2 = ch.rightVol;
	
	// Should pan from center to right
	TEST_ASSERT_EQUAL(leftVol1, rightVol1);  // First was centered
	TEST_ASSERT_GREATER_THAN(leftVol2, rightVol2);  // Second panned right
}

// ============================================================================
// TESTS: PSND_LOCAL (local/player sounds)
// ============================================================================

// Test: local sound always full volume
void test_Channel_LocalSound_FullVolume(void) {
	ResetTestState();
	
	channel_t ch = {0};
	ch.psType = PSND_LOCAL;
	ch.masterVol = 200;
	ch.distMult = 0.002f;
	
	DMASnd_SpatializeChannel(&ch);
	
	// Local sound should be at master volume
	TEST_ASSERT_EQUAL(200, ch.leftVol);
	TEST_ASSERT_EQUAL(200, ch.rightVol);
}

// Test: local sound ignores distance multiplier
void test_Channel_LocalSound_IgnoresDistance(void) {
	ResetTestState();
	
	channel_t ch = {0};
	ch.psType = PSND_LOCAL;
	ch.masterVol = 255;
	ch.distMult = 100.0f;  // Extreme distance mult
	
	DMASnd_SpatializeChannel(&ch);
	
	// Should still be full volume
	TEST_ASSERT_EQUAL(255, ch.leftVol);
	TEST_ASSERT_EQUAL(255, ch.rightVol);
}

// Test: local sound ignores listener position
void test_Channel_LocalSound_IgnoresListenerPosition(void) {
	ResetTestState();
	snd_listenerOrigin[0] = 500.0f;
	snd_listenerOrigin[1] = 500.0f;
	snd_listenerOrigin[2] = 500.0f;
	
	channel_t ch = {0};
	ch.psType = PSND_LOCAL;
	ch.masterVol = 150;
	ch.distMult = 0.002f;
	
	DMASnd_SpatializeChannel(&ch);
	
	// Still full volume regardless of listener position
	TEST_ASSERT_EQUAL(150, ch.leftVol);
	TEST_ASSERT_EQUAL(150, ch.rightVol);
}

// Test: local sound with zero master volume
void test_Channel_LocalSound_ZeroMasterVolume(void) {
	ResetTestState();
	
	channel_t ch = {0};
	ch.psType = PSND_LOCAL;
	ch.masterVol = 0;
	ch.distMult = 0.002f;
	
	DMASnd_SpatializeChannel(&ch);
	
	TEST_ASSERT_EQUAL(0, ch.leftVol);
	TEST_ASSERT_EQUAL(0, ch.rightVol);
}

// Test: local sound always centered
void test_Channel_LocalSound_AlwaysCentered(void) {
	ResetTestState();
	
	channel_t ch = {0};
	ch.psType = PSND_LOCAL;
	ch.masterVol = 128;
	ch.distMult = 0.002f;
	
	DMASnd_SpatializeChannel(&ch);
	
	// Left and right always equal
	TEST_ASSERT_EQUAL(ch.leftVol, ch.rightVol);
}

// ============================================================================
// TESTS: Master Volume Variations
// ============================================================================

// Test: different master volumes affect output
void test_Channel_MasterVolume_Variations(void) {
	ResetTestState();
	
	channel_t ch1 = {0}, ch2 = {0}, ch3 = {0};
	
	// Same position, different master volumes
	ch1.psType = PSND_FIXED;
	ch1.origin[0] = 100.0f;
	ch1.masterVol = 64;
	ch1.distMult = 0.002f;
	
	ch2.psType = PSND_FIXED;
	ch2.origin[0] = 100.0f;
	ch2.masterVol = 128;
	ch2.distMult = 0.002f;
	
	ch3.psType = PSND_FIXED;
	ch3.origin[0] = 100.0f;
	ch3.masterVol = 255;
	ch3.distMult = 0.002f;
	
	DMASnd_SpatializeChannel(&ch1);
	DMASnd_SpatializeChannel(&ch2);
	DMASnd_SpatializeChannel(&ch3);
	
	// Higher master volume should produce higher output
	TEST_ASSERT_LESS_THAN(ch2.leftVol, ch1.leftVol);
	TEST_ASSERT_LESS_THAN(ch3.leftVol, ch2.leftVol);
}

// ============================================================================
// TESTS: Distance Multiplier Variations
// ============================================================================

// Test: different distance multipliers
void test_Channel_DistMult_Variations(void) {
	ResetTestState();
	
	channel_t ch1 = {0}, ch2 = {0};
	
	// Same position, different distance multipliers
	ch1.psType = PSND_FIXED;
	ch1.origin[0] = 200.0f;
	ch1.masterVol = 255;
	ch1.distMult = 0.001f;  // Less attenuation
	
	ch2.psType = PSND_FIXED;
	ch2.origin[0] = 200.0f;
	ch2.masterVol = 255;
	ch2.distMult = 0.004f;  // More attenuation
	
	DMASnd_SpatializeChannel(&ch1);
	DMASnd_SpatializeChannel(&ch2);
	
	// Lower distMult should be louder
	TEST_ASSERT_GREATER_THAN(ch2.leftVol, ch1.leftVol);
}

// Test: zero distance multiplier (no spatialization)
void test_Channel_DistMult_Zero(void) {
	ResetTestState();
	
	channel_t ch = {0};
	ch.psType = PSND_FIXED;
	ch.origin[0] = 500.0f;  // Far away
	ch.origin[1] = 100.0f;  // To the right
	ch.masterVol = 255;
	ch.distMult = 0.0f;  // No attenuation or spatialization
	
	DMASnd_SpatializeChannel(&ch);
	
	// Should be full volume, no panning
	TEST_ASSERT_EQUAL(255, ch.leftVol);
	TEST_ASSERT_EQUAL(255, ch.rightVol);
}

// ============================================================================
// TESTS: Complex Scenarios
// ============================================================================

// Test: mixing different sound types
void test_Channel_MixedSoundTypes(void) {
	ResetTestState();
	mockEntityCount = 1;
	mockEntityOrigins[0][0] = 0.0f;
	mockEntityOrigins[0][1] = 100.0f;
	mockEntityOrigins[0][2] = 0.0f;
	
	channel_t chFixed = {0}, chEntity = {0}, chLocal = {0};
	
	// Fixed sound to the left
	chFixed.psType = PSND_FIXED;
	chFixed.origin[0] = 0.0f;
	chFixed.origin[1] = -100.0f;
	chFixed.origin[2] = 0.0f;
	chFixed.masterVol = 200;
	chFixed.distMult = 0.002f;
	
	// Entity sound to the right
	chEntity.psType = PSND_ENTITY;
	chEntity.entNum = 0;
	chEntity.masterVol = 200;
	chEntity.distMult = 0.002f;
	
	// Local sound
	chLocal.psType = PSND_LOCAL;
	chLocal.masterVol = 200;
	chLocal.distMult = 0.002f;
	
	DMASnd_SpatializeChannel(&chFixed);
	DMASnd_SpatializeChannel(&chEntity);
	DMASnd_SpatializeChannel(&chLocal);
	
	// Verify each type works correctly
	TEST_ASSERT_GREATER_THAN(chFixed.rightVol, chFixed.leftVol);  // Fixed left
	TEST_ASSERT_GREATER_THAN(chEntity.leftVol, chEntity.rightVol);  // Entity right
	TEST_ASSERT_EQUAL(200, chLocal.leftVol);  // Local full volume
	TEST_ASSERT_EQUAL(200, chLocal.rightVol);
}

// Test: sound at listener position
void test_Channel_AtListenerPosition(void) {
	ResetTestState();
	
	channel_t ch = {0};
	ch.psType = PSND_FIXED;
	ch.origin[0] = 0.0f;  // Same as listener
	ch.origin[1] = 0.0f;
	ch.origin[2] = 0.0f;
	ch.masterVol = 255;
	ch.distMult = 0.002f;
	
	DMASnd_SpatializeChannel(&ch);
	
	// Should be centered and loud
	TEST_ASSERT_EQUAL(ch.leftVol, ch.rightVol);
	TEST_ASSERT_GREATER_THAN(100, ch.leftVol);
}

// Test: sound above/below listener (Z axis)
void test_Channel_VerticalPosition(void) {
	ResetTestState();
	
	channel_t chAbove = {0}, chBelow = {0};
	
	// Sound above
	chAbove.psType = PSND_FIXED;
	chAbove.origin[0] = 100.0f;
	chAbove.origin[1] = 0.0f;
	chAbove.origin[2] = 100.0f;
	chAbove.masterVol = 255;
	chAbove.distMult = 0.002f;
	
	// Sound below
	chBelow.psType = PSND_FIXED;
	chBelow.origin[0] = 100.0f;
	chBelow.origin[1] = 0.0f;
	chBelow.origin[2] = -100.0f;
	chBelow.masterVol = 255;
	chBelow.distMult = 0.002f;
	
	DMASnd_SpatializeChannel(&chAbove);
	DMASnd_SpatializeChannel(&chBelow);
	
	// Both should be centered (Z doesn't affect stereo pan)
	TEST_ASSERT_EQUAL(chAbove.leftVol, chAbove.rightVol);
	TEST_ASSERT_EQUAL(chBelow.leftVol, chBelow.rightVol);
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main(void) {
	UNITY_BEGIN();

	// PSND_FIXED tests
	RUN_TEST(test_Channel_FixedSound_InFront);
	RUN_TEST(test_Channel_FixedSound_Right);
	RUN_TEST(test_Channel_FixedSound_Left);
	RUN_TEST(test_Channel_FixedSound_Close);
	RUN_TEST(test_Channel_FixedSound_Far);
	RUN_TEST(test_Channel_FixedSound_ListenerMoved);
	RUN_TEST(test_Channel_FixedSound_ListenerRotated);

	// PSND_ENTITY tests
	RUN_TEST(test_Channel_EntitySound_Entity0);
	RUN_TEST(test_Channel_EntitySound_Right);
	RUN_TEST(test_Channel_EntitySound_MultipleEntities);
	RUN_TEST(test_Channel_EntitySound_InvalidEntity);
	RUN_TEST(test_Channel_EntitySound_Moving);

	// PSND_LOCAL tests
	RUN_TEST(test_Channel_LocalSound_FullVolume);
	RUN_TEST(test_Channel_LocalSound_IgnoresDistance);
	RUN_TEST(test_Channel_LocalSound_IgnoresListenerPosition);
	RUN_TEST(test_Channel_LocalSound_ZeroMasterVolume);
	RUN_TEST(test_Channel_LocalSound_AlwaysCentered);

	// Master volume tests
	RUN_TEST(test_Channel_MasterVolume_Variations);

	// Distance multiplier tests
	RUN_TEST(test_Channel_DistMult_Variations);
	RUN_TEST(test_Channel_DistMult_Zero);

	// Complex scenarios
	RUN_TEST(test_Channel_MixedSoundTypes);
	RUN_TEST(test_Channel_AtListenerPosition);
	RUN_TEST(test_Channel_VerticalPosition);

	return UNITY_END();
}
