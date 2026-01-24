/*
 * test_resolution.c - Unit tests for resolution string parsing
 * Tests parsing strings like "1920x1080" into width and height integers
 */

#include "../unity/unity.h"
#include <string.h>
#include <stdio.h>

// Mock implementation of resolution parsing function
// Returns qTrue on success, qFalse on failure
typedef enum { qFalse, qTrue } qBool;

static qBool Parse_Resolution(const char *str, int *width, int *height) {
	int w, h;
	
	if (!str || !width || !height)
		return qFalse;
	
	// Try standard format: WIDTHxHEIGHT
	if (sscanf(str, "%dx%d", &w, &h) == 2) {
		if (w > 0 && h > 0) {
			*width = w;
			*height = h;
			return qTrue;
		}
	}
	
	// Try with space: WIDTH x HEIGHT
	if (sscanf(str, "%d x %d", &w, &h) == 2) {
		if (w > 0 && h > 0) {
			*width = w;
			*height = h;
			return qTrue;
		}
	}
	
	return qFalse;
}

// Mock implementation of aspect ratio calculation
// Returns aspect ratio as a float (width / height)
static float Calculate_AspectRatio(int width, int height) {
	if (height <= 0)
		return 0.0f;
	
	return (float)width / (float)height;
}

// Mock implementation to get aspect ratio string (e.g., "16:9", "4:3")
// Returns common aspect ratio name or NULL if not recognized
static const char* Get_AspectRatioString(int width, int height) {
	if (width <= 0 || height <= 0)
		return NULL;
	
	// Calculate GCD for simplified ratio
	int gcd_val = width;
	int temp = height;
	int remainder;
	
	while (temp != 0) {
		remainder = gcd_val % temp;
		gcd_val = temp;
		temp = remainder;
	}
	
	int ratio_w = width / gcd_val;
	int ratio_h = height / gcd_val;
	
	// Check common ratios
	if (ratio_w == 16 && ratio_h == 9) return "16:9";
	if (ratio_w == 4 && ratio_h == 3) return "4:3";
	if (ratio_w == 16 && ratio_h == 10) return "16:10";
	if (ratio_w == 8 && ratio_h == 5) return "16:10"; // 8:5 is same as 16:10
	if (ratio_w == 21 && ratio_h == 9) return "21:9";
	if (ratio_w == 64 && ratio_h == 27) return "21:9"; // 64:27 is same as 21:9 (2560x1080)
	if (ratio_w == 32 && ratio_h == 9) return "32:9";
	if (ratio_w == 5 && ratio_h == 4) return "5:4";
	if (ratio_w == 1 && ratio_h == 1) return "1:1";
	
	return "custom";
}

// ============================================================================
// TESTS: Standard Format (WIDTHxHEIGHT)
// ============================================================================

// Test: parse 1920x1080
void test_Resolution_1920x1080(void) {
	int width = 0, height = 0;
	qBool result = Parse_Resolution("1920x1080", &width, &height);
	
	TEST_ASSERT_TRUE(result);
	TEST_ASSERT_EQUAL(1920, width);
	TEST_ASSERT_EQUAL(1080, height);
}

// Test: parse 1280x720
void test_Resolution_1280x720(void) {
	int width = 0, height = 0;
	qBool result = Parse_Resolution("1280x720", &width, &height);
	
	TEST_ASSERT_TRUE(result);
	TEST_ASSERT_EQUAL(1280, width);
	TEST_ASSERT_EQUAL(720, height);
}

// Test: parse 640x480
void test_Resolution_640x480(void) {
	int width = 0, height = 0;
	qBool result = Parse_Resolution("640x480", &width, &height);
	
	TEST_ASSERT_TRUE(result);
	TEST_ASSERT_EQUAL(640, width);
	TEST_ASSERT_EQUAL(480, height);
}

// Test: parse 2560x1440
void test_Resolution_2560x1440(void) {
	int width = 0, height = 0;
	qBool result = Parse_Resolution("2560x1440", &width, &height);
	
	TEST_ASSERT_TRUE(result);
	TEST_ASSERT_EQUAL(2560, width);
	TEST_ASSERT_EQUAL(1440, height);
}

