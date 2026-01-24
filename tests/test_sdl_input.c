/*
Test suite for SDL2 input translation functions
Tests SDL_Keycode to keyNum_t mapping in SDL2_TranslateKey
*/

#include "unity.h"
#include <SDL2/SDL.h>
#include <math.h>
#include <string.h>

// Redefine the keyNum_t enum from cgame/cg_shared.h
typedef enum keyNum_s {
	K_BADKEY		=	-1,

	K_TAB			=	9,
	K_ENTER			=	13,
	K_ESCAPE		=	27,
	K_SPACE			=	32,

	K_BACKSPACE		=	127,
	K_UPARROW,
	K_DOWNARROW,
	K_LEFTARROW,
	K_RIGHTARROW,

	K_ALT,
	K_CTRL,

	K_SHIFT,
	K_LSHIFT,
	K_RSHIFT,

	K_CAPSLOCK,

	K_F1,
	K_F2,
	K_F3,
	K_F4,
	K_F5,
	K_F6,
	K_F7,
	K_F8,
	K_F9,
	K_F10,
	K_F11,
	K_F12,

	K_INS,
	K_DEL,
	K_PGDN,
	K_PGUP,
	K_HOME,
	K_END,

	K_KP_HOME		=	160,
	K_KP_UPARROW,
	K_KP_PGUP,
	K_KP_LEFTARROW,
	K_KP_FIVE,
	K_KP_RIGHTARROW,
	K_KP_END,
	K_KP_DOWNARROW,
	K_KP_PGDN,
	K_KP_ENTER,
	K_KP_INS,
	K_KP_DEL,
	K_KP_SLASH,
	K_KP_MINUS,
	K_KP_PLUS,

	K_MOUSE1	=		200,
	K_MOUSE2,
	K_MOUSE3,
	K_MOUSE4,
	K_MOUSE5,

	K_JOY1,
	K_JOY2,
	K_JOY3,
	K_JOY4,

	K_AUX1,
	K_AUX2,
	K_AUX3,
	K_AUX4,
	K_AUX5,
	K_AUX6,
	K_AUX7,
	K_AUX8,
	K_AUX9,
	K_AUX10,
	K_AUX11,
	K_AUX12,
	K_AUX13,
	K_AUX14,
	K_AUX15,
	K_AUX16,
	K_AUX17,
	K_AUX18,
	K_AUX19,
	K_AUX20,
	K_AUX21,
	K_AUX22,
	K_AUX23,
	K_AUX24,
	K_AUX25,
	K_AUX26,
	K_AUX27,
	K_AUX28,
	K_AUX29,
	K_AUX30,
	K_AUX31,
	K_AUX32,

	K_MWHEELDOWN,
	K_MWHEELUP,
	K_MWHEELLEFT,
	K_MWHEELRIGHT,

	K_PAUSE			=	255,

	K_MAXKEYS
} keyNum_t;

// Copy of SDL2_TranslateKey from sdl2/sdl_input.c for testing
static keyNum_t SDL2_TranslateKey(SDL_Keycode key)
{
	keyNum_t result;

	// Handle printable ASCII range (32-126)
	if (key >= 32 && key < 127)
		return (keyNum_t)key;

	switch (key)
	{
	case SDLK_ESCAPE:        return K_ESCAPE;
	case SDLK_RETURN:        return K_ENTER;
	case SDLK_TAB:           return K_TAB;
	case SDLK_BACKSPACE:     return K_BACKSPACE;
	case SDLK_SPACE:         return K_SPACE;

	case SDLK_UP:            return K_UPARROW;
	case SDLK_DOWN:          return K_DOWNARROW;
	case SDLK_LEFT:          return K_LEFTARROW;
	case SDLK_RIGHT:         return K_RIGHTARROW;

	case SDLK_LALT:
	case SDLK_RALT:          return K_ALT;

	case SDLK_LCTRL:
	case SDLK_RCTRL:         return K_CTRL;

	case SDLK_LSHIFT:        return K_LSHIFT;
	case SDLK_RSHIFT:        return K_RSHIFT;

	case SDLK_CAPSLOCK:      return K_CAPSLOCK;

	case SDLK_F1:            return K_F1;
	case SDLK_F2:            return K_F2;
	case SDLK_F3:            return K_F3;
	case SDLK_F4:            return K_F4;
	case SDLK_F5:            return K_F5;
	case SDLK_F6:            return K_F6;
	case SDLK_F7:            return K_F7;
	case SDLK_F8:            return K_F8;
	case SDLK_F9:            return K_F9;
	case SDLK_F10:           return K_F10;
	case SDLK_F11:           return K_F11;
	case SDLK_F12:           return K_F12;

	case SDLK_INSERT:        return K_INS;
	case SDLK_DELETE:        return K_DEL;
	case SDLK_PAGEDOWN:      return K_PGDN;
	case SDLK_PAGEUP:        return K_PGUP;
	case SDLK_HOME:          return K_HOME;
	case SDLK_END:           return K_END;

	case SDLK_KP_7:          return K_KP_HOME;
	case SDLK_KP_8:          return K_KP_UPARROW;
	case SDLK_KP_9:          return K_KP_PGUP;
	case SDLK_KP_4:          return K_KP_LEFTARROW;
	case SDLK_KP_5:          return K_KP_FIVE;
	case SDLK_KP_6:          return K_KP_RIGHTARROW;
	case SDLK_KP_1:          return K_KP_END;
	case SDLK_KP_2:          return K_KP_DOWNARROW;
	case SDLK_KP_3:          return K_KP_PGDN;
	case SDLK_KP_ENTER:      return K_KP_ENTER;
	case SDLK_KP_0:          return K_KP_INS;
	case SDLK_KP_PERIOD:     return K_KP_DEL;
	case SDLK_KP_DIVIDE:     return K_KP_SLASH;
	case SDLK_KP_MINUS:      return K_KP_MINUS;
	case SDLK_KP_PLUS:       return K_KP_PLUS;

	case SDLK_PAUSE:         return K_PAUSE;

	default:                 return K_BADKEY;
	}

	return result;
}

// Copy of SDL2_TranslateMouseButton from sdl2/sdl_input.c for testing
static keyNum_t SDL2_TranslateMouseButton(Uint8 button)
{
	switch (button) {
	case SDL_BUTTON_LEFT:    return K_MOUSE1;
	case SDL_BUTTON_RIGHT:   return K_MOUSE2;
	case SDL_BUTTON_MIDDLE:  return K_MOUSE3;
	case SDL_BUTTON_X1:      return K_MOUSE4;
	case SDL_BUTTON_X2:      return K_MOUSE5;
	default:
		return K_BADKEY;
	}
}

// Copy of SDL2_TranslateControllerButton from sdl2/sdl_input.c for testing
static keyNum_t SDL2_TranslateControllerButton(Uint8 button)
{
	int b = (int)button;
	if (b < 0)
		return K_BADKEY;

	if (b < 4)
		return (keyNum_t)(K_JOY1 + b);

	b -= 4;
	if (b >= 0 && b < 28)
		return (keyNum_t)(K_AUX1 + b);

	return K_BADKEY;
}

