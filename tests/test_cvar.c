/*
Test suite for Cvar_Register and console variable system
Tests cvar creation, default values, flags, and retrieval
*/

#include "unity.h"
#include <string.h>
#include <stdlib.h>

// Cvar flags from shared/shared.h
enum {
	CVAR_ARCHIVE		= 1 << 0,
	CVAR_USERINFO		= 1 << 1,
	CVAR_SERVERINFO		= 1 << 2,
	CVAR_READONLY		= 1 << 3,
	CVAR_LATCH_SERVER	= 1 << 4,
	CVAR_LATCH_VIDEO	= 1 << 5,
	CVAR_LATCH_AUDIO	= 1 << 6,
	CVAR_RESET_GAMEDIR	= 1 << 7,
	CVAR_CHEAT			= 1 << 8,
};

typedef int qBool;
#define qFalse 0
#define qTrue 1

// Simplified cVar_t structure
typedef struct cVar_s {
	char			*name;
	char			*string;
	char			*latchedString;
	int				flags;
	qBool			modified;
	float			floatVal;
	int				intVal;
	char			*defaultString;
	struct cVar_s	*hashNext;
} cVar_t;

// Mock cvar storage (simplified version)
#define MAX_TEST_CVARS 64
static cVar_t test_cvars[MAX_TEST_CVARS];
static int test_cvar_count = 0;

// Mock functions to simulate the cvar system
static void Test_CvarReset(void) {
	for (int i = 0; i < test_cvar_count; i++) {
		free(test_cvars[i].name);
		free(test_cvars[i].string);
		free(test_cvars[i].defaultString);
	}
	test_cvar_count = 0;
	memset(test_cvars, 0, sizeof(test_cvars));
}

static cVar_t* Test_CvarExists(const char *varName) {
	for (int i = 0; i < test_cvar_count; i++) {
		if (strcmp(test_cvars[i].name, varName) == 0)
			return &test_cvars[i];
	}
	return NULL;
}

static cVar_t* Test_CvarRegister(char *varName, char *varValue, int flags) {
	if (!varName || !varValue)
		return NULL;
	
	// Check if already exists
	cVar_t *existing = Test_CvarExists(varName);
	if (existing) {
		// Update default and add flags
		free(existing->defaultString);
		existing->defaultString = strdup(varValue);
		existing->flags |= flags;
		return existing;
	}
	
	// Create new
	if (test_cvar_count >= MAX_TEST_CVARS)
		return NULL;
	
	cVar_t *var = &test_cvars[test_cvar_count++];
	var->name = strdup(varName);
	var->string = strdup(varValue);
	var->defaultString = strdup(varValue);
	var->floatVal = (float)atof(varValue);
	var->intVal = atoi(varValue);
	var->flags = flags;
	var->modified = qFalse;  // Not modified until actually changed
	var->latchedString = NULL;
	var->hashNext = NULL;
	
	return var;
}

// ============================================================================
// TESTS: Cvar creation
// ============================================================================

void test_Cvar_Register_NewCvar(void) {
	Test_CvarReset();
	
	cVar_t *var = Test_CvarRegister("test_var", "42", 0);
	
	TEST_ASSERT_NOT_NULL(var);
	TEST_ASSERT_EQUAL_STRING("test_var", var->name);
	TEST_ASSERT_EQUAL_STRING("42", var->string);
	TEST_ASSERT_EQUAL(42, var->intVal);
	TEST_ASSERT_EQUAL(1, test_cvar_count);
}

void test_Cvar_Register_NullName(void) {
	Test_CvarReset();
	
	cVar_t *var = Test_CvarRegister(NULL, "value", 0);
	
	TEST_ASSERT_NULL(var);
	TEST_ASSERT_EQUAL(0, test_cvar_count);
}

void test_Cvar_Register_NullValue(void) {
	Test_CvarReset();
	
	cVar_t *var = Test_CvarRegister("test_var", NULL, 0);
	
	TEST_ASSERT_NULL(var);
	TEST_ASSERT_EQUAL(0, test_cvar_count);
}