// Test: parse 3840x2160 (4K)
void test_Resolution_3840x2160(void) {
	int width = 0, height = 0;
	qBool result = Parse_Resolution("3840x2160", &width, &height);
	
	TEST_ASSERT_TRUE(result);
	TEST_ASSERT_EQUAL(3840, width);
	TEST_ASSERT_EQUAL(2160, height);
}

// Test: parse 800x600
void test_Resolution_800x600(void) {
	int width = 0, height = 0;
	qBool result = Parse_Resolution("800x600", &width, &height);
	
	TEST_ASSERT_TRUE(result);
	TEST_ASSERT_EQUAL(800, width);
	TEST_ASSERT_EQUAL(600, height);
}

// Test: parse 1024x768
void test_Resolution_1024x768(void) {
	int width = 0, height = 0;
	qBool result = Parse_Resolution("1024x768", &width, &height);
	
	TEST_ASSERT_TRUE(result);
	TEST_ASSERT_EQUAL(1024, width);
	TEST_ASSERT_EQUAL(768, height);
}

// Test: parse 1600x900
void test_Resolution_1600x900(void) {
	int width = 0, height = 0;
	qBool result = Parse_Resolution("1600x900", &width, &height);
	
	TEST_ASSERT_TRUE(result);
	TEST_ASSERT_EQUAL(1600, width);
	TEST_ASSERT_EQUAL(900, height);
}

// Test: parse 1366x768 (common laptop resolution)
void test_Resolution_1366x768(void) {
	int width = 0, height = 0;
	qBool result = Parse_Resolution("1366x768", &width, &height);
	
	TEST_ASSERT_TRUE(result);
	TEST_ASSERT_EQUAL(1366, width);
	TEST_ASSERT_EQUAL(768, height);
}

// Test: parse 1440x900
void test_Resolution_1440x900(void) {
	int width = 0, height = 0;
	qBool result = Parse_Resolution("1440x900", &width, &height);
	
	TEST_ASSERT_TRUE(result);
	TEST_ASSERT_EQUAL(1440, width);
	TEST_ASSERT_EQUAL(900, height);
}

// ============================================================================
// TESTS: Format with Spaces (WIDTH x HEIGHT)
// ============================================================================

// Test: parse with spaces "1920 x 1080"
void test_Resolution_WithSpaces(void) {
	int width = 0, height = 0;
	qBool result = Parse_Resolution("1920 x 1080", &width, &height);
	
	TEST_ASSERT_TRUE(result);
	TEST_ASSERT_EQUAL(1920, width);
	TEST_ASSERT_EQUAL(1080, height);
}

// Test: parse with spaces "1280 x 720"
void test_Resolution_WithSpaces_1280x720(void) {
	int width = 0, height = 0;
	qBool result = Parse_Resolution("1280 x 720", &width, &height);
	
	TEST_ASSERT_TRUE(result);
	TEST_ASSERT_EQUAL(1280, width);
	TEST_ASSERT_EQUAL(720, height);
}

// Test: parse with spaces "640 x 480"
void test_Resolution_WithSpaces_640x480(void) {
	int width = 0, height = 0;
	qBool result = Parse_Resolution("640 x 480", &width, &height);
	
	TEST_ASSERT_TRUE(result);
	TEST_ASSERT_EQUAL(640, width);
	TEST_ASSERT_EQUAL(480, height);
}

// ============================================================================
// TESTS: Ultrawide and Special Resolutions
// ============================================================================

// Test: parse 2560x1080 (ultrawide)
void test_Resolution_2560x1080_Ultrawide(void) {
	int width = 0, height = 0;
	qBool result = Parse_Resolution("2560x1080", &width, &height);
	
	TEST_ASSERT_TRUE(result);
	TEST_ASSERT_EQUAL(2560, width);
	TEST_ASSERT_EQUAL(1080, height);
}

