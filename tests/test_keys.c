/*
 * test_keys.c - Unit tests for key name to keynum conversion
 */

#include "../unity/unity.h"
#include <string.h>
#include <ctype.h>

// Key enum values from cg_shared.h
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

// ============================================================================
// Mock Key System
// ============================================================================

typedef struct keyName_s {
	char *name;
	keyNum_t keyNum;
} keyName_t;

// Key name mapping table (from cl_keys.c)
static keyName_t key_Names[] = {
	{"TAB",			K_TAB},
	{"ENTER",		K_ENTER},
	{"ESCAPE",		K_ESCAPE},
	{"SPACE",		K_SPACE},
	{"BACKSPACE",	K_BACKSPACE},

	{"UPARROW",		K_UPARROW},
	{"DOWNARROW",	K_DOWNARROW},
	{"LEFTARROW",	K_LEFTARROW},
	{"RIGHTARROW",	K_RIGHTARROW},

	{"ALT",			K_ALT},
	{"CTRL",		K_CTRL},

	{"SHIFT",		K_SHIFT},
	{"LSHIFT",		K_LSHIFT},
	{"RSHIFT",		K_RSHIFT},
	
	{"F1",			K_F1},
	{"F2",			K_F2},
	{"F3",			K_F3},
	{"F4",			K_F4},
	{"F5",			K_F5},
	{"F6",			K_F6},
	{"F7",			K_F7},
	{"F8",			K_F8},
	{"F9",			K_F9},
	{"F10",			K_F10},
	{"F11",			K_F11},
	{"F12",			K_F12},

	{"INS",			K_INS},
	{"DEL",			K_DEL},
	{"PGDN",		K_PGDN},
	{"PGUP",		K_PGUP},
	{"HOME",		K_HOME},
	{"END",			K_END},

	{"MOUSE1",		K_MOUSE1},
	{"MOUSE2",		K_MOUSE2},
	{"MOUSE3",		K_MOUSE3},
	{"MOUSE4",		K_MOUSE4},
	{"MOUSE5",		K_MOUSE5},

	{"JOY1",		K_JOY1},
	{"JOY2",		K_JOY2},
	{"JOY3",		K_JOY3},
	{"JOY4",		K_JOY4},

	{"AUX1",		K_AUX1},
	{"AUX2",		K_AUX2},
	{"AUX3",		K_AUX3},
	{"AUX4",		K_AUX4},
	{"AUX5",		K_AUX5},
	{"AUX6",		K_AUX6},
	{"AUX7",		K_AUX7},
	{"AUX8",		K_AUX8},
	{"AUX9",		K_AUX9},
	{"AUX10",		K_AUX10},
	{"AUX11",		K_AUX11},
	{"AUX12",		K_AUX12},
	{"AUX13",		K_AUX13},
	{"AUX14",		K_AUX14},
	{"AUX15",		K_AUX15},
	{"AUX16",		K_AUX16},
	{"AUX17",		K_AUX17},
	{"AUX18",		K_AUX18},
	{"AUX19",		K_AUX19},
	{"AUX20",		K_AUX20},
	{"AUX21",		K_AUX21},
	{"AUX22",		K_AUX22},
	{"AUX23",		K_AUX23},
	{"AUX24",		K_AUX24},
	{"AUX25",		K_AUX25},
	{"AUX26",		K_AUX26},
	{"AUX27",		K_AUX27},
	{"AUX28",		K_AUX28},
	{"AUX29",		K_AUX29},
	{"AUX30",		K_AUX30},
	{"AUX31",		K_AUX31},
	{"AUX32",		K_AUX32},

	{"KP_HOME",			K_KP_HOME},
	{"KP_UPARROW",		K_KP_UPARROW},
	{"KP_PGUP",			K_KP_PGUP},
	{"KP_LEFTARROW",	K_KP_LEFTARROW},
	{"KP_5",			K_KP_FIVE},
	{"KP_RIGHTARROW",	K_KP_RIGHTARROW},
	{"KP_END",			K_KP_END},
	{"KP_DOWNARROW",	K_KP_DOWNARROW},
	{"KP_PGDN",			K_KP_PGDN},
	{"KP_ENTER",		K_KP_ENTER},
	{"KP_INS",			K_KP_INS},
	{"KP_DEL",			K_KP_DEL},
	{"KP_SLASH",		K_KP_SLASH},
	{"KP_MINUS",		K_KP_MINUS},
	{"KP_PLUS",			K_KP_PLUS},

	{"MWHEELUP",	K_MWHEELUP},
	{"MWHEELDOWN",	K_MWHEELDOWN},
	{"MWHEELLEFT",	K_MWHEELLEFT},
	{"MWHEELRIGHT",	K_MWHEELRIGHT},

	{"PAUSE",		K_PAUSE},

	{"SEMICOLON",	';'},

	{NULL,			0}
};

// Case-insensitive string compare
static int Q_stricmp(const char *s1, const char *s2) {
	while (*s1 && *s2) {
		if (tolower(*s1) != tolower(*s2))
			return tolower(*s1) - tolower(*s2);
		s1++;
		s2++;
	}
	return tolower(*s1) - tolower(*s2);
}

// Mock implementation of Key_StringToKeynum
static keyNum_t Test_Key_StringToKeynum(char *str) {
	keyName_t *kn;
	
	if (!str || !str[0])
		return -1;
	if (!str[1])
		return (int)(unsigned char)str[0];

	for (kn = key_Names; kn->name; kn++) {
		if (!Q_stricmp(str, kn->name))
			return kn->keyNum;
	}

	return -1;
}

// Mock implementation of Key_KeynumToString
static const char* Test_Key_KeynumToString(keyNum_t keyNum) {
	keyName_t *kn;
	static char tinystr[2];
	
	if (keyNum == -1)
		return "<KEY NOT FOUND>";

	if (keyNum > 32 && keyNum < 127) {
		// Printable ascii
		tinystr[0] = keyNum;
		tinystr[1] = 0;
		return tinystr;
	}
	
	for (kn = key_Names; kn->name; kn++) {
		if (keyNum == kn->keyNum)
			return kn->name;
	}

	return "<UNKNOWN KEYNUM>";
}

// Mock key binding storage
#define MAX_KEYS 256
static char* key_bindings[MAX_KEYS];

// Helper to reset all bindings
static void Test_ResetBindings(void) {
	int i;
	for (i = 0; i < MAX_KEYS; i++) {
		if (key_bindings[i]) {
			free(key_bindings[i]);
			key_bindings[i] = NULL;
		}
	}
}

// Mock implementation of Key_SetBinding
static void Test_Key_SetBinding(keyNum_t keyNum, const char *binding) {
	if (keyNum < 0 || keyNum >= MAX_KEYS)
		return;
	
	// Free existing binding
	if (key_bindings[keyNum]) {
		free(key_bindings[keyNum]);
		key_bindings[keyNum] = NULL;
	}
	
	// Set new binding (if not NULL or empty)
	if (binding && binding[0]) {
		key_bindings[keyNum] = strdup(binding);
	}
}

// Mock implementation of Key_GetBinding
static const char* Test_Key_GetBinding(keyNum_t keyNum) {
	if (keyNum < 0 || keyNum >= MAX_KEYS)
		return "";
	
	if (key_bindings[keyNum])
		return key_bindings[keyNum];
	
	return "";
}

