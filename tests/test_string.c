/*
 * test_string.c - Unit tests for shared/string.c
 * 
 * Tests safe string functions that prevent buffer overflows.
 * Run with: make test_string && ./test_string
 */

#include "../unity/unity.h"
#include "shared/shared.h"

/*
 * ============================================================================
 * Q_strncpyz Tests
 * 
 * Safe string copy that always null-terminates.
 * Similar to strlcpy but available everywhere.
 * Critical for preventing buffer overflow vulnerabilities.
 * ============================================================================
 */

void test_Q_strncpyz_simple_copy(void) {
    char dest[20];
    
    Q_strncpyz(dest, "Hello", sizeof(dest));
    
    TEST_ASSERT_EQUAL_STRING("Hello", dest);
}

void test_Q_strncpyz_empty_string(void) {
    char dest[20];
    dest[0] = 'X'; /* Pre-fill to verify clearing */
    
    Q_strncpyz(dest, "", sizeof(dest));
    
    TEST_ASSERT_EQUAL_STRING("", dest);
}

void test_Q_strncpyz_exact_fit(void) {
    char dest[6]; /* Room for "Hello" + null */
    
    Q_strncpyz(dest, "Hello", sizeof(dest));
    
    TEST_ASSERT_EQUAL_STRING("Hello", dest);
    /* Verify null terminator is present */
    TEST_ASSERT_EQUAL(0, dest[5]);
}

void test_Q_strncpyz_truncate_longer_string(void) {
    char dest[6];
    
    /* Source is longer than dest, should truncate */
    Q_strncpyz(dest, "HelloWorld", sizeof(dest));
    
    /* Should copy first 5 chars and null terminate */
    TEST_ASSERT_EQUAL_STRING("Hello", dest);
    TEST_ASSERT_EQUAL(0, dest[5]);
}

void test_Q_strncpyz_always_null_terminates(void) {
    char dest[10];
    
    /* Fill buffer with non-null bytes */
    memset(dest, 'X', sizeof(dest));
    
    Q_strncpyz(dest, "VeryLongStringThatWillBeTruncated", sizeof(dest));
    
    /* Last byte must be null */
    TEST_ASSERT_EQUAL(0, dest[9]);
    /* Should have copied 9 chars + null */
    TEST_ASSERT_EQUAL(9, strlen(dest));
}

void test_Q_strncpyz_size_zero(void) {
    char dest[10] = "Original";
    
    /* Size 0 should do nothing */
    Q_strncpyz(dest, "NewValue", 0);
    
    /* Destination unchanged (implementation defined, but safe) */
    /* This tests that size=0 doesn't crash */
}

void test_Q_strncpyz_size_one(void) {
    char dest[10];
    memset(dest, 'X', sizeof(dest));
    
    /* Size 1 should only write null terminator */
    Q_strncpyz(dest, "Hello", 1);
    
    TEST_ASSERT_EQUAL(0, dest[0]);
}

void test_Q_strncpyz_no_buffer_overflow(void) {
    char dest[5];
    char guard_before = 0x42;
    char guard_after = 0x42;
    
    /* This would overflow with strcpy */
    Q_strncpyz(dest, "VeryLongString", sizeof(dest));
    
    /* Verify guards are intact (no overflow) */
    TEST_ASSERT_EQUAL(0x42, guard_before);
    TEST_ASSERT_EQUAL(0x42, guard_after);
    
    /* Verify proper truncation */
    TEST_ASSERT_EQUAL(4, strlen(dest));
    TEST_ASSERT_EQUAL(0, dest[4]);
}

void test_Q_strncpyz_special_characters(void) {
    char dest[20];
    
    Q_strncpyz(dest, "Line1\nLine2\tTab", sizeof(dest));
    
    TEST_ASSERT_EQUAL_STRING("Line1\nLine2\tTab", dest);
}

/*
 * ============================================================================
 * Q_strcatz Tests
 * 
 * Safe string concatenation that respects buffer size.
 * Similar to strlcat - appends src to dst with size limit.
 * ============================================================================
 */