void test_Cvar_Register_EmptyString(void) {
	Test_CvarReset();
	
	cVar_t *var = Test_CvarRegister("test_empty", "", 0);
	
	TEST_ASSERT_NOT_NULL(var);
	TEST_ASSERT_EQUAL_STRING("", var->string);
	TEST_ASSERT_EQUAL(0, var->intVal);
}

// ============================================================================
// TESTS: Default values
// ============================================================================

void test_Cvar_Register_IntegerDefault(void) {
	Test_CvarReset();
	
	cVar_t *var = Test_CvarRegister("test_int", "123", 0);
	
	TEST_ASSERT_NOT_NULL(var);
	TEST_ASSERT_EQUAL(123, var->intVal);
	TEST_ASSERT_EQUAL_STRING("123", var->defaultString);
}

void test_Cvar_Register_FloatDefault(void) {
	Test_CvarReset();
	
	cVar_t *var = Test_CvarRegister("test_float", "3.14", 0);
	
	TEST_ASSERT_NOT_NULL(var);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.14f, var->floatVal);
	TEST_ASSERT_EQUAL_STRING("3.14", var->defaultString);
}

void test_Cvar_Register_NegativeValue(void) {
	Test_CvarReset();
	
	cVar_t *var = Test_CvarRegister("test_negative", "-42", 0);
	
	TEST_ASSERT_NOT_NULL(var);
	TEST_ASSERT_EQUAL(-42, var->intVal);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, -42.0f, var->floatVal);
}

void test_Cvar_Register_StringValue(void) {
	Test_CvarReset();
	
	cVar_t *var = Test_CvarRegister("test_string", "hello world", 0);
	
	TEST_ASSERT_NOT_NULL(var);
	TEST_ASSERT_EQUAL_STRING("hello world", var->string);
	TEST_ASSERT_EQUAL(0, var->intVal);  // Non-numeric string parses as 0
}

void test_Cvar_Register_ZeroValue(void) {
	Test_CvarReset();
	
	cVar_t *var = Test_CvarRegister("test_zero", "0", 0);
	
	TEST_ASSERT_NOT_NULL(var);
	TEST_ASSERT_EQUAL(0, var->intVal);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, var->floatVal);
}

// ============================================================================
// TESTS: CVAR_ARCHIVE flag behavior
// ============================================================================

void test_Cvar_Register_ArchiveFlag(void) {
	Test_CvarReset();
	
	cVar_t *var = Test_CvarRegister("test_archive", "1", CVAR_ARCHIVE);
	
	TEST_ASSERT_NOT_NULL(var);
	TEST_ASSERT_TRUE(var->flags & CVAR_ARCHIVE);
}

void test_Cvar_Register_NoFlags(void) {
	Test_CvarReset();
	
	cVar_t *var = Test_CvarRegister("test_noflags", "1", 0);
	
	TEST_ASSERT_NOT_NULL(var);
	TEST_ASSERT_EQUAL(0, var->flags);
}

void test_Cvar_Register_MultipleFlags(void) {
	Test_CvarReset();
	
	cVar_t *var = Test_CvarRegister("test_multiflags", "1", CVAR_ARCHIVE | CVAR_READONLY);
	
	TEST_ASSERT_NOT_NULL(var);
	TEST_ASSERT_TRUE(var->flags & CVAR_ARCHIVE);
	TEST_ASSERT_TRUE(var->flags & CVAR_READONLY);
}

void test_Cvar_Register_UserInfoFlag(void) {
	Test_CvarReset();
	
	cVar_t *var = Test_CvarRegister("test_userinfo", "value", CVAR_USERINFO);
	
	TEST_ASSERT_NOT_NULL(var);
	TEST_ASSERT_TRUE(var->flags & CVAR_USERINFO);
}

void test_Cvar_Register_LatchFlags(void) {
	Test_CvarReset();
	
	cVar_t *var = Test_CvarRegister("test_latch", "1", CVAR_LATCH_VIDEO);
	
	TEST_ASSERT_NOT_NULL(var);
	TEST_ASSERT_TRUE(var->flags & CVAR_LATCH_VIDEO);
}