// ============================================================================
// TESTS: Basic Key Names
// ============================================================================

// Test: convert ESCAPE key
void test_Key_StringToKeynum_Escape(void) {
	keyNum_t result = Test_Key_StringToKeynum("ESCAPE");
	
	TEST_ASSERT_EQUAL(K_ESCAPE, result);
}

// Test: convert ENTER key
void test_Key_StringToKeynum_Enter(void) {
	keyNum_t result = Test_Key_StringToKeynum("ENTER");
	
	TEST_ASSERT_EQUAL(K_ENTER, result);
}

// Test: convert TAB key
void test_Key_StringToKeynum_Tab(void) {
	keyNum_t result = Test_Key_StringToKeynum("TAB");
	
	TEST_ASSERT_EQUAL(K_TAB, result);
}

// Test: convert SPACE key
void test_Key_StringToKeynum_Space(void) {
	keyNum_t result = Test_Key_StringToKeynum("SPACE");
	
	TEST_ASSERT_EQUAL(K_SPACE, result);
}

// Test: convert BACKSPACE key
void test_Key_StringToKeynum_Backspace(void) {
	keyNum_t result = Test_Key_StringToKeynum("BACKSPACE");
	
	TEST_ASSERT_EQUAL(K_BACKSPACE, result);
}

// ============================================================================
// TESTS: Mouse Buttons
// ============================================================================

// Test: convert MOUSE1
void test_Key_StringToKeynum_Mouse1(void) {
	keyNum_t result = Test_Key_StringToKeynum("MOUSE1");
	
	TEST_ASSERT_EQUAL(K_MOUSE1, result);
}

// Test: convert MOUSE2
void test_Key_StringToKeynum_Mouse2(void) {
	keyNum_t result = Test_Key_StringToKeynum("MOUSE2");
	
	TEST_ASSERT_EQUAL(K_MOUSE2, result);
}

// Test: convert MOUSE3
void test_Key_StringToKeynum_Mouse3(void) {
	keyNum_t result = Test_Key_StringToKeynum("MOUSE3");
	
	TEST_ASSERT_EQUAL(K_MOUSE3, result);
}

// Test: convert MOUSE4
void test_Key_StringToKeynum_Mouse4(void) {
	keyNum_t result = Test_Key_StringToKeynum("MOUSE4");
	
	TEST_ASSERT_EQUAL(K_MOUSE4, result);
}

// Test: convert MOUSE5
void test_Key_StringToKeynum_Mouse5(void) {
	keyNum_t result = Test_Key_StringToKeynum("MOUSE5");
	
	TEST_ASSERT_EQUAL(K_MOUSE5, result);
}

// ============================================================================
// TESTS: Joystick Buttons
// ============================================================================

// Test: convert JOY1
void test_Key_StringToKeynum_Joy1(void) {
	keyNum_t result = Test_Key_StringToKeynum("JOY1");
	
	TEST_ASSERT_EQUAL(K_JOY1, result);
}

// Test: convert JOY2
void test_Key_StringToKeynum_Joy2(void) {
	keyNum_t result = Test_Key_StringToKeynum("JOY2");
	
	TEST_ASSERT_EQUAL(K_JOY2, result);
}

// Test: convert JOY3
void test_Key_StringToKeynum_Joy3(void) {
	keyNum_t result = Test_Key_StringToKeynum("JOY3");
	
	TEST_ASSERT_EQUAL(K_JOY3, result);
}

// Test: convert JOY4
void test_Key_StringToKeynum_Joy4(void) {
	keyNum_t result = Test_Key_StringToKeynum("JOY4");
	
	TEST_ASSERT_EQUAL(K_JOY4, result);
}

// ============================================================================
// TESTS: Auxiliary Keys
// ============================================================================

// Test: convert AUX1
void test_Key_StringToKeynum_Aux1(void) {
	keyNum_t result = Test_Key_StringToKeynum("AUX1");
	
	TEST_ASSERT_EQUAL(K_AUX1, result);
}

// Test: convert AUX5
void test_Key_StringToKeynum_Aux5(void) {
	keyNum_t result = Test_Key_StringToKeynum("AUX5");
	
	TEST_ASSERT_EQUAL(K_AUX5, result);
}

// Test: convert AUX10
void test_Key_StringToKeynum_Aux10(void) {
	keyNum_t result = Test_Key_StringToKeynum("AUX10");
	
	TEST_ASSERT_EQUAL(K_AUX10, result);
}

// Test: convert AUX20
void test_Key_StringToKeynum_Aux20(void) {
	keyNum_t result = Test_Key_StringToKeynum("AUX20");
	
	TEST_ASSERT_EQUAL(K_AUX20, result);
}

// Test: convert AUX32
void test_Key_StringToKeynum_Aux32(void) {
	keyNum_t result = Test_Key_StringToKeynum("AUX32");
	
	TEST_ASSERT_EQUAL(K_AUX32, result);
}

// ============================================================================
// TESTS: Function Keys
// ============================================================================

// Test: convert F1
void test_Key_StringToKeynum_F1(void) {
	keyNum_t result = Test_Key_StringToKeynum("F1");
	
	TEST_ASSERT_EQUAL(K_F1, result);
}

// Test: convert F5
void test_Key_StringToKeynum_F5(void) {
	keyNum_t result = Test_Key_StringToKeynum("F5");
	
	TEST_ASSERT_EQUAL(K_F5, result);
}

// Test: convert F12
void test_Key_StringToKeynum_F12(void) {
	keyNum_t result = Test_Key_StringToKeynum("F12");
	
	TEST_ASSERT_EQUAL(K_F12, result);
}

// ============================================================================
// TESTS: Arrow Keys
// ============================================================================

// Test: convert UPARROW
void test_Key_StringToKeynum_UpArrow(void) {
	keyNum_t result = Test_Key_StringToKeynum("UPARROW");
	
	TEST_ASSERT_EQUAL(K_UPARROW, result);
}

// Test: convert DOWNARROW
void test_Key_StringToKeynum_DownArrow(void) {
	keyNum_t result = Test_Key_StringToKeynum("DOWNARROW");
	
	TEST_ASSERT_EQUAL(K_DOWNARROW, result);
}

// Test: convert LEFTARROW
void test_Key_StringToKeynum_LeftArrow(void) {
	keyNum_t result = Test_Key_StringToKeynum("LEFTARROW");
	
	TEST_ASSERT_EQUAL(K_LEFTARROW, result);
}

// Test: convert RIGHTARROW
void test_Key_StringToKeynum_RightArrow(void) {
	keyNum_t result = Test_Key_StringToKeynum("RIGHTARROW");
	
	TEST_ASSERT_EQUAL(K_RIGHTARROW, result);
}

// ============================================================================
// TESTS: Case Insensitivity
// ============================================================================

// Test: lowercase escape
void test_Key_StringToKeynum_LowercaseEscape(void) {
	keyNum_t result = Test_Key_StringToKeynum("escape");
	
	TEST_ASSERT_EQUAL(K_ESCAPE, result);
}

// Test: lowercase mouse1
void test_Key_StringToKeynum_LowercaseMouse1(void) {
	keyNum_t result = Test_Key_StringToKeynum("mouse1");
	
	TEST_ASSERT_EQUAL(K_MOUSE1, result);
}