// Gamepad binding structure from sdl2/sdl_input.c
typedef struct sdl2_gamepad_bind_s {
	keyNum_t key;
	const char *cmd;
} sdl2_gamepad_bind_t;

// Default gamepad bindings (from SDL2_ApplyDefaultGamepadBinds)
static const sdl2_gamepad_bind_t binds_default[] = {
	// Triggers
	{ K_AUX28, "+attack" },
	{ K_AUX27, "+use" },

	// Face buttons
	{ K_JOY1, "+moveup" },
	{ K_JOY2, "+movedown" },
	{ K_JOY3, "invuse" },
	{ K_JOY4, "inven" },

	// Bumpers
	{ K_AUX6, "weapprev" },
	{ K_AUX7, "weapnext" },

	// D-pad
	{ K_AUX8, "invuse" },
	{ K_AUX9, "invdrop" },
	{ K_AUX10, "invprev" },
	{ K_AUX11, "invnext" },

	// Misc
	{ K_AUX1, "score" },
	{ K_AUX4, "centerview" },
	{ K_AUX5, "+speed" },
};

// Zoom preset bindings (alternate)
static const sdl2_gamepad_bind_t binds_zoom[] = {
	// Triggers
	{ K_AUX28, "+attack" },
	{ K_AUX27, "+zoom" },

	// Face buttons
	{ K_JOY1, "+moveup" },
	{ K_JOY2, "+movedown" },
	{ K_JOY3, "+use" },
	{ K_JOY4, "inven" },

	// Bumpers
	{ K_AUX6, "weapprev" },
	{ K_AUX7, "weapnext" },

	// D-pad
	{ K_AUX8, "invuse" },
	{ K_AUX9, "invdrop" },
	{ K_AUX10, "invprev" },
	{ K_AUX11, "invnext" },

	// Misc
	{ K_AUX1, "score" },
	{ K_AUX4, "centerview" },
	{ K_AUX5, "+speed" },
};

#define BINDS_DEFAULT_COUNT (sizeof(binds_default) / sizeof(binds_default[0]))
#define BINDS_ZOOM_COUNT (sizeof(binds_zoom) / sizeof(binds_zoom[0]))

// Helper to find binding by key
static const char* find_binding(const sdl2_gamepad_bind_t *binds, int count, keyNum_t key) {
	for (int i = 0; i < count; i++) {
		if (binds[i].key == key)
			return binds[i].cmd;
	}
	return NULL;
}

// Copy of SDL2_ApplyDeadzone from sdl2/sdl_input.c for testing
static float SDL2_ApplyDeadzone(float x, float dz)
{
	float ax = fabsf(x);
	if (ax <= dz)
		return 0.0f;
	if (dz >= 0.999f)
		return 0.0f;
	return (x > 0.0f ? 1.0f : -1.0f) * ((ax - dz) / (1.0f - dz));
}

// Helper function for float comparison with tolerance
static int float_equals(float a, float b, float epsilon) {
	return fabsf(a - b) < epsilon;
}

// ============================================================================
// TESTS: Gamepad binding system
// ============================================================================

void test_GamepadBinds_DefaultTriggers(void) {
	// Verify trigger bindings in default preset
	const char *rt = find_binding(binds_default, BINDS_DEFAULT_COUNT, K_AUX28);
	const char *lt = find_binding(binds_default, BINDS_DEFAULT_COUNT, K_AUX27);
	
	TEST_ASSERT_NOT_NULL(rt);
	TEST_ASSERT_NOT_NULL(lt);
	TEST_ASSERT_EQUAL_STRING("+attack", rt);
	TEST_ASSERT_EQUAL_STRING("+use", lt);
}

void test_GamepadBinds_ZoomTriggers(void) {
	// Verify trigger bindings in zoom preset
	const char *rt = find_binding(binds_zoom, BINDS_ZOOM_COUNT, K_AUX28);
	const char *lt = find_binding(binds_zoom, BINDS_ZOOM_COUNT, K_AUX27);
	
	TEST_ASSERT_NOT_NULL(rt);
	TEST_ASSERT_NOT_NULL(lt);
	TEST_ASSERT_EQUAL_STRING("+attack", rt);
	TEST_ASSERT_EQUAL_STRING("+zoom", lt);
}

void test_GamepadBinds_FaceButtons(void) {
	// Verify face button bindings (A/B/X/Y or Cross/Circle/Square/Triangle)
	const char *joy1 = find_binding(binds_default, BINDS_DEFAULT_COUNT, K_JOY1);
	const char *joy2 = find_binding(binds_default, BINDS_DEFAULT_COUNT, K_JOY2);
	const char *joy3 = find_binding(binds_default, BINDS_DEFAULT_COUNT, K_JOY3);
	const char *joy4 = find_binding(binds_default, BINDS_DEFAULT_COUNT, K_JOY4);
	
	TEST_ASSERT_NOT_NULL(joy1);
	TEST_ASSERT_NOT_NULL(joy2);
	TEST_ASSERT_NOT_NULL(joy3);
	TEST_ASSERT_NOT_NULL(joy4);
	TEST_ASSERT_EQUAL_STRING("+moveup", joy1);
	TEST_ASSERT_EQUAL_STRING("+movedown", joy2);
	TEST_ASSERT_EQUAL_STRING("invuse", joy3);
	TEST_ASSERT_EQUAL_STRING("inven", joy4);
}

void test_GamepadBinds_ZoomFaceButtons(void) {
	// Verify face button bindings differ in zoom preset (X/Square becomes +use)
	const char *joy3_default = find_binding(binds_default, BINDS_DEFAULT_COUNT, K_JOY3);
	const char *joy3_zoom = find_binding(binds_zoom, BINDS_ZOOM_COUNT, K_JOY3);
	
	TEST_ASSERT_EQUAL_STRING("invuse", joy3_default);
	TEST_ASSERT_EQUAL_STRING("+use", joy3_zoom);
}

void test_GamepadBinds_Bumpers(void) {
	// Verify bumper bindings (LB/RB or L1/R1)
	const char *lb = find_binding(binds_default, BINDS_DEFAULT_COUNT, K_AUX6);
	const char *rb = find_binding(binds_default, BINDS_DEFAULT_COUNT, K_AUX7);
	
	TEST_ASSERT_NOT_NULL(lb);
	TEST_ASSERT_NOT_NULL(rb);
	TEST_ASSERT_EQUAL_STRING("weapprev", lb);
	TEST_ASSERT_EQUAL_STRING("weapnext", rb);
}

void test_GamepadBinds_DPad(void) {
	// Verify D-pad bindings
	const char *up = find_binding(binds_default, BINDS_DEFAULT_COUNT, K_AUX8);
	const char *down = find_binding(binds_default, BINDS_DEFAULT_COUNT, K_AUX9);
	const char *left = find_binding(binds_default, BINDS_DEFAULT_COUNT, K_AUX10);
	const char *right = find_binding(binds_default, BINDS_DEFAULT_COUNT, K_AUX11);
	
	TEST_ASSERT_NOT_NULL(up);
	TEST_ASSERT_NOT_NULL(down);
	TEST_ASSERT_NOT_NULL(left);
	TEST_ASSERT_NOT_NULL(right);
	TEST_ASSERT_EQUAL_STRING("invuse", up);
	TEST_ASSERT_EQUAL_STRING("invdrop", down);
	TEST_ASSERT_EQUAL_STRING("invprev", left);
	TEST_ASSERT_EQUAL_STRING("invnext", right);
}