// Test: parse 3440x1440 (ultrawide)
void test_Resolution_3440x1440_Ultrawide(void) {
	int width = 0, height = 0;
	qBool result = Parse_Resolution("3440x1440", &width, &height);
	
	TEST_ASSERT_TRUE(result);
	TEST_ASSERT_EQUAL(3440, width);
	TEST_ASSERT_EQUAL(1440, height);
}

// Test: parse 5120x1440 (super ultrawide)
void test_Resolution_5120x1440_SuperUltrawide(void) {
	int width = 0, height = 0;
	qBool result = Parse_Resolution("5120x1440", &width, &height);
	
	TEST_ASSERT_TRUE(result);
	TEST_ASSERT_EQUAL(5120, width);
	TEST_ASSERT_EQUAL(1440, height);
}

// Test: parse 320x240 (old low-res)
void test_Resolution_320x240_LowRes(void) {
	int width = 0, height = 0;
	qBool result = Parse_Resolution("320x240", &width, &height);
	
	TEST_ASSERT_TRUE(result);
	TEST_ASSERT_EQUAL(320, width);
	TEST_ASSERT_EQUAL(240, height);
}

// Test: parse 7680x4320 (8K)
void test_Resolution_7680x4320_8K(void) {
	int width = 0, height = 0;
	qBool result = Parse_Resolution("7680x4320", &width, &height);
	
	TEST_ASSERT_TRUE(result);
	TEST_ASSERT_EQUAL(7680, width);
	TEST_ASSERT_EQUAL(4320, height);
}

// ============================================================================
// TESTS: Invalid Input
// ============================================================================

// Test: NULL string
void test_Resolution_NullString(void) {
	int width = 0, height = 0;
	qBool result = Parse_Resolution(NULL, &width, &height);
	
	TEST_ASSERT_FALSE(result);
}

// Test: NULL width pointer
void test_Resolution_NullWidth(void) {
	int height = 0;
	qBool result = Parse_Resolution("1920x1080", NULL, &height);
	
	TEST_ASSERT_FALSE(result);
}

// Test: NULL height pointer
void test_Resolution_NullHeight(void) {
	int width = 0;
	qBool result = Parse_Resolution("1920x1080", &width, NULL);
	
	TEST_ASSERT_FALSE(result);
}

// Test: empty string
void test_Resolution_EmptyString(void) {
	int width = 0, height = 0;
	qBool result = Parse_Resolution("", &width, &height);
	
	TEST_ASSERT_FALSE(result);
}

// Test: invalid format - no 'x'
void test_Resolution_NoX(void) {
	int width = 0, height = 0;
	qBool result = Parse_Resolution("1920-1080", &width, &height);
	
	TEST_ASSERT_FALSE(result);
}

// Test: invalid format - only width
void test_Resolution_OnlyWidth(void) {
	int width = 0, height = 0;
	qBool result = Parse_Resolution("1920", &width, &height);
	
	TEST_ASSERT_FALSE(result);
}

// Test: invalid format - text
void test_Resolution_InvalidText(void) {
	int width = 0, height = 0;
	qBool result = Parse_Resolution("invalid", &width, &height);
	
	TEST_ASSERT_FALSE(result);
}

// Test: negative width
void test_Resolution_NegativeWidth(void) {
	int width = 0, height = 0;
	qBool result = Parse_Resolution("-1920x1080", &width, &height);
	
	TEST_ASSERT_FALSE(result);
}

// Test: negative height
void test_Resolution_NegativeHeight(void) {
	int width = 0, height = 0;
	qBool result = Parse_Resolution("1920x-1080", &width, &height);
	
	TEST_ASSERT_FALSE(result);
}

// Test: zero width
void test_Resolution_ZeroWidth(void) {
	int width = 0, height = 0;
	qBool result = Parse_Resolution("0x1080", &width, &height);
	
	TEST_ASSERT_FALSE(result);
}

// Test: zero height
void test_Resolution_ZeroHeight(void) {
	int width = 0, height = 0;
	qBool result = Parse_Resolution("1920x0", &width, &height);
	
	TEST_ASSERT_FALSE(result);
}

