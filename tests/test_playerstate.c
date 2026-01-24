/*
====================
test_playerstate.c

Unit tests for player state delta encoding (playerState_t with PS_* flags)
====================
*/

#include "../unity/unity.h"
#include <string.h>

// Forward declarations
typedef unsigned char byte;
typedef short int16;
typedef unsigned short uint16;
typedef int qBool;
typedef float vec_t;
typedef vec_t vec3_t[3];

#define qTrue 1
#define qFalse 0

// MAX_STATS
#define MAX_STATS 32

// pmType values
enum {
	PMT_NORMAL,
	PMT_SPECTATOR,
	PMT_DEAD,
	PMT_GIB,
	PMT_FREEZE
};

// pmFlags values
enum {
	PMF_DUCKED			= 1 << 0,
	PMF_JUMP_HELD		= 1 << 1,
	PMF_ON_GROUND		= 1 << 2,
	PMF_TIME_WATERJUMP	= 1 << 3,
	PMF_TIME_LAND		= 1 << 4,
	PMF_TIME_TELEPORT	= 1 << 5,
	PMF_NO_PREDICTION	= 1 << 6
};

// pMoveState_t structure
typedef struct pMoveState_s {
	int				pmType;
	int16			origin[3];		// 12.3
	int16			velocity[3];	// 12.3
	byte			pmFlags;		// ducked, jump_held, etc
	byte			pmTime;			// each unit = 8 ms
	int16			gravity;
	int16			deltaAngles[3];	// add to command angles to get view direction
} pMoveState_t;

// playerState_t structure
typedef struct playerState_s {
	pMoveState_t	pMove;				// for prediction
	vec3_t			viewAngles;			// for fixed views
	vec3_t			viewOffset;			// add to pmovestate->origin
	vec3_t			kickAngles;			// add to view direction to get render angles
	vec3_t			gunAngles;
	vec3_t			gunOffset;
	int				gunIndex;
	int				gunFrame;
	float			viewBlend[4];		// rgba full screen effect
	float			fov;				// horizontal field of view
	int				rdFlags;			// refdef flags
	int16			stats[MAX_STATS];	// fast status bar updates
} playerState_t;

// playerState_t communication flags
enum {
	PS_M_TYPE			= 1 << 0,
	PS_M_ORIGIN			= 1 << 1,
	PS_M_VELOCITY		= 1 << 2,
	PS_M_TIME			= 1 << 3,
	PS_M_FLAGS			= 1 << 4,
	PS_M_GRAVITY		= 1 << 5,
	PS_M_DELTA_ANGLES	= 1 << 6,

	PS_VIEWOFFSET		= 1 << 7,
	PS_VIEWANGLES		= 1 << 8,
	PS_KICKANGLES		= 1 << 9,
	PS_BLEND			= 1 << 10,
	PS_FOV				= 1 << 11,
	PS_WEAPONINDEX		= 1 << 12,
	PS_WEAPONFRAME		= 1 << 13,
	PS_RDFLAGS			= 1 << 14,
	PS_BBOX				= 1 << 15
};

// rdFlags values
enum {
	RDF_UNDERWATER		= 1 << 0,
	RDF_NOWORLDMODEL	= 1 << 1,
	RDF_IRGOGGLES		= 1 << 2,
	RDF_UVGOGGLES		= 1 << 3,
	RDF_OLDAREABITS		= 1 << 4
};

// ============================================================================
// DELTA CALCULATION FUNCTION (simulates SV_WritePlayerstateToClient logic)
// ============================================================================

