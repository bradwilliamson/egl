/*
 * test_cmd.c - Unit tests for console command system
 */

#include "../unity/unity.h"
#include "../common/common.h"
#include <string.h>
#include <stdlib.h>

// ============================================================================
// Mock Command System
// ============================================================================

#define MAX_TEST_CMDS 128
#define MAX_TEST_TOKENS 64
#define MAX_TEST_ARGS_LEN 512

typedef struct testCmd_s {
	char *name;
	void (*function)(void);
	const char *description;
	qBool inUse;
	int callCount;  // Track how many times the command was called
} testCmd_t;

static testCmd_t test_cmds[MAX_TEST_CMDS];
static int test_cmd_count = 0;

// Mock argument parsing system
static int test_argc = 0;
static char *test_argv[MAX_TEST_TOKENS];
static char test_args_buffer[MAX_TEST_ARGS_LEN];

// Reset argument parsing
static void Test_ArgReset(void) {
	for (int i = 0; i < test_argc; i++) {
		if (test_argv[i]) {
			free(test_argv[i]);
			test_argv[i] = NULL;
		}
	}
	test_argc = 0;
	memset(test_args_buffer, 0, sizeof(test_args_buffer));
}

// Mock Cmd_TokenizeString - simple parser that handles quotes
static void Test_TokenizeString(const char *text) {
	Test_ArgReset();
	
	if (!text || !*text)
		return;
	
	const char *p = text;
	char token[256];
	int token_idx = 0;
	qBool in_quotes = qFalse;
	
	while (*p) {
		// Skip leading whitespace
		while (*p && *p <= ' ' && !in_quotes) {
			p++;
		}
		
		if (!*p)
			break;
		
		// Parse token
		token_idx = 0;
		while (*p) {
			if (*p == '"') {
				in_quotes = !in_quotes;
				p++;
				continue;
			}
			
			if (*p <= ' ' && !in_quotes) {
				break;
			}
			
			token[token_idx++] = *p++;
			
			if (token_idx >= 255)
				break;
		}
		
		token[token_idx] = 0;
		
		if (token_idx > 0 && test_argc < MAX_TEST_TOKENS) {
			test_argv[test_argc++] = strdup(token);
		}
	}
}

// Mock Cmd_Argc
static int Test_Argc(void) {
	return test_argc;
}

// Mock Cmd_Argv
static const char* Test_Argv(int arg) {
	if (arg < 0 || arg >= test_argc)
		return "";
	return test_argv[arg];
}

// Reset the test command system
static void Test_CmdReset(void) {
	for (int i = 0; i < test_cmd_count; i++) {
		if (test_cmds[i].name) {
			free(test_cmds[i].name);
		}
	}
	test_cmd_count = 0;
	memset(test_cmds, 0, sizeof(test_cmds));
}

// Mock implementation of Cmd_AddCommand
static void* Test_CmdAddCommand(const char *name, void (*function)(void), const char *description) {
	if (!name || !function)
		return NULL;
	
	// Check if command already exists
	for (int i = 0; i < test_cmd_count; i++) {
		if (test_cmds[i].inUse && strcmp(test_cmds[i].name, name) == 0) {
			return NULL;  // Already exists
		}
	}
	
	if (test_cmd_count >= MAX_TEST_CMDS)
		return NULL;
	
	testCmd_t *cmd = &test_cmds[test_cmd_count++];
	cmd->name = strdup(name);
	cmd->function = function;
	cmd->description = description;
	cmd->inUse = qTrue;
	cmd->callCount = 0;
	
	return cmd;
}

// Mock implementation of Cmd_Exists
static void* Test_CmdExists(const char *name) {
	if (!name)
		return NULL;
	
	for (int i = 0; i < test_cmd_count; i++) {
		if (test_cmds[i].inUse && strcmp(test_cmds[i].name, name) == 0) {
			return &test_cmds[i];
		}
	}
	
	return NULL;
}

// Mock implementation of Cmd_RemoveCommand
static void Test_CmdRemoveCommand(const char *name, void *command) {
	if (!command)
		return;
	
	testCmd_t *cmd = (testCmd_t*)command;
	if (!cmd->inUse)
		return;
	
	free(cmd->name);
	cmd->name = NULL;
	cmd->function = NULL;
	cmd->description = NULL;
	cmd->inUse = qFalse;
	cmd->callCount = 0;
}

// ============================================================================
// Test Command Functions
// ============================================================================

static int test_func1_called = 0;
static int test_func2_called = 0;
static int test_func3_called = 0;

static void TestCmd_Func1(void) {
	test_func1_called++;
}

static void TestCmd_Func2(void) {
	test_func2_called++;
}

static void TestCmd_Func3(void) {
	test_func3_called++;
}

// ============================================================================
// TESTS: Command Registration
// ============================================================================

// Test: register a new command
void test_Cmd_AddCommand_NewCommand(void) {
	Test_CmdReset();
	test_func1_called = 0;
	
	void *cmd = Test_CmdAddCommand("test_cmd", TestCmd_Func1, "Test command");
	
	TEST_ASSERT_NOT_NULL(cmd);
	TEST_ASSERT_EQUAL(1, test_cmd_count);
}