// Test: both zero
void test_Resolution_BothZero(void) {
	int width = 0, height = 0;
	qBool result = Parse_Resolution("0x0", &width, &height);
	
	TEST_ASSERT_FALSE(result);
}

// Test: extra characters after
void test_Resolution_ExtraCharsAfter(void) {
	int width = 0, height = 0;
	// sscanf will stop at invalid char, so this should still work
	qBool result = Parse_Resolution("1920x1080abc", &width, &height);
	
	TEST_ASSERT_TRUE(result);
	TEST_ASSERT_EQUAL(1920, width);
	TEST_ASSERT_EQUAL(1080, height);
}

// Test: extra characters before
void test_Resolution_ExtraCharsBefore(void) {
	int width = 0, height = 0;
	qBool result = Parse_Resolution("abc1920x1080", &width, &height);
	
	TEST_ASSERT_FALSE(result);
}

// ============================================================================
// TESTS: Edge Cases
// ============================================================================

// Test: very large resolution
void test_Resolution_VeryLarge(void) {
	int width = 0, height = 0;
	qBool result = Parse_Resolution("15360x8640", &width, &height);
	
	TEST_ASSERT_TRUE(result);
	TEST_ASSERT_EQUAL(15360, width);
	TEST_ASSERT_EQUAL(8640, height);
}

// Test: minimum valid resolution
void test_Resolution_Minimum(void) {
	int width = 0, height = 0;
	qBool result = Parse_Resolution("1x1", &width, &height);
	
	TEST_ASSERT_TRUE(result);
	TEST_ASSERT_EQUAL(1, width);
	TEST_ASSERT_EQUAL(1, height);
}

// Test: square resolution
void test_Resolution_Square(void) {
	int width = 0, height = 0;
	qBool result = Parse_Resolution("1024x1024", &width, &height);
	
	TEST_ASSERT_TRUE(result);
	TEST_ASSERT_EQUAL(1024, width);
	TEST_ASSERT_EQUAL(1024, height);
}

// Test: tall resolution (portrait)
void test_Resolution_Portrait(void) {
	int width = 0, height = 0;
	qBool result = Parse_Resolution("1080x1920", &width, &height);
	
	TEST_ASSERT_TRUE(result);
	TEST_ASSERT_EQUAL(1080, width);
	TEST_ASSERT_EQUAL(1920, height);
}

// Test: whitespace before
void test_Resolution_WhitespaceBefore(void) {
	int width = 0, height = 0;
	qBool result = Parse_Resolution("  1920x1080", &width, &height);
	
	TEST_ASSERT_TRUE(result);
	TEST_ASSERT_EQUAL(1920, width);
	TEST_ASSERT_EQUAL(1080, height);
}

// Test: capital X
void test_Resolution_CapitalX(void) {
	int width = 0, height = 0;
	qBool result = Parse_Resolution("1920X1080", &width, &height);
	
	TEST_ASSERT_FALSE(result); // lowercase 'x' only
}

// Test: multiple x separators
void test_Resolution_MultipleX(void) {
	int width = 0, height = 0;
	qBool result = Parse_Resolution("1920x1080x60", &width, &height);
	
	// Should parse first two numbers
	TEST_ASSERT_TRUE(result);
	TEST_ASSERT_EQUAL(1920, width);
	TEST_ASSERT_EQUAL(1080, height);
}

// ============================================================================
// TESTS: Aspect Ratio Calculation
// ============================================================================

// Test: 16:9 aspect ratio (1920x1080)
void test_AspectRatio_16_9_FullHD(void) {
	float ratio = Calculate_AspectRatio(1920, 1080);
	
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.7778f, ratio); // 16/9 = 1.7778
}

// Test: 16:9 aspect ratio (1280x720)
void test_AspectRatio_16_9_HD(void) {
	float ratio = Calculate_AspectRatio(1280, 720);
	
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.7778f, ratio);
}

// Test: 4:3 aspect ratio (640x480)
void test_AspectRatio_4_3_VGA(void) {
	float ratio = Calculate_AspectRatio(640, 480);
	
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.3333f, ratio); // 4/3 = 1.3333
}