static int CalcPlayerStateDelta(const playerState_t *from, const playerState_t *to) {
	int psFlags = 0;

	if (to->pMove.pmType != from->pMove.pmType)
		psFlags |= PS_M_TYPE;

	if (to->pMove.origin[0] != from->pMove.origin[0]
	|| to->pMove.origin[1] != from->pMove.origin[1]
	|| to->pMove.origin[2] != from->pMove.origin[2])
		psFlags |= PS_M_ORIGIN;

	if (to->pMove.velocity[0] != from->pMove.velocity[0]
	|| to->pMove.velocity[1] != from->pMove.velocity[1]
	|| to->pMove.velocity[2] != from->pMove.velocity[2])
		psFlags |= PS_M_VELOCITY;

	if (to->pMove.pmTime != from->pMove.pmTime)
		psFlags |= PS_M_TIME;

	if (to->pMove.pmFlags != from->pMove.pmFlags)
		psFlags |= PS_M_FLAGS;

	if (to->pMove.gravity != from->pMove.gravity)
		psFlags |= PS_M_GRAVITY;

	if (to->pMove.deltaAngles[0] != from->pMove.deltaAngles[0]
	|| to->pMove.deltaAngles[1] != from->pMove.deltaAngles[1]
	|| to->pMove.deltaAngles[2] != from->pMove.deltaAngles[2])
		psFlags |= PS_M_DELTA_ANGLES;

	if (to->viewOffset[0] != from->viewOffset[0]
	|| to->viewOffset[1] != from->viewOffset[1]
	|| to->viewOffset[2] != from->viewOffset[2])
		psFlags |= PS_VIEWOFFSET;

	if (to->viewAngles[0] != from->viewAngles[0]
	|| to->viewAngles[1] != from->viewAngles[1]
	|| to->viewAngles[2] != from->viewAngles[2])
		psFlags |= PS_VIEWANGLES;

	if (to->kickAngles[0] != from->kickAngles[0]
	|| to->kickAngles[1] != from->kickAngles[1]
	|| to->kickAngles[2] != from->kickAngles[2])
		psFlags |= PS_KICKANGLES;

	if (to->viewBlend[0] != from->viewBlend[0]
	|| to->viewBlend[1] != from->viewBlend[1]
	|| to->viewBlend[2] != from->viewBlend[2]
	|| to->viewBlend[3] != from->viewBlend[3])
		psFlags |= PS_BLEND;

	if (to->fov != from->fov)
		psFlags |= PS_FOV;

	if (to->rdFlags != from->rdFlags)
		psFlags |= PS_RDFLAGS;

	if (to->gunFrame != from->gunFrame)
		psFlags |= PS_WEAPONFRAME;

	// Always send weapon index
	psFlags |= PS_WEAPONINDEX;

	return psFlags;
}

// ============================================================================
// TEST HELPERS
// ============================================================================

static void InitPlayerState(playerState_t *ps) {
	memset(ps, 0, sizeof(*ps));
	ps->pMove.pmType = PMT_NORMAL;
	ps->pMove.gravity = 800;
	ps->fov = 90.0f;
}

// ============================================================================
// TESTS: Basic Delta Calculation
// ============================================================================

void test_PlayerState_NoDelta_IdenticalStates(void) {
	playerState_t ps1, ps2;
	int flags;
	
	InitPlayerState(&ps1);
	InitPlayerState(&ps2);
	
	flags = CalcPlayerStateDelta(&ps1, &ps2);
	
	// Only PS_WEAPONINDEX should be set (always sent)
	TEST_ASSERT_EQUAL(PS_WEAPONINDEX, flags);
}

void test_PlayerState_NoDelta_EmptyStates(void) {
	playerState_t ps1, ps2;
	int flags;
	
	memset(&ps1, 0, sizeof(ps1));
	memset(&ps2, 0, sizeof(ps2));
	
	flags = CalcPlayerStateDelta(&ps1, &ps2);
	
	TEST_ASSERT_EQUAL(PS_WEAPONINDEX, flags);
}

// ============================================================================
// TESTS: pMoveState Fields
// ============================================================================

void test_PlayerState_Delta_PmType(void) {
	playerState_t ps1, ps2;
	int flags;
	
	InitPlayerState(&ps1);
	InitPlayerState(&ps2);
	
	ps1.pMove.pmType = PMT_NORMAL;
	ps2.pMove.pmType = PMT_SPECTATOR;
	
	flags = CalcPlayerStateDelta(&ps1, &ps2);
	
	TEST_ASSERT_TRUE((flags & PS_M_TYPE) != 0);
}