// Test: register command with NULL name fails
void test_Cmd_AddCommand_NullName(void) {
	Test_CmdReset();
	
	void *cmd = Test_CmdAddCommand(NULL, TestCmd_Func1, "Test");
	
	TEST_ASSERT_NULL(cmd);
	TEST_ASSERT_EQUAL(0, test_cmd_count);
}

// Test: register command with NULL function fails
void test_Cmd_AddCommand_NullFunction(void) {
	Test_CmdReset();
	
	void *cmd = Test_CmdAddCommand("test", NULL, "Test");
	
	TEST_ASSERT_NULL(cmd);
	TEST_ASSERT_EQUAL(0, test_cmd_count);
}

// Test: register command with NULL description (should still work)
void test_Cmd_AddCommand_NullDescription(void) {
	Test_CmdReset();
	
	void *cmd = Test_CmdAddCommand("test", TestCmd_Func1, NULL);
	
	TEST_ASSERT_NOT_NULL(cmd);
	TEST_ASSERT_EQUAL(1, test_cmd_count);
}

// Test: cannot register duplicate command
void test_Cmd_AddCommand_Duplicate(void) {
	Test_CmdReset();
	
	void *cmd1 = Test_CmdAddCommand("test", TestCmd_Func1, "First");
	void *cmd2 = Test_CmdAddCommand("test", TestCmd_Func2, "Second");
	
	TEST_ASSERT_NOT_NULL(cmd1);
	TEST_ASSERT_NULL(cmd2);  // Should fail
	TEST_ASSERT_EQUAL(1, test_cmd_count);
}

// Test: register multiple different commands
void test_Cmd_AddCommand_Multiple(void) {
	Test_CmdReset();
	
	void *cmd1 = Test_CmdAddCommand("cmd1", TestCmd_Func1, "Command 1");
	void *cmd2 = Test_CmdAddCommand("cmd2", TestCmd_Func2, "Command 2");
	void *cmd3 = Test_CmdAddCommand("cmd3", TestCmd_Func3, "Command 3");
	
	TEST_ASSERT_NOT_NULL(cmd1);
	TEST_ASSERT_NOT_NULL(cmd2);
	TEST_ASSERT_NOT_NULL(cmd3);
	TEST_ASSERT_EQUAL(3, test_cmd_count);
}

// Test: command names are case-sensitive in storage
void test_Cmd_AddCommand_CaseSensitive(void) {
	Test_CmdReset();
	
	void *cmd1 = Test_CmdAddCommand("test", TestCmd_Func1, "Lowercase");
	void *cmd2 = Test_CmdAddCommand("TEST", TestCmd_Func2, "Uppercase");
	
	TEST_ASSERT_NOT_NULL(cmd1);
	TEST_ASSERT_NOT_NULL(cmd2);
	TEST_ASSERT_EQUAL(2, test_cmd_count);
}

// Test: register command with long name
void test_Cmd_AddCommand_LongName(void) {
	Test_CmdReset();
	
	void *cmd = Test_CmdAddCommand("this_is_a_very_long_command_name", TestCmd_Func1, "Long name test");
	
	TEST_ASSERT_NOT_NULL(cmd);
	TEST_ASSERT_EQUAL(1, test_cmd_count);
}

// Test: register command with special characters
void test_Cmd_AddCommand_SpecialChars(void) {
	Test_CmdReset();
	
	void *cmd = Test_CmdAddCommand("test_cmd-123", TestCmd_Func1, "Special chars");
	
	TEST_ASSERT_NOT_NULL(cmd);
	TEST_ASSERT_EQUAL(1, test_cmd_count);
}

// Test: register command with empty description
void test_Cmd_AddCommand_EmptyDescription(void) {
	Test_CmdReset();
	
	void *cmd = Test_CmdAddCommand("test", TestCmd_Func1, "");
	
	TEST_ASSERT_NOT_NULL(cmd);
	TEST_ASSERT_EQUAL(1, test_cmd_count);
}

// ============================================================================
// TESTS: Command Lookup
// ============================================================================

// Test: find existing command
void test_Cmd_Exists_Found(void) {
	Test_CmdReset();
	
	void *cmd = Test_CmdAddCommand("testcmd", TestCmd_Func1, "Test");
	void *found = Test_CmdExists("testcmd");
	
	TEST_ASSERT_NOT_NULL(found);
	TEST_ASSERT_EQUAL_PTR(cmd, found);
}

// Test: lookup non-existent command
void test_Cmd_Exists_NotFound(void) {
	Test_CmdReset();
	
	Test_CmdAddCommand("testcmd", TestCmd_Func1, "Test");
	void *found = Test_CmdExists("nonexistent");
	
	TEST_ASSERT_NULL(found);
}

// Test: lookup with NULL name
void test_Cmd_Exists_NullName(void) {
	Test_CmdReset();
	
	Test_CmdAddCommand("testcmd", TestCmd_Func1, "Test");
	void *found = Test_CmdExists(NULL);
	
	TEST_ASSERT_NULL(found);
}

// Test: lookup with empty string
void test_Cmd_Exists_EmptyString(void) {
	Test_CmdReset();
	
	Test_CmdAddCommand("testcmd", TestCmd_Func1, "Test");
	void *found = Test_CmdExists("");
	
	TEST_ASSERT_NULL(found);
}