// Test: 4:3 aspect ratio (1024x768)
void test_AspectRatio_4_3_XGA(void) {
	float ratio = Calculate_AspectRatio(1024, 768);
	
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.3333f, ratio);
}

// Test: 16:10 aspect ratio (1280x800)
void test_AspectRatio_16_10(void) {
	float ratio = Calculate_AspectRatio(1280, 800);
	
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.6f, ratio); // 16/10 = 1.6
}

// Test: 16:10 aspect ratio (1440x900)
void test_AspectRatio_16_10_WXGA(void) {
	float ratio = Calculate_AspectRatio(1440, 900);
	
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.6f, ratio);
}

// Test: 21:9 ultrawide (2560x1080)
void test_AspectRatio_21_9_Ultrawide(void) {
	float ratio = Calculate_AspectRatio(2560, 1080);
	
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 2.3704f, ratio); // 21/9 = 2.3704
}

// Test: 21:9 ultrawide (3440x1440)
void test_AspectRatio_21_9_Ultrawide_QHD(void) {
	float ratio = Calculate_AspectRatio(3440, 1440);
	
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 2.3889f, ratio); // Actually 43:18
}

// Test: 32:9 super ultrawide (5120x1440)
void test_AspectRatio_32_9_SuperUltrawide(void) {
	float ratio = Calculate_AspectRatio(5120, 1440);
	
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.5556f, ratio); // 32/9 = 3.5556
}

// Test: 5:4 aspect ratio (1280x1024)
void test_AspectRatio_5_4(void) {
	float ratio = Calculate_AspectRatio(1280, 1024);
	
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.25f, ratio); // 5/4 = 1.25
}

// Test: 1:1 square (1024x1024)
void test_AspectRatio_1_1_Square(void) {
	float ratio = Calculate_AspectRatio(1024, 1024);
	
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, ratio);
}

// Test: portrait orientation (1080x1920)
void test_AspectRatio_Portrait(void) {
	float ratio = Calculate_AspectRatio(1080, 1920);
	
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5625f, ratio); // 9/16 = 0.5625
}

// Test: zero height returns zero
void test_AspectRatio_ZeroHeight(void) {
	float ratio = Calculate_AspectRatio(1920, 0);
	
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, ratio);
}

// Test: negative height returns zero
void test_AspectRatio_NegativeHeight(void) {
	float ratio = Calculate_AspectRatio(1920, -1080);
	
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, ratio);
}

// ============================================================================
// TESTS: Aspect Ratio String Recognition
// ============================================================================

// Test: recognize 16:9 (1920x1080)
void test_AspectRatioString_16_9_FullHD(void) {
	const char *ratio = Get_AspectRatioString(1920, 1080);
	
	TEST_ASSERT_NOT_NULL(ratio);
	TEST_ASSERT_EQUAL_STRING("16:9", ratio);
}

// Test: recognize 16:9 (1280x720)
void test_AspectRatioString_16_9_HD(void) {
	const char *ratio = Get_AspectRatioString(1280, 720);
	
	TEST_ASSERT_NOT_NULL(ratio);
	TEST_ASSERT_EQUAL_STRING("16:9", ratio);
}

// Test: recognize 16:9 (2560x1440)
void test_AspectRatioString_16_9_QHD(void) {
	const char *ratio = Get_AspectRatioString(2560, 1440);
	
	TEST_ASSERT_NOT_NULL(ratio);
	TEST_ASSERT_EQUAL_STRING("16:9", ratio);
}

// Test: recognize 4:3 (640x480)
void test_AspectRatioString_4_3_VGA(void) {
	const char *ratio = Get_AspectRatioString(640, 480);
	
	TEST_ASSERT_NOT_NULL(ratio);
	TEST_ASSERT_EQUAL_STRING("4:3", ratio);
}

// Test: recognize 4:3 (1024x768)
void test_AspectRatioString_4_3_XGA(void) {
	const char *ratio = Get_AspectRatioString(1024, 768);
	
	TEST_ASSERT_NOT_NULL(ratio);
	TEST_ASSERT_EQUAL_STRING("4:3", ratio);
}