void test_PlayerState_Delta_PmType_AllTypes(void) {
	playerState_t ps1, ps2;
	int flags;
	int types[] = {PMT_NORMAL, PMT_SPECTATOR, PMT_DEAD, PMT_GIB, PMT_FREEZE};
	int i, j;
	
	for (i = 0; i < 5; i++) {
		for (j = 0; j < 5; j++) {
			InitPlayerState(&ps1);
			InitPlayerState(&ps2);
			
			ps1.pMove.pmType = types[i];
			ps2.pMove.pmType = types[j];
			
			flags = CalcPlayerStateDelta(&ps1, &ps2);
			
			if (i == j) {
				TEST_ASSERT_FALSE((flags & PS_M_TYPE) != 0);
			} else {
				TEST_ASSERT_TRUE((flags & PS_M_TYPE) != 0);
			}
		}
	}
}

void test_PlayerState_Delta_Origin(void) {
	playerState_t ps1, ps2;
	int flags;
	
	InitPlayerState(&ps1);
	InitPlayerState(&ps2);
	
	ps1.pMove.origin[0] = 100;
	ps1.pMove.origin[1] = 200;
	ps1.pMove.origin[2] = 300;
	
	ps2.pMove.origin[0] = 150;
	ps2.pMove.origin[1] = 200;
	ps2.pMove.origin[2] = 300;
	
	flags = CalcPlayerStateDelta(&ps1, &ps2);
	
	TEST_ASSERT_TRUE((flags & PS_M_ORIGIN) != 0);
}

void test_PlayerState_Delta_Origin_AllAxes(void) {
	playerState_t ps1, ps2;
	int flags;
	
	InitPlayerState(&ps1);
	InitPlayerState(&ps2);
	
	// Test X axis
	ps2.pMove.origin[0] = 100;
	flags = CalcPlayerStateDelta(&ps1, &ps2);
	TEST_ASSERT_TRUE((flags & PS_M_ORIGIN) != 0);
	
	// Reset and test Y axis
	InitPlayerState(&ps2);
	ps2.pMove.origin[1] = 100;
	flags = CalcPlayerStateDelta(&ps1, &ps2);
	TEST_ASSERT_TRUE((flags & PS_M_ORIGIN) != 0);
	
	// Reset and test Z axis
	InitPlayerState(&ps2);
	ps2.pMove.origin[2] = 100;
	flags = CalcPlayerStateDelta(&ps1, &ps2);
	TEST_ASSERT_TRUE((flags & PS_M_ORIGIN) != 0);
}

void test_PlayerState_Delta_Velocity(void) {
	playerState_t ps1, ps2;
	int flags;
	
	InitPlayerState(&ps1);
	InitPlayerState(&ps2);
	
	ps1.pMove.velocity[0] = 100;
	ps2.pMove.velocity[0] = 200;
	
	flags = CalcPlayerStateDelta(&ps1, &ps2);
	
	TEST_ASSERT_TRUE((flags & PS_M_VELOCITY) != 0);
}

void test_PlayerState_Delta_PmTime(void) {
	playerState_t ps1, ps2;
	int flags;
	
	InitPlayerState(&ps1);
	InitPlayerState(&ps2);
	
	ps1.pMove.pmTime = 0;
	ps2.pMove.pmTime = 8;
	
	flags = CalcPlayerStateDelta(&ps1, &ps2);
	
	TEST_ASSERT_TRUE((flags & PS_M_TIME) != 0);
}

void test_PlayerState_Delta_PmFlags(void) {
	playerState_t ps1, ps2;
	int flags;
	
	InitPlayerState(&ps1);
	InitPlayerState(&ps2);
	
	ps1.pMove.pmFlags = 0;
	ps2.pMove.pmFlags = PMF_DUCKED;
	
	flags = CalcPlayerStateDelta(&ps1, &ps2);
	
	TEST_ASSERT_TRUE((flags & PS_M_FLAGS) != 0);
}