void test_GamepadBinds_MiscButtons(void) {
	// Verify misc button bindings (Back/Select, Left stick, Right stick)
	const char *back = find_binding(binds_default, BINDS_DEFAULT_COUNT, K_AUX1);
	const char *lstick = find_binding(binds_default, BINDS_DEFAULT_COUNT, K_AUX4);
	const char *rstick = find_binding(binds_default, BINDS_DEFAULT_COUNT, K_AUX5);
	
	TEST_ASSERT_NOT_NULL(back);
	TEST_ASSERT_NOT_NULL(lstick);
	TEST_ASSERT_NOT_NULL(rstick);
	TEST_ASSERT_EQUAL_STRING("score", back);
	TEST_ASSERT_EQUAL_STRING("centerview", lstick);
	TEST_ASSERT_EQUAL_STRING("+speed", rstick);
}

void test_GamepadBinds_BindingCount(void) {
	// Verify expected number of bindings
	TEST_ASSERT_EQUAL(15, BINDS_DEFAULT_COUNT);
	TEST_ASSERT_EQUAL(15, BINDS_ZOOM_COUNT);
}

void test_GamepadBinds_NoNullCommands(void) {
	// Verify no bindings have NULL commands
	for (int i = 0; i < (int)BINDS_DEFAULT_COUNT; i++) {
		TEST_ASSERT_NOT_NULL(binds_default[i].cmd);
		TEST_ASSERT_TRUE(binds_default[i].cmd[0] != '\0');
	}
	for (int i = 0; i < (int)BINDS_ZOOM_COUNT; i++) {
		TEST_ASSERT_NOT_NULL(binds_zoom[i].cmd);
		TEST_ASSERT_TRUE(binds_zoom[i].cmd[0] != '\0');
	}
}

void test_GamepadBinds_ValidKeyNumbers(void) {
	// Verify all keys are valid (JOY1-4 or AUX1-32)
	for (int i = 0; i < (int)BINDS_DEFAULT_COUNT; i++) {
		keyNum_t key = binds_default[i].key;
		int is_joy = (key >= K_JOY1 && key <= K_JOY4);
		int is_aux = (key >= K_AUX1 && key <= K_AUX32);
		TEST_ASSERT_TRUE(is_joy || is_aux);
	}
}

void test_GamepadBinds_UniqueKeys(void) {
	// Verify no duplicate key bindings in default preset
	for (int i = 0; i < (int)BINDS_DEFAULT_COUNT; i++) {
		for (int j = i + 1; j < (int)BINDS_DEFAULT_COUNT; j++) {
			TEST_ASSERT_TRUE(binds_default[i].key != binds_default[j].key);
		}
	}
}

void test_GamepadBinds_AttackCommand(void) {
	// Verify attack command uses + prefix (continuous action)
	const char *rt = find_binding(binds_default, BINDS_DEFAULT_COUNT, K_AUX28);
	TEST_ASSERT_TRUE(rt[0] == '+');
}

void test_GamepadBinds_WeaponSwitching(void) {
	// Verify bumpers control weapon switching
	const char *lb = find_binding(binds_default, BINDS_DEFAULT_COUNT, K_AUX6);
	const char *rb = find_binding(binds_default, BINDS_DEFAULT_COUNT, K_AUX7);
	
	// Both should be weapon commands
	TEST_ASSERT_TRUE(strncmp(lb, "weap", 4) == 0);
	TEST_ASSERT_TRUE(strncmp(rb, "weap", 4) == 0);
}

void test_GamepadBinds_InventoryManagement(void) {
	// Verify D-pad manages inventory
	const char *up = find_binding(binds_default, BINDS_DEFAULT_COUNT, K_AUX8);
	const char *down = find_binding(binds_default, BINDS_DEFAULT_COUNT, K_AUX9);
	const char *left = find_binding(binds_default, BINDS_DEFAULT_COUNT, K_AUX10);
	const char *right = find_binding(binds_default, BINDS_DEFAULT_COUNT, K_AUX11);
	
	// All should be inventory commands
	TEST_ASSERT_TRUE(strncmp(up, "inv", 3) == 0);
	TEST_ASSERT_TRUE(strncmp(down, "inv", 3) == 0);
	TEST_ASSERT_TRUE(strncmp(left, "inv", 3) == 0);
	TEST_ASSERT_TRUE(strncmp(right, "inv", 3) == 0);
}

// ============================================================================
// TESTS: Deadzone application
// ============================================================================

void test_SDL2_ApplyDeadzone_ZeroDeadzone_Zero(void) {
	// With 0.0 deadzone, zero input should return zero
	float result = SDL2_ApplyDeadzone(0.0f, 0.0f);
	TEST_ASSERT_TRUE(float_equals(0.0f, result, 0.001f));
}

void test_SDL2_ApplyDeadzone_ZeroDeadzone_Half(void) {
	// With 0.0 deadzone, input should pass through unchanged
	float result = SDL2_ApplyDeadzone(0.5f, 0.0f);
	TEST_ASSERT_TRUE(float_equals(0.5f, result, 0.001f));
}

void test_SDL2_ApplyDeadzone_ZeroDeadzone_Full(void) {
	// With 0.0 deadzone, full input should pass through
	float result = SDL2_ApplyDeadzone(1.0f, 0.0f);
	TEST_ASSERT_TRUE(float_equals(1.0f, result, 0.001f));
}

void test_SDL2_ApplyDeadzone_ZeroDeadzone_Negative(void) {
	// With 0.0 deadzone, negative input should pass through
	float result = SDL2_ApplyDeadzone(-0.5f, 0.0f);
	TEST_ASSERT_TRUE(float_equals(-0.5f, result, 0.001f));
}

void test_SDL2_ApplyDeadzone_Point15_BelowThreshold(void) {
	// Input below 0.15 deadzone should return zero
	float result = SDL2_ApplyDeadzone(0.10f, 0.15f);
	TEST_ASSERT_TRUE(float_equals(0.0f, result, 0.001f));
}

void test_SDL2_ApplyDeadzone_Point15_AtThreshold(void) {
	// Input exactly at 0.15 deadzone should return zero
	float result = SDL2_ApplyDeadzone(0.15f, 0.15f);
	TEST_ASSERT_TRUE(float_equals(0.0f, result, 0.001f));
}

void test_SDL2_ApplyDeadzone_Point15_SlightlyAbove(void) {
	// Input slightly above 0.15 deadzone should scale to near zero
	// Formula: (0.20 - 0.15) / (1.0 - 0.15) = 0.05 / 0.85 ≈ 0.0588
	float result = SDL2_ApplyDeadzone(0.20f, 0.15f);
	float expected = (0.20f - 0.15f) / (1.0f - 0.15f);
	TEST_ASSERT_TRUE(float_equals(expected, result, 0.001f));
}