// ============================================================================
// TESTS: Retrieving existing cvars
// ============================================================================

void test_Cvar_Register_RetrieveExisting(void) {
	Test_CvarReset();
	
	// Register initial cvar
	cVar_t *var1 = Test_CvarRegister("test_existing", "100", 0);
	TEST_ASSERT_NOT_NULL(var1);
	
	// Register again with same name
	cVar_t *var2 = Test_CvarRegister("test_existing", "200", 0);
	
	// Should return the same cvar
	TEST_ASSERT_EQUAL_PTR(var1, var2);
	TEST_ASSERT_EQUAL(1, test_cvar_count);  // Only one cvar created
}

void test_Cvar_Register_UpdateDefault(void) {
	Test_CvarReset();
	
	// Register with initial default
	cVar_t *var1 = Test_CvarRegister("test_update", "100", 0);
	TEST_ASSERT_EQUAL_STRING("100", var1->defaultString);
	
	// Register again with new default
	cVar_t *var2 = Test_CvarRegister("test_update", "200", 0);
	
	// Default should be updated
	TEST_ASSERT_EQUAL_PTR(var1, var2);
	TEST_ASSERT_EQUAL_STRING("200", var2->defaultString);
}

void test_Cvar_Register_AddFlags(void) {
	Test_CvarReset();
	
	// Register without flags
	cVar_t *var1 = Test_CvarRegister("test_addflags", "1", 0);
	TEST_ASSERT_EQUAL(0, var1->flags);
	
	// Register again with ARCHIVE flag
	cVar_t *var2 = Test_CvarRegister("test_addflags", "1", CVAR_ARCHIVE);
	
	// Flags should be added (OR'd)
	TEST_ASSERT_EQUAL_PTR(var1, var2);
	TEST_ASSERT_TRUE(var2->flags & CVAR_ARCHIVE);
}

void test_Cvar_Register_AccumulateFlags(void) {
	Test_CvarReset();
	
	// Register with ARCHIVE
	cVar_t *var1 = Test_CvarRegister("test_accumflags", "1", CVAR_ARCHIVE);
	
	// Register again with READONLY
	cVar_t *var2 = Test_CvarRegister("test_accumflags", "1", CVAR_READONLY);
	
	// Both flags should be present
	TEST_ASSERT_EQUAL_PTR(var1, var2);
	TEST_ASSERT_TRUE(var2->flags & CVAR_ARCHIVE);
	TEST_ASSERT_TRUE(var2->flags & CVAR_READONLY);
}

void test_Cvar_Register_FindByName(void) {
	Test_CvarReset();
	
	// Register multiple cvars
	Test_CvarRegister("cvar1", "1", 0);
	Test_CvarRegister("cvar2", "2", 0);
	Test_CvarRegister("cvar3", "3", 0);
	
	// Find specific one
	cVar_t *found = Test_CvarExists("cvar2");
	
	TEST_ASSERT_NOT_NULL(found);
	TEST_ASSERT_EQUAL_STRING("cvar2", found->name);
	TEST_ASSERT_EQUAL(2, found->intVal);
}

void test_Cvar_Register_NotFound(void) {
	Test_CvarReset();
	
	Test_CvarRegister("cvar1", "1", 0);
	
	cVar_t *found = Test_CvarExists("nonexistent");
	
	TEST_ASSERT_NULL(found);
}

// ============================================================================
// TESTS: Modified flag
// ============================================================================

void test_Cvar_Register_ModifiedFlag(void) {
	Test_CvarReset();
	
	cVar_t *var = Test_CvarRegister("test_modified", "1", 0);
	
	TEST_ASSERT_NOT_NULL(var);
	TEST_ASSERT_FALSE(var->modified);  // Should not be modified on creation
}

// ============================================================================
// TESTS: Multiple cvars
// ============================================================================