// Test: find command by exact name match
void test_Cmd_Exists_ExactMatch(void) {
	Test_CmdReset();
	
	Test_CmdAddCommand("cmd1", TestCmd_Func1, "Test 1");
	Test_CmdAddCommand("cmd2", TestCmd_Func2, "Test 2");
	Test_CmdAddCommand("cmd3", TestCmd_Func3, "Test 3");
	
	void *found1 = Test_CmdExists("cmd1");
	void *found2 = Test_CmdExists("cmd2");
	void *found3 = Test_CmdExists("cmd3");
	
	TEST_ASSERT_NOT_NULL(found1);
	TEST_ASSERT_NOT_NULL(found2);
	TEST_ASSERT_NOT_NULL(found3);
	
	testCmd_t *tcmd1 = (testCmd_t*)found1;
	testCmd_t *tcmd2 = (testCmd_t*)found2;
	testCmd_t *tcmd3 = (testCmd_t*)found3;
	
	TEST_ASSERT_EQUAL_STRING("cmd1", tcmd1->name);
	TEST_ASSERT_EQUAL_STRING("cmd2", tcmd2->name);
	TEST_ASSERT_EQUAL_STRING("cmd3", tcmd3->name);
}

// Test: lookup after adding many commands
void test_Cmd_Exists_ManyCommands(void) {
	Test_CmdReset();
	
	// Add 10 commands
	for (int i = 0; i < 10; i++) {
		char name[32];
		sprintf(name, "cmd%d", i);
		Test_CmdAddCommand(name, TestCmd_Func1, "Test");
	}
	
	// Find a command in the middle
	void *found = Test_CmdExists("cmd5");
	TEST_ASSERT_NOT_NULL(found);
	
	testCmd_t *tcmd = (testCmd_t*)found;
	TEST_ASSERT_EQUAL_STRING("cmd5", tcmd->name);
}

// ============================================================================
// TESTS: Command Removal
// ============================================================================

// Test: remove existing command
void test_Cmd_RemoveCommand_Success(void) {
	Test_CmdReset();
	
	void *cmd = Test_CmdAddCommand("test", TestCmd_Func1, "Test");
	TEST_ASSERT_NOT_NULL(cmd);
	
	Test_CmdRemoveCommand("test", cmd);
	
	void *found = Test_CmdExists("test");
	TEST_ASSERT_NULL(found);
}

// Test: remove command with NULL pointer
void test_Cmd_RemoveCommand_NullPointer(void) {
	Test_CmdReset();
	
	Test_CmdAddCommand("test", TestCmd_Func1, "Test");
	
	// Should not crash
	Test_CmdRemoveCommand("test", NULL);
	
	// Command should still exist
	void *found = Test_CmdExists("test");
	TEST_ASSERT_NOT_NULL(found);
}

// Test: remove one command among many
void test_Cmd_RemoveCommand_Selective(void) {
	Test_CmdReset();
	
	void *cmd1 = Test_CmdAddCommand("cmd1", TestCmd_Func1, "Test 1");
	void *cmd2 = Test_CmdAddCommand("cmd2", TestCmd_Func2, "Test 2");
	void *cmd3 = Test_CmdAddCommand("cmd3", TestCmd_Func3, "Test 3");
	
	// Remove the middle one
	Test_CmdRemoveCommand("cmd2", cmd2);
	
	// cmd1 and cmd3 should still exist
	TEST_ASSERT_NOT_NULL(Test_CmdExists("cmd1"));
	TEST_ASSERT_NULL(Test_CmdExists("cmd2"));
	TEST_ASSERT_NOT_NULL(Test_CmdExists("cmd3"));
}

// Test: remove all commands
void test_Cmd_RemoveCommand_RemoveAll(void) {
	Test_CmdReset();
	
	void *cmd1 = Test_CmdAddCommand("cmd1", TestCmd_Func1, "Test 1");
	void *cmd2 = Test_CmdAddCommand("cmd2", TestCmd_Func2, "Test 2");
	void *cmd3 = Test_CmdAddCommand("cmd3", TestCmd_Func3, "Test 3");
	
	Test_CmdRemoveCommand("cmd1", cmd1);
	Test_CmdRemoveCommand("cmd2", cmd2);
	Test_CmdRemoveCommand("cmd3", cmd3);
	
	TEST_ASSERT_NULL(Test_CmdExists("cmd1"));
	TEST_ASSERT_NULL(Test_CmdExists("cmd2"));
	TEST_ASSERT_NULL(Test_CmdExists("cmd3"));
}

// Test: add command after removing it
void test_Cmd_RemoveCommand_ReAdd(void) {
	Test_CmdReset();
	
	void *cmd1 = Test_CmdAddCommand("test", TestCmd_Func1, "First");
	TEST_ASSERT_NOT_NULL(cmd1);
	
	Test_CmdRemoveCommand("test", cmd1);
	TEST_ASSERT_NULL(Test_CmdExists("test"));
	
	// Should be able to add again
	void *cmd2 = Test_CmdAddCommand("test", TestCmd_Func2, "Second");
	TEST_ASSERT_NOT_NULL(cmd2);
	TEST_ASSERT_NOT_NULL(Test_CmdExists("test"));
}

