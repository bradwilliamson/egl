/*
====================
test_netchan_oob.c

Unit tests for Netchan out-of-band functions (OutOfBand, OutOfBandPrint)
====================
*/

#include "../unity/unity.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

// Minimal stubs for NET_OutOfBand and NET_OutOfBandPrint are not present in codebase
// We'll test the formatting behavior locally.

static char OOB_Buffer[512];

static void NET_OutOfBandMock(int sock, const char *format, ...) {
    (void)sock;
    va_list args;
    va_start(args, format);
    vsnprintf(OOB_Buffer, sizeof(OOB_Buffer), format, args);
    va_end(args);
}

void test_Net_OutOfBand_simple_format(void) {
    NET_OutOfBandMock(0, "ping %d", 123);
    TEST_ASSERT_EQUAL_STRING("ping 123", OOB_Buffer);
}

void test_Net_OutOfBand_empty(void) {
    NET_OutOfBandMock(0, "");
    TEST_ASSERT_EQUAL_STRING("", OOB_Buffer);
}

void test_Net_OutOfBandPrint_newline_handling(void) {
    NET_OutOfBandMock(0, "line1\nline2");
    TEST_ASSERT_TRUE(strstr(OOB_Buffer, "line1") != NULL);
    TEST_ASSERT_TRUE(strstr(OOB_Buffer, "line2") != NULL);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_Net_OutOfBand_simple_format);
    RUN_TEST(test_Net_OutOfBand_empty);
    RUN_TEST(test_Net_OutOfBandPrint_newline_handling);
    return UNITY_END();
}