void test_Cvar_Register_MultipleCvars(void) {
	Test_CvarReset();
	
	cVar_t *var1 = Test_CvarRegister("cvar_a", "10", CVAR_ARCHIVE);
	cVar_t *var2 = Test_CvarRegister("cvar_b", "20", 0);
	cVar_t *var3 = Test_CvarRegister("cvar_c", "30", CVAR_READONLY);
	
	TEST_ASSERT_NOT_NULL(var1);
	TEST_ASSERT_NOT_NULL(var2);
	TEST_ASSERT_NOT_NULL(var3);
	TEST_ASSERT_EQUAL(3, test_cvar_count);
	
	// Verify each is unique
	TEST_ASSERT_TRUE(var1 != var2);
	TEST_ASSERT_TRUE(var2 != var3);
	TEST_ASSERT_TRUE(var1 != var3);
}

void test_Cvar_Register_LargeValue(void) {
	Test_CvarReset();
	
	cVar_t *var = Test_CvarRegister("test_large", "999999", 0);
	
	TEST_ASSERT_NOT_NULL(var);
	TEST_ASSERT_EQUAL(999999, var->intVal);
}

void test_Cvar_Register_ScientificNotation(void) {
	Test_CvarReset();
	
	cVar_t *var = Test_CvarRegister("test_scientific", "1.5e2", 0);
	
	TEST_ASSERT_NOT_NULL(var);
	TEST_ASSERT_FLOAT_WITHIN(0.1f, 150.0f, var->floatVal);
}


// ============================================================================
// CVAR VALIDATION TESTS
// ============================================================================

// Mock cvar set function for testing validation
static qBool Test_CvarSet(cVar_t *var, const char *value) {
	if (!var || !value)
		return qFalse;
	
	// Check READONLY flag
	if (var->flags & CVAR_READONLY) {
		return qFalse;  // Reject the change
	}
	
	// Check CHEAT flag (simulate cheats being off)
	if (var->flags & CVAR_CHEAT) {
		// In a real scenario, this would check if cheats are enabled
		// For testing, we'll simulate cheats being OFF
		return qFalse;  // Reject cheat cvar changes when cheats off
	}
	
	// Check LATCH flags - store in latchedString instead
	if (var->flags & (CVAR_LATCH_SERVER|CVAR_LATCH_VIDEO|CVAR_LATCH_AUDIO)) {
		// For testing, we'll simulate the latching behavior
		// In real code, this would set latchedString
		return qTrue;  // Accept but latch
	}
	
	// No validation issues - apply the change
	if (var->string) {
		free(var->string);
	}
	var->string = strdup(value);
	var->intVal = atoi(value);
	var->floatVal = atof(value);
	var->modified = qTrue;
	
	return qTrue;
}

// Test: READONLY flag prevents modification
void test_Cvar_Validation_ReadonlyProtection(void) {
	Test_CvarReset();
	
	cVar_t *readonly_var = Test_CvarRegister("readonly_test", "100", CVAR_READONLY);
	TEST_ASSERT_NOT_NULL(readonly_var);
	TEST_ASSERT_EQUAL_STRING("100", readonly_var->string);
	
	// Try to modify it - should fail
	qBool result = Test_CvarSet(readonly_var, "200");
	TEST_ASSERT_FALSE(result);
	TEST_ASSERT_EQUAL_STRING("100", readonly_var->string);  // Should remain unchanged
}

// Test: non-READONLY cvar can be modified
void test_Cvar_Validation_NormalModification(void) {
	Test_CvarReset();
	
	cVar_t *normal_var = Test_CvarRegister("normal_test", "50", 0);
	TEST_ASSERT_NOT_NULL(normal_var);
	TEST_ASSERT_EQUAL_STRING("50", normal_var->string);
	
	// Should be able to modify it
	qBool result = Test_CvarSet(normal_var, "75");
	TEST_ASSERT_TRUE(result);
	TEST_ASSERT_EQUAL_STRING("75", normal_var->string);
	TEST_ASSERT_EQUAL(75, normal_var->intVal);
}