// Test: remove command and verify it can't be removed again
void test_Cmd_RemoveCommand_DoubleRemove(void) {
	Test_CmdReset();
	
	void *cmd = Test_CmdAddCommand("test", TestCmd_Func1, "Test");
	
	Test_CmdRemoveCommand("test", cmd);
	
	// Second removal should be safe (no-op)
	Test_CmdRemoveCommand("test", cmd);
	
	// Still shouldn't exist
	TEST_ASSERT_NULL(Test_CmdExists("test"));
}

// ============================================================================
// TESTS: Integration
// ============================================================================

// Test: complete workflow - add, find, execute, remove
void test_Cmd_Integration_Complete(void) {
	Test_CmdReset();
	test_func1_called = 0;
	
	// Add command
	void *cmd = Test_CmdAddCommand("testcmd", TestCmd_Func1, "Test command");
	TEST_ASSERT_NOT_NULL(cmd);
	
	// Verify it exists
	void *found = Test_CmdExists("testcmd");
	TEST_ASSERT_NOT_NULL(found);
	TEST_ASSERT_EQUAL_PTR(cmd, found);
	
	// Execute it
	testCmd_t *tcmd = (testCmd_t*)found;
	tcmd->function();
	TEST_ASSERT_EQUAL(1, test_func1_called);
	
	// Remove it
	Test_CmdRemoveCommand("testcmd", cmd);
	TEST_ASSERT_NULL(Test_CmdExists("testcmd"));
}

// Test: register many commands and verify all exist
void test_Cmd_Integration_ManyCommands(void) {
	Test_CmdReset();
	
	void *cmds[20];
	
	// Add 20 commands
	for (int i = 0; i < 20; i++) {
		char name[32];
		sprintf(name, "cmd_%d", i);
		cmds[i] = Test_CmdAddCommand(name, TestCmd_Func1, "Test");
		TEST_ASSERT_NOT_NULL(cmds[i]);
	}
	
	// Verify all exist
	for (int i = 0; i < 20; i++) {
		char name[32];
		sprintf(name, "cmd_%d", i);
		void *found = Test_CmdExists(name);
		TEST_ASSERT_NOT_NULL(found);
		TEST_ASSERT_EQUAL_PTR(cmds[i], found);
	}
	
	TEST_ASSERT_EQUAL(20, test_cmd_count);
}

// Test: alternating add and remove
void test_Cmd_Integration_AlternatingOps(void) {
	Test_CmdReset();
	
	void *cmd1 = Test_CmdAddCommand("cmd1", TestCmd_Func1, "Test 1");
	TEST_ASSERT_NOT_NULL(cmd1);
	
	void *cmd2 = Test_CmdAddCommand("cmd2", TestCmd_Func2, "Test 2");
	TEST_ASSERT_NOT_NULL(cmd2);
	
	Test_CmdRemoveCommand("cmd1", cmd1);
	TEST_ASSERT_NULL(Test_CmdExists("cmd1"));
	TEST_ASSERT_NOT_NULL(Test_CmdExists("cmd2"));
	
	void *cmd3 = Test_CmdAddCommand("cmd3", TestCmd_Func3, "Test 3");
	TEST_ASSERT_NOT_NULL(cmd3);
	
	Test_CmdRemoveCommand("cmd2", cmd2);
	TEST_ASSERT_NULL(Test_CmdExists("cmd2"));
	TEST_ASSERT_NOT_NULL(Test_CmdExists("cmd3"));
	
	Test_CmdRemoveCommand("cmd3", cmd3);
	TEST_ASSERT_NULL(Test_CmdExists("cmd3"));
}

// ============================================================================
// TESTS: Argument Parsing
// ============================================================================

// Test: parse simple command with no arguments
void test_Cmd_Argc_NoArgs(void) {
	Test_TokenizeString("cmd");
	
	TEST_ASSERT_EQUAL(1, Test_Argc());
	TEST_ASSERT_EQUAL_STRING("cmd", Test_Argv(0));
}

// Test: parse command with single argument
void test_Cmd_Argc_OneArg(void) {
	Test_TokenizeString("cmd arg1");
	
	TEST_ASSERT_EQUAL(2, Test_Argc());
	TEST_ASSERT_EQUAL_STRING("cmd", Test_Argv(0));
	TEST_ASSERT_EQUAL_STRING("arg1", Test_Argv(1));
}

// Test: parse command with multiple arguments
void test_Cmd_Argc_MultipleArgs(void) {
	Test_TokenizeString("cmd arg1 arg2 arg3");
	
	TEST_ASSERT_EQUAL(4, Test_Argc());
	TEST_ASSERT_EQUAL_STRING("cmd", Test_Argv(0));
	TEST_ASSERT_EQUAL_STRING("arg1", Test_Argv(1));
	TEST_ASSERT_EQUAL_STRING("arg2", Test_Argv(2));
	TEST_ASSERT_EQUAL_STRING("arg3", Test_Argv(3));
}

// Test: parse empty command line
void test_Cmd_Argc_Empty(void) {
	Test_TokenizeString("");
	
	TEST_ASSERT_EQUAL(0, Test_Argc());
}

// Test: parse command line with only whitespace
void test_Cmd_Argc_WhitespaceOnly(void) {
	Test_TokenizeString("    ");
	
	TEST_ASSERT_EQUAL(0, Test_Argc());
}