// Test: lowercase joy1
void test_Key_StringToKeynum_LowercaseJoy1(void) {
	keyNum_t result = Test_Key_StringToKeynum("joy1");
	
	TEST_ASSERT_EQUAL(K_JOY1, result);
}

// Test: lowercase aux5
void test_Key_StringToKeynum_LowercaseAux5(void) {
	keyNum_t result = Test_Key_StringToKeynum("aux5");
	
	TEST_ASSERT_EQUAL(K_AUX5, result);
}

// Test: mixed case
void test_Key_StringToKeynum_MixedCase(void) {
	keyNum_t result1 = Test_Key_StringToKeynum("EsCaPe");
	keyNum_t result2 = Test_Key_StringToKeynum("MoUsE3");
	keyNum_t result3 = Test_Key_StringToKeynum("JoY2");
	
	TEST_ASSERT_EQUAL(K_ESCAPE, result1);
	TEST_ASSERT_EQUAL(K_MOUSE3, result2);
	TEST_ASSERT_EQUAL(K_JOY2, result3);
}

// ============================================================================
// TESTS: Single Character Keys
// ============================================================================

// Test: single character 'a'
void test_Key_StringToKeynum_SingleChar_a(void) {
	keyNum_t result = Test_Key_StringToKeynum("a");
	
	TEST_ASSERT_EQUAL((int)'a', result);
}

// Test: single character 'z'
void test_Key_StringToKeynum_SingleChar_z(void) {
	keyNum_t result = Test_Key_StringToKeynum("z");
	
	TEST_ASSERT_EQUAL((int)'z', result);
}

// Test: single character '0'
void test_Key_StringToKeynum_SingleChar_0(void) {
	keyNum_t result = Test_Key_StringToKeynum("0");
	
	TEST_ASSERT_EQUAL((int)'0', result);
}

// Test: single character '9'
void test_Key_StringToKeynum_SingleChar_9(void) {
	keyNum_t result = Test_Key_StringToKeynum("9");
	
	TEST_ASSERT_EQUAL((int)'9', result);
}

// ============================================================================
// TESTS: Special Cases
// ============================================================================

// Test: NULL pointer
void test_Key_StringToKeynum_Null(void) {
	keyNum_t result = Test_Key_StringToKeynum(NULL);
	
	TEST_ASSERT_EQUAL(-1, result);
}

// Test: empty string
void test_Key_StringToKeynum_EmptyString(void) {
	keyNum_t result = Test_Key_StringToKeynum("");
	
	TEST_ASSERT_EQUAL(-1, result);
}

// Test: invalid key name
void test_Key_StringToKeynum_InvalidName(void) {
	keyNum_t result = Test_Key_StringToKeynum("INVALIDKEY");
	
	TEST_ASSERT_EQUAL(-1, result);
}

// Test: partial key name
void test_Key_StringToKeynum_PartialName(void) {
	keyNum_t result = Test_Key_StringToKeynum("ESC");
	
	TEST_ASSERT_EQUAL(-1, result);  // Should not match ESCAPE
}

// ============================================================================
// TESTS: Keypad Keys
// ============================================================================

// Test: convert KP_HOME
void test_Key_StringToKeynum_KP_Home(void) {
	keyNum_t result = Test_Key_StringToKeynum("KP_HOME");
	
	TEST_ASSERT_EQUAL(K_KP_HOME, result);
}

// Test: convert KP_ENTER
void test_Key_StringToKeynum_KP_Enter(void) {
	keyNum_t result = Test_Key_StringToKeynum("KP_ENTER");
	
	TEST_ASSERT_EQUAL(K_KP_ENTER, result);
}

// Test: convert KP_5
void test_Key_StringToKeynum_KP_5(void) {
	keyNum_t result = Test_Key_StringToKeynum("KP_5");
	
	TEST_ASSERT_EQUAL(K_KP_FIVE, result);
}

// ============================================================================
// TESTS: Mouse Wheel
// ============================================================================

// Test: convert MWHEELUP
void test_Key_StringToKeynum_MWheelUp(void) {
	keyNum_t result = Test_Key_StringToKeynum("MWHEELUP");
	
	TEST_ASSERT_EQUAL(K_MWHEELUP, result);
}

// Test: convert MWHEELDOWN
void test_Key_StringToKeynum_MWheelDown(void) {
	keyNum_t result = Test_Key_StringToKeynum("MWHEELDOWN");
	
	TEST_ASSERT_EQUAL(K_MWHEELDOWN, result);
}

// ============================================================================
// TESTS: Modifier Keys
// ============================================================================

// Test: convert SHIFT
void test_Key_StringToKeynum_Shift(void) {
	keyNum_t result = Test_Key_StringToKeynum("SHIFT");
	
	TEST_ASSERT_EQUAL(K_SHIFT, result);
}

// Test: convert CTRL
void test_Key_StringToKeynum_Ctrl(void) {
	keyNum_t result = Test_Key_StringToKeynum("CTRL");
	
	TEST_ASSERT_EQUAL(K_CTRL, result);
}

// Test: convert ALT
void test_Key_StringToKeynum_Alt(void) {
	keyNum_t result = Test_Key_StringToKeynum("ALT");
	
	TEST_ASSERT_EQUAL(K_ALT, result);
}

// ============================================================================
// TESTS: Special Navigation Keys
// ============================================================================

// Test: convert HOME
void test_Key_StringToKeynum_Home(void) {
	keyNum_t result = Test_Key_StringToKeynum("HOME");
	
	TEST_ASSERT_EQUAL(K_HOME, result);
}

// Test: convert END
void test_Key_StringToKeynum_End(void) {
	keyNum_t result = Test_Key_StringToKeynum("END");
	
	TEST_ASSERT_EQUAL(K_END, result);
}

// Test: convert PGUP
void test_Key_StringToKeynum_PgUp(void) {
	keyNum_t result = Test_Key_StringToKeynum("PGUP");
	
	TEST_ASSERT_EQUAL(K_PGUP, result);
}

// Test: convert PGDN
void test_Key_StringToKeynum_PgDn(void) {
	keyNum_t result = Test_Key_StringToKeynum("PGDN");
	
	TEST_ASSERT_EQUAL(K_PGDN, result);
}

// ============================================================================
// TESTS: Key_KeynumToString - Basic Keys
// ============================================================================

// Test: convert K_ESCAPE to string
void test_Key_KeynumToString_Escape(void) {
	const char *result = Test_Key_KeynumToString(K_ESCAPE);
	
	TEST_ASSERT_EQUAL_STRING("ESCAPE", result);
}

// Test: convert K_ENTER to string
void test_Key_KeynumToString_Enter(void) {
	const char *result = Test_Key_KeynumToString(K_ENTER);
	
	TEST_ASSERT_EQUAL_STRING("ENTER", result);
}

// Test: convert K_TAB to string
void test_Key_KeynumToString_Tab(void) {
	const char *result = Test_Key_KeynumToString(K_TAB);
	
	TEST_ASSERT_EQUAL_STRING("TAB", result);
}

// Test: convert K_SPACE to string
void test_Key_KeynumToString_Space(void) {
	const char *result = Test_Key_KeynumToString(K_SPACE);
	
	TEST_ASSERT_EQUAL_STRING("SPACE", result);
}