void test_SDL2_ApplyDeadzone_Point15_MaxDeflection(void) {
	// Input at max deflection should return 1.0 after scaling
	// Formula: (1.0 - 0.15) / (1.0 - 0.15) = 1.0
	float result = SDL2_ApplyDeadzone(1.0f, 0.15f);
	TEST_ASSERT_TRUE(float_equals(1.0f, result, 0.001f));
}

void test_SDL2_ApplyDeadzone_Point15_NegativeAbove(void) {
	// Negative input beyond deadzone should scale correctly
	// Formula: -(0.50 - 0.15) / (1.0 - 0.15) = -0.35 / 0.85 ≈ -0.4118
	float result = SDL2_ApplyDeadzone(-0.50f, 0.15f);
	float expected = -(0.50f - 0.15f) / (1.0f - 0.15f);
	TEST_ASSERT_TRUE(float_equals(expected, result, 0.001f));
}

void test_SDL2_ApplyDeadzone_Point5_BelowThreshold(void) {
	// Input below 0.5 deadzone should return zero
	float result = SDL2_ApplyDeadzone(0.25f, 0.5f);
	TEST_ASSERT_TRUE(float_equals(0.0f, result, 0.001f));
}

void test_SDL2_ApplyDeadzone_Point5_AtThreshold(void) {
	// Input exactly at 0.5 deadzone should return zero
	float result = SDL2_ApplyDeadzone(0.5f, 0.5f);
	TEST_ASSERT_TRUE(float_equals(0.0f, result, 0.001f));
}

void test_SDL2_ApplyDeadzone_Point5_SlightlyAbove(void) {
	// Input slightly above 0.5 deadzone should scale
	// Formula: (0.55 - 0.5) / (1.0 - 0.5) = 0.05 / 0.5 = 0.1
	float result = SDL2_ApplyDeadzone(0.55f, 0.5f);
	float expected = (0.55f - 0.5f) / (1.0f - 0.5f);
	TEST_ASSERT_TRUE(float_equals(expected, result, 0.001f));
}

void test_SDL2_ApplyDeadzone_Point5_ThreeQuarters(void) {
	// Input at 0.75 with 0.5 deadzone
	// Formula: (0.75 - 0.5) / (1.0 - 0.5) = 0.25 / 0.5 = 0.5
	float result = SDL2_ApplyDeadzone(0.75f, 0.5f);
	float expected = (0.75f - 0.5f) / (1.0f - 0.5f);
	TEST_ASSERT_TRUE(float_equals(expected, result, 0.001f));
}

void test_SDL2_ApplyDeadzone_Point5_MaxDeflection(void) {
	// Input at max deflection should return 1.0 after scaling
	// Formula: (1.0 - 0.5) / (1.0 - 0.5) = 1.0
	float result = SDL2_ApplyDeadzone(1.0f, 0.5f);
	TEST_ASSERT_TRUE(float_equals(1.0f, result, 0.001f));
}

void test_SDL2_ApplyDeadzone_Point95_BelowThreshold(void) {
	// Input below 0.95 deadzone should return zero
	float result = SDL2_ApplyDeadzone(0.8f, 0.95f);
	TEST_ASSERT_TRUE(float_equals(0.0f, result, 0.001f));
}

void test_SDL2_ApplyDeadzone_Point95_AtThreshold(void) {
	// Input exactly at 0.95 deadzone should return zero
	float result = SDL2_ApplyDeadzone(0.95f, 0.95f);
	TEST_ASSERT_TRUE(float_equals(0.0f, result, 0.001f));
}

void test_SDL2_ApplyDeadzone_Point95_SlightlyAbove(void) {
	// Input slightly above 0.95 deadzone should scale significantly
	// Formula: (0.97 - 0.95) / (1.0 - 0.95) = 0.02 / 0.05 = 0.4
	float result = SDL2_ApplyDeadzone(0.97f, 0.95f);
	float expected = (0.97f - 0.95f) / (1.0f - 0.95f);
	TEST_ASSERT_TRUE(float_equals(expected, result, 0.001f));
}

void test_SDL2_ApplyDeadzone_Point95_MaxDeflection(void) {
	// Input at max deflection should return 1.0 after scaling
	// Formula: (1.0 - 0.95) / (1.0 - 0.95) = 1.0
	float result = SDL2_ApplyDeadzone(1.0f, 0.95f);
	TEST_ASSERT_TRUE(float_equals(1.0f, result, 0.001f));
}

void test_SDL2_ApplyDeadzone_Point95_NegativeMax(void) {
	// Negative input at max deflection should return -1.0 after scaling
	float result = SDL2_ApplyDeadzone(-1.0f, 0.95f);
	TEST_ASSERT_TRUE(float_equals(-1.0f, result, 0.001f));
}

void test_SDL2_ApplyDeadzone_ExtremeDeadzone(void) {
	// Deadzone >= 0.999 should always return zero
	float result1 = SDL2_ApplyDeadzone(0.5f, 0.999f);
	float result2 = SDL2_ApplyDeadzone(1.0f, 0.999f);
	TEST_ASSERT_TRUE(float_equals(0.0f, result1, 0.001f));
	TEST_ASSERT_TRUE(float_equals(0.0f, result2, 0.001f));
}

void test_SDL2_ApplyDeadzone_SignPreservation(void) {
	// Verify sign is preserved through deadzone application
	float pos = SDL2_ApplyDeadzone(0.6f, 0.15f);
	float neg = SDL2_ApplyDeadzone(-0.6f, 0.15f);
	TEST_ASSERT_TRUE(pos > 0.0f);
	TEST_ASSERT_TRUE(neg < 0.0f);
	TEST_ASSERT_TRUE(float_equals(pos, -neg, 0.001f));
}

void test_SDL2_ApplyDeadzone_ScalingConsistency(void) {
	// Verify output is always in range [-1, 1]
	float dz = 0.15f;
	for (float x = -1.0f; x <= 1.0f; x += 0.1f) {
		float result = SDL2_ApplyDeadzone(x, dz);
		TEST_ASSERT_TRUE(result >= -1.0f && result <= 1.0f);
	}
}

void test_SDL2_ApplyDeadzone_MidrangeScaling(void) {
	// Verify scaling at various midrange values with 0.15 deadzone
	float inputs[] = {0.3f, 0.5f, 0.7f, 0.9f};
	for (int i = 0; i < 4; i++) {
		float x = inputs[i];
		float result = SDL2_ApplyDeadzone(x, 0.15f);
		float expected = (x - 0.15f) / (1.0f - 0.15f);
		TEST_ASSERT_TRUE(float_equals(expected, result, 0.001f));
	}
}

// ============================================================================
// TESTS: Controller buttons
// ============================================================================

void test_SDL2_TranslateControllerButton_JOY1(void) {
	TEST_ASSERT_EQUAL(K_JOY1, SDL2_TranslateControllerButton(0));
}

void test_SDL2_TranslateControllerButton_JOY2(void) {
	TEST_ASSERT_EQUAL(K_JOY2, SDL2_TranslateControllerButton(1));
}

void test_SDL2_TranslateControllerButton_JOY3(void) {
	TEST_ASSERT_EQUAL(K_JOY3, SDL2_TranslateControllerButton(2));
}