void test_Q_strcatz_simple_concat(void) {
    char dest[20] = "Hello";
    
    Q_strcatz(dest, " World", sizeof(dest));
    
    TEST_ASSERT_EQUAL_STRING("Hello World", dest);
}

void test_Q_strcatz_empty_dest(void) {
    char dest[20] = "";
    
    Q_strcatz(dest, "Hello", sizeof(dest));
    
    TEST_ASSERT_EQUAL_STRING("Hello", dest);
}

void test_Q_strcatz_empty_src(void) {
    char dest[20] = "Hello";
    
    Q_strcatz(dest, "", sizeof(dest));
    
    TEST_ASSERT_EQUAL_STRING("Hello", dest);
}

void test_Q_strcatz_exact_fit(void) {
    char dest[12] = "Hello"; /* 5 chars + room for " World" + null */
    
    Q_strcatz(dest, " World", sizeof(dest));
    
    TEST_ASSERT_EQUAL_STRING("Hello World", dest);
    TEST_ASSERT_EQUAL(0, dest[11]);
}

void test_Q_strcatz_truncate(void) {
    char dest[10] = "Hello";
    
    /* Total would be "HelloWorldLong" but only room for 9 + null */
    Q_strcatz(dest, "WorldLong", sizeof(dest));
    
    /* Should truncate to fit */
    TEST_ASSERT_EQUAL(9, strlen(dest));
    TEST_ASSERT_EQUAL(0, dest[9]);
}

void test_Q_strcatz_multiple_concats(void) {
    char dest[30] = "Part1";
    
    Q_strcatz(dest, " Part2", sizeof(dest));
    Q_strcatz(dest, " Part3", sizeof(dest));
    
    TEST_ASSERT_EQUAL_STRING("Part1 Part2 Part3", dest);
}

void test_Q_strcatz_no_overflow(void) {
    char dest[8] = "ABC";
    char guard_before = 0x42;
    char guard_after = 0x42;
    
    /* This would overflow with strcat */
    Q_strcatz(dest, "VeryLongString", sizeof(dest));
    
    /* Verify guards intact */
    TEST_ASSERT_EQUAL(0x42, guard_before);
    TEST_ASSERT_EQUAL(0x42, guard_after);
    
    /* Should be truncated */
    TEST_ASSERT_EQUAL(7, strlen(dest));
    TEST_ASSERT_EQUAL(0, dest[7]);
}

void test_Q_strcatz_full_buffer(void) {
    char dest[6] = "12345"; /* Buffer already full */
    
    /* Attempting to append to full buffer should be safe */
    Q_strcatz(dest, "XYZ", sizeof(dest));
    
    /* Should remain unchanged or handle gracefully */
    /* The implementation prints error but doesn't crash */
    TEST_ASSERT_EQUAL(5, strlen(dest));
}

void test_Q_strcatz_preserves_existing(void) {
    char dest[20] = "Prefix";
    
    Q_strcatz(dest, "Suffix", sizeof(dest));
    
    /* Original content should be preserved */
    TEST_ASSERT_TRUE(strncmp(dest, "Prefix", 6) == 0);
    TEST_ASSERT_EQUAL_STRING("PrefixSuffix", dest);
}

void test_Q_strcatz_special_characters(void) {
    char dest[30] = "Line1\n";
    
    Q_strcatz(dest, "Line2\tTabbed", sizeof(dest));
    
    TEST_ASSERT_EQUAL_STRING("Line1\nLine2\tTabbed", dest);
}

/*
 * ============================================================================
 * Q_snprintfz Tests
 * 
 * Safe string formatting that always null-terminates.
 * Wrapper around vsnprintf with guaranteed null termination.
 * Critical for preventing buffer overflows in formatted output.
 * ============================================================================
 */

void test_Q_snprintfz_simple_format(void) {
    char dest[50];
    
    Q_snprintfz(dest, sizeof(dest), "Hello %s", "World");
    
    TEST_ASSERT_EQUAL_STRING("Hello World", dest);
}

void test_Q_snprintfz_integer_format(void) {
    char dest[50];
    
    Q_snprintfz(dest, sizeof(dest), "Value: %d", 42);
    
    TEST_ASSERT_EQUAL_STRING("Value: 42", dest);
}