// Test: convert K_BACKSPACE to string
void test_Key_KeynumToString_Backspace(void) {
	const char *result = Test_Key_KeynumToString(K_BACKSPACE);
	
	TEST_ASSERT_EQUAL_STRING("BACKSPACE", result);
}

// ============================================================================
// TESTS: Key_KeynumToString - Mouse Buttons
// ============================================================================

// Test: convert K_MOUSE1 to string
void test_Key_KeynumToString_Mouse1(void) {
	const char *result = Test_Key_KeynumToString(K_MOUSE1);
	
	TEST_ASSERT_EQUAL_STRING("MOUSE1", result);
}

// Test: convert K_MOUSE2 to string
void test_Key_KeynumToString_Mouse2(void) {
	const char *result = Test_Key_KeynumToString(K_MOUSE2);
	
	TEST_ASSERT_EQUAL_STRING("MOUSE2", result);
}

// Test: convert K_MOUSE3 to string
void test_Key_KeynumToString_Mouse3(void) {
	const char *result = Test_Key_KeynumToString(K_MOUSE3);
	
	TEST_ASSERT_EQUAL_STRING("MOUSE3", result);
}

// Test: convert K_MOUSE5 to string
void test_Key_KeynumToString_Mouse5(void) {
	const char *result = Test_Key_KeynumToString(K_MOUSE5);
	
	TEST_ASSERT_EQUAL_STRING("MOUSE5", result);
}

// ============================================================================
// TESTS: Key_KeynumToString - Joystick Buttons
// ============================================================================

// Test: convert K_JOY1 to string
void test_Key_KeynumToString_Joy1(void) {
	const char *result = Test_Key_KeynumToString(K_JOY1);
	
	TEST_ASSERT_EQUAL_STRING("JOY1", result);
}

// Test: convert K_JOY2 to string
void test_Key_KeynumToString_Joy2(void) {
	const char *result = Test_Key_KeynumToString(K_JOY2);
	
	TEST_ASSERT_EQUAL_STRING("JOY2", result);
}

// Test: convert K_JOY4 to string
void test_Key_KeynumToString_Joy4(void) {
	const char *result = Test_Key_KeynumToString(K_JOY4);
	
	TEST_ASSERT_EQUAL_STRING("JOY4", result);
}

// ============================================================================
// TESTS: Key_KeynumToString - Auxiliary Keys
// ============================================================================

// Test: convert K_AUX1 to string
void test_Key_KeynumToString_Aux1(void) {
	const char *result = Test_Key_KeynumToString(K_AUX1);
	
	TEST_ASSERT_EQUAL_STRING("AUX1", result);
}

// Test: convert K_AUX5 to string
void test_Key_KeynumToString_Aux5(void) {
	const char *result = Test_Key_KeynumToString(K_AUX5);
	
	TEST_ASSERT_EQUAL_STRING("AUX5", result);
}

// Test: convert K_AUX10 to string
void test_Key_KeynumToString_Aux10(void) {
	const char *result = Test_Key_KeynumToString(K_AUX10);
	
	TEST_ASSERT_EQUAL_STRING("AUX10", result);
}

// Test: convert K_AUX28 to string
void test_Key_KeynumToString_Aux28(void) {
	const char *result = Test_Key_KeynumToString(K_AUX28);
	
	TEST_ASSERT_EQUAL_STRING("AUX28", result);
}

// Test: convert K_AUX32 to string
void test_Key_KeynumToString_Aux32(void) {
	const char *result = Test_Key_KeynumToString(K_AUX32);
	
	TEST_ASSERT_EQUAL_STRING("AUX32", result);
}

// ============================================================================
// TESTS: Key_KeynumToString - Function Keys
// ============================================================================

// Test: convert K_F1 to string
void test_Key_KeynumToString_F1(void) {
	const char *result = Test_Key_KeynumToString(K_F1);
	
	TEST_ASSERT_EQUAL_STRING("F1", result);
}

// Test: convert K_F6 to string
void test_Key_KeynumToString_F6(void) {
	const char *result = Test_Key_KeynumToString(K_F6);
	
	TEST_ASSERT_EQUAL_STRING("F6", result);
}

// Test: convert K_F12 to string
void test_Key_KeynumToString_F12(void) {
	const char *result = Test_Key_KeynumToString(K_F12);
	
	TEST_ASSERT_EQUAL_STRING("F12", result);
}

// ============================================================================
// TESTS: Key_KeynumToString - Arrow Keys
// ============================================================================

// Test: convert K_UPARROW to string
void test_Key_KeynumToString_UpArrow(void) {
	const char *result = Test_Key_KeynumToString(K_UPARROW);
	
	TEST_ASSERT_EQUAL_STRING("UPARROW", result);
}

// Test: convert K_DOWNARROW to string
void test_Key_KeynumToString_DownArrow(void) {
	const char *result = Test_Key_KeynumToString(K_DOWNARROW);
	
	TEST_ASSERT_EQUAL_STRING("DOWNARROW", result);
}

// Test: convert K_LEFTARROW to string
void test_Key_KeynumToString_LeftArrow(void) {
	const char *result = Test_Key_KeynumToString(K_LEFTARROW);
	
	TEST_ASSERT_EQUAL_STRING("LEFTARROW", result);
}

// Test: convert K_RIGHTARROW to string
void test_Key_KeynumToString_RightArrow(void) {
	const char *result = Test_Key_KeynumToString(K_RIGHTARROW);
	
	TEST_ASSERT_EQUAL_STRING("RIGHTARROW", result);
}

// ============================================================================
// TESTS: Key_KeynumToString - Printable ASCII
// ============================================================================

// Test: convert 'a' (ASCII 97) to string
void test_Key_KeynumToString_ASCII_a(void) {
	const char *result = Test_Key_KeynumToString((keyNum_t)'a');
	
	TEST_ASSERT_EQUAL_STRING("a", result);
}

// Test: convert 'z' (ASCII 122) to string
void test_Key_KeynumToString_ASCII_z(void) {
	const char *result = Test_Key_KeynumToString((keyNum_t)'z');
	
	TEST_ASSERT_EQUAL_STRING("z", result);
}

// Test: convert '0' (ASCII 48) to string
void test_Key_KeynumToString_ASCII_0(void) {
	const char *result = Test_Key_KeynumToString((keyNum_t)'0');
	
	TEST_ASSERT_EQUAL_STRING("0", result);
}

// Test: convert '9' (ASCII 57) to string
void test_Key_KeynumToString_ASCII_9(void) {
	const char *result = Test_Key_KeynumToString((keyNum_t)'9');
	
	TEST_ASSERT_EQUAL_STRING("9", result);
}

// Test: convert '!' (ASCII 33) to string
void test_Key_KeynumToString_ASCII_exclamation(void) {
	const char *result = Test_Key_KeynumToString((keyNum_t)'!');
	
	TEST_ASSERT_EQUAL_STRING("!", result);
}

// Test: convert '~' (ASCII 126) to string
void test_Key_KeynumToString_ASCII_tilde(void) {
	const char *result = Test_Key_KeynumToString((keyNum_t)'~');
	
	TEST_ASSERT_EQUAL_STRING("~", result);
}

// ============================================================================
// TESTS: Key_KeynumToString - Special Cases
// ============================================================================