void test_SDL2_TranslateControllerButton_JOY4(void) {
	TEST_ASSERT_EQUAL(K_JOY4, SDL2_TranslateControllerButton(3));
}

void test_SDL2_TranslateControllerButton_AUX1(void) {
	TEST_ASSERT_EQUAL(K_AUX1, SDL2_TranslateControllerButton(4));
}

void test_SDL2_TranslateControllerButton_AUX2(void) {
	TEST_ASSERT_EQUAL(K_AUX2, SDL2_TranslateControllerButton(5));
}

void test_SDL2_TranslateControllerButton_AUX10(void) {
	TEST_ASSERT_EQUAL(K_AUX10, SDL2_TranslateControllerButton(13));
}

void test_SDL2_TranslateControllerButton_AUX20(void) {
	TEST_ASSERT_EQUAL(K_AUX20, SDL2_TranslateControllerButton(23));
}

void test_SDL2_TranslateControllerButton_AUX28(void) {
	TEST_ASSERT_EQUAL(K_AUX28, SDL2_TranslateControllerButton(31));
}

void test_SDL2_TranslateControllerButton_OutOfRange(void) {
	// Button 32 and beyond should return K_BADKEY
	TEST_ASSERT_EQUAL(K_BADKEY, SDL2_TranslateControllerButton(32));
	TEST_ASSERT_EQUAL(K_BADKEY, SDL2_TranslateControllerButton(100));
	TEST_ASSERT_EQUAL(K_BADKEY, SDL2_TranslateControllerButton(255));
}

void test_SDL2_TranslateControllerButton_AllJoyButtons(void) {
	// Verify all K_JOY1-4 buttons are correctly mapped
	TEST_ASSERT_EQUAL(K_JOY1, SDL2_TranslateControllerButton(0));
	TEST_ASSERT_EQUAL(K_JOY2, SDL2_TranslateControllerButton(1));
	TEST_ASSERT_EQUAL(K_JOY3, SDL2_TranslateControllerButton(2));
	TEST_ASSERT_EQUAL(K_JOY4, SDL2_TranslateControllerButton(3));
}

void test_SDL2_TranslateControllerButton_AuxButtonRange(void) {
	// Verify the AUX button range (buttons 4-31 map to K_AUX1-28)
	TEST_ASSERT_EQUAL(K_AUX1, SDL2_TranslateControllerButton(4));
	TEST_ASSERT_EQUAL(K_AUX5, SDL2_TranslateControllerButton(8));
	TEST_ASSERT_EQUAL(K_AUX15, SDL2_TranslateControllerButton(18));
	TEST_ASSERT_EQUAL(K_AUX28, SDL2_TranslateControllerButton(31));
}

void test_SDL2_TranslateControllerButton_BoundaryValues(void) {
	// Test boundary between JOY and AUX ranges
	TEST_ASSERT_EQUAL(K_JOY4, SDL2_TranslateControllerButton(3));  // Last JOY button
	TEST_ASSERT_EQUAL(K_AUX1, SDL2_TranslateControllerButton(4));  // First AUX button
	// Test boundary at end of AUX range
	TEST_ASSERT_EQUAL(K_AUX28, SDL2_TranslateControllerButton(31)); // Last AUX button
	TEST_ASSERT_EQUAL(K_BADKEY, SDL2_TranslateControllerButton(32)); // First invalid button
}

void test_SDL2_TranslateControllerButton_SequentialMapping(void) {
	// Verify sequential buttons map to sequential key numbers
	for (int i = 0; i < 4; i++) {
		TEST_ASSERT_EQUAL(K_JOY1 + i, SDL2_TranslateControllerButton(i));
	}
	for (int i = 0; i < 28; i++) {
		TEST_ASSERT_EQUAL(K_AUX1 + i, SDL2_TranslateControllerButton(4 + i));
	}
}

// ============================================================================
// TESTS: Mouse buttons
// ============================================================================

void test_SDL2_TranslateMouseButton_LeftButton(void) {
	TEST_ASSERT_EQUAL(K_MOUSE1, SDL2_TranslateMouseButton(SDL_BUTTON_LEFT));
}

void test_SDL2_TranslateMouseButton_RightButton(void) {
	TEST_ASSERT_EQUAL(K_MOUSE2, SDL2_TranslateMouseButton(SDL_BUTTON_RIGHT));
}

void test_SDL2_TranslateMouseButton_MiddleButton(void) {
	TEST_ASSERT_EQUAL(K_MOUSE3, SDL2_TranslateMouseButton(SDL_BUTTON_MIDDLE));
}

void test_SDL2_TranslateMouseButton_X1Button(void) {
	TEST_ASSERT_EQUAL(K_MOUSE4, SDL2_TranslateMouseButton(SDL_BUTTON_X1));
}

void test_SDL2_TranslateMouseButton_X2Button(void) {
	TEST_ASSERT_EQUAL(K_MOUSE5, SDL2_TranslateMouseButton(SDL_BUTTON_X2));
}

void test_SDL2_TranslateMouseButton_UnmappedButton(void) {
	// Test that an unmapped button value returns K_BADKEY
	TEST_ASSERT_EQUAL(K_BADKEY, SDL2_TranslateMouseButton(255));
}

void test_SDL2_TranslateMouseButton_AllButtons(void) {
	// Verify all standard mouse buttons are correctly mapped
	TEST_ASSERT_EQUAL(K_MOUSE1, SDL2_TranslateMouseButton(SDL_BUTTON_LEFT));
	TEST_ASSERT_EQUAL(K_MOUSE2, SDL2_TranslateMouseButton(SDL_BUTTON_RIGHT));
	TEST_ASSERT_EQUAL(K_MOUSE3, SDL2_TranslateMouseButton(SDL_BUTTON_MIDDLE));
	TEST_ASSERT_EQUAL(K_MOUSE4, SDL2_TranslateMouseButton(SDL_BUTTON_X1));
	TEST_ASSERT_EQUAL(K_MOUSE5, SDL2_TranslateMouseButton(SDL_BUTTON_X2));
}

// ============================================================================
// TESTS: Special keys
// ============================================================================

void test_SDL2_TranslateKey_Escape(void) {
	TEST_ASSERT_EQUAL(K_ESCAPE, SDL2_TranslateKey(SDLK_ESCAPE));
}

void test_SDL2_TranslateKey_Return(void) {
	TEST_ASSERT_EQUAL(K_ENTER, SDL2_TranslateKey(SDLK_RETURN));
}

void test_SDL2_TranslateKey_Tab(void) {
	TEST_ASSERT_EQUAL(K_TAB, SDL2_TranslateKey(SDLK_TAB));
}

void test_SDL2_TranslateKey_Backspace(void) {
	TEST_ASSERT_EQUAL(K_BACKSPACE, SDL2_TranslateKey(SDLK_BACKSPACE));
}

void test_SDL2_TranslateKey_Space(void) {
	TEST_ASSERT_EQUAL(K_SPACE, SDL2_TranslateKey(SDLK_SPACE));
}

// ============================================================================
// TESTS: Arrow keys
// ============================================================================

void test_SDL2_TranslateKey_UpArrow(void) {
	TEST_ASSERT_EQUAL(K_UPARROW, SDL2_TranslateKey(SDLK_UP));
}