// Test: recognize 16:10 (1280x800)
void test_AspectRatioString_16_10(void) {
	const char *ratio = Get_AspectRatioString(1280, 800);
	
	TEST_ASSERT_NOT_NULL(ratio);
	TEST_ASSERT_EQUAL_STRING("16:10", ratio);
}

// Test: recognize 16:10 (1440x900)
void test_AspectRatioString_16_10_WXGA(void) {
	const char *ratio = Get_AspectRatioString(1440, 900);
	
	TEST_ASSERT_NOT_NULL(ratio);
	TEST_ASSERT_EQUAL_STRING("16:10", ratio);
}

// Test: recognize 21:9 (2560x1080)
void test_AspectRatioString_21_9_Ultrawide(void) {
	const char *ratio = Get_AspectRatioString(2560, 1080);
	
	TEST_ASSERT_NOT_NULL(ratio);
	TEST_ASSERT_EQUAL_STRING("21:9", ratio);
}

// Test: recognize 32:9 (5120x1440)
void test_AspectRatioString_32_9_SuperUltrawide(void) {
	const char *ratio = Get_AspectRatioString(5120, 1440);
	
	TEST_ASSERT_NOT_NULL(ratio);
	TEST_ASSERT_EQUAL_STRING("32:9", ratio);
}

// Test: recognize 5:4 (1280x1024)
void test_AspectRatioString_5_4(void) {
	const char *ratio = Get_AspectRatioString(1280, 1024);
	
	TEST_ASSERT_NOT_NULL(ratio);
	TEST_ASSERT_EQUAL_STRING("5:4", ratio);
}

// Test: recognize 1:1 (1024x1024)
void test_AspectRatioString_1_1_Square(void) {
	const char *ratio = Get_AspectRatioString(1024, 1024);
	
	TEST_ASSERT_NOT_NULL(ratio);
	TEST_ASSERT_EQUAL_STRING("1:1", ratio);
}

// Test: custom aspect ratio (1366x768)
void test_AspectRatioString_Custom_1366x768(void) {
	const char *ratio = Get_AspectRatioString(1366, 768);
	
	TEST_ASSERT_NOT_NULL(ratio);
	TEST_ASSERT_EQUAL_STRING("custom", ratio); // 683:384, not a standard ratio
}

// Test: custom aspect ratio (3440x1440)
void test_AspectRatioString_Custom_3440x1440(void) {
	const char *ratio = Get_AspectRatioString(3440, 1440);
	
	TEST_ASSERT_NOT_NULL(ratio);
	TEST_ASSERT_EQUAL_STRING("custom", ratio); // 43:18, close to 21:9 but not exact
}

// Test: zero width returns NULL
void test_AspectRatioString_ZeroWidth(void) {
	const char *ratio = Get_AspectRatioString(0, 1080);
	
	TEST_ASSERT_NULL(ratio);
}

// Test: zero height returns NULL
void test_AspectRatioString_ZeroHeight(void) {
	const char *ratio = Get_AspectRatioString(1920, 0);
	
	TEST_ASSERT_NULL(ratio);
}