// Test: parse quoted string with spaces
void test_Cmd_Argv_QuotedString(void) {
	Test_TokenizeString("say \"hello world\"");
	
	TEST_ASSERT_EQUAL(2, Test_Argc());
	TEST_ASSERT_EQUAL_STRING("say", Test_Argv(0));
	TEST_ASSERT_EQUAL_STRING("hello world", Test_Argv(1));
}

// Test: parse multiple quoted strings
void test_Cmd_Argv_MultipleQuoted(void) {
	Test_TokenizeString("cmd \"first arg\" \"second arg\"");
	
	TEST_ASSERT_EQUAL(3, Test_Argc());
	TEST_ASSERT_EQUAL_STRING("cmd", Test_Argv(0));
	TEST_ASSERT_EQUAL_STRING("first arg", Test_Argv(1));
	TEST_ASSERT_EQUAL_STRING("second arg", Test_Argv(2));
}

// Test: parse mixed quoted and unquoted arguments
void test_Cmd_Argv_MixedQuoted(void) {
	Test_TokenizeString("bind key \"some command\"");
	
	TEST_ASSERT_EQUAL(3, Test_Argc());
	TEST_ASSERT_EQUAL_STRING("bind", Test_Argv(0));
	TEST_ASSERT_EQUAL_STRING("key", Test_Argv(1));
	TEST_ASSERT_EQUAL_STRING("some command", Test_Argv(2));
}

// Test: parse empty quoted string
void test_Cmd_Argv_EmptyQuoted(void) {
	Test_TokenizeString("cmd \"\"");
	
	TEST_ASSERT_EQUAL(1, Test_Argc());  // Empty quotes result in no token
	TEST_ASSERT_EQUAL_STRING("cmd", Test_Argv(0));
}

// Test: Cmd_Argv returns empty string for out of range
void test_Cmd_Argv_OutOfRange(void) {
	Test_TokenizeString("cmd arg1");
	
	TEST_ASSERT_EQUAL_STRING("", Test_Argv(10));
	TEST_ASSERT_EQUAL_STRING("", Test_Argv(-1));
}

// Test: Cmd_Argv boundary conditions
void test_Cmd_Argv_Boundaries(void) {
	Test_TokenizeString("cmd arg1 arg2");
	
	// Valid indices
	TEST_ASSERT_EQUAL_STRING("cmd", Test_Argv(0));
	TEST_ASSERT_EQUAL_STRING("arg1", Test_Argv(1));
	TEST_ASSERT_EQUAL_STRING("arg2", Test_Argv(2));
	
	// Just beyond valid range
	TEST_ASSERT_EQUAL_STRING("", Test_Argv(3));
}

// Test: parse command with extra whitespace
void test_Cmd_Argc_ExtraWhitespace(void) {
	Test_TokenizeString("cmd    arg1     arg2");
	
	TEST_ASSERT_EQUAL(3, Test_Argc());
	TEST_ASSERT_EQUAL_STRING("cmd", Test_Argv(0));
	TEST_ASSERT_EQUAL_STRING("arg1", Test_Argv(1));
	TEST_ASSERT_EQUAL_STRING("arg2", Test_Argv(2));
}

// Test: parse command with leading whitespace
void test_Cmd_Argc_LeadingWhitespace(void) {
	Test_TokenizeString("   cmd arg1");
	
	TEST_ASSERT_EQUAL(2, Test_Argc());
	TEST_ASSERT_EQUAL_STRING("cmd", Test_Argv(0));
	TEST_ASSERT_EQUAL_STRING("arg1", Test_Argv(1));
}

// Test: parse command with trailing whitespace
void test_Cmd_Argc_TrailingWhitespace(void) {
	Test_TokenizeString("cmd arg1   ");
	
	TEST_ASSERT_EQUAL(2, Test_Argc());
	TEST_ASSERT_EQUAL_STRING("cmd", Test_Argv(0));
	TEST_ASSERT_EQUAL_STRING("arg1", Test_Argv(1));
}

// Test: parse quoted string at start
void test_Cmd_Argv_QuotedAtStart(void) {
	Test_TokenizeString("\"hello world\" arg2");
	
	TEST_ASSERT_EQUAL(2, Test_Argc());
	TEST_ASSERT_EQUAL_STRING("hello world", Test_Argv(0));
	TEST_ASSERT_EQUAL_STRING("arg2", Test_Argv(1));
}

// Test: parse quoted string with special characters
void test_Cmd_Argv_QuotedSpecialChars(void) {
	Test_TokenizeString("say \"!@#$%^&*()\"");
	
	TEST_ASSERT_EQUAL(2, Test_Argc());
	TEST_ASSERT_EQUAL_STRING("say", Test_Argv(0));
	TEST_ASSERT_EQUAL_STRING("!@#$%^&*()", Test_Argv(1));
}

// Test: parse many arguments
void test_Cmd_Argc_ManyArgs(void) {
	Test_TokenizeString("cmd a1 a2 a3 a4 a5 a6 a7 a8 a9 a10");
	
	TEST_ASSERT_EQUAL(11, Test_Argc());
	TEST_ASSERT_EQUAL_STRING("cmd", Test_Argv(0));
	TEST_ASSERT_EQUAL_STRING("a5", Test_Argv(5));
	TEST_ASSERT_EQUAL_STRING("a10", Test_Argv(10));
}