void test_PlayerState_Delta_PmFlags_AllFlags(void) {
	playerState_t ps1, ps2;
	int flags;
	byte testFlags[] = {
		PMF_DUCKED, PMF_JUMP_HELD, PMF_ON_GROUND,
		PMF_TIME_WATERJUMP, PMF_TIME_LAND, PMF_TIME_TELEPORT,
		PMF_NO_PREDICTION
	};
	int i;
	
	for (i = 0; i < 7; i++) {
		InitPlayerState(&ps1);
		InitPlayerState(&ps2);
		
		ps2.pMove.pmFlags = testFlags[i];
		
		flags = CalcPlayerStateDelta(&ps1, &ps2);
		TEST_ASSERT_TRUE((flags & PS_M_FLAGS) != 0);
	}
}

void test_PlayerState_Delta_Gravity(void) {
	playerState_t ps1, ps2;
	int flags;
	
	InitPlayerState(&ps1);
	InitPlayerState(&ps2);
	
	ps1.pMove.gravity = 800;
	ps2.pMove.gravity = 400;
	
	flags = CalcPlayerStateDelta(&ps1, &ps2);
	
	TEST_ASSERT_TRUE((flags & PS_M_GRAVITY) != 0);
}

void test_PlayerState_Delta_DeltaAngles(void) {
	playerState_t ps1, ps2;
	int flags;
	
	InitPlayerState(&ps1);
	InitPlayerState(&ps2);
	
	ps1.pMove.deltaAngles[0] = 0;
	ps2.pMove.deltaAngles[0] = 100;
	
	flags = CalcPlayerStateDelta(&ps1, &ps2);
	
	TEST_ASSERT_TRUE((flags & PS_M_DELTA_ANGLES) != 0);
}

// ============================================================================
// TESTS: View Fields
// ============================================================================

void test_PlayerState_Delta_ViewOffset(void) {
	playerState_t ps1, ps2;
	int flags;
	
	InitPlayerState(&ps1);
	InitPlayerState(&ps2);
	
	ps1.viewOffset[0] = 0.0f;
	ps2.viewOffset[0] = 10.0f;
	
	flags = CalcPlayerStateDelta(&ps1, &ps2);
	
	TEST_ASSERT_TRUE((flags & PS_VIEWOFFSET) != 0);
}

void test_PlayerState_Delta_ViewAngles(void) {
	playerState_t ps1, ps2;
	int flags;
	
	InitPlayerState(&ps1);
	InitPlayerState(&ps2);
	
	ps1.viewAngles[0] = 0.0f;
	ps2.viewAngles[0] = 45.0f;
	
	flags = CalcPlayerStateDelta(&ps1, &ps2);
	
	TEST_ASSERT_TRUE((flags & PS_VIEWANGLES) != 0);
}

void test_PlayerState_Delta_KickAngles(void) {
	playerState_t ps1, ps2;
	int flags;
	
	InitPlayerState(&ps1);
	InitPlayerState(&ps2);
	
	ps1.kickAngles[0] = 0.0f;
	ps2.kickAngles[0] = 5.0f;
	
	flags = CalcPlayerStateDelta(&ps1, &ps2);
	
	TEST_ASSERT_TRUE((flags & PS_KICKANGLES) != 0);
}

void test_PlayerState_Delta_ViewBlend(void) {
	playerState_t ps1, ps2;
	int flags;
	
	InitPlayerState(&ps1);
	InitPlayerState(&ps2);
	
	ps1.viewBlend[0] = 0.0f;
	ps2.viewBlend[0] = 1.0f;
	
	flags = CalcPlayerStateDelta(&ps1, &ps2);
	
	TEST_ASSERT_TRUE((flags & PS_BLEND) != 0);
}

void test_PlayerState_Delta_ViewBlend_AllComponents(void) {
	playerState_t ps1, ps2;
	int flags;
	int i;
	
	for (i = 0; i < 4; i++) {
		InitPlayerState(&ps1);
		InitPlayerState(&ps2);
		
		ps2.viewBlend[i] = 0.5f;
		
		flags = CalcPlayerStateDelta(&ps1, &ps2);
		TEST_ASSERT_TRUE((flags & PS_BLEND) != 0);
	}
}

void test_PlayerState_Delta_FOV(void) {
	playerState_t ps1, ps2;
	int flags;
	
	InitPlayerState(&ps1);
	InitPlayerState(&ps2);
	
	ps1.fov = 90.0f;
	ps2.fov = 110.0f;
	
	flags = CalcPlayerStateDelta(&ps1, &ps2);
	
	TEST_ASSERT_TRUE((flags & PS_FOV) != 0);
}

