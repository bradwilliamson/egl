/*
 * test_byteswap.c - Unit tests for shared/byteswap.c
 * 
 * Tests byte-order conversion functions critical for network protocol.
 * Run with: make test_byteswap && ./test_byteswap
 */

#include "../unity/unity.h"
#include "shared/shared.h"

/* Stubs for functions needed by shared code */
void Com_Printf(comPrint_t flags, char *fmt, ...) {
    /* Silently ignore for tests */
}

/*
 * ============================================================================
 * Byte Swap Tests
 * 
 * These are critical for cross-platform network protocol compatibility.
 * Network byte order is big-endian; most modern CPUs are little-endian.
 * ============================================================================
 */

void test_LittleShort_identity(void) {
    /* On little-endian systems, should be identity */
    short val = 0x1234;
    short result = LittleShort(val);
    
    /* This test may behave differently on big-endian systems */
    /* For now, we test that roundtrip works */
    TEST_ASSERT_EQUAL(val, LittleShort(result));
}

void test_LittleLong_identity(void) {
    int val = 0x12345678;
    int result = LittleLong(val);
    TEST_ASSERT_EQUAL(val, LittleLong(result));
}

void test_LittleFloat_roundtrip(void) {
    float val = 123.456f;
    float result = LittleFloat(val);
    result = LittleFloat(result);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, val, result);
}

void test_BigShort_swap(void) {
    /* BigShort should swap bytes on little-endian */
    short val = 0x1234;
    short swapped = BigShort(val);
    short restored = BigShort(swapped);
    TEST_ASSERT_EQUAL(val, restored);
}

void test_BigLong_swap(void) {
    int val = 0x12345678;
    int swapped = BigLong(val);
    int restored = BigLong(swapped);
    TEST_ASSERT_EQUAL(val, restored);
}

void test_BigFloat_roundtrip(void) {
    float val = 987.654f;
    float swapped = BigFloat(val);
    float restored = BigFloat(swapped);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, val, restored);
}

void test_swap_zero(void) {
    /* Zero should always be zero regardless of byte order */
    TEST_ASSERT_EQUAL(0, LittleShort(0));
    TEST_ASSERT_EQUAL(0, LittleLong(0));
    TEST_ASSERT_EQUAL(0, BigShort(0));
    TEST_ASSERT_EQUAL(0, BigLong(0));
}

void test_swap_negative_one(void) {
    /* -1 (all bits set) should remain -1 */
    TEST_ASSERT_EQUAL(-1, LittleShort(-1));
    TEST_ASSERT_EQUAL(-1, LittleLong(-1));
    TEST_ASSERT_EQUAL(-1, BigShort(-1));
    TEST_ASSERT_EQUAL(-1, BigLong(-1));
}

void test_known_swap_value(void) {
    /* Test a specific known swap (assuming little-endian host) */
    /* 0x0102 in memory as bytes: 02 01 */
    /* After big-endian swap: 01 02 = 0x0102 as value on LE, 0x0201 on BE */
    
    short val = 0x0102;
    short swapped = BigShort(val);
    
    /* The swapped value, when interpreted, should have bytes reversed */
    /* This is a sanity check that swap is actually happening */
    /* On little-endian: BigShort(0x0102) should give 0x0201 */
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    TEST_ASSERT_EQUAL(0x0201, swapped);
#endif
    /* Always verify roundtrip works */
    TEST_ASSERT_EQUAL(val, BigShort(swapped));
}

/*
 * ============================================================================
 * Test Runner
 * ============================================================================
 */

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_LittleShort_identity);
    RUN_TEST(test_LittleLong_identity);
    RUN_TEST(test_LittleFloat_roundtrip);
    RUN_TEST(test_BigShort_swap);
    RUN_TEST(test_BigLong_swap);
    RUN_TEST(test_BigFloat_roundtrip);
    RUN_TEST(test_swap_zero);
    RUN_TEST(test_swap_negative_one);
    RUN_TEST(test_known_swap_value);
    
    return UNITY_END();
}