// Test: convert -1 (K_BADKEY) to string
void test_Key_KeynumToString_BadKey(void) {
	const char *result = Test_Key_KeynumToString(-1);
	
	TEST_ASSERT_EQUAL_STRING("<KEY NOT FOUND>", result);
}

// Test: convert invalid keynum to string
void test_Key_KeynumToString_InvalidKeynum(void) {
	const char *result = Test_Key_KeynumToString((keyNum_t)999);
	
	TEST_ASSERT_EQUAL_STRING("<UNKNOWN KEYNUM>", result);
}

// Test: convert out of range keynum to string
void test_Key_KeynumToString_OutOfRange(void) {
	const char *result = Test_Key_KeynumToString((keyNum_t)5000);
	
	TEST_ASSERT_EQUAL_STRING("<UNKNOWN KEYNUM>", result);
}

// ============================================================================
// TESTS: Key_KeynumToString - Keypad Keys
// ============================================================================

// Test: convert K_KP_HOME to string
void test_Key_KeynumToString_KP_Home(void) {
	const char *result = Test_Key_KeynumToString(K_KP_HOME);
	
	TEST_ASSERT_EQUAL_STRING("KP_HOME", result);
}

// Test: convert K_KP_ENTER to string
void test_Key_KeynumToString_KP_Enter(void) {
	const char *result = Test_Key_KeynumToString(K_KP_ENTER);
	
	TEST_ASSERT_EQUAL_STRING("KP_ENTER", result);
}

// Test: convert K_KP_FIVE to string
void test_Key_KeynumToString_KP_5(void) {
	const char *result = Test_Key_KeynumToString(K_KP_FIVE);
	
	TEST_ASSERT_EQUAL_STRING("KP_5", result);
}

// ============================================================================
// TESTS: Key_KeynumToString - Mouse Wheel
// ============================================================================

// Test: convert K_MWHEELUP to string
void test_Key_KeynumToString_MWheelUp(void) {
	const char *result = Test_Key_KeynumToString(K_MWHEELUP);
	
	TEST_ASSERT_EQUAL_STRING("MWHEELUP", result);
}

// Test: convert K_MWHEELDOWN to string
void test_Key_KeynumToString_MWheelDown(void) {
	const char *result = Test_Key_KeynumToString(K_MWHEELDOWN);
	
	TEST_ASSERT_EQUAL_STRING("MWHEELDOWN", result);
}

// ============================================================================
// TESTS: Key_KeynumToString - Modifier Keys
// ============================================================================

// Test: convert K_SHIFT to string
void test_Key_KeynumToString_Shift(void) {
	const char *result = Test_Key_KeynumToString(K_SHIFT);
	
	TEST_ASSERT_EQUAL_STRING("SHIFT", result);
}

// Test: convert K_CTRL to string
void test_Key_KeynumToString_Ctrl(void) {
	const char *result = Test_Key_KeynumToString(K_CTRL);
	
	TEST_ASSERT_EQUAL_STRING("CTRL", result);
}

// Test: convert K_ALT to string
void test_Key_KeynumToString_Alt(void) {
	const char *result = Test_Key_KeynumToString(K_ALT);
	
	TEST_ASSERT_EQUAL_STRING("ALT", result);
}

// ============================================================================
// TESTS: Key_KeynumToString - Navigation Keys
// ============================================================================

// Test: convert K_HOME to string
void test_Key_KeynumToString_Home(void) {
	const char *result = Test_Key_KeynumToString(K_HOME);
	
	TEST_ASSERT_EQUAL_STRING("HOME", result);
}

// Test: convert K_END to string
void test_Key_KeynumToString_End(void) {
	const char *result = Test_Key_KeynumToString(K_END);
	
	TEST_ASSERT_EQUAL_STRING("END", result);
}

// Test: convert K_PGUP to string
void test_Key_KeynumToString_PgUp(void) {
	const char *result = Test_Key_KeynumToString(K_PGUP);
	
	TEST_ASSERT_EQUAL_STRING("PGUP", result);
}

// Test: convert K_PGDN to string
void test_Key_KeynumToString_PgDn(void) {
	const char *result = Test_Key_KeynumToString(K_PGDN);
	
	TEST_ASSERT_EQUAL_STRING("PGDN", result);
}

// Test: convert K_INS to string
void test_Key_KeynumToString_Ins(void) {
	const char *result = Test_Key_KeynumToString(K_INS);
	
	TEST_ASSERT_EQUAL_STRING("INS", result);
}

// Test: convert K_DEL to string
void test_Key_KeynumToString_Del(void) {
	const char *result = Test_Key_KeynumToString(K_DEL);
	
	TEST_ASSERT_EQUAL_STRING("DEL", result);
}

// ============================================================================
// TESTS: Key_KeynumToString - Roundtrip Conversion
// ============================================================================

// Test: string to keynum and back
void test_Key_Roundtrip_Escape(void) {
	keyNum_t keynum = Test_Key_StringToKeynum("ESCAPE");
	const char *str = Test_Key_KeynumToString(keynum);
	
	TEST_ASSERT_EQUAL_STRING("ESCAPE", str);
}

// Test: string to keynum and back (mouse)
void test_Key_Roundtrip_Mouse1(void) {
	keyNum_t keynum = Test_Key_StringToKeynum("MOUSE1");
	const char *str = Test_Key_KeynumToString(keynum);
	
	TEST_ASSERT_EQUAL_STRING("MOUSE1", str);
}

// Test: string to keynum and back (joy)
void test_Key_Roundtrip_Joy3(void) {
	keyNum_t keynum = Test_Key_StringToKeynum("JOY3");
	const char *str = Test_Key_KeynumToString(keynum);
	
	TEST_ASSERT_EQUAL_STRING("JOY3", str);
}

// Test: string to keynum and back (aux)
void test_Key_Roundtrip_Aux15(void) {
	keyNum_t keynum = Test_Key_StringToKeynum("AUX15");
	const char *str = Test_Key_KeynumToString(keynum);
	
	TEST_ASSERT_EQUAL_STRING("AUX15", str);
}

// Test: keynum to string and back (printable ASCII)
void test_Key_Roundtrip_ASCII(void) {
	const char *str1 = Test_Key_KeynumToString((keyNum_t)'g');
	keyNum_t keynum = Test_Key_StringToKeynum("g");
	const char *str2 = Test_Key_KeynumToString(keynum);
	
	TEST_ASSERT_EQUAL_STRING("g", str1);
	TEST_ASSERT_EQUAL_STRING("g", str2);
}

// ============================================================================
// TESTS: Key_SetBinding and Key_GetBinding
// ============================================================================

// Test: set and get basic binding
void test_Key_SetBinding_Basic(void) {
	Test_ResetBindings();
	
	Test_Key_SetBinding('w', "+forward");
	const char *result = Test_Key_GetBinding('w');
	
	TEST_ASSERT_EQUAL_STRING("+forward", result);
}

// Test: set and get multiple bindings
void test_Key_SetBinding_Multiple(void) {
	Test_ResetBindings();
	
	Test_Key_SetBinding('w', "+forward");
	Test_Key_SetBinding('s', "+back");
	Test_Key_SetBinding('a', "+moveleft");
	Test_Key_SetBinding('d', "+moveright");
	
	TEST_ASSERT_EQUAL_STRING("+forward", Test_Key_GetBinding('w'));
	TEST_ASSERT_EQUAL_STRING("+back", Test_Key_GetBinding('s'));
	TEST_ASSERT_EQUAL_STRING("+moveleft", Test_Key_GetBinding('a'));
	TEST_ASSERT_EQUAL_STRING("+moveright", Test_Key_GetBinding('d'));
}