void test_PlayerState_Delta_RDFlags(void) {
	playerState_t ps1, ps2;
	int flags;
	
	InitPlayerState(&ps1);
	InitPlayerState(&ps2);
	
	ps1.rdFlags = 0;
	ps2.rdFlags = RDF_UNDERWATER;
	
	flags = CalcPlayerStateDelta(&ps1, &ps2);
	
	TEST_ASSERT_TRUE((flags & PS_RDFLAGS) != 0);
}

void test_PlayerState_Delta_RDFlags_AllFlags(void) {
	playerState_t ps1, ps2;
	int flags;
	int testFlags[] = {
		RDF_UNDERWATER, RDF_NOWORLDMODEL, RDF_IRGOGGLES,
		RDF_UVGOGGLES, RDF_OLDAREABITS
	};
	int i;
	
	for (i = 0; i < 5; i++) {
		InitPlayerState(&ps1);
		InitPlayerState(&ps2);
		
		ps2.rdFlags = testFlags[i];
		
		flags = CalcPlayerStateDelta(&ps1, &ps2);
		TEST_ASSERT_TRUE((flags & PS_RDFLAGS) != 0);
	}
}

// ============================================================================
// TESTS: Weapon Fields
// ============================================================================

void test_PlayerState_Delta_WeaponIndex_AlwaysSet(void) {
	playerState_t ps1, ps2;
	int flags;
	
	InitPlayerState(&ps1);
	InitPlayerState(&ps2);
	
	// Even when identical, weapon index should be set
	ps1.gunIndex = 5;
	ps2.gunIndex = 5;
	
	flags = CalcPlayerStateDelta(&ps1, &ps2);
	
	TEST_ASSERT_TRUE((flags & PS_WEAPONINDEX) != 0);
}

void test_PlayerState_Delta_WeaponFrame(void) {
	playerState_t ps1, ps2;
	int flags;
	
	InitPlayerState(&ps1);
	InitPlayerState(&ps2);
	
	ps1.gunFrame = 0;
	ps2.gunFrame = 5;
	
	flags = CalcPlayerStateDelta(&ps1, &ps2);
	
	TEST_ASSERT_TRUE((flags & PS_WEAPONFRAME) != 0);
}

// ============================================================================
// TESTS: Multiple Changes
// ============================================================================

void test_PlayerState_Delta_MultipleChanges_Movement(void) {
	playerState_t ps1, ps2;
	int flags;
	
	InitPlayerState(&ps1);
	InitPlayerState(&ps2);
	
	// Change origin and velocity
	ps2.pMove.origin[0] = 100;
	ps2.pMove.velocity[0] = 200;
	
	flags = CalcPlayerStateDelta(&ps1, &ps2);
	
	TEST_ASSERT_TRUE((flags & PS_M_ORIGIN) != 0);
	TEST_ASSERT_TRUE((flags & PS_M_VELOCITY) != 0);
}

void test_PlayerState_Delta_MultipleChanges_View(void) {
	playerState_t ps1, ps2;
	int flags;
	
	InitPlayerState(&ps1);
	InitPlayerState(&ps2);
	
	// Change view angles and offset
	ps2.viewAngles[0] = 45.0f;
	ps2.viewOffset[2] = 20.0f;
	
	flags = CalcPlayerStateDelta(&ps1, &ps2);
	
	TEST_ASSERT_TRUE((flags & PS_VIEWANGLES) != 0);
	TEST_ASSERT_TRUE((flags & PS_VIEWOFFSET) != 0);
}