// Test: parse quoted string with numbers
void test_Cmd_Argv_QuotedNumbers(void) {
	Test_TokenizeString("echo \"123 456 789\"");
	
	TEST_ASSERT_EQUAL(2, Test_Argc());
	TEST_ASSERT_EQUAL_STRING("echo", Test_Argv(0));
	TEST_ASSERT_EQUAL_STRING("123 456 789", Test_Argv(1));
}

// Test: single character arguments
void test_Cmd_Argv_SingleChars(void) {
	Test_TokenizeString("cmd a b c d");
	
	TEST_ASSERT_EQUAL(5, Test_Argc());
	TEST_ASSERT_EQUAL_STRING("cmd", Test_Argv(0));
	TEST_ASSERT_EQUAL_STRING("a", Test_Argv(1));
	TEST_ASSERT_EQUAL_STRING("b", Test_Argv(2));
	TEST_ASSERT_EQUAL_STRING("c", Test_Argv(3));
	TEST_ASSERT_EQUAL_STRING("d", Test_Argv(4));
}

// Test: arguments with hyphens and underscores
void test_Cmd_Argv_SpecialNames(void) {
	Test_TokenizeString("bind my-key_1 +attack");
	
	TEST_ASSERT_EQUAL(3, Test_Argc());
	TEST_ASSERT_EQUAL_STRING("bind", Test_Argv(0));
	TEST_ASSERT_EQUAL_STRING("my-key_1", Test_Argv(1));
	TEST_ASSERT_EQUAL_STRING("+attack", Test_Argv(2));
}

// ============================================================================
// TESTS: Cmd_TokenizeString
// ============================================================================

// Test: tokenize simple command
void test_Cmd_TokenizeString_SimpleCommand(void) {
	Test_TokenizeString("echo hello");
	
	TEST_ASSERT_EQUAL(2, Test_Argc());
	TEST_ASSERT_EQUAL_STRING("echo", Test_Argv(0));
	TEST_ASSERT_EQUAL_STRING("hello", Test_Argv(1));
}

// Test: tokenize command with tabs
void test_Cmd_TokenizeString_Tabs(void) {
	Test_TokenizeString("cmd\targ1\targ2");
	
	TEST_ASSERT_EQUAL(3, Test_Argc());
	TEST_ASSERT_EQUAL_STRING("cmd", Test_Argv(0));
	TEST_ASSERT_EQUAL_STRING("arg1", Test_Argv(1));
	TEST_ASSERT_EQUAL_STRING("arg2", Test_Argv(2));
}

// Test: tokenize with semicolon (should be treated as argument)
void test_Cmd_TokenizeString_Semicolon(void) {
	Test_TokenizeString("bind key ;");
	
	TEST_ASSERT_EQUAL(3, Test_Argc());
	TEST_ASSERT_EQUAL_STRING("bind", Test_Argv(0));
	TEST_ASSERT_EQUAL_STRING("key", Test_Argv(1));
	TEST_ASSERT_EQUAL_STRING(";", Test_Argv(2));
}

// Test: tokenize long quoted string
void test_Cmd_TokenizeString_LongQuotedString(void) {
	Test_TokenizeString("say \"This is a very long message with many words\"");
	
	TEST_ASSERT_EQUAL(2, Test_Argc());
	TEST_ASSERT_EQUAL_STRING("say", Test_Argv(0));
	TEST_ASSERT_EQUAL_STRING("This is a very long message with many words", Test_Argv(1));
}

// Test: tokenize quotes within quotes behavior
void test_Cmd_TokenizeString_NestedQuotes(void) {
	Test_TokenizeString("cmd \"arg with \\\"quotes\\\"\"");
	
	// Our simple parser doesn't handle escaped quotes, so this will just parse the outer quotes
	TEST_ASSERT_EQUAL(2, Test_Argc());
	TEST_ASSERT_EQUAL_STRING("cmd", Test_Argv(0));
}

// Test: tokenize command with equals sign
void test_Cmd_TokenizeString_EqualsSign(void) {
	Test_TokenizeString("set var=value");
	
	TEST_ASSERT_EQUAL(2, Test_Argc());
	TEST_ASSERT_EQUAL_STRING("set", Test_Argv(0));
	TEST_ASSERT_EQUAL_STRING("var=value", Test_Argv(1));
}

// Test: tokenize paths with slashes
void test_Cmd_TokenizeString_Paths(void) {
	Test_TokenizeString("exec configs/myconfig.cfg");
	
	TEST_ASSERT_EQUAL(2, Test_Argc());
	TEST_ASSERT_EQUAL_STRING("exec", Test_Argv(0));
	TEST_ASSERT_EQUAL_STRING("configs/myconfig.cfg", Test_Argv(1));
}

// Test: tokenize with backslashes
void test_Cmd_TokenizeString_Backslashes(void) {
	Test_TokenizeString("exec C:\\configs\\test.cfg");
	
	TEST_ASSERT_EQUAL(2, Test_Argc());
	TEST_ASSERT_EQUAL_STRING("exec", Test_Argv(0));
	TEST_ASSERT_EQUAL_STRING("C:\\configs\\test.cfg", Test_Argv(1));
}