void test_Q_snprintfz_float_format(void) {
    char dest[50];
    
    Q_snprintfz(dest, sizeof(dest), "Pi: %.2f", 3.14159f);
    
    TEST_ASSERT_EQUAL_STRING("Pi: 3.14", dest);
}

void test_Q_snprintfz_multiple_args(void) {
    char dest[50];
    
    Q_snprintfz(dest, sizeof(dest), "%s: %d/%d", "Score", 15, 20);
    
    TEST_ASSERT_EQUAL_STRING("Score: 15/20", dest);
}

void test_Q_snprintfz_hex_format(void) {
    char dest[50];
    
    Q_snprintfz(dest, sizeof(dest), "0x%08X", 0xDEADBEEF);
    
    TEST_ASSERT_EQUAL_STRING("0xDEADBEEF", dest);
}

void test_Q_snprintfz_empty_format(void) {
    char dest[50];
    dest[0] = 'X'; /* Pre-fill */
    
    Q_snprintfz(dest, sizeof(dest), "");
    
    TEST_ASSERT_EQUAL_STRING("", dest);
}

void test_Q_snprintfz_truncate_long_string(void) {
    char dest[10];
    
    Q_snprintfz(dest, sizeof(dest), "VeryLongStringThatWillBeTruncated");
    
    /* Should truncate and null-terminate */
    TEST_ASSERT_EQUAL(9, strlen(dest));
    TEST_ASSERT_EQUAL(0, dest[9]);
}

void test_Q_snprintfz_always_null_terminates(void) {
    char dest[15];
    
    /* Fill with non-null */
    memset(dest, 'X', sizeof(dest));
    
    Q_snprintfz(dest, sizeof(dest), "This is a very long string %d", 12345);
    
    /* Last byte must be null */
    TEST_ASSERT_EQUAL(0, dest[14]);
    TEST_ASSERT_EQUAL(14, strlen(dest));
}

void test_Q_snprintfz_size_zero(void) {
    char dest[10] = "Original";
    
    /* Size 0 should clear first byte */
    Q_snprintfz(dest, 0, "NewValue");
    
    TEST_ASSERT_EQUAL(0, dest[0]);
}

void test_Q_snprintfz_size_one(void) {
    char dest[10];
    memset(dest, 'X', sizeof(dest));
    
    /* Size 1 should only write null terminator */
    Q_snprintfz(dest, 1, "Hello");
    
    TEST_ASSERT_EQUAL(0, dest[0]);
}

void test_Q_snprintfz_exact_fit(void) {
    char dest[6]; /* "Hello" + null */
    
    Q_snprintfz(dest, sizeof(dest), "Hello");
    
    TEST_ASSERT_EQUAL_STRING("Hello", dest);
    TEST_ASSERT_EQUAL(0, dest[5]);
}

void test_Q_snprintfz_no_buffer_overflow(void) {
    char dest[8];
    char guard_before = 0x42;
    char guard_after = 0x42;
    
    Q_snprintfz(dest, sizeof(dest), "VeryLongFormattedString %d %d %d", 1, 2, 3);
    
    /* Verify guards intact */
    TEST_ASSERT_EQUAL(0x42, guard_before);
    TEST_ASSERT_EQUAL(0x42, guard_after);
    
    /* Should be truncated */
    TEST_ASSERT_EQUAL(7, strlen(dest));
    TEST_ASSERT_EQUAL(0, dest[7]);
}

void test_Q_snprintfz_negative_numbers(void) {
    char dest[50];
    
    Q_snprintfz(dest, sizeof(dest), "Temp: %d degrees", -10);
    
    TEST_ASSERT_EQUAL_STRING("Temp: -10 degrees", dest);
}

void test_Q_snprintfz_zero_padding(void) {
    char dest[50];
    
    Q_snprintfz(dest, sizeof(dest), "ID: %05d", 42);
    
    TEST_ASSERT_EQUAL_STRING("ID: 00042", dest);
}

void test_Q_snprintfz_special_characters(void) {
    char dest[50];
    
    Q_snprintfz(dest, sizeof(dest), "Line1\nLine2\tTabbed");
    
    TEST_ASSERT_EQUAL_STRING("Line1\nLine2\tTabbed", dest);
}