void test_SDL2_TranslateKey_DownArrow(void) {
	TEST_ASSERT_EQUAL(K_DOWNARROW, SDL2_TranslateKey(SDLK_DOWN));
}

void test_SDL2_TranslateKey_LeftArrow(void) {
	TEST_ASSERT_EQUAL(K_LEFTARROW, SDL2_TranslateKey(SDLK_LEFT));
}

void test_SDL2_TranslateKey_RightArrow(void) {
	TEST_ASSERT_EQUAL(K_RIGHTARROW, SDL2_TranslateKey(SDLK_RIGHT));
}

// ============================================================================
// TESTS: Function keys
// ============================================================================

void test_SDL2_TranslateKey_F1(void) {
	TEST_ASSERT_EQUAL(K_F1, SDL2_TranslateKey(SDLK_F1));
}

void test_SDL2_TranslateKey_F2(void) {
	TEST_ASSERT_EQUAL(K_F2, SDL2_TranslateKey(SDLK_F2));
}

void test_SDL2_TranslateKey_F3(void) {
	TEST_ASSERT_EQUAL(K_F3, SDL2_TranslateKey(SDLK_F3));
}

void test_SDL2_TranslateKey_F4(void) {
	TEST_ASSERT_EQUAL(K_F4, SDL2_TranslateKey(SDLK_F4));
}

void test_SDL2_TranslateKey_F5(void) {
	TEST_ASSERT_EQUAL(K_F5, SDL2_TranslateKey(SDLK_F5));
}

void test_SDL2_TranslateKey_F6(void) {
	TEST_ASSERT_EQUAL(K_F6, SDL2_TranslateKey(SDLK_F6));
}

void test_SDL2_TranslateKey_F7(void) {
	TEST_ASSERT_EQUAL(K_F7, SDL2_TranslateKey(SDLK_F7));
}

void test_SDL2_TranslateKey_F8(void) {
	TEST_ASSERT_EQUAL(K_F8, SDL2_TranslateKey(SDLK_F8));
}

void test_SDL2_TranslateKey_F9(void) {
	TEST_ASSERT_EQUAL(K_F9, SDL2_TranslateKey(SDLK_F9));
}

void test_SDL2_TranslateKey_F10(void) {
	TEST_ASSERT_EQUAL(K_F10, SDL2_TranslateKey(SDLK_F10));
}

void test_SDL2_TranslateKey_F11(void) {
	TEST_ASSERT_EQUAL(K_F11, SDL2_TranslateKey(SDLK_F11));
}

void test_SDL2_TranslateKey_F12(void) {
	TEST_ASSERT_EQUAL(K_F12, SDL2_TranslateKey(SDLK_F12));
}

// ============================================================================
// TESTS: Modifier keys
// ============================================================================

void test_SDL2_TranslateKey_LeftAlt(void) {
	TEST_ASSERT_EQUAL(K_ALT, SDL2_TranslateKey(SDLK_LALT));
}

void test_SDL2_TranslateKey_RightAlt(void) {
	TEST_ASSERT_EQUAL(K_ALT, SDL2_TranslateKey(SDLK_RALT));
}

void test_SDL2_TranslateKey_LeftCtrl(void) {
	TEST_ASSERT_EQUAL(K_CTRL, SDL2_TranslateKey(SDLK_LCTRL));
}

void test_SDL2_TranslateKey_RightCtrl(void) {
	TEST_ASSERT_EQUAL(K_CTRL, SDL2_TranslateKey(SDLK_RCTRL));
}

void test_SDL2_TranslateKey_LeftShift(void) {
	TEST_ASSERT_EQUAL(K_LSHIFT, SDL2_TranslateKey(SDLK_LSHIFT));
}

void test_SDL2_TranslateKey_RightShift(void) {
	TEST_ASSERT_EQUAL(K_RSHIFT, SDL2_TranslateKey(SDLK_RSHIFT));
}

void test_SDL2_TranslateKey_CapsLock(void) {
	TEST_ASSERT_EQUAL(K_CAPSLOCK, SDL2_TranslateKey(SDLK_CAPSLOCK));
}

// ============================================================================
// TESTS: Navigation keys
// ============================================================================

void test_SDL2_TranslateKey_Insert(void) {
	TEST_ASSERT_EQUAL(K_INS, SDL2_TranslateKey(SDLK_INSERT));
}

void test_SDL2_TranslateKey_Delete(void) {
	TEST_ASSERT_EQUAL(K_DEL, SDL2_TranslateKey(SDLK_DELETE));
}

void test_SDL2_TranslateKey_Home(void) {
	TEST_ASSERT_EQUAL(K_HOME, SDL2_TranslateKey(SDLK_HOME));
}

void test_SDL2_TranslateKey_End(void) {
	TEST_ASSERT_EQUAL(K_END, SDL2_TranslateKey(SDLK_END));
}

void test_SDL2_TranslateKey_PageUp(void) {
	TEST_ASSERT_EQUAL(K_PGUP, SDL2_TranslateKey(SDLK_PAGEUP));
}

void test_SDL2_TranslateKey_PageDown(void) {
	TEST_ASSERT_EQUAL(K_PGDN, SDL2_TranslateKey(SDLK_PAGEDOWN));
}

void test_SDL2_TranslateKey_Pause(void) {
	TEST_ASSERT_EQUAL(K_PAUSE, SDL2_TranslateKey(SDLK_PAUSE));
}

// ============================================================================
// TESTS: Keypad keys
// ============================================================================

void test_SDL2_TranslateKey_KP_Enter(void) {
	TEST_ASSERT_EQUAL(K_KP_ENTER, SDL2_TranslateKey(SDLK_KP_ENTER));
}

void test_SDL2_TranslateKey_KP_0(void) {
	TEST_ASSERT_EQUAL(K_KP_INS, SDL2_TranslateKey(SDLK_KP_0));
}

void test_SDL2_TranslateKey_KP_1(void) {
	TEST_ASSERT_EQUAL(K_KP_END, SDL2_TranslateKey(SDLK_KP_1));
}

void test_SDL2_TranslateKey_KP_2(void) {
	TEST_ASSERT_EQUAL(K_KP_DOWNARROW, SDL2_TranslateKey(SDLK_KP_2));
}

void test_SDL2_TranslateKey_KP_3(void) {
	TEST_ASSERT_EQUAL(K_KP_PGDN, SDL2_TranslateKey(SDLK_KP_3));
}

void test_SDL2_TranslateKey_KP_4(void) {
	TEST_ASSERT_EQUAL(K_KP_LEFTARROW, SDL2_TranslateKey(SDLK_KP_4));
}

void test_SDL2_TranslateKey_KP_5(void) {
	TEST_ASSERT_EQUAL(K_KP_FIVE, SDL2_TranslateKey(SDLK_KP_5));
}

void test_SDL2_TranslateKey_KP_6(void) {
	TEST_ASSERT_EQUAL(K_KP_RIGHTARROW, SDL2_TranslateKey(SDLK_KP_6));
}

void test_SDL2_TranslateKey_KP_7(void) {
	TEST_ASSERT_EQUAL(K_KP_HOME, SDL2_TranslateKey(SDLK_KP_7));
}