// Test: overwrite existing binding
void test_Key_SetBinding_Overwrite(void) {
	Test_ResetBindings();
	
	Test_Key_SetBinding('w', "+forward");
	TEST_ASSERT_EQUAL_STRING("+forward", Test_Key_GetBinding('w'));
	
	Test_Key_SetBinding('w', "+speed");
	TEST_ASSERT_EQUAL_STRING("+speed", Test_Key_GetBinding('w'));
}

// Test: unbind key (set to NULL)
void test_Key_SetBinding_UnbindNull(void) {
	Test_ResetBindings();
	
	Test_Key_SetBinding('w', "+forward");
	TEST_ASSERT_EQUAL_STRING("+forward", Test_Key_GetBinding('w'));
	
	Test_Key_SetBinding('w', NULL);
	TEST_ASSERT_EQUAL_STRING("", Test_Key_GetBinding('w'));
}

// Test: unbind key (set to empty string)
void test_Key_SetBinding_UnbindEmpty(void) {
	Test_ResetBindings();
	
	Test_Key_SetBinding('w', "+forward");
	TEST_ASSERT_EQUAL_STRING("+forward", Test_Key_GetBinding('w'));
	
	Test_Key_SetBinding('w', "");
	TEST_ASSERT_EQUAL_STRING("", Test_Key_GetBinding('w'));
}

// Test: get binding for unbound key
void test_Key_GetBinding_Unbound(void) {
	Test_ResetBindings();
	
	const char *result = Test_Key_GetBinding('q');
	
	TEST_ASSERT_EQUAL_STRING("", result);
}

// Test: bind ESCAPE key
void test_Key_SetBinding_Escape(void) {
	Test_ResetBindings();
	
	Test_Key_SetBinding(K_ESCAPE, "togglemenu");
	const char *result = Test_Key_GetBinding(K_ESCAPE);
	
	TEST_ASSERT_EQUAL_STRING("togglemenu", result);
}

// Test: bind MOUSE1
void test_Key_SetBinding_Mouse1(void) {
	Test_ResetBindings();
	
	Test_Key_SetBinding(K_MOUSE1, "+attack");
	const char *result = Test_Key_GetBinding(K_MOUSE1);
	
	TEST_ASSERT_EQUAL_STRING("+attack", result);
}

// Test: bind SPACE key
void test_Key_SetBinding_Space(void) {
	Test_ResetBindings();
	
	Test_Key_SetBinding(K_SPACE, "+moveup");
	const char *result = Test_Key_GetBinding(K_SPACE);
	
	TEST_ASSERT_EQUAL_STRING("+moveup", result);
}

// Test: bind F1 key
void test_Key_SetBinding_F1(void) {
	Test_ResetBindings();
	
	Test_Key_SetBinding(K_F1, "cmd help");
	const char *result = Test_Key_GetBinding(K_F1);
	
	TEST_ASSERT_EQUAL_STRING("cmd help", result);
}

// Test: bind SHIFT key
void test_Key_SetBinding_Shift(void) {
	Test_ResetBindings();
	
	Test_Key_SetBinding(K_SHIFT, "+speed");
	const char *result = Test_Key_GetBinding(K_SHIFT);
	
	TEST_ASSERT_EQUAL_STRING("+speed", result);
}

// Test: bind CTRL key
void test_Key_SetBinding_Ctrl(void) {
	Test_ResetBindings();
	
	Test_Key_SetBinding(K_CTRL, "+strafe");
	const char *result = Test_Key_GetBinding(K_CTRL);
	
	TEST_ASSERT_EQUAL_STRING("+strafe", result);
}

// Test: bind complex command with spaces
void test_Key_SetBinding_ComplexCommand(void) {
	Test_ResetBindings();
	
	Test_Key_SetBinding('t', "messagemode");
	const char *result = Test_Key_GetBinding('t');
	
	TEST_ASSERT_EQUAL_STRING("messagemode", result);
}

// Test: bind command with semicolons
void test_Key_SetBinding_SemicolonCommand(void) {
	Test_ResetBindings();
	
	Test_Key_SetBinding(K_F12, "screenshot; echo Screenshot taken");
	const char *result = Test_Key_GetBinding(K_F12);
	
	TEST_ASSERT_EQUAL_STRING("screenshot; echo Screenshot taken", result);
}

// Test: bind all movement keys
void test_Key_SetBinding_AllMovement(void) {
	Test_ResetBindings();
	
	Test_Key_SetBinding('w', "+forward");
	Test_Key_SetBinding('s', "+back");
	Test_Key_SetBinding('a', "+moveleft");
	Test_Key_SetBinding('d', "+moveright");
	Test_Key_SetBinding(K_SPACE, "+moveup");
	Test_Key_SetBinding('c', "+movedown");
	
	TEST_ASSERT_EQUAL_STRING("+forward", Test_Key_GetBinding('w'));
	TEST_ASSERT_EQUAL_STRING("+back", Test_Key_GetBinding('s'));
	TEST_ASSERT_EQUAL_STRING("+moveleft", Test_Key_GetBinding('a'));
	TEST_ASSERT_EQUAL_STRING("+moveright", Test_Key_GetBinding('d'));
	TEST_ASSERT_EQUAL_STRING("+moveup", Test_Key_GetBinding(K_SPACE));
	TEST_ASSERT_EQUAL_STRING("+movedown", Test_Key_GetBinding('c'));
}

// Test: invalid keynum (negative)
void test_Key_SetBinding_InvalidNegative(void) {
	Test_ResetBindings();
	
	Test_Key_SetBinding(-1, "+forward");
	const char *result = Test_Key_GetBinding(-1);
	
	TEST_ASSERT_EQUAL_STRING("", result);
}

// Test: invalid keynum (too large)
void test_Key_SetBinding_InvalidTooLarge(void) {
	Test_ResetBindings();
	
	Test_Key_SetBinding(999, "+forward");
	const char *result = Test_Key_GetBinding(999);
	
	TEST_ASSERT_EQUAL_STRING("", result);
}

// Test: bind keypad keys
void test_Key_SetBinding_Keypad(void) {
	Test_ResetBindings();
	
	Test_Key_SetBinding(K_KP_ENTER, "+attack");
	Test_Key_SetBinding(K_KP_FIVE, "centerview");
	
	TEST_ASSERT_EQUAL_STRING("+attack", Test_Key_GetBinding(K_KP_ENTER));
	TEST_ASSERT_EQUAL_STRING("centerview", Test_Key_GetBinding(K_KP_FIVE));
}

// Test: bind arrow keys
void test_Key_SetBinding_Arrows(void) {
	Test_ResetBindings();
	
	Test_Key_SetBinding(K_UPARROW, "+forward");
	Test_Key_SetBinding(K_DOWNARROW, "+back");
	Test_Key_SetBinding(K_LEFTARROW, "+left");
	Test_Key_SetBinding(K_RIGHTARROW, "+right");
	
	TEST_ASSERT_EQUAL_STRING("+forward", Test_Key_GetBinding(K_UPARROW));
	TEST_ASSERT_EQUAL_STRING("+back", Test_Key_GetBinding(K_DOWNARROW));
	TEST_ASSERT_EQUAL_STRING("+left", Test_Key_GetBinding(K_LEFTARROW));
	TEST_ASSERT_EQUAL_STRING("+right", Test_Key_GetBinding(K_RIGHTARROW));
}