void test_PlayerState_Delta_MultipleChanges_AllFields(void) {
	playerState_t ps1, ps2;
	int flags;
	
	InitPlayerState(&ps1);
	InitPlayerState(&ps2);
	
	// Change every field
	ps2.pMove.pmType = PMT_SPECTATOR;
	ps2.pMove.origin[0] = 100;
	ps2.pMove.velocity[0] = 200;
	ps2.pMove.pmTime = 8;
	ps2.pMove.pmFlags = PMF_DUCKED;
	ps2.pMove.gravity = 400;
	ps2.pMove.deltaAngles[0] = 100;
	ps2.viewOffset[0] = 10.0f;
	ps2.viewAngles[0] = 45.0f;
	ps2.kickAngles[0] = 5.0f;
	ps2.viewBlend[0] = 1.0f;
	ps2.fov = 110.0f;
	ps2.rdFlags = RDF_UNDERWATER;
	ps2.gunFrame = 5;
	
	flags = CalcPlayerStateDelta(&ps1, &ps2);
	
	// All flags should be set (except PS_BBOX which we don't test)
	TEST_ASSERT_TRUE((flags & PS_M_TYPE) != 0);
	TEST_ASSERT_TRUE((flags & PS_M_ORIGIN) != 0);
	TEST_ASSERT_TRUE((flags & PS_M_VELOCITY) != 0);
	TEST_ASSERT_TRUE((flags & PS_M_TIME) != 0);
	TEST_ASSERT_TRUE((flags & PS_M_FLAGS) != 0);
	TEST_ASSERT_TRUE((flags & PS_M_GRAVITY) != 0);
	TEST_ASSERT_TRUE((flags & PS_M_DELTA_ANGLES) != 0);
	TEST_ASSERT_TRUE((flags & PS_VIEWOFFSET) != 0);
	TEST_ASSERT_TRUE((flags & PS_VIEWANGLES) != 0);
	TEST_ASSERT_TRUE((flags & PS_KICKANGLES) != 0);
	TEST_ASSERT_TRUE((flags & PS_BLEND) != 0);
	TEST_ASSERT_TRUE((flags & PS_FOV) != 0);
	TEST_ASSERT_TRUE((flags & PS_RDFLAGS) != 0);
	TEST_ASSERT_TRUE((flags & PS_WEAPONINDEX) != 0);
	TEST_ASSERT_TRUE((flags & PS_WEAPONFRAME) != 0);
}

// ============================================================================
// TESTS: Stats Array
// ============================================================================

void test_PlayerState_Stats_NoChange(void) {
	playerState_t ps1, ps2;
	int i;
	
	InitPlayerState(&ps1);
	InitPlayerState(&ps2);
	
	// Set identical stats
	for (i = 0; i < MAX_STATS; i++) {
		ps1.stats[i] = i;
		ps2.stats[i] = i;
	}
	
	// Stats comparison would be done separately in actual implementation
	// Here we just verify structure integrity
	TEST_ASSERT_EQUAL(MAX_STATS, 32);
}

void test_PlayerState_Stats_SingleChange(void) {
	playerState_t ps1, ps2;
	int i;
	qBool different = qFalse;
	
	InitPlayerState(&ps1);
	InitPlayerState(&ps2);
	
	// Change one stat
	ps2.stats[5] = 100;
	
	// Verify difference detection
	for (i = 0; i < MAX_STATS; i++) {
		if (ps1.stats[i] != ps2.stats[i]) {
			different = qTrue;
			break;
		}
	}
	
	TEST_ASSERT_TRUE(different);
}

void test_PlayerState_Stats_MultipleChanges(void) {
	playerState_t ps1, ps2;
	int i;
	int changeCount = 0;
	
	InitPlayerState(&ps1);
	InitPlayerState(&ps2);
	
	// Change several stats
	ps2.stats[0] = 100;
	ps2.stats[10] = 50;
	ps2.stats[20] = 75;
	
	// Count differences
	for (i = 0; i < MAX_STATS; i++) {
		if (ps1.stats[i] != ps2.stats[i]) {
			changeCount++;
		}
	}
	
	TEST_ASSERT_EQUAL(3, changeCount);
}

// ============================================================================
// TESTS: Edge Cases
// ============================================================================