void test_SDL2_TranslateKey_KP_8(void) {
	TEST_ASSERT_EQUAL(K_KP_UPARROW, SDL2_TranslateKey(SDLK_KP_8));
}

void test_SDL2_TranslateKey_KP_9(void) {
	TEST_ASSERT_EQUAL(K_KP_PGUP, SDL2_TranslateKey(SDLK_KP_9));
}

void test_SDL2_TranslateKey_KP_Period(void) {
	TEST_ASSERT_EQUAL(K_KP_DEL, SDL2_TranslateKey(SDLK_KP_PERIOD));
}

void test_SDL2_TranslateKey_KP_Divide(void) {
	TEST_ASSERT_EQUAL(K_KP_SLASH, SDL2_TranslateKey(SDLK_KP_DIVIDE));
}

void test_SDL2_TranslateKey_KP_Minus(void) {
	TEST_ASSERT_EQUAL(K_KP_MINUS, SDL2_TranslateKey(SDLK_KP_MINUS));
}

void test_SDL2_TranslateKey_KP_Plus(void) {
	TEST_ASSERT_EQUAL(K_KP_PLUS, SDL2_TranslateKey(SDLK_KP_PLUS));
}

// ============================================================================
// TESTS: Printable ASCII characters
// ============================================================================

void test_SDL2_TranslateKey_ASCII_Lowercase(void) {
	// Test lowercase letters a-z (ASCII 97-122)
	TEST_ASSERT_EQUAL('a', SDL2_TranslateKey('a'));
	TEST_ASSERT_EQUAL('m', SDL2_TranslateKey('m'));
	TEST_ASSERT_EQUAL('z', SDL2_TranslateKey('z'));
}

void test_SDL2_TranslateKey_ASCII_Uppercase(void) {
	// Test uppercase letters A-Z (ASCII 65-90)
	TEST_ASSERT_EQUAL('A', SDL2_TranslateKey('A'));
	TEST_ASSERT_EQUAL('M', SDL2_TranslateKey('M'));
	TEST_ASSERT_EQUAL('Z', SDL2_TranslateKey('Z'));
}

void test_SDL2_TranslateKey_ASCII_Digits(void) {
	// Test digits 0-9 (ASCII 48-57)
	TEST_ASSERT_EQUAL('0', SDL2_TranslateKey('0'));
	TEST_ASSERT_EQUAL('5', SDL2_TranslateKey('5'));
	TEST_ASSERT_EQUAL('9', SDL2_TranslateKey('9'));
}

void test_SDL2_TranslateKey_ASCII_Symbols(void) {
	// Test common symbols
	TEST_ASSERT_EQUAL('!', SDL2_TranslateKey('!'));
	TEST_ASSERT_EQUAL('@', SDL2_TranslateKey('@'));
	TEST_ASSERT_EQUAL('#', SDL2_TranslateKey('#'));
	TEST_ASSERT_EQUAL('$', SDL2_TranslateKey('$'));
	TEST_ASSERT_EQUAL('%', SDL2_TranslateKey('%'));
	TEST_ASSERT_EQUAL('[', SDL2_TranslateKey('['));
	TEST_ASSERT_EQUAL(']', SDL2_TranslateKey(']'));
	TEST_ASSERT_EQUAL('/', SDL2_TranslateKey('/'));
	TEST_ASSERT_EQUAL('\\', SDL2_TranslateKey('\\'));
	TEST_ASSERT_EQUAL('=', SDL2_TranslateKey('='));
	TEST_ASSERT_EQUAL('-', SDL2_TranslateKey('-'));
	TEST_ASSERT_EQUAL('`', SDL2_TranslateKey('`'));
	TEST_ASSERT_EQUAL('~', SDL2_TranslateKey('~'));
}

void test_SDL2_TranslateKey_ASCII_BoundaryValues(void) {
	// Test boundary values of printable ASCII range (32-126)
	TEST_ASSERT_EQUAL(32, SDL2_TranslateKey(32));   // Space
	TEST_ASSERT_EQUAL(126, SDL2_TranslateKey(126)); // Tilde '~'
}

// ============================================================================
// TESTS: Unmapped keys
// ============================================================================

void test_SDL2_TranslateKey_UnmappedKey(void) {
	// Test that unmapped keys return K_BADKEY
	// SDLK_NUMLOCKCLEAR is not mapped
	TEST_ASSERT_EQUAL(K_BADKEY, SDL2_TranslateKey(SDLK_NUMLOCKCLEAR));
}