// Test: tokenize quoted path with spaces
void test_Cmd_TokenizeString_QuotedPath(void) {
	Test_TokenizeString("exec \"C:\\My Configs\\config.cfg\"");
	
	TEST_ASSERT_EQUAL(2, Test_Argc());
	TEST_ASSERT_EQUAL_STRING("exec", Test_Argv(0));
	TEST_ASSERT_EQUAL_STRING("C:\\My Configs\\config.cfg", Test_Argv(1));
}

// Test: tokenize with mixed case
void test_Cmd_TokenizeString_MixedCase(void) {
	Test_TokenizeString("ConNeCt MySeRvEr");
	
	TEST_ASSERT_EQUAL(2, Test_Argc());
	TEST_ASSERT_EQUAL_STRING("ConNeCt", Test_Argv(0));
	TEST_ASSERT_EQUAL_STRING("MySeRvEr", Test_Argv(1));
}

// Test: tokenize numbers and decimals
void test_Cmd_TokenizeString_Numbers(void) {
	Test_TokenizeString("set sensitivity 3.5");
	
	TEST_ASSERT_EQUAL(3, Test_Argc());
	TEST_ASSERT_EQUAL_STRING("set", Test_Argv(0));
	TEST_ASSERT_EQUAL_STRING("sensitivity", Test_Argv(1));
	TEST_ASSERT_EQUAL_STRING("3.5", Test_Argv(2));
}

// Test: tokenize negative numbers
void test_Cmd_TokenizeString_NegativeNumbers(void) {
	Test_TokenizeString("setenv offset -100");
	
	TEST_ASSERT_EQUAL(3, Test_Argc());
	TEST_ASSERT_EQUAL_STRING("setenv", Test_Argv(0));
	TEST_ASSERT_EQUAL_STRING("offset", Test_Argv(1));
	TEST_ASSERT_EQUAL_STRING("-100", Test_Argv(2));
}

// Test: tokenize URL
void test_Cmd_TokenizeString_URL(void) {
	Test_TokenizeString("connect example.com:27910");
	
	TEST_ASSERT_EQUAL(2, Test_Argc());
	TEST_ASSERT_EQUAL_STRING("connect", Test_Argv(0));
	TEST_ASSERT_EQUAL_STRING("example.com:27910", Test_Argv(1));
}

// Test: tokenize with commas
void test_Cmd_TokenizeString_Commas(void) {
	Test_TokenizeString("list item1,item2,item3");
	
	TEST_ASSERT_EQUAL(2, Test_Argc());
	TEST_ASSERT_EQUAL_STRING("list", Test_Argv(0));
	TEST_ASSERT_EQUAL_STRING("item1,item2,item3", Test_Argv(1));
}

// Test: tokenize IP address
void test_Cmd_TokenizeString_IPAddress(void) {
	Test_TokenizeString("connect 192.168.1.1:27910");
	
	TEST_ASSERT_EQUAL(2, Test_Argc());
	TEST_ASSERT_EQUAL_STRING("connect", Test_Argv(0));
	TEST_ASSERT_EQUAL_STRING("192.168.1.1:27910", Test_Argv(1));
}

// Test: tokenize with plus/minus prefixes
void test_Cmd_TokenizeString_PrefixedCommands(void) {
	Test_TokenizeString("bind key +forward");
	
	TEST_ASSERT_EQUAL(3, Test_Argc());
	TEST_ASSERT_EQUAL_STRING("bind", Test_Argv(0));
	TEST_ASSERT_EQUAL_STRING("key", Test_Argv(1));
	TEST_ASSERT_EQUAL_STRING("+forward", Test_Argv(2));
}

// Test: tokenize very long command line
void test_Cmd_TokenizeString_LongCommandLine(void) {
	Test_TokenizeString("cmd a b c d e f g h i j k l m n o p q r s t u v w x y z");
	
	TEST_ASSERT_EQUAL(27, Test_Argc());
	TEST_ASSERT_EQUAL_STRING("cmd", Test_Argv(0));
	TEST_ASSERT_EQUAL_STRING("a", Test_Argv(1));
	TEST_ASSERT_EQUAL_STRING("z", Test_Argv(26));
}

// Test: tokenize with multiple quote pairs
void test_Cmd_TokenizeString_MultipleQuotePairs(void) {
	Test_TokenizeString("echo \"hello\" \"world\" \"test\"");
	
	TEST_ASSERT_EQUAL(4, Test_Argc());
	TEST_ASSERT_EQUAL_STRING("echo", Test_Argv(0));
	TEST_ASSERT_EQUAL_STRING("hello", Test_Argv(1));
	TEST_ASSERT_EQUAL_STRING("world", Test_Argv(2));
	TEST_ASSERT_EQUAL_STRING("test", Test_Argv(3));
}

// Test: tokenize with parentheses
void test_Cmd_TokenizeString_Parentheses(void) {
	Test_TokenizeString("calc (1+2)*3");
	
	TEST_ASSERT_EQUAL(2, Test_Argc());
	TEST_ASSERT_EQUAL_STRING("calc", Test_Argv(0));
	TEST_ASSERT_EQUAL_STRING("(1+2)*3", Test_Argv(1));
}