// Test: CHEAT flag prevents modification when cheats are off
void test_Cvar_Validation_CheatProtection(void) {
	Test_CvarReset();
	
	cVar_t *cheat_var = Test_CvarRegister("cheat_test", "1", CVAR_CHEAT);
	TEST_ASSERT_NOT_NULL(cheat_var);
	TEST_ASSERT_EQUAL_STRING("1", cheat_var->string);
	
	// Try to modify cheat cvar when cheats are off - should fail
	qBool result = Test_CvarSet(cheat_var, "999");
	TEST_ASSERT_FALSE(result);
	TEST_ASSERT_EQUAL_STRING("1", cheat_var->string);  // Should remain at default
}

// Test: LATCH_VIDEO flag accepts changes but latches them
void test_Cvar_Validation_LatchVideoFlag(void) {
	Test_CvarReset();
	
	cVar_t *latch_var = Test_CvarRegister("vid_width", "640", CVAR_LATCH_VIDEO);
	TEST_ASSERT_NOT_NULL(latch_var);
	TEST_ASSERT_EQUAL_STRING("640", latch_var->string);
	
	// Should accept the change (but in real code, it would be latched)
	qBool result = Test_CvarSet(latch_var, "1920");
	TEST_ASSERT_TRUE(result);  // Accepted for latching
}

// Test: LATCH_AUDIO flag accepts changes but latches them
void test_Cvar_Validation_LatchAudioFlag(void) {
	Test_CvarReset();
	
	cVar_t *latch_var = Test_CvarRegister("s_volume", "0.7", CVAR_LATCH_AUDIO);
	TEST_ASSERT_NOT_NULL(latch_var);
	TEST_ASSERT_EQUAL_STRING("0.7", latch_var->string);
	
	// Should accept the change (but in real code, it would be latched)
	qBool result = Test_CvarSet(latch_var, "1.0");
	TEST_ASSERT_TRUE(result);  // Accepted for latching
}

// Test: LATCH_SERVER flag accepts changes but latches them
void test_Cvar_Validation_LatchServerFlag(void) {
	Test_CvarReset();
	
	cVar_t *latch_var = Test_CvarRegister("deathmatch", "0", CVAR_LATCH_SERVER);
	TEST_ASSERT_NOT_NULL(latch_var);
	TEST_ASSERT_EQUAL_STRING("0", latch_var->string);
	
	// Should accept the change (but in real code, it would be latched)
	qBool result = Test_CvarSet(latch_var, "1");
	TEST_ASSERT_TRUE(result);  // Accepted for latching
}

// Test: modified flag is set after successful change
void test_Cvar_Validation_ModifiedFlagSet(void) {
	Test_CvarReset();
	
	cVar_t *var = Test_CvarRegister("test_modified", "initial", 0);
	TEST_ASSERT_NOT_NULL(var);
	TEST_ASSERT_FALSE(var->modified);  // Should not be modified initially
	
	// Modify the cvar
	qBool result = Test_CvarSet(var, "changed");
	TEST_ASSERT_TRUE(result);
	TEST_ASSERT_TRUE(var->modified);  // Should now be marked as modified
}

// Test: multiple cvars with different protection levels
void test_Cvar_Validation_MixedProtection(void) {
	Test_CvarReset();
	
	cVar_t *normal = Test_CvarRegister("normal", "1", 0);
	cVar_t *readonly = Test_CvarRegister("readonly", "2", CVAR_READONLY);
	cVar_t *cheat = Test_CvarRegister("cheat", "3", CVAR_CHEAT);
	
	TEST_ASSERT_NOT_NULL(normal);
	TEST_ASSERT_NOT_NULL(readonly);
	TEST_ASSERT_NOT_NULL(cheat);
	
	// Normal should change
	TEST_ASSERT_TRUE(Test_CvarSet(normal, "10"));
	TEST_ASSERT_EQUAL_STRING("10", normal->string);
	
	// Readonly should not change
	TEST_ASSERT_FALSE(Test_CvarSet(readonly, "20"));
	TEST_ASSERT_EQUAL_STRING("2", readonly->string);
	
	// Cheat should not change (cheats off)
	TEST_ASSERT_FALSE(Test_CvarSet(cheat, "30"));
	TEST_ASSERT_EQUAL_STRING("3", cheat->string);
}