// Test: bind mouse buttons
void test_Key_SetBinding_MouseButtons(void) {
	Test_ResetBindings();
	
	Test_Key_SetBinding(K_MOUSE1, "+attack");
	Test_Key_SetBinding(K_MOUSE2, "+attack2");
	Test_Key_SetBinding(K_MOUSE3, "impulse 1");
	
	TEST_ASSERT_EQUAL_STRING("+attack", Test_Key_GetBinding(K_MOUSE1));
	TEST_ASSERT_EQUAL_STRING("+attack2", Test_Key_GetBinding(K_MOUSE2));
	TEST_ASSERT_EQUAL_STRING("impulse 1", Test_Key_GetBinding(K_MOUSE3));
}

// Test: bind mouse wheel
void test_Key_SetBinding_MouseWheel(void) {
	Test_ResetBindings();
	
	Test_Key_SetBinding(K_MWHEELUP, "weapnext");
	Test_Key_SetBinding(K_MWHEELDOWN, "weapprev");
	
	TEST_ASSERT_EQUAL_STRING("weapnext", Test_Key_GetBinding(K_MWHEELUP));
	TEST_ASSERT_EQUAL_STRING("weapprev", Test_Key_GetBinding(K_MWHEELDOWN));
}

// Test: bind function keys
void test_Key_SetBinding_FunctionKeys(void) {
	Test_ResetBindings();
	
	Test_Key_SetBinding(K_F1, "cmd help");
	Test_Key_SetBinding(K_F5, "screenshot");
	Test_Key_SetBinding(K_F12, "quit");
	
	TEST_ASSERT_EQUAL_STRING("cmd help", Test_Key_GetBinding(K_F1));
	TEST_ASSERT_EQUAL_STRING("screenshot", Test_Key_GetBinding(K_F5));
	TEST_ASSERT_EQUAL_STRING("quit", Test_Key_GetBinding(K_F12));
}

// Test: bind joystick buttons
void test_Key_SetBinding_Joystick(void) {
	Test_ResetBindings();
	
	Test_Key_SetBinding(K_JOY1, "+attack");
	Test_Key_SetBinding(K_JOY2, "+jump");
	
	TEST_ASSERT_EQUAL_STRING("+attack", Test_Key_GetBinding(K_JOY1));
	TEST_ASSERT_EQUAL_STRING("+jump", Test_Key_GetBinding(K_JOY2));
}

// Test: bind auxiliary keys
void test_Key_SetBinding_Auxiliary(void) {
	Test_ResetBindings();
	
	Test_Key_SetBinding(K_AUX1, "say Hello");
	Test_Key_SetBinding(K_AUX16, "disconnect");
	
	TEST_ASSERT_EQUAL_STRING("say Hello", Test_Key_GetBinding(K_AUX1));
	TEST_ASSERT_EQUAL_STRING("disconnect", Test_Key_GetBinding(K_AUX16));
}

// Test: get binding preserves original string
void test_Key_GetBinding_PreservesString(void) {
	Test_ResetBindings();
	
	const char *original = "+forward; +speed; centerview";
	Test_Key_SetBinding('w', original);
	const char *result = Test_Key_GetBinding('w');
	
	TEST_ASSERT_EQUAL_STRING(original, result);
}