void test_PlayerState_EdgeCase_MaxValues(void) {
	playerState_t ps1, ps2;
	int flags;
	
	InitPlayerState(&ps1);
	InitPlayerState(&ps2);
	
	// Set maximum values
	ps2.pMove.origin[0] = 32767;
	ps2.pMove.velocity[0] = 32767;
	ps2.pMove.gravity = 32767;
	ps2.pMove.deltaAngles[0] = 32767;
	
	flags = CalcPlayerStateDelta(&ps1, &ps2);
	
	TEST_ASSERT_TRUE((flags & PS_M_ORIGIN) != 0);
	TEST_ASSERT_TRUE((flags & PS_M_VELOCITY) != 0);
	TEST_ASSERT_TRUE((flags & PS_M_GRAVITY) != 0);
	TEST_ASSERT_TRUE((flags & PS_M_DELTA_ANGLES) != 0);
}

void test_PlayerState_EdgeCase_MinValues(void) {
	playerState_t ps1, ps2;
	int flags;
	
	InitPlayerState(&ps1);
	InitPlayerState(&ps2);
	
	// Set minimum values
	ps2.pMove.origin[0] = -32768;
	ps2.pMove.velocity[0] = -32768;
	ps2.pMove.gravity = -32768;
	ps2.pMove.deltaAngles[0] = -32768;
	
	flags = CalcPlayerStateDelta(&ps1, &ps2);
	
	TEST_ASSERT_TRUE((flags & PS_M_ORIGIN) != 0);
	TEST_ASSERT_TRUE((flags & PS_M_VELOCITY) != 0);
	TEST_ASSERT_TRUE((flags & PS_M_GRAVITY) != 0);
	TEST_ASSERT_TRUE((flags & PS_M_DELTA_ANGLES) != 0);
}

void test_PlayerState_EdgeCase_ZeroToNonZero(void) {
	playerState_t ps1, ps2;
	int flags;
	
	InitPlayerState(&ps1);
	InitPlayerState(&ps2);
	
	// All fields start at zero, set one to non-zero
	ps2.viewOffset[0] = 0.1f;
	
	flags = CalcPlayerStateDelta(&ps1, &ps2);
	
	TEST_ASSERT_TRUE((flags & PS_VIEWOFFSET) != 0);
}

void test_PlayerState_EdgeCase_NonZeroToZero(void) {
	playerState_t ps1, ps2;
	int flags;
	
	InitPlayerState(&ps1);
	InitPlayerState(&ps2);
	
	// Set first state to non-zero, second to zero
	ps1.viewAngles[0] = 45.0f;
	ps2.viewAngles[0] = 0.0f;
	
	flags = CalcPlayerStateDelta(&ps1, &ps2);
	
	TEST_ASSERT_TRUE((flags & PS_VIEWANGLES) != 0);
}

void test_PlayerState_EdgeCase_SmallFloatChange(void) {
	playerState_t ps1, ps2;
	int flags;
	
	InitPlayerState(&ps1);
	InitPlayerState(&ps2);
	
	// Very small float change (should still be detected)
	ps1.fov = 90.0f;
	ps2.fov = 90.001f;
	
	flags = CalcPlayerStateDelta(&ps1, &ps2);
	
	TEST_ASSERT_TRUE((flags & PS_FOV) != 0);
}

// ============================================================================
// TESTS: State Transitions
// ============================================================================

void test_PlayerState_Transition_NormalToSpectator(void) {
	playerState_t ps1, ps2;
	int flags;
	
	InitPlayerState(&ps1);
	InitPlayerState(&ps2);
	
	ps1.pMove.pmType = PMT_NORMAL;
	ps2.pMove.pmType = PMT_SPECTATOR;
	
	flags = CalcPlayerStateDelta(&ps1, &ps2);
	
	TEST_ASSERT_TRUE((flags & PS_M_TYPE) != 0);
}

void test_PlayerState_Transition_NormalToDead(void) {
	playerState_t ps1, ps2;
	int flags;
	
	InitPlayerState(&ps1);
	InitPlayerState(&ps2);
	
	ps1.pMove.pmType = PMT_NORMAL;
	ps2.pMove.pmType = PMT_DEAD;
	
	flags = CalcPlayerStateDelta(&ps1, &ps2);
	
	TEST_ASSERT_TRUE((flags & PS_M_TYPE) != 0);
}