// Test: READONLY + CHEAT combined flags
void test_Cvar_Validation_ReadonlyCheat(void) {
	Test_CvarReset();
	
	cVar_t *var = Test_CvarRegister("protected", "100", CVAR_READONLY | CVAR_CHEAT);
	TEST_ASSERT_NOT_NULL(var);
	
	// READONLY should take precedence
	qBool result = Test_CvarSet(var, "200");
	TEST_ASSERT_FALSE(result);
	TEST_ASSERT_EQUAL_STRING("100", var->string);
}

// Test: float value modification
void test_Cvar_Validation_FloatModification(void) {
	Test_CvarReset();
	
	cVar_t *var = Test_CvarRegister("sensitivity", "3.0", 0);
	TEST_ASSERT_NOT_NULL(var);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.0f, var->floatVal);
	
	qBool result = Test_CvarSet(var, "5.5");
	TEST_ASSERT_TRUE(result);
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 5.5f, var->floatVal);
}

// Test: negative value modification
void test_Cvar_Validation_NegativeModification(void) {
	Test_CvarReset();
	
	cVar_t *var = Test_CvarRegister("offset", "0", 0);
	TEST_ASSERT_NOT_NULL(var);
	
	qBool result = Test_CvarSet(var, "-10");
	TEST_ASSERT_TRUE(result);
	TEST_ASSERT_EQUAL(-10, var->intVal);
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main(void) {
	UNITY_BEGIN();

	// Cvar creation
	RUN_TEST(test_Cvar_Register_NewCvar);
	RUN_TEST(test_Cvar_Register_NullName);
	RUN_TEST(test_Cvar_Register_NullValue);
	RUN_TEST(test_Cvar_Register_EmptyString);

	// Default values
	RUN_TEST(test_Cvar_Register_IntegerDefault);
	RUN_TEST(test_Cvar_Register_FloatDefault);
	RUN_TEST(test_Cvar_Register_NegativeValue);
	RUN_TEST(test_Cvar_Register_StringValue);
	RUN_TEST(test_Cvar_Register_ZeroValue);

	// CVAR_ARCHIVE flag behavior
	RUN_TEST(test_Cvar_Register_ArchiveFlag);
	RUN_TEST(test_Cvar_Register_NoFlags);
	RUN_TEST(test_Cvar_Register_MultipleFlags);
	RUN_TEST(test_Cvar_Register_UserInfoFlag);
	RUN_TEST(test_Cvar_Register_LatchFlags);

	// Retrieving existing cvars
	RUN_TEST(test_Cvar_Register_RetrieveExisting);
	RUN_TEST(test_Cvar_Register_UpdateDefault);
	RUN_TEST(test_Cvar_Register_AddFlags);
	RUN_TEST(test_Cvar_Register_AccumulateFlags);
	RUN_TEST(test_Cvar_Register_FindByName);
	RUN_TEST(test_Cvar_Register_NotFound);

	// Modified flag
	RUN_TEST(test_Cvar_Register_ModifiedFlag);

	// Multiple cvars
	RUN_TEST(test_Cvar_Register_MultipleCvars);
	RUN_TEST(test_Cvar_Register_LargeValue);
	RUN_TEST(test_Cvar_Register_ScientificNotation);

	// Cvar validation
	RUN_TEST(test_Cvar_Validation_ReadonlyProtection);
	RUN_TEST(test_Cvar_Validation_NormalModification);
	RUN_TEST(test_Cvar_Validation_CheatProtection);
	RUN_TEST(test_Cvar_Validation_LatchVideoFlag);
	RUN_TEST(test_Cvar_Validation_LatchAudioFlag);
	RUN_TEST(test_Cvar_Validation_LatchServerFlag);
	RUN_TEST(test_Cvar_Validation_ModifiedFlagSet);
	RUN_TEST(test_Cvar_Validation_MixedProtection);
	RUN_TEST(test_Cvar_Validation_ReadonlyCheat);
	RUN_TEST(test_Cvar_Validation_FloatModification);
	RUN_TEST(test_Cvar_Validation_NegativeModification);

	return UNITY_END();
}