// Test: tokenize with brackets
void test_Cmd_TokenizeString_Brackets(void) {
	Test_TokenizeString("set array[0] value");
	
	TEST_ASSERT_EQUAL(3, Test_Argc());
	TEST_ASSERT_EQUAL_STRING("set", Test_Argv(0));
	TEST_ASSERT_EQUAL_STRING("array[0]", Test_Argv(1));
	TEST_ASSERT_EQUAL_STRING("value", Test_Argv(2));
}


int main(void) {
	UNITY_BEGIN();

	// Command registration
	RUN_TEST(test_Cmd_AddCommand_NewCommand);
	RUN_TEST(test_Cmd_AddCommand_NullName);
	RUN_TEST(test_Cmd_AddCommand_NullFunction);
	RUN_TEST(test_Cmd_AddCommand_NullDescription);
	RUN_TEST(test_Cmd_AddCommand_Duplicate);
	RUN_TEST(test_Cmd_AddCommand_Multiple);
	RUN_TEST(test_Cmd_AddCommand_CaseSensitive);
	RUN_TEST(test_Cmd_AddCommand_LongName);
	RUN_TEST(test_Cmd_AddCommand_SpecialChars);
	RUN_TEST(test_Cmd_AddCommand_EmptyDescription);

	// Command lookup
	RUN_TEST(test_Cmd_Exists_Found);
	RUN_TEST(test_Cmd_Exists_NotFound);
	RUN_TEST(test_Cmd_Exists_NullName);
	RUN_TEST(test_Cmd_Exists_EmptyString);
	RUN_TEST(test_Cmd_Exists_ExactMatch);
	RUN_TEST(test_Cmd_Exists_ManyCommands);

	// Command removal
	RUN_TEST(test_Cmd_RemoveCommand_Success);
	RUN_TEST(test_Cmd_RemoveCommand_NullPointer);
	RUN_TEST(test_Cmd_RemoveCommand_Selective);
	RUN_TEST(test_Cmd_RemoveCommand_RemoveAll);
	RUN_TEST(test_Cmd_RemoveCommand_ReAdd);
	RUN_TEST(test_Cmd_RemoveCommand_DoubleRemove);

	// Integration tests
	RUN_TEST(test_Cmd_Integration_Complete);
	RUN_TEST(test_Cmd_Integration_ManyCommands);
	RUN_TEST(test_Cmd_Integration_AlternatingOps);

	// Argument parsing
	RUN_TEST(test_Cmd_Argc_NoArgs);
	RUN_TEST(test_Cmd_Argc_OneArg);
	RUN_TEST(test_Cmd_Argc_MultipleArgs);
	RUN_TEST(test_Cmd_Argc_Empty);
	RUN_TEST(test_Cmd_Argc_WhitespaceOnly);
	RUN_TEST(test_Cmd_Argv_QuotedString);
	RUN_TEST(test_Cmd_Argv_MultipleQuoted);
	RUN_TEST(test_Cmd_Argv_MixedQuoted);
	RUN_TEST(test_Cmd_Argv_EmptyQuoted);
	RUN_TEST(test_Cmd_Argv_OutOfRange);
	RUN_TEST(test_Cmd_Argv_Boundaries);
	RUN_TEST(test_Cmd_Argc_ExtraWhitespace);
	RUN_TEST(test_Cmd_Argc_LeadingWhitespace);
	RUN_TEST(test_Cmd_Argc_TrailingWhitespace);
	RUN_TEST(test_Cmd_Argv_QuotedAtStart);
	RUN_TEST(test_Cmd_Argv_QuotedSpecialChars);
	RUN_TEST(test_Cmd_Argc_ManyArgs);
	RUN_TEST(test_Cmd_Argv_QuotedNumbers);
	RUN_TEST(test_Cmd_Argv_SingleChars);
	RUN_TEST(test_Cmd_Argv_SpecialNames);

	// Tokenization tests
	RUN_TEST(test_Cmd_TokenizeString_SimpleCommand);
	RUN_TEST(test_Cmd_TokenizeString_Tabs);
	RUN_TEST(test_Cmd_TokenizeString_Semicolon);
	RUN_TEST(test_Cmd_TokenizeString_LongQuotedString);
	RUN_TEST(test_Cmd_TokenizeString_NestedQuotes);
	RUN_TEST(test_Cmd_TokenizeString_EqualsSign);
	RUN_TEST(test_Cmd_TokenizeString_Paths);
	RUN_TEST(test_Cmd_TokenizeString_Backslashes);
	RUN_TEST(test_Cmd_TokenizeString_QuotedPath);
	RUN_TEST(test_Cmd_TokenizeString_MixedCase);
	RUN_TEST(test_Cmd_TokenizeString_Numbers);
	RUN_TEST(test_Cmd_TokenizeString_NegativeNumbers);
	RUN_TEST(test_Cmd_TokenizeString_URL);
	RUN_TEST(test_Cmd_TokenizeString_Commas);
	RUN_TEST(test_Cmd_TokenizeString_IPAddress);
	RUN_TEST(test_Cmd_TokenizeString_PrefixedCommands);
	RUN_TEST(test_Cmd_TokenizeString_LongCommandLine);
	RUN_TEST(test_Cmd_TokenizeString_MultipleQuotePairs);
	RUN_TEST(test_Cmd_TokenizeString_Parentheses);
	RUN_TEST(test_Cmd_TokenizeString_Brackets);

	return UNITY_END();
}