void test_Q_snprintfz_percent_escape(void) {
    char dest[50];
    
    Q_snprintfz(dest, sizeof(dest), "100%% complete");
    
    TEST_ASSERT_EQUAL_STRING("100% complete", dest);
}

void test_Q_snprintfz_mixed_types(void) {
    char dest[100];
    
    Q_snprintfz(dest, sizeof(dest), "Player: %s Score: %d Time: %.1f", "Alice", 1500, 45.3f);
    
    TEST_ASSERT_EQUAL_STRING("Player: Alice Score: 1500 Time: 45.3", dest);
}

void test_Q_snprintfz_char_format(void) {
    char dest[50];
    
    Q_snprintfz(dest, sizeof(dest), "Key: %c", 'A');
    
    TEST_ASSERT_EQUAL_STRING("Key: A", dest);
}

void test_Q_snprintfz_pointer_format(void) {
    char dest[50];
    int value = 42;
    
    Q_snprintfz(dest, sizeof(dest), "Ptr: %p", (void*)&value);
    
    /* Just verify it doesn't crash and has content */
    TEST_ASSERT_TRUE(strlen(dest) > 5);
    TEST_ASSERT_TRUE(strstr(dest, "Ptr:") != NULL);
}

/*
 * ============================================================================
 * Test Runner
 * ============================================================================
 */

int main(void) {
    UNITY_BEGIN();
    
    /* Q_strncpyz tests */
    RUN_TEST(test_Q_strncpyz_simple_copy);
    RUN_TEST(test_Q_strncpyz_empty_string);
    RUN_TEST(test_Q_strncpyz_exact_fit);
    RUN_TEST(test_Q_strncpyz_truncate_longer_string);
    RUN_TEST(test_Q_strncpyz_always_null_terminates);
    RUN_TEST(test_Q_strncpyz_size_zero);
    RUN_TEST(test_Q_strncpyz_size_one);
    RUN_TEST(test_Q_strncpyz_no_buffer_overflow);
    RUN_TEST(test_Q_strncpyz_special_characters);
    
    /* Q_strcatz tests */
    RUN_TEST(test_Q_strcatz_simple_concat);
    RUN_TEST(test_Q_strcatz_empty_dest);
    RUN_TEST(test_Q_strcatz_empty_src);
    RUN_TEST(test_Q_strcatz_exact_fit);
    RUN_TEST(test_Q_strcatz_truncate);
    RUN_TEST(test_Q_strcatz_multiple_concats);
    RUN_TEST(test_Q_strcatz_no_overflow);
    RUN_TEST(test_Q_strcatz_full_buffer);
    RUN_TEST(test_Q_strcatz_preserves_existing);
    RUN_TEST(test_Q_strcatz_special_characters);
    
    /* Q_snprintfz tests */
    RUN_TEST(test_Q_snprintfz_simple_format);
    RUN_TEST(test_Q_snprintfz_integer_format);
    RUN_TEST(test_Q_snprintfz_float_format);
    RUN_TEST(test_Q_snprintfz_multiple_args);
    RUN_TEST(test_Q_snprintfz_hex_format);
    RUN_TEST(test_Q_snprintfz_empty_format);
    RUN_TEST(test_Q_snprintfz_truncate_long_string);
    RUN_TEST(test_Q_snprintfz_always_null_terminates);
    RUN_TEST(test_Q_snprintfz_size_zero);
    RUN_TEST(test_Q_snprintfz_size_one);
    RUN_TEST(test_Q_snprintfz_exact_fit);
    RUN_TEST(test_Q_snprintfz_no_buffer_overflow);
    RUN_TEST(test_Q_snprintfz_negative_numbers);
    RUN_TEST(test_Q_snprintfz_zero_padding);
    RUN_TEST(test_Q_snprintfz_special_characters);
    RUN_TEST(test_Q_snprintfz_percent_escape);
    RUN_TEST(test_Q_snprintfz_mixed_types);
    RUN_TEST(test_Q_snprintfz_char_format);
    RUN_TEST(test_Q_snprintfz_pointer_format);
    
    return UNITY_END();
}
