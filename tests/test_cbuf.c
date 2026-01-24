/*
====================
test_cbuf.c

Unit tests for Cbuf_AddText, Cbuf_InsertText, Cbuf_Execute
====================
*/

#include "../unity/unity.h"
#include <string.h>

// Provide minimal stubs and access to internal buffer by including cbuf.c directly
static char lastExecuted[1024];

void Cmd_ExecuteString (char *s) {
    strncpy(lastExecuted, s, sizeof(lastExecuted)-1);
    lastExecuted[sizeof(lastExecuted)-1] = '\0';
}

void Com_Printf(int flags, char *fmt, ...) { (void)flags; (void)fmt; }

void Com_DevPrintf(int flags, char *fmt, ...) { (void)flags; (void)fmt; }

int com_cmdWait = 0;

cVar_t dedicated_stub = { .intVal = 0 };

#define dedicated (&dedicated_stub)

#include "../common/cbuf.c"

void setUp(void) {
    // Init before each test
    memset(lastExecuted, 0, sizeof(lastExecuted));
    Cbuf_Init();
}

void tearDown(void) {
}

void test_Cbuf_AddText_and_Execute_simple(void) {
    Cbuf_AddText("say hello\n");
    Cbuf_Execute();
    TEST_ASSERT_EQUAL_STRING("say hello", lastExecuted);
}

void test_Cbuf_InsertText_precedence(void) {
    // Add second, insert first
    Cbuf_AddText("second\n");
    Cbuf_InsertText("first\n");

    // Execute first command
    Cbuf_Execute();
    TEST_ASSERT_EQUAL_STRING("first", lastExecuted);

    // Execute second command
    Cbuf_Execute();
    TEST_ASSERT_EQUAL_STRING("second", lastExecuted);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_Cbuf_AddText_and_Execute_simple);
RUN_TEST(test_Cbuf_InsertText_precedence);

RUN_TEST(test_Cbuf_AddText_append);
RUN_TEST(test_Cbuf_Execute_multiple);

    return UNITY_END();
}