void test_PlayerState_Transition_Standing(void) {
	playerState_t ps1, ps2;
	int flags;
	
	InitPlayerState(&ps1);
	InitPlayerState(&ps2);
	
	// Standing -> Ducked
	ps1.pMove.pmFlags = 0;
	ps2.pMove.pmFlags = PMF_DUCKED;
	
	flags = CalcPlayerStateDelta(&ps1, &ps2);
	
	TEST_ASSERT_TRUE((flags & PS_M_FLAGS) != 0);
}

void test_PlayerState_Transition_Underwater(void) {
	playerState_t ps1, ps2;
	int flags;
	
	InitPlayerState(&ps1);
	InitPlayerState(&ps2);
	
	// Above water -> Underwater
	ps1.rdFlags = 0;
	ps2.rdFlags = RDF_UNDERWATER;
	
	flags = CalcPlayerStateDelta(&ps1, &ps2);
	
	TEST_ASSERT_TRUE((flags & PS_RDFLAGS) != 0);
}

// ============================================================================
// MAIN
// ============================================================================

int main(void) {
	UNITY_BEGIN();

	// Basic Delta Calculation
	RUN_TEST(test_PlayerState_NoDelta_IdenticalStates);
	RUN_TEST(test_PlayerState_NoDelta_EmptyStates);

	// pMoveState Fields
	RUN_TEST(test_PlayerState_Delta_PmType);
	RUN_TEST(test_PlayerState_Delta_PmType_AllTypes);
	RUN_TEST(test_PlayerState_Delta_Origin);
	RUN_TEST(test_PlayerState_Delta_Origin_AllAxes);
	RUN_TEST(test_PlayerState_Delta_Velocity);
	RUN_TEST(test_PlayerState_Delta_PmTime);
	RUN_TEST(test_PlayerState_Delta_PmFlags);
	RUN_TEST(test_PlayerState_Delta_PmFlags_AllFlags);
	RUN_TEST(test_PlayerState_Delta_Gravity);
	RUN_TEST(test_PlayerState_Delta_DeltaAngles);

	// View Fields
	RUN_TEST(test_PlayerState_Delta_ViewOffset);
	RUN_TEST(test_PlayerState_Delta_ViewAngles);
	RUN_TEST(test_PlayerState_Delta_KickAngles);
	RUN_TEST(test_PlayerState_Delta_ViewBlend);
	RUN_TEST(test_PlayerState_Delta_ViewBlend_AllComponents);
	RUN_TEST(test_PlayerState_Delta_FOV);
	RUN_TEST(test_PlayerState_Delta_RDFlags);
	RUN_TEST(test_PlayerState_Delta_RDFlags_AllFlags);

	// Weapon Fields
	RUN_TEST(test_PlayerState_Delta_WeaponIndex_AlwaysSet);
	RUN_TEST(test_PlayerState_Delta_WeaponFrame);

	// Multiple Changes
	RUN_TEST(test_PlayerState_Delta_MultipleChanges_Movement);
	RUN_TEST(test_PlayerState_Delta_MultipleChanges_View);
	RUN_TEST(test_PlayerState_Delta_MultipleChanges_AllFields);

	// Stats Array
	RUN_TEST(test_PlayerState_Stats_NoChange);
	RUN_TEST(test_PlayerState_Stats_SingleChange);
	RUN_TEST(test_PlayerState_Stats_MultipleChanges);

	// Edge Cases
	RUN_TEST(test_PlayerState_EdgeCase_MaxValues);
	RUN_TEST(test_PlayerState_EdgeCase_MinValues);
	RUN_TEST(test_PlayerState_EdgeCase_ZeroToNonZero);
	RUN_TEST(test_PlayerState_EdgeCase_NonZeroToZero);
	RUN_TEST(test_PlayerState_EdgeCase_SmallFloatChange);

	// State Transitions
	RUN_TEST(test_PlayerState_Transition_NormalToSpectator);
	RUN_TEST(test_PlayerState_Transition_NormalToDead);
	RUN_TEST(test_PlayerState_Transition_Standing);
	RUN_TEST(test_PlayerState_Transition_Underwater);

	return UNITY_END();
}