// Test: multiple overwrites
void test_Key_SetBinding_MultipleOverwrites(void) {
	Test_ResetBindings();
	
	Test_Key_SetBinding('1', "use blaster");
	TEST_ASSERT_EQUAL_STRING("use blaster", Test_Key_GetBinding('1'));
	
	Test_Key_SetBinding('1', "use shotgun");
	TEST_ASSERT_EQUAL_STRING("use shotgun", Test_Key_GetBinding('1'));
	
	Test_Key_SetBinding('1', "use super shotgun");
	TEST_ASSERT_EQUAL_STRING("use super shotgun", Test_Key_GetBinding('1'));
	
	Test_Key_SetBinding('1', "use machinegun");
	TEST_ASSERT_EQUAL_STRING("use machinegun", Test_Key_GetBinding('1'));
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main(void) {
	UNITY_BEGIN();

	// Basic keys
	RUN_TEST(test_Key_StringToKeynum_Escape);
	RUN_TEST(test_Key_StringToKeynum_Enter);
	RUN_TEST(test_Key_StringToKeynum_Tab);
	RUN_TEST(test_Key_StringToKeynum_Space);
	RUN_TEST(test_Key_StringToKeynum_Backspace);

	// Mouse buttons
	RUN_TEST(test_Key_StringToKeynum_Mouse1);
	RUN_TEST(test_Key_StringToKeynum_Mouse2);
	RUN_TEST(test_Key_StringToKeynum_Mouse3);
	RUN_TEST(test_Key_StringToKeynum_Mouse4);
	RUN_TEST(test_Key_StringToKeynum_Mouse5);

	// Joystick buttons
	RUN_TEST(test_Key_StringToKeynum_Joy1);
	RUN_TEST(test_Key_StringToKeynum_Joy2);
	RUN_TEST(test_Key_StringToKeynum_Joy3);
	RUN_TEST(test_Key_StringToKeynum_Joy4);

	// Auxiliary keys
	RUN_TEST(test_Key_StringToKeynum_Aux1);
	RUN_TEST(test_Key_StringToKeynum_Aux5);
	RUN_TEST(test_Key_StringToKeynum_Aux10);
	RUN_TEST(test_Key_StringToKeynum_Aux20);
	RUN_TEST(test_Key_StringToKeynum_Aux32);

	// Function keys
	RUN_TEST(test_Key_StringToKeynum_F1);
	RUN_TEST(test_Key_StringToKeynum_F5);
	RUN_TEST(test_Key_StringToKeynum_F12);

	// Arrow keys
	RUN_TEST(test_Key_StringToKeynum_UpArrow);
	RUN_TEST(test_Key_StringToKeynum_DownArrow);
	RUN_TEST(test_Key_StringToKeynum_LeftArrow);
	RUN_TEST(test_Key_StringToKeynum_RightArrow);

	// Case insensitivity
	RUN_TEST(test_Key_StringToKeynum_LowercaseEscape);
	RUN_TEST(test_Key_StringToKeynum_LowercaseMouse1);
	RUN_TEST(test_Key_StringToKeynum_LowercaseJoy1);
	RUN_TEST(test_Key_StringToKeynum_LowercaseAux5);
	RUN_TEST(test_Key_StringToKeynum_MixedCase);

	// Single character keys
	RUN_TEST(test_Key_StringToKeynum_SingleChar_a);
	RUN_TEST(test_Key_StringToKeynum_SingleChar_z);
	RUN_TEST(test_Key_StringToKeynum_SingleChar_0);
	RUN_TEST(test_Key_StringToKeynum_SingleChar_9);

	// Special cases
	RUN_TEST(test_Key_StringToKeynum_Null);
	RUN_TEST(test_Key_StringToKeynum_EmptyString);
	RUN_TEST(test_Key_StringToKeynum_InvalidName);
	RUN_TEST(test_Key_StringToKeynum_PartialName);

	// Keypad keys
	RUN_TEST(test_Key_StringToKeynum_KP_Home);
	RUN_TEST(test_Key_StringToKeynum_KP_Enter);
	RUN_TEST(test_Key_StringToKeynum_KP_5);

	// Mouse wheel
	RUN_TEST(test_Key_StringToKeynum_MWheelUp);
	RUN_TEST(test_Key_StringToKeynum_MWheelDown);

	// Modifier keys
	RUN_TEST(test_Key_StringToKeynum_Shift);
	RUN_TEST(test_Key_StringToKeynum_Ctrl);
	RUN_TEST(test_Key_StringToKeynum_Alt);

	// Navigation keys
	RUN_TEST(test_Key_StringToKeynum_Home);
	RUN_TEST(test_Key_StringToKeynum_End);
	RUN_TEST(test_Key_StringToKeynum_PgUp);
	RUN_TEST(test_Key_StringToKeynum_PgDn);

	// Key_KeynumToString - Basic keys
	RUN_TEST(test_Key_KeynumToString_Escape);
	RUN_TEST(test_Key_KeynumToString_Enter);
	RUN_TEST(test_Key_KeynumToString_Tab);
	RUN_TEST(test_Key_KeynumToString_Space);
	RUN_TEST(test_Key_KeynumToString_Backspace);

	// Key_KeynumToString - Mouse buttons
	RUN_TEST(test_Key_KeynumToString_Mouse1);
	RUN_TEST(test_Key_KeynumToString_Mouse2);
	RUN_TEST(test_Key_KeynumToString_Mouse3);
	RUN_TEST(test_Key_KeynumToString_Mouse5);

	// Key_KeynumToString - Joystick buttons
	RUN_TEST(test_Key_KeynumToString_Joy1);
	RUN_TEST(test_Key_KeynumToString_Joy2);
	RUN_TEST(test_Key_KeynumToString_Joy4);

	// Key_KeynumToString - Auxiliary keys
	RUN_TEST(test_Key_KeynumToString_Aux1);
	RUN_TEST(test_Key_KeynumToString_Aux5);
	RUN_TEST(test_Key_KeynumToString_Aux10);
	RUN_TEST(test_Key_KeynumToString_Aux28);
	RUN_TEST(test_Key_KeynumToString_Aux32);

	// Key_KeynumToString - Function keys
	RUN_TEST(test_Key_KeynumToString_F1);
	RUN_TEST(test_Key_KeynumToString_F6);
	RUN_TEST(test_Key_KeynumToString_F12);

	// Key_KeynumToString - Arrow keys
	RUN_TEST(test_Key_KeynumToString_UpArrow);
	RUN_TEST(test_Key_KeynumToString_DownArrow);
	RUN_TEST(test_Key_KeynumToString_LeftArrow);
	RUN_TEST(test_Key_KeynumToString_RightArrow);

	// Key_KeynumToString - Printable ASCII
	RUN_TEST(test_Key_KeynumToString_ASCII_a);
	RUN_TEST(test_Key_KeynumToString_ASCII_z);
	RUN_TEST(test_Key_KeynumToString_ASCII_0);
	RUN_TEST(test_Key_KeynumToString_ASCII_9);
	RUN_TEST(test_Key_KeynumToString_ASCII_exclamation);
	RUN_TEST(test_Key_KeynumToString_ASCII_tilde);

	// Key_KeynumToString - Special cases
	RUN_TEST(test_Key_KeynumToString_BadKey);
	RUN_TEST(test_Key_KeynumToString_InvalidKeynum);
	RUN_TEST(test_Key_KeynumToString_OutOfRange);

	// Key_KeynumToString - Keypad keys
	RUN_TEST(test_Key_KeynumToString_KP_Home);
	RUN_TEST(test_Key_KeynumToString_KP_Enter);
	RUN_TEST(test_Key_KeynumToString_KP_5);

	// Key_KeynumToString - Mouse wheel
	RUN_TEST(test_Key_KeynumToString_MWheelUp);
	RUN_TEST(test_Key_KeynumToString_MWheelDown);

	// Key_KeynumToString - Modifier keys
	RUN_TEST(test_Key_KeynumToString_Shift);
	RUN_TEST(test_Key_KeynumToString_Ctrl);
	RUN_TEST(test_Key_KeynumToString_Alt);

	// Key_KeynumToString - Navigation keys
	RUN_TEST(test_Key_KeynumToString_Home);
	RUN_TEST(test_Key_KeynumToString_End);
	RUN_TEST(test_Key_KeynumToString_PgUp);
	RUN_TEST(test_Key_KeynumToString_PgDn);
	RUN_TEST(test_Key_KeynumToString_Ins);
	RUN_TEST(test_Key_KeynumToString_Del);

	// Roundtrip conversion tests
	RUN_TEST(test_Key_Roundtrip_Escape);
	RUN_TEST(test_Key_Roundtrip_Mouse1);
	RUN_TEST(test_Key_Roundtrip_Joy3);
	RUN_TEST(test_Key_Roundtrip_Aux15);
	RUN_TEST(test_Key_Roundtrip_ASCII);

	// Key_SetBinding and Key_GetBinding - Basic tests
	RUN_TEST(test_Key_SetBinding_Basic);
	RUN_TEST(test_Key_SetBinding_Multiple);
	RUN_TEST(test_Key_SetBinding_Overwrite);
	RUN_TEST(test_Key_SetBinding_UnbindNull);
	RUN_TEST(test_Key_SetBinding_UnbindEmpty);
	RUN_TEST(test_Key_GetBinding_Unbound);

	// Key_SetBinding - Special keys
	RUN_TEST(test_Key_SetBinding_Escape);
	RUN_TEST(test_Key_SetBinding_Mouse1);
	RUN_TEST(test_Key_SetBinding_Space);
	RUN_TEST(test_Key_SetBinding_F1);
	RUN_TEST(test_Key_SetBinding_Shift);
	RUN_TEST(test_Key_SetBinding_Ctrl);

	// Key_SetBinding - Complex commands
	RUN_TEST(test_Key_SetBinding_ComplexCommand);
	RUN_TEST(test_Key_SetBinding_SemicolonCommand);
	RUN_TEST(test_Key_SetBinding_AllMovement);

	// Key_SetBinding - Invalid cases
	RUN_TEST(test_Key_SetBinding_InvalidNegative);
	RUN_TEST(test_Key_SetBinding_InvalidTooLarge);

	// Key_SetBinding - Key types
	RUN_TEST(test_Key_SetBinding_Keypad);
	RUN_TEST(test_Key_SetBinding_Arrows);
	RUN_TEST(test_Key_SetBinding_MouseButtons);
	RUN_TEST(test_Key_SetBinding_MouseWheel);
	RUN_TEST(test_Key_SetBinding_FunctionKeys);
	RUN_TEST(test_Key_SetBinding_Joystick);
	RUN_TEST(test_Key_SetBinding_Auxiliary);

	// Key_GetBinding - Advanced tests
	RUN_TEST(test_Key_GetBinding_PreservesString);
	RUN_TEST(test_Key_SetBinding_MultipleOverwrites);

	return UNITY_END();
}