void test_SDL2_TranslateKey_NonPrintableASCII(void) {
	// Test keys outside printable ASCII range that aren't specially mapped
	// ASCII 31 (Unit Separator) and 127+ outside our special cases
	TEST_ASSERT_EQUAL(K_BADKEY, SDL2_TranslateKey(31));
	TEST_ASSERT_EQUAL(K_BADKEY, SDL2_TranslateKey(128));
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main(int argc, char *argv[]) {
	(void)argc;
	(void)argv;
	UNITY_BEGIN();

	// Gamepad binding system
	RUN_TEST(test_GamepadBinds_DefaultTriggers);
	RUN_TEST(test_GamepadBinds_ZoomTriggers);
	RUN_TEST(test_GamepadBinds_FaceButtons);
	RUN_TEST(test_GamepadBinds_ZoomFaceButtons);
	RUN_TEST(test_GamepadBinds_Bumpers);
	RUN_TEST(test_GamepadBinds_DPad);
	RUN_TEST(test_GamepadBinds_MiscButtons);
	RUN_TEST(test_GamepadBinds_BindingCount);
	RUN_TEST(test_GamepadBinds_NoNullCommands);
	RUN_TEST(test_GamepadBinds_ValidKeyNumbers);
	RUN_TEST(test_GamepadBinds_UniqueKeys);
	RUN_TEST(test_GamepadBinds_AttackCommand);
	RUN_TEST(test_GamepadBinds_WeaponSwitching);
	RUN_TEST(test_GamepadBinds_InventoryManagement);

	// Deadzone application
	RUN_TEST(test_SDL2_ApplyDeadzone_ZeroDeadzone_Zero);
	RUN_TEST(test_SDL2_ApplyDeadzone_ZeroDeadzone_Half);
	RUN_TEST(test_SDL2_ApplyDeadzone_ZeroDeadzone_Full);
	RUN_TEST(test_SDL2_ApplyDeadzone_ZeroDeadzone_Negative);
	RUN_TEST(test_SDL2_ApplyDeadzone_Point15_BelowThreshold);
	RUN_TEST(test_SDL2_ApplyDeadzone_Point15_AtThreshold);
	RUN_TEST(test_SDL2_ApplyDeadzone_Point15_SlightlyAbove);
	RUN_TEST(test_SDL2_ApplyDeadzone_Point15_MaxDeflection);
	RUN_TEST(test_SDL2_ApplyDeadzone_Point15_NegativeAbove);
	RUN_TEST(test_SDL2_ApplyDeadzone_Point5_BelowThreshold);
	RUN_TEST(test_SDL2_ApplyDeadzone_Point5_AtThreshold);
	RUN_TEST(test_SDL2_ApplyDeadzone_Point5_SlightlyAbove);
	RUN_TEST(test_SDL2_ApplyDeadzone_Point5_ThreeQuarters);
	RUN_TEST(test_SDL2_ApplyDeadzone_Point5_MaxDeflection);
	RUN_TEST(test_SDL2_ApplyDeadzone_Point95_BelowThreshold);
	RUN_TEST(test_SDL2_ApplyDeadzone_Point95_AtThreshold);
	RUN_TEST(test_SDL2_ApplyDeadzone_Point95_SlightlyAbove);
	RUN_TEST(test_SDL2_ApplyDeadzone_Point95_MaxDeflection);
	RUN_TEST(test_SDL2_ApplyDeadzone_Point95_NegativeMax);
	RUN_TEST(test_SDL2_ApplyDeadzone_ExtremeDeadzone);
	RUN_TEST(test_SDL2_ApplyDeadzone_SignPreservation);
	RUN_TEST(test_SDL2_ApplyDeadzone_ScalingConsistency);
	RUN_TEST(test_SDL2_ApplyDeadzone_MidrangeScaling);

	// Controller buttons
	RUN_TEST(test_SDL2_TranslateControllerButton_JOY1);
	RUN_TEST(test_SDL2_TranslateControllerButton_JOY2);
	RUN_TEST(test_SDL2_TranslateControllerButton_JOY3);
	RUN_TEST(test_SDL2_TranslateControllerButton_JOY4);
	RUN_TEST(test_SDL2_TranslateControllerButton_AUX1);
	RUN_TEST(test_SDL2_TranslateControllerButton_AUX2);
	RUN_TEST(test_SDL2_TranslateControllerButton_AUX10);
	RUN_TEST(test_SDL2_TranslateControllerButton_AUX20);
	RUN_TEST(test_SDL2_TranslateControllerButton_AUX28);
	RUN_TEST(test_SDL2_TranslateControllerButton_OutOfRange);
	RUN_TEST(test_SDL2_TranslateControllerButton_AllJoyButtons);
	RUN_TEST(test_SDL2_TranslateControllerButton_AuxButtonRange);
	RUN_TEST(test_SDL2_TranslateControllerButton_BoundaryValues);
	RUN_TEST(test_SDL2_TranslateControllerButton_SequentialMapping);

	// Mouse buttons
	RUN_TEST(test_SDL2_TranslateMouseButton_LeftButton);
	RUN_TEST(test_SDL2_TranslateMouseButton_RightButton);
	RUN_TEST(test_SDL2_TranslateMouseButton_MiddleButton);
	RUN_TEST(test_SDL2_TranslateMouseButton_X1Button);
	RUN_TEST(test_SDL2_TranslateMouseButton_X2Button);
	RUN_TEST(test_SDL2_TranslateMouseButton_UnmappedButton);
	RUN_TEST(test_SDL2_TranslateMouseButton_AllButtons);

	// Special keys
	RUN_TEST(test_SDL2_TranslateKey_Escape);
	RUN_TEST(test_SDL2_TranslateKey_Return);
	RUN_TEST(test_SDL2_TranslateKey_Tab);
	RUN_TEST(test_SDL2_TranslateKey_Backspace);
	RUN_TEST(test_SDL2_TranslateKey_Space);

	// Arrow keys
	RUN_TEST(test_SDL2_TranslateKey_UpArrow);
	RUN_TEST(test_SDL2_TranslateKey_DownArrow);
	RUN_TEST(test_SDL2_TranslateKey_LeftArrow);
	RUN_TEST(test_SDL2_TranslateKey_RightArrow);

	// Function keys
	RUN_TEST(test_SDL2_TranslateKey_F1);
	RUN_TEST(test_SDL2_TranslateKey_F2);
	RUN_TEST(test_SDL2_TranslateKey_F3);
	RUN_TEST(test_SDL2_TranslateKey_F4);
	RUN_TEST(test_SDL2_TranslateKey_F5);
	RUN_TEST(test_SDL2_TranslateKey_F6);
	RUN_TEST(test_SDL2_TranslateKey_F7);
	RUN_TEST(test_SDL2_TranslateKey_F8);
	RUN_TEST(test_SDL2_TranslateKey_F9);
	RUN_TEST(test_SDL2_TranslateKey_F10);
	RUN_TEST(test_SDL2_TranslateKey_F11);
	RUN_TEST(test_SDL2_TranslateKey_F12);

	// Modifier keys
	RUN_TEST(test_SDL2_TranslateKey_LeftAlt);
	RUN_TEST(test_SDL2_TranslateKey_RightAlt);
	RUN_TEST(test_SDL2_TranslateKey_LeftCtrl);
	RUN_TEST(test_SDL2_TranslateKey_RightCtrl);
	RUN_TEST(test_SDL2_TranslateKey_LeftShift);
	RUN_TEST(test_SDL2_TranslateKey_RightShift);
	RUN_TEST(test_SDL2_TranslateKey_CapsLock);

	// Navigation keys
	RUN_TEST(test_SDL2_TranslateKey_Insert);
	RUN_TEST(test_SDL2_TranslateKey_Delete);
	RUN_TEST(test_SDL2_TranslateKey_Home);
	RUN_TEST(test_SDL2_TranslateKey_End);
	RUN_TEST(test_SDL2_TranslateKey_PageUp);
	RUN_TEST(test_SDL2_TranslateKey_PageDown);
	RUN_TEST(test_SDL2_TranslateKey_Pause);

	// Keypad keys
	RUN_TEST(test_SDL2_TranslateKey_KP_Enter);
	RUN_TEST(test_SDL2_TranslateKey_KP_0);
	RUN_TEST(test_SDL2_TranslateKey_KP_1);
	RUN_TEST(test_SDL2_TranslateKey_KP_2);
	RUN_TEST(test_SDL2_TranslateKey_KP_3);
	RUN_TEST(test_SDL2_TranslateKey_KP_4);
	RUN_TEST(test_SDL2_TranslateKey_KP_5);
	RUN_TEST(test_SDL2_TranslateKey_KP_6);
	RUN_TEST(test_SDL2_TranslateKey_KP_7);
	RUN_TEST(test_SDL2_TranslateKey_KP_8);
	RUN_TEST(test_SDL2_TranslateKey_KP_9);
	RUN_TEST(test_SDL2_TranslateKey_KP_Period);
	RUN_TEST(test_SDL2_TranslateKey_KP_Divide);
	RUN_TEST(test_SDL2_TranslateKey_KP_Minus);
	RUN_TEST(test_SDL2_TranslateKey_KP_Plus);

	// ASCII characters
	RUN_TEST(test_SDL2_TranslateKey_ASCII_Lowercase);
	RUN_TEST(test_SDL2_TranslateKey_ASCII_Uppercase);
	RUN_TEST(test_SDL2_TranslateKey_ASCII_Digits);
	RUN_TEST(test_SDL2_TranslateKey_ASCII_Symbols);
	RUN_TEST(test_SDL2_TranslateKey_ASCII_BoundaryValues);

	// Unmapped keys
	RUN_TEST(test_SDL2_TranslateKey_UnmappedKey);
	RUN_TEST(test_SDL2_TranslateKey_NonPrintableASCII);

	return UNITY_END();
}
