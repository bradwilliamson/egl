/*
 * test_com_parse.c - Unit tests for Com_Parse config file tokenizer
 */

#include "../unity/unity.h"
#include <string.h>

#define MAX_TOKEN_CHARS 512

// Mock implementation of Com_Parse
static char com_token[MAX_TOKEN_CHARS];

static char *Test_Com_Parse(char **dataPtr) {
	int		c;
	int		len;
	char	*data;

	data = *dataPtr;
	len = 0;
	com_token[0] = 0;
	
	if (!data) {
		*dataPtr = NULL;
		return "";
	}
		
	// Skip whitespace
skipwhite:
	while ((c = *data) <= ' ') {
		if (c == 0) {
			*dataPtr = NULL;
			return "";
		}
		data++;
	}
	
	// Skip // comments
	if (c == '/' && data[1] == '/') {
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}

	// Handle quoted strings specially
	if (c == '\"') {
		data++;
		for ( ; ; ) {
			c = *data++;
			if (c == '\"' || !c) {
				com_token[len] = 0;
				*dataPtr = data;
				return com_token;
			}
			if (len < MAX_TOKEN_CHARS) {
				com_token[len] = c;
				len++;
			}
		}
	}

	// Parse a regular word
	do {
		if (len < MAX_TOKEN_CHARS) {
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	} while (c > 32);

	if (len >= MAX_TOKEN_CHARS)
		len = 0;
	com_token[len] = 0;

	*dataPtr = data;
	return com_token;
}

// ============================================================================
// TESTS: Basic Unquoted Tokens
// ============================================================================

// Test: parse simple unquoted token
void test_Com_Parse_SimpleToken(void) {
	char buffer[] = "token";
	char *data = buffer;
	char *result = Test_Com_Parse(&data);
	
	TEST_ASSERT_EQUAL_STRING("token", result);
	// After parsing last token, data points to null terminator
	// Parsing again will return empty and set data to NULL
	char *next = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("", next);
	TEST_ASSERT_NULL(data);
}

// Test: parse multiple tokens separated by space
void test_Com_Parse_MultipleTokens(void) {
	char buffer[] = "first second third";
	char *data = buffer;
	
	char *token1 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("first", token1);
	TEST_ASSERT_NOT_NULL(data);
	
	char *token2 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("second", token2);
	TEST_ASSERT_NOT_NULL(data);
	
	char *token3 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("third", token3);
	// After last token, data points to '\0', not NULL yet
	// Need to parse again to get NULL
	char *token4 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("", token4);
	TEST_ASSERT_NULL(data);
}

// Test: parse token with leading whitespace
void test_Com_Parse_LeadingWhitespace(void) {
	char buffer[] = "   token";
	char *data = buffer;
	
	char *result = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("token", result);
}

// Test: parse token with trailing whitespace
void test_Com_Parse_TrailingWhitespace(void) {
	char buffer[] = "token   ";
	char *data = buffer;
	
	char *result = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("token", result);
}

// Test: parse token with tabs
void test_Com_Parse_TabSeparated(void) {
	char buffer[] = "first\tsecond\tthird";
	char *data = buffer;
	
	char *token1 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("first", token1);
	
	char *token2 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("second", token2);
	
	char *token3 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("third", token3);
}

// Test: parse empty string
void test_Com_Parse_EmptyString(void) {
	char buffer[] = "";
	char *data = buffer;
	
	char *result = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("", result);
	TEST_ASSERT_NULL(data);
}

// Test: parse NULL pointer
void test_Com_Parse_NullPointer(void) {
	char *data = NULL;
	
	char *result = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("", result);
	TEST_ASSERT_NULL(data);
}

// Test: parse only whitespace
void test_Com_Parse_OnlyWhitespace(void) {
	char buffer[] = "   \t\n  ";
	char *data = buffer;
	
	char *result = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("", result);
	TEST_ASSERT_NULL(data);
}

// ============================================================================
// TESTS: Quoted Strings
// ============================================================================

// Test: parse simple quoted string
void test_Com_Parse_QuotedString(void) {
	char buffer[] = "\"hello world\"";
	char *data = buffer;
	
	char *result = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("hello world", result);
}

// Test: quoted string preserves spaces
void test_Com_Parse_QuotedPreservesSpaces(void) {
	char buffer[] = "\"  multiple   spaces  \"";
	char *data = buffer;
	
	char *result = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("  multiple   spaces  ", result);
}

// Test: quoted string with special characters
void test_Com_Parse_QuotedSpecialChars(void) {
	char buffer[] = "\"!@#$%^&*()_+-=[]{}|;:,.<>?\"";
	char *data = buffer;
	
	char *result = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("!@#$%^&*()_+-=[]{}|;:,.<>?", result);
}

// Test: empty quoted string
void test_Com_Parse_QuotedEmpty(void) {
	char buffer[] = "\"\"";
	char *data = buffer;
	
	char *result = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("", result);
}

// Test: quoted string followed by unquoted token
void test_Com_Parse_QuotedThenUnquoted(void) {
	char buffer[] = "\"quoted\" unquoted";
	char *data = buffer;
	
	char *token1 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("quoted", token1);
	
	char *token2 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("unquoted", token2);
}

// Test: unquoted token followed by quoted string
void test_Com_Parse_UnquotedThenQuoted(void) {
	char buffer[] = "unquoted \"quoted\"";
	char *data = buffer;
	
	char *token1 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("unquoted", token1);
	
	char *token2 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("quoted", token2);
}

// Test: unterminated quoted string (ends at null)
void test_Com_Parse_UnterminatedQuoted(void) {
	char buffer[] = "\"unterminated";
	char *data = buffer;
	
	char *result = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("unterminated", result);
}

// Test: quoted string with newlines
void test_Com_Parse_QuotedWithNewlines(void) {
	char buffer[] = "\"line1\nline2\nline3\"";
	char *data = buffer;
	
	char *result = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("line1\nline2\nline3", result);
}

// Test: quoted string with tabs
void test_Com_Parse_QuotedWithTabs(void) {
	char buffer[] = "\"column1\tcolumn2\tcolumn3\"";
	char *data = buffer;
	
	char *result = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("column1\tcolumn2\tcolumn3", result);
}

// ============================================================================
// TESTS: Comments
// ============================================================================

// Test: skip single-line comment
void test_Com_Parse_SingleLineComment(void) {
	char buffer[] = "// this is a comment";
	char *data = buffer;
	
	char *result = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("", result);
	TEST_ASSERT_NULL(data);
}

// Test: token after comment
void test_Com_Parse_TokenAfterComment(void) {
	char buffer[] = "// comment\ntoken";
	char *data = buffer;
	
	char *result = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("token", result);
}

// Test: token before comment
void test_Com_Parse_TokenBeforeComment(void) {
	char buffer[] = "token // comment";
	char *data = buffer;
	
	char *result = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("token", result);
}

// Test: multiple lines with comments
void test_Com_Parse_MultipleLinesWithComments(void) {
	char buffer[] = "first\n// comment\nsecond\n// another comment\nthird";
	char *data = buffer;
	
	char *token1 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("first", token1);
	
	char *token2 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("second", token2);
	
	char *token3 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("third", token3);
}

// Test: comment without newline at end
void test_Com_Parse_CommentNoNewline(void) {
	char buffer[] = "token // comment";
	char *data = buffer;
	
	char *token1 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("token", token1);
	
	char *token2 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("", token2);
}

// Test: only comment
void test_Com_Parse_OnlyComment(void) {
	char buffer[] = "// only a comment";
	char *data = buffer;
	
	char *result = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("", result);
}

// Test: double slash in quoted string (not a comment)
void test_Com_Parse_DoubleSlashInQuoted(void) {
	char buffer[] = "\"http://example.com\"";
	char *data = buffer;
	
	char *result = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("http://example.com", result);
}

// Test: single slash (not a comment)
void test_Com_Parse_SingleSlash(void) {
	char buffer[] = "path/to/file";
	char *data = buffer;
	
	char *result = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("path/to/file", result);
}

// ============================================================================
// TESTS: Newlines and Whitespace Handling
// ============================================================================

// Test: newline as token separator
void test_Com_Parse_NewlineAsSeparator(void) {
	char buffer[] = "first\nsecond\nthird";
	char *data = buffer;
	
	char *token1 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("first", token1);
	
	char *token2 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("second", token2);
	
	char *token3 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("third", token3);
}

// Test: multiple newlines
void test_Com_Parse_MultipleNewlines(void) {
	char buffer[] = "first\n\n\nsecond";
	char *data = buffer;
	
	char *token1 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("first", token1);
	
	char *token2 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("second", token2);
}

// Test: carriage return and newline
void test_Com_Parse_CRLF(void) {
	char buffer[] = "first\r\nsecond";
	char *data = buffer;
	
	char *token1 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("first", token1);
	
	char *token2 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("second", token2);
}

// Test: mixed whitespace
void test_Com_Parse_MixedWhitespace(void) {
	char buffer[] = "  first \t\n  second  \r\n\t third  ";
	char *data = buffer;
	
	char *token1 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("first", token1);
	
	char *token2 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("second", token2);
	
	char *token3 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("third", token3);
}

// ============================================================================
// TESTS: Config File Examples
// ============================================================================

// Test: key-value pair
void test_Com_Parse_KeyValuePair(void) {
	char buffer[] = "cvar_name \"value\"";
	char *data = buffer;
	
	char *key = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("cvar_name", key);
	
	char *value = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("value", value);
}

// Test: multiple key-value pairs
void test_Com_Parse_MultipleKeyValues(void) {
	char buffer[] = "name \"player\"\nfov 90\nsensitivity 3.5";
	char *data = buffer;
	
	char *key1 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("name", key1);
	char *val1 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("player", val1);
	
	char *key2 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("fov", key2);
	char *val2 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("90", val2);
	
	char *key3 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("sensitivity", key3);
	char *val3 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("3.5", val3);
}

// Test: config with comments
void test_Com_Parse_ConfigWithComments(void) {
	char buffer[] = "// Player settings\nname \"player\"\n// Graphics\nfov 90";
	char *data = buffer;
	
	char *key1 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("name", key1);
	char *val1 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("player", val1);
	
	char *key2 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("fov", key2);
	char *val2 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("90", val2);
}

// Test: entity definition format
void test_Com_Parse_EntityFormat(void) {
	char buffer[] = "{\n\"classname\" \"info_player_start\"\n\"origin\" \"0 0 24\"\n}";
	char *data = buffer;
	
	char *token1 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("{", token1);
	
	char *key1 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("classname", key1);
	char *val1 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("info_player_start", val1);
	
	char *key2 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("origin", key2);
	char *val2 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("0 0 24", val2);
	
	char *token2 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("}", token2);
}

// Test: command with arguments
void test_Com_Parse_CommandWithArgs(void) {
	char buffer[] = "exec autoexec.cfg";
	char *data = buffer;
	
	char *cmd = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("exec", cmd);
	
	char *arg = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("autoexec.cfg", arg);
}

// Test: bind command format
void test_Com_Parse_BindCommand(void) {
	char buffer[] = "bind w \"+forward\"";
	char *data = buffer;
	
	char *cmd = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("bind", cmd);
	
	char *key = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("w", key);
	
	char *action = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("+forward", action);
}

// ============================================================================
// TESTS: Edge Cases
// ============================================================================

// Test: very long token (up to MAX_TOKEN_CHARS)
void test_Com_Parse_LongToken(void) {
	char buffer[MAX_TOKEN_CHARS + 100];
	int i;
	
	// Create a token with MAX_TOKEN_CHARS-1 characters
	for (i = 0; i < MAX_TOKEN_CHARS - 1; i++) {
		buffer[i] = 'a';
	}
	buffer[i] = ' ';
	buffer[i + 1] = '\0';
	
	char *data = buffer;
	char *result = Test_Com_Parse(&data);
	
	TEST_ASSERT_EQUAL(MAX_TOKEN_CHARS - 1, strlen(result));
}

// Test: token exceeding MAX_TOKEN_CHARS (should be truncated or reset)
void test_Com_Parse_TooLongToken(void) {
	char buffer[MAX_TOKEN_CHARS + 100];
	int i;
	
	// Create a token exceeding MAX_TOKEN_CHARS
	for (i = 0; i < MAX_TOKEN_CHARS + 50; i++) {
		buffer[i] = 'a';
	}
	buffer[i] = ' ';
	buffer[i + 1] = '\0';
	
	char *data = buffer;
	char *result = Test_Com_Parse(&data);
	
	// Token should be empty (reset to 0) when exceeding MAX_TOKEN_CHARS
	TEST_ASSERT_EQUAL_STRING("", result);
}

// Test: quoted string exceeding MAX_TOKEN_CHARS
void test_Com_Parse_TooLongQuotedString(void) {
	char buffer[MAX_TOKEN_CHARS + 100];
	int i;
	
	buffer[0] = '\"';
	for (i = 1; i <= MAX_TOKEN_CHARS; i++) {
		buffer[i] = 'a';
	}
	buffer[i] = '\"';
	buffer[i + 1] = '\0';
	
	char *data = buffer;
	char *result = Test_Com_Parse(&data);
	
	// Should truncate at MAX_TOKEN_CHARS
	TEST_ASSERT_EQUAL(MAX_TOKEN_CHARS, strlen(result));
}

// Test: numbers as tokens
void test_Com_Parse_Numbers(void) {
	char buffer[] = "123 456.789 -10 +5";
	char *data = buffer;
	
	char *token1 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("123", token1);
	
	char *token2 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("456.789", token2);
	
	char *token3 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("-10", token3);
	
	char *token4 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("+5", token4);
}

// Test: special characters as tokens  
void test_Com_Parse_SpecialChars(void) {
	// Each special char separated by space becomes a token
	char buffer[] = "{ } [ ] ( ) ; : , . < > ! @ # $ % ^ & * - + = | / ?";
	char *data = buffer;
	int count = 0;
	
	while (data) {
		char *token = Test_Com_Parse(&data);
		if (token[0] == '\0')
			break;
		count++;
	}
	
	// 26 special chars: { } [ ] ( ) ; : , . < > ! @ # $ % ^ & * - + = | / ?
	TEST_ASSERT_EQUAL(26, count);
}

// Test: consecutive quotes
void test_Com_Parse_ConsecutiveQuotes(void) {
	char buffer[] = "\"first\"\"second\"";
	char *data = buffer;
	
	char *token1 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("first", token1);
	
	char *token2 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("second", token2);
}

// Test: quote at end of string
void test_Com_Parse_QuoteAtEnd(void) {
	char buffer[] = "token\"";
	char *data = buffer;
	
	char *token1 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("token\"", token1); // Quote is part of the token
	
	char *token2 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("", token2);
}

// ============================================================================
// TESTS: CVAR Config File Parsing
// ============================================================================

// Test: set command with quoted value
void test_Com_Parse_SetSensitivity(void) {
	char buffer[] = "set sensitivity \"3.5\"";
	char *data = buffer;
	
	char *cmd = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("set", cmd);
	
	char *cvar = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("sensitivity", cvar);
	
	char *value = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("3.5", value);
}

// Test: set command with unquoted numeric value
void test_Com_Parse_SetFov(void) {
	char buffer[] = "set fov 90";
	char *data = buffer;
	
	char *cmd = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("set", cmd);
	
	char *cvar = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("fov", cvar);
	
	char *value = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("90", value);
}

// Test: set command with quoted string value
void test_Com_Parse_SetName(void) {
	char buffer[] = "set name \"player\"";
	char *data = buffer;
	
	char *cmd = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("set", cmd);
	
	char *cvar = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("name", cvar);
	
	char *value = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("player", value);
}

// Test: seta command (archive flag)
void test_Com_Parse_Seta(void) {
	char buffer[] = "seta r_mode 4";
	char *data = buffer;
	
	char *cmd = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("seta", cmd);
	
	char *cvar = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("r_mode", cvar);
	
	char *value = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("4", value);
}

// Test: sets command (server flag)
void test_Com_Parse_Sets(void) {
	char buffer[] = "sets hostname \"My Server\"";
	char *data = buffer;
	
	char *cmd = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("sets", cmd);
	
	char *cvar = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("hostname", cvar);
	
	char *value = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("My Server", value);
}

// Test: setu command (userinfo flag)
void test_Com_Parse_Setu(void) {
	char buffer[] = "setu rate 25000";
	char *data = buffer;
	
	char *cmd = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("setu", cmd);
	
	char *cvar = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("rate", cvar);
	
	char *value = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("25000", value);
}

// Test: set with boolean value (0/1)
void test_Com_Parse_SetBoolean(void) {
	char buffer[] = "set cl_showfps 1";
	char *data = buffer;
	
	char *cmd = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("set", cmd);
	
	char *cvar = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("cl_showfps", cvar);
	
	char *value = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("1", value);
}

// Test: set with negative value
void test_Com_Parse_SetNegative(void) {
	char buffer[] = "set r_brightness \"-0.5\"";
	char *data = buffer;
	
	char *cmd = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("set", cmd);
	
	char *cvar = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("r_brightness", cvar);
	
	char *value = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("-0.5", value);
}

// Test: set with empty quoted value
void test_Com_Parse_SetEmpty(void) {
	char buffer[] = "set model \"\"";
	char *data = buffer;
	
	char *cmd = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("set", cmd);
	
	char *cvar = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("model", cvar);
	
	char *value = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("", value);
}

// Test: multiple set commands on separate lines
void test_Com_Parse_MultipleSets(void) {
	char buffer[] = "set fov 90\nset sensitivity 3.5\nset name \"player\"";
	char *data = buffer;
	
	// First set command
	char *cmd1 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("set", cmd1);
	char *cvar1 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("fov", cvar1);
	char *value1 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("90", value1);
	
	// Second set command
	char *cmd2 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("set", cmd2);
	char *cvar2 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("sensitivity", cvar2);
	char *value2 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("3.5", value2);
	
	// Third set command
	char *cmd3 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("set", cmd3);
	char *cvar3 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("name", cvar3);
	char *value3 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("player", value3);
}

// Test: set command with comment
void test_Com_Parse_SetWithComment(void) {
	char buffer[] = "set fov 90 // Field of view";
	char *data = buffer;
	
	char *cmd = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("set", cmd);
	
	char *cvar = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("fov", cvar);
	
	char *value = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("90", value);
	
	// Comment should be consumed, next parse returns empty
	char *next = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("", next);
}

// Test: set command after comment
void test_Com_Parse_SetAfterComment(void) {
	char buffer[] = "// Graphics settings\nset fov 90";
	char *data = buffer;
	
	char *cmd = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("set", cmd);
	
	char *cvar = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("fov", cvar);
	
	char *value = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("90", value);
}

// Test: set with value containing spaces (quoted)
void test_Com_Parse_SetValueWithSpaces(void) {
	char buffer[] = "set message \"Hello World From Server\"";
	char *data = buffer;
	
	char *cmd = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("set", cmd);
	
	char *cvar = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("message", cvar);
	
	char *value = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("Hello World From Server", value);
}

// Test: set with path value
void test_Com_Parse_SetPath(void) {
	char buffer[] = "set basedir \"C:\\\\games\\\\quake2\"";
	char *data = buffer;
	
	char *cmd = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("set", cmd);
	
	char *cvar = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("basedir", cvar);
	
	char *value = Test_Com_Parse(&data);
	// Com_Parse doesn't process escape sequences - backslashes are preserved literally
	TEST_ASSERT_EQUAL_STRING("C:\\\\games\\\\quake2", value);
}

// Test: set with color value (RGB)
void test_Com_Parse_SetColor(void) {
	char buffer[] = "set crosshair_color \"1.0 0.5 0.0\"";
	char *data = buffer;
	
	char *cmd = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("set", cmd);
	
	char *cvar = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("crosshair_color", cvar);
	
	char *value = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("1.0 0.5 0.0", value);
}

// Test: config file with mixed commands and comments
void test_Com_Parse_FullConfigExample(void) {
	char buffer[] = 
		"// Player configuration\n"
		"set name \"Player\"\n"
		"set fov 90\n"
		"\n"
		"// Graphics settings\n"
		"seta r_mode 4\n"
		"seta r_fullscreen 1\n"
		"\n"
		"// Network\n"
		"setu rate 25000";
	
	char *data = buffer;
	
	// Parse first set command
	char *cmd1 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("set", cmd1);
	char *cvar1 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("name", cvar1);
	char *value1 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("Player", value1);
	
	// Parse second set command
	char *cmd2 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("set", cmd2);
	char *cvar2 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("fov", cvar2);
	char *value2 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("90", value2);
	
	// Parse first seta command
	char *cmd3 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("seta", cmd3);
	char *cvar3 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("r_mode", cvar3);
	char *value3 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("4", value3);
	
	// Parse second seta command
	char *cmd4 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("seta", cmd4);
	char *cvar4 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("r_fullscreen", cvar4);
	char *value4 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("1", value4);
	
	// Parse setu command
	char *cmd5 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("setu", cmd5);
	char *cvar5 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("rate", cvar5);
	char *value5 = Test_Com_Parse(&data);
	TEST_ASSERT_EQUAL_STRING("25000", value5);
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main(void) {
	UNITY_BEGIN();

	// Basic unquoted tokens
	RUN_TEST(test_Com_Parse_SimpleToken);
	RUN_TEST(test_Com_Parse_MultipleTokens);
	RUN_TEST(test_Com_Parse_LeadingWhitespace);
	RUN_TEST(test_Com_Parse_TrailingWhitespace);
	RUN_TEST(test_Com_Parse_TabSeparated);
	RUN_TEST(test_Com_Parse_EmptyString);
	RUN_TEST(test_Com_Parse_NullPointer);
	RUN_TEST(test_Com_Parse_OnlyWhitespace);

	// Quoted strings
	RUN_TEST(test_Com_Parse_QuotedString);
	RUN_TEST(test_Com_Parse_QuotedPreservesSpaces);
	RUN_TEST(test_Com_Parse_QuotedSpecialChars);
	RUN_TEST(test_Com_Parse_QuotedEmpty);
	RUN_TEST(test_Com_Parse_QuotedThenUnquoted);
	RUN_TEST(test_Com_Parse_UnquotedThenQuoted);
	RUN_TEST(test_Com_Parse_UnterminatedQuoted);
	RUN_TEST(test_Com_Parse_QuotedWithNewlines);
	RUN_TEST(test_Com_Parse_QuotedWithTabs);

	// Comments
	RUN_TEST(test_Com_Parse_SingleLineComment);
	RUN_TEST(test_Com_Parse_TokenAfterComment);
	RUN_TEST(test_Com_Parse_TokenBeforeComment);
	RUN_TEST(test_Com_Parse_MultipleLinesWithComments);
	RUN_TEST(test_Com_Parse_CommentNoNewline);
	RUN_TEST(test_Com_Parse_OnlyComment);
	RUN_TEST(test_Com_Parse_DoubleSlashInQuoted);
	RUN_TEST(test_Com_Parse_SingleSlash);

	// Newlines and whitespace
	RUN_TEST(test_Com_Parse_NewlineAsSeparator);
	RUN_TEST(test_Com_Parse_MultipleNewlines);
	RUN_TEST(test_Com_Parse_CRLF);
	RUN_TEST(test_Com_Parse_MixedWhitespace);

	// Config file examples
	RUN_TEST(test_Com_Parse_KeyValuePair);
	RUN_TEST(test_Com_Parse_MultipleKeyValues);
	RUN_TEST(test_Com_Parse_ConfigWithComments);
	RUN_TEST(test_Com_Parse_EntityFormat);
	RUN_TEST(test_Com_Parse_CommandWithArgs);
	RUN_TEST(test_Com_Parse_BindCommand);

	// Edge cases
	RUN_TEST(test_Com_Parse_LongToken);
	RUN_TEST(test_Com_Parse_TooLongToken);
	RUN_TEST(test_Com_Parse_TooLongQuotedString);
	RUN_TEST(test_Com_Parse_Numbers);
	RUN_TEST(test_Com_Parse_SpecialChars);
	RUN_TEST(test_Com_Parse_ConsecutiveQuotes);
	RUN_TEST(test_Com_Parse_QuoteAtEnd);

	// CVAR config file parsing
	RUN_TEST(test_Com_Parse_SetSensitivity);
	RUN_TEST(test_Com_Parse_SetFov);
	RUN_TEST(test_Com_Parse_SetName);
	RUN_TEST(test_Com_Parse_Seta);
	RUN_TEST(test_Com_Parse_Sets);
	RUN_TEST(test_Com_Parse_Setu);
	RUN_TEST(test_Com_Parse_SetBoolean);
	RUN_TEST(test_Com_Parse_SetNegative);
	RUN_TEST(test_Com_Parse_SetEmpty);
	RUN_TEST(test_Com_Parse_MultipleSets);
	RUN_TEST(test_Com_Parse_SetWithComment);
	RUN_TEST(test_Com_Parse_SetAfterComment);
	RUN_TEST(test_Com_Parse_SetValueWithSpaces);
	RUN_TEST(test_Com_Parse_SetPath);
	RUN_TEST(test_Com_Parse_SetColor);
	RUN_TEST(test_Com_Parse_FullConfigExample);

	return UNITY_END();
}