// Test: negative values return NULL
void test_AspectRatioString_NegativeValues(void) {
	const char *ratio = Get_AspectRatioString(-1920, 1080);
	
	TEST_ASSERT_NULL(ratio);
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main(void) {
	UNITY_BEGIN();

	// Standard format tests
	RUN_TEST(test_Resolution_1920x1080);
	RUN_TEST(test_Resolution_1280x720);
	RUN_TEST(test_Resolution_640x480);
	RUN_TEST(test_Resolution_2560x1440);
	RUN_TEST(test_Resolution_3840x2160);
	RUN_TEST(test_Resolution_800x600);
	RUN_TEST(test_Resolution_1024x768);
	RUN_TEST(test_Resolution_1600x900);
	RUN_TEST(test_Resolution_1366x768);
	RUN_TEST(test_Resolution_1440x900);

	// Format with spaces
	RUN_TEST(test_Resolution_WithSpaces);
	RUN_TEST(test_Resolution_WithSpaces_1280x720);
	RUN_TEST(test_Resolution_WithSpaces_640x480);

	// Ultrawide and special resolutions
	RUN_TEST(test_Resolution_2560x1080_Ultrawide);
	RUN_TEST(test_Resolution_3440x1440_Ultrawide);
	RUN_TEST(test_Resolution_5120x1440_SuperUltrawide);
	RUN_TEST(test_Resolution_320x240_LowRes);
	RUN_TEST(test_Resolution_7680x4320_8K);

	// Invalid input tests
	RUN_TEST(test_Resolution_NullString);
	RUN_TEST(test_Resolution_NullWidth);
	RUN_TEST(test_Resolution_NullHeight);
	RUN_TEST(test_Resolution_EmptyString);
	RUN_TEST(test_Resolution_NoX);
	RUN_TEST(test_Resolution_OnlyWidth);
	RUN_TEST(test_Resolution_InvalidText);
	RUN_TEST(test_Resolution_NegativeWidth);
	RUN_TEST(test_Resolution_NegativeHeight);
	RUN_TEST(test_Resolution_ZeroWidth);
	RUN_TEST(test_Resolution_ZeroHeight);
	RUN_TEST(test_Resolution_BothZero);
	RUN_TEST(test_Resolution_ExtraCharsAfter);
	RUN_TEST(test_Resolution_ExtraCharsBefore);

	// Edge cases
	RUN_TEST(test_Resolution_VeryLarge);
	RUN_TEST(test_Resolution_Minimum);
	RUN_TEST(test_Resolution_Square);
	RUN_TEST(test_Resolution_Portrait);
	RUN_TEST(test_Resolution_WhitespaceBefore);
	RUN_TEST(test_Resolution_CapitalX);
	RUN_TEST(test_Resolution_MultipleX);

	// Aspect ratio calculation tests
	RUN_TEST(test_AspectRatio_16_9_FullHD);
	RUN_TEST(test_AspectRatio_16_9_HD);
	RUN_TEST(test_AspectRatio_4_3_VGA);
	RUN_TEST(test_AspectRatio_4_3_XGA);
	RUN_TEST(test_AspectRatio_16_10);
	RUN_TEST(test_AspectRatio_16_10_WXGA);
	RUN_TEST(test_AspectRatio_21_9_Ultrawide);
	RUN_TEST(test_AspectRatio_21_9_Ultrawide_QHD);
	RUN_TEST(test_AspectRatio_32_9_SuperUltrawide);
	RUN_TEST(test_AspectRatio_5_4);
	RUN_TEST(test_AspectRatio_1_1_Square);
	RUN_TEST(test_AspectRatio_Portrait);
	RUN_TEST(test_AspectRatio_ZeroHeight);
	RUN_TEST(test_AspectRatio_NegativeHeight);

	// Aspect ratio string recognition tests
	RUN_TEST(test_AspectRatioString_16_9_FullHD);
	RUN_TEST(test_AspectRatioString_16_9_HD);
	RUN_TEST(test_AspectRatioString_16_9_QHD);
	RUN_TEST(test_AspectRatioString_4_3_VGA);
	RUN_TEST(test_AspectRatioString_4_3_XGA);
	RUN_TEST(test_AspectRatioString_16_10);
	RUN_TEST(test_AspectRatioString_16_10_WXGA);
	RUN_TEST(test_AspectRatioString_21_9_Ultrawide);
	RUN_TEST(test_AspectRatioString_32_9_SuperUltrawide);
	RUN_TEST(test_AspectRatioString_5_4);
	RUN_TEST(test_AspectRatioString_1_1_Square);
	RUN_TEST(test_AspectRatioString_Custom_1366x768);
	RUN_TEST(test_AspectRatioString_Custom_3440x1440);
	RUN_TEST(test_AspectRatioString_ZeroWidth);
	RUN_TEST(test_AspectRatioString_ZeroHeight);
	RUN_TEST(test_AspectRatioString_NegativeValues);

	return UNITY_END();
}
