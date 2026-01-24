/* Unity Test Framework - Minimal Single-Header Version
 * Based on Unity by ThrowTheSwitch.org
 * Simplified for EGL testing needs
 * License: MIT
 */

#ifndef UNITY_H
#define UNITY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Test Counters */
static int Unity_TestsRun = 0;
static int Unity_TestsFailed = 0;
static int Unity_CurrentTestFailed = 0;
static const char* Unity_CurrentTestName = NULL;

/* Colors for terminal output */
#define UNITY_COLOR_RED    "\033[31m"
#define UNITY_COLOR_GREEN  "\033[32m"
#define UNITY_COLOR_YELLOW "\033[33m"
#define UNITY_COLOR_RESET  "\033[0m"

/* Core Macros */
#define UNITY_BEGIN() \
    do { \
        Unity_TestsRun = 0; \
        Unity_TestsFailed = 0; \
        printf("\n" UNITY_COLOR_YELLOW "=== EGL Unit Tests ===" UNITY_COLOR_RESET "\n\n"); \
    } while(0)

#define UNITY_END() \
    ( printf("\n" UNITY_COLOR_YELLOW "=======================" UNITY_COLOR_RESET "\n"), \
      printf("Tests Run: %d\n", Unity_TestsRun), \
      printf(UNITY_COLOR_GREEN "Passed: %d" UNITY_COLOR_RESET "\n", Unity_TestsRun - Unity_TestsFailed), \
      (Unity_TestsFailed > 0 ? printf(UNITY_COLOR_RED "Failed: %d" UNITY_COLOR_RESET "\n", Unity_TestsFailed) : 0), \
      (Unity_TestsFailed > 0 ? 1 : 0) )

#define RUN_TEST(func) \
    do { \
        Unity_CurrentTestFailed = 0; \
        Unity_CurrentTestName = #func; \
        Unity_TestsRun++; \
        printf("  %s ... ", #func); \
        fflush(stdout); \
        func(); \
        if (Unity_CurrentTestFailed) { \
            printf(UNITY_COLOR_RED "FAIL" UNITY_COLOR_RESET "\n"); \
            Unity_TestsFailed++; \
        } else { \
            printf(UNITY_COLOR_GREEN "PASS" UNITY_COLOR_RESET "\n"); \
        } \
    } while(0)

/* Assertion Helpers */
#define UNITY_FAIL_MESSAGE(msg) \
    do { \
        Unity_CurrentTestFailed = 1; \
        printf("\n    ASSERTION FAILED: %s\n    at %s:%d\n    ", msg, __FILE__, __LINE__); \
    } while(0)

/* Assertions */
#define TEST_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            UNITY_FAIL_MESSAGE("Expected TRUE, was FALSE"); \
        } \
    } while(0)

#define TEST_ASSERT_TRUE(condition) TEST_ASSERT(condition)
#define TEST_ASSERT_FALSE(condition) TEST_ASSERT(!(condition))

#define TEST_ASSERT_EQUAL(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            Unity_CurrentTestFailed = 1; \
            printf("\n    ASSERTION FAILED: Expected %d, was %d\n    at %s:%d\n    ", \
                   (int)(expected), (int)(actual), __FILE__, __LINE__); \
        } \
    } while(0)

#define TEST_ASSERT_EQUAL_HEX32(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            Unity_CurrentTestFailed = 1; \
            printf("\n    ASSERTION FAILED: Expected 0x%08X, was 0x%08X\n    at %s:%d\n    ", \
                   (unsigned int)(expected), (unsigned int)(actual), __FILE__, __LINE__); \
        } \
    } while(0)

#define TEST_ASSERT_EQUAL_STRING(expected, actual) \
    do { \
        if (strcmp((expected), (actual)) != 0) { \
            Unity_CurrentTestFailed = 1; \
            printf("\n    ASSERTION FAILED: Expected \"%s\", was \"%s\"\n    at %s:%d\n    ", \
                   (expected), (actual), __FILE__, __LINE__); \
        } \
    } while(0)

#define TEST_ASSERT_NULL(pointer) \
    do { \
        if ((pointer) != NULL) { \
            Unity_CurrentTestFailed = 1; \
            printf("\n    ASSERTION FAILED: Expected NULL\n    at %s:%d\n    ", __FILE__, __LINE__); \
        } \
    } while(0)

#define TEST_ASSERT_NOT_NULL(pointer) \
    do { \
        if ((pointer) == NULL) { \
            Unity_CurrentTestFailed = 1; \
            printf("\n    ASSERTION FAILED: Expected non-NULL\n    at %s:%d\n    ", __FILE__, __LINE__); \
        } \
    } while(0)

#define TEST_ASSERT_EQUAL_PTR(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            Unity_CurrentTestFailed = 1; \
            printf("\n    ASSERTION FAILED: Expected ptr %p, was %p\n    at %s:%d\n    ", \
                   (void*)(expected), (void*)(actual), __FILE__, __LINE__); \
        } \
    } while(0)

#define TEST_ASSERT_FLOAT_WITHIN(delta, expected, actual) \
    do { \
        float _diff = fabsf((float)(expected) - (float)(actual)); \
        if (_diff > (float)(delta)) { \
            Unity_CurrentTestFailed = 1; \
            printf("\n    ASSERTION FAILED: Expected %f +/- %f, was %f (diff: %f)\n    at %s:%d\n    ", \
                   (float)(expected), (float)(delta), (float)(actual), _diff, __FILE__, __LINE__); \
        } \
    } while(0)

#define TEST_ASSERT_LESS_THAN(threshold, actual) \
    do { \
        if ((actual) >= (threshold)) { \
            Unity_CurrentTestFailed = 1; \
            printf("\n    ASSERTION FAILED: Expected < %d, was %d\n    at %s:%d\n    ", \
                   (int)(threshold), (int)(actual), __FILE__, __LINE__); \
        } \
    } while(0)

#define TEST_ASSERT_GREATER_THAN(threshold, actual) \
    do { \
        if ((actual) <= (threshold)) { \
            Unity_CurrentTestFailed = 1; \
            printf("\n    ASSERTION FAILED: Expected > %d, was %d\n    at %s:%d\n    ", \
                   (int)(threshold), (int)(actual), __FILE__, __LINE__); \
        } \
    } while(0)

#define TEST_ASSERT_EQUAL_MEMORY(expected, actual, len) \
    do { \
        if (memcmp((expected), (actual), (len)) != 0) { \
            Unity_CurrentTestFailed = 1; \
            printf("\n    ASSERTION FAILED: Memory mismatch (%d bytes)\n    at %s:%d\n    ", \
                   (int)(len), __FILE__, __LINE__); \
        } \
    } while(0)

#define TEST_FAIL_MESSAGE(message) \
    do { \
        UNITY_FAIL_MESSAGE(message); \
    } while(0)

#define TEST_IGNORE_MESSAGE(message) \
    do { \
        printf("\n    IGNORED: %s\n    ", message); \
    } while(0)

#define TEST_IGNORE() TEST_IGNORE_MESSAGE("Test ignored")

/* Default setUp/tearDown if not provided */
#ifndef UNITY_NO_SETUP_TEARDOWN
void setUp(void) __attribute__((weak));
void tearDown(void) __attribute__((weak));
void setUp(void) {}
void tearDown(void) {}
#endif

#endif /* UNITY_H */
