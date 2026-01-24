/*
====================
test_pmove_quantization.c

Unit tests for pmove position quantization to 1/8 unit precision
====================
*/

#include "../unity/unity.h"
#include <math.h>
#include <string.h>

// Forward declarations
typedef float vec_t;
typedef vec_t vec3_t[3];
typedef short int16;

#define QUANTIZE_SCALE 8.0f
#define QUANTIZE_UNIT (1.0f / QUANTIZE_SCALE)

// Vector operations
static void VectorCopy(const vec3_t in, vec3_t out) {
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

static void VectorSet(vec3_t v, float x, float y, float z) {
	v[0] = x;
	v[1] = y;
	v[2] = z;
}

static int VectorsEqual(const vec3_t v1, const vec3_t v2, float epsilon) {
	return (fabsf(v1[0] - v2[0]) < epsilon &&
	        fabsf(v1[1] - v2[1]) < epsilon &&
	        fabsf(v1[2] - v2[2]) < epsilon);
}

// Quantization functions (from pmove.c)
static void QuantizePosition(const vec3_t in, int16 out[3]) {
	out[0] = (int16)(in[0] * QUANTIZE_SCALE);
	out[1] = (int16)(in[1] * QUANTIZE_SCALE);
	out[2] = (int16)(in[2] * QUANTIZE_SCALE);
}

static void UnquantizePosition(const int16 in[3], vec3_t out) {
	out[0] = in[0] * QUANTIZE_UNIT;
	out[1] = in[1] * QUANTIZE_UNIT;
	out[2] = in[2] * QUANTIZE_UNIT;
}

static void QuantizeVelocity(const vec3_t in, int16 out[3]) {
	out[0] = (int16)(in[0] * QUANTIZE_SCALE);
	out[1] = (int16)(in[1] * QUANTIZE_SCALE);
	out[2] = (int16)(in[2] * QUANTIZE_SCALE);
}

static void UnquantizeVelocity(const int16 in[3], vec3_t out) {
	out[0] = in[0] * QUANTIZE_UNIT;
	out[1] = in[1] * QUANTIZE_UNIT;
	out[2] = in[2] * QUANTIZE_UNIT;
}

// Roundtrip quantization (position -> quantized -> position)
static void QuantizeRoundtrip(const vec3_t in, vec3_t out) {
	int16 quantized[3];
	QuantizePosition(in, quantized);
	UnquantizePosition(quantized, out);
}

// ============================================================================
// TESTS: Basic Quantization Roundtrip
// ============================================================================

void test_QuantizeRoundtrip_ExactMultiples(void) {
	vec3_t original, result;
	
	// Test exact multiples of 1/8 = 0.125
	VectorSet(original, 10.125f, 20.250f, 30.375f);
	QuantizeRoundtrip(original, result);
	
	// Should be exact (no precision loss)
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.125f, result[0]);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 20.250f, result[1]);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 30.375f, result[2]);
}

void test_QuantizeRoundtrip_Zero(void) {
	vec3_t original, result;
	
	VectorSet(original, 0.0f, 0.0f, 0.0f);
	QuantizeRoundtrip(original, result);
	
	// Zero should remain zero
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, result[0]);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, result[1]);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, result[2]);
}

void test_QuantizeRoundtrip_Integers(void) {
	vec3_t original, result;
	
	VectorSet(original, 100.0f, -50.0f, 25.0f);
	QuantizeRoundtrip(original, result);
	
	// Integers should be exact
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, result[0]);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, -50.0f, result[1]);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 25.0f, result[2]);
}

void test_QuantizeRoundtrip_NegativeValues(void) {
	vec3_t original, result;
	
	VectorSet(original, -10.125f, -20.250f, -30.375f);
	QuantizeRoundtrip(original, result);
	
	// Negative exact multiples should be exact
	TEST_ASSERT_FLOAT_WITHIN(0.001f, -10.125f, result[0]);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, -20.250f, result[1]);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, -30.375f, result[2]);
}

// ============================================================================
// TESTS: Quantization Precision Loss
// ============================================================================

void test_QuantizeRoundtrip_PrecisionLoss(void) {
	vec3_t original, result;
	
	// Value that doesn't align to 1/8 grid
	VectorSet(original, 10.1f, 20.2f, 30.3f);
	QuantizeRoundtrip(original, result);
	
	// Should be quantized using truncation (not rounding)
	// 10.1 * 8 = 80.8 -> truncates to 80 -> 80/8 = 10.0
	// 20.2 * 8 = 161.6 -> truncates to 161 -> 161/8 = 20.125
	// 30.3 * 8 = 242.4 -> truncates to 242 -> 242/8 = 30.25
	
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, result[0]);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 20.125f, result[1]);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 30.25f, result[2]);
}

void test_QuantizeRoundtrip_RoundingBehavior(void) {
	vec3_t original, result;
	
	// Test truncation behavior: values below 0.125 threshold truncate down
	VectorSet(original, 10.0625f, 0.0f, 0.0f);
	QuantizeRoundtrip(original, result);
	
	// 10.0625 * 8 = 80.5 -> truncates to 80 -> 80/8 = 10.0
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, result[0]);
	
	// Test truncation: 10.124 (just below 10.125) should truncate down
	VectorSet(original, 10.124f, 0.0f, 0.0f);
	QuantizeRoundtrip(original, result);
	
	// 10.124 * 8 = 80.992 -> truncates to 80 -> 80/8 = 10.0
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, result[0]);
	
	// Test truncation: 10.125 should be exact
	VectorSet(original, 10.125f, 0.0f, 0.0f);
	QuantizeRoundtrip(original, result);
	
	// 10.125 * 8 = 81.0 -> exact -> 81/8 = 10.125
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.125f, result[0]);
}

void test_QuantizeRoundtrip_MaxPrecisionLoss(void) {
	vec3_t original, result;
	
	// Maximum precision loss occurs at .0625 + epsilon
	// Test values just above and below quantization points
	float testValues[] = {0.0624f, 0.0626f, 0.1874f, 0.1876f};
	int i;
	
	for (i = 0; i < 4; i++) {
		VectorSet(original, testValues[i], 0.0f, 0.0f);
		QuantizeRoundtrip(original, result);
		
		// Result should be within 1/16 (0.0625) of original
		float diff = fabsf(result[0] - original[0]);
		TEST_ASSERT_TRUE(diff <= 0.0625f + 0.001f);
	}
}

// ============================================================================
// TESTS: Velocity Quantization
// ============================================================================

void test_QuantizeVelocity_Roundtrip(void) {
	vec3_t original, result;
	int16 quantized[3];
	
	VectorSet(original, 100.5f, -50.25f, 25.125f);
	QuantizeVelocity(original, quantized);
	UnquantizeVelocity(quantized, result);
	
	// Should match position quantization
	// 100.5 * 8 = 804 -> 804/8 = 100.5 (exact)
	// -50.25 * 8 = -402 -> -402/8 = -50.25 (exact)
	// 25.125 * 8 = 201 -> 201/8 = 25.125 (exact)
	
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.5f, result[0]);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, -50.25f, result[1]);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 25.125f, result[2]);
}

void test_QuantizeVelocity_PrecisionLoss(void) {
	vec3_t original, result;
	int16 quantized[3];
	
	VectorSet(original, 10.1f, 20.2f, 30.3f);
	QuantizeVelocity(original, quantized);
	UnquantizeVelocity(quantized, result);
	
	// Same truncation as position
	// 10.1 * 8 = 80.8 -> truncates to 80 -> 80/8 = 10.0
	// 20.2 * 8 = 161.6 -> truncates to 161 -> 161/8 = 20.125
	// 30.3 * 8 = 242.4 -> truncates to 242 -> 242/8 = 30.25
	
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, result[0]);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 20.125f, result[1]);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 30.25f, result[2]);
}

// ============================================================================
// TESTS: Quantization Edge Cases
// ============================================================================

void test_QuantizeRoundtrip_VerySmallValues(void) {
	vec3_t original, result;
	
	// Very small values
	VectorSet(original, 0.001f, -0.001f, 0.01f);
	QuantizeRoundtrip(original, result);
	
	// Should quantize to 0 (below 1/8 threshold)
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, result[0]);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, result[1]);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, result[2]);
}

void test_QuantizeRoundtrip_LargeValues(void) {
	vec3_t original, result;
	
	// Large values (within int16 range when multiplied by 8)
	VectorSet(original, 1000.0f, -2000.0f, 500.0f);
	QuantizeRoundtrip(original, result);
	
	// Should be exact (integers)
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 1000.0f, result[0]);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, -2000.0f, result[1]);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 500.0f, result[2]);
}

void test_QuantizeRoundtrip_AllAxes(void) {
	vec3_t original, result;
	
	// Test all axes with different values
	VectorSet(original, 1.125f, 2.250f, 3.375f);
	QuantizeRoundtrip(original, result);
	
	// All should be exact
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.125f, result[0]);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.250f, result[1]);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.375f, result[2]);
}

// ============================================================================
// TESTS: Quantization Error Bounds
// ============================================================================

void test_QuantizeRoundtrip_ErrorBounds(void) {
	vec3_t original, result;
	int i;
	
	// Test many values to ensure error is always within bounds
	for (i = 0; i < 100; i++) {
		// Generate test values from -100 to 100
		float x = ((float)i - 50.0f) * 2.0f + 0.123f; // Add offset for non-grid values
		float y = ((float)i - 50.0f) * 1.5f + 0.456f;
		float z = ((float)i - 50.0f) * 0.8f + 0.789f;
		
		VectorSet(original, x, y, z);
		QuantizeRoundtrip(original, result);
		
		// Error should never exceed 1/8 (0.125) in any axis
		float errorX = fabsf(result[0] - original[0]);
		float errorY = fabsf(result[1] - original[1]);
		float errorZ = fabsf(result[2] - original[2]);
		
		TEST_ASSERT_TRUE(errorX <= 0.125f + 0.001f);
		TEST_ASSERT_TRUE(errorY <= 0.125f + 0.001f);
		TEST_ASSERT_TRUE(errorZ <= 0.125f + 0.001f);
	}
}

void test_QuantizeRoundtrip_ConsistentRounding(void) {
	vec3_t original, result1, result2;
	
	// Same value quantized twice should give same result
	VectorSet(original, 10.123f, 20.456f, 30.789f);
	QuantizeRoundtrip(original, result1);
	QuantizeRoundtrip(original, result2);
	
	// Results should be identical
	TEST_ASSERT_TRUE(VectorsEqual(result1, result2, 0.001f));
}

// ============================================================================
// TESTS: Network Precision Simulation
// ============================================================================

void test_QuantizeRoundtrip_NetworkPrecision(void) {
	vec3_t original, quantized, result;
	int16 netOrigin[3];
	
	// Simulate network transmission: position -> quantize -> network -> unquantize
	VectorSet(original, 100.123f, 200.456f, 300.789f);
	
	// Client quantizes for network
	QuantizePosition(original, netOrigin);
	
	// Network transmits int16 values
	// Server/client receives and unquantizes
	UnquantizePosition(netOrigin, result);
	
	// Result should match expected quantization
	// 100.123 * 8 = 800.984 -> truncates to 800 -> 800/8 = 100.0
	// 200.456 * 8 = 1603.648 -> truncates to 1603 -> 1603/8 = 200.375
	// 300.789 * 8 = 2406.312 -> truncates to 2406 -> 2406/8 = 300.75
	
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, result[0]);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 200.375f, result[1]);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 300.750f, result[2]);
}

void test_QuantizeRoundtrip_PlayerMovement(void) {
	vec3_t startPos, endPos, quantized;
	int16 netOrigin[3];
	
	// Simulate player moving and position being quantized
	VectorSet(startPos, 0.0f, 0.0f, 0.0f);
	
	// Player moves to a non-grid position
	VectorSet(endPos, 10.123f, 20.456f, 0.0f);
	
	// Position gets quantized (like in pmove)
	QuantizePosition(endPos, netOrigin);
	UnquantizePosition(netOrigin, quantized);
	
	// Should be on 1/8 grid
	float xError = fmodf(quantized[0], QUANTIZE_UNIT);
	float yError = fmodf(quantized[1], QUANTIZE_UNIT);
	float zError = fmodf(quantized[2], QUANTIZE_UNIT);
	
	// All components should be exact multiples of 1/8
	TEST_ASSERT_TRUE(fabsf(xError) < 0.001f);
	TEST_ASSERT_TRUE(fabsf(yError) < 0.001f);
	TEST_ASSERT_TRUE(fabsf(zError) < 0.001f);
}

// ============================================================================
// TESTS: Jitter Behavior (Conceptual)
// ============================================================================

void test_QuantizeRoundtrip_JitterRange(void) {
	vec3_t original, result;
	int16 base[3], jittered[3];
	int i, j;
	
	// Test that jitter can adjust position by ±1/8 in each axis
	VectorSet(original, 10.0f, 20.0f, 30.0f);
	QuantizePosition(original, base);
	
	// Test all 8 jitter combinations (like in pmove)
	int jitterBits[8] = {0,4,1,2,3,5,6,7};
	int sign[3] = {1, 1, 1}; // Assume positive for simplicity
	
	for (i = 0; i < 8; i++) {
		int bits = jitterBits[i];
		
		for (j = 0; j < 3; j++) {
			jittered[j] = base[j];
			if (bits & (1 << j))
				jittered[j] += sign[j];
		}
		
		UnquantizePosition(jittered, result);
		
		// Jittered position should differ from base by multiples of 1/8
		float diffX = result[0] - original[0];
		float diffY = result[1] - original[1];
		float diffZ = result[2] - original[2];
		
		// Differences should be multiples of 1/8
		float xMod = fmodf(fabsf(diffX), QUANTIZE_UNIT);
		float yMod = fmodf(fabsf(diffY), QUANTIZE_UNIT);
		float zMod = fmodf(fabsf(diffZ), QUANTIZE_UNIT);
		
		TEST_ASSERT_TRUE(xMod < 0.001f);
		TEST_ASSERT_TRUE(yMod < 0.001f);
		TEST_ASSERT_TRUE(zMod < 0.001f);
	}
}

// ============================================================================
// TESTS: Roundtrip Consistency
// ============================================================================

void test_QuantizeRoundtrip_MultipleRounds(void) {
	vec3_t original, result1, result2, result3;
	
	// Quantizing multiple times should give consistent results
	VectorSet(original, 10.123f, 20.456f, 30.789f);
	
	QuantizeRoundtrip(original, result1);
	QuantizeRoundtrip(result1, result2);
	QuantizeRoundtrip(result2, result3);
	
	// All results should be identical after first quantization
	TEST_ASSERT_TRUE(VectorsEqual(result1, result2, 0.001f));
	TEST_ASSERT_TRUE(VectorsEqual(result2, result3, 0.001f));
}

void test_QuantizeRoundtrip_InverseOperations(void) {
	vec3_t original, temp, result;
	int16 quantized[3];
	
	// quantize -> unquantize -> quantize should be idempotent
	VectorSet(original, 10.123f, 20.456f, 30.789f);
	
	QuantizePosition(original, quantized);
	UnquantizePosition(quantized, temp);
	QuantizePosition(temp, quantized);
	UnquantizePosition(quantized, result);
	
	// Final result should equal temp
	TEST_ASSERT_TRUE(VectorsEqual(temp, result, 0.001f));
}

// ============================================================================
// TESTS: Boundary Values
// ============================================================================

void test_QuantizeRoundtrip_Int16Boundaries(void) {
	vec3_t result;
	int16 quantized[3];
	
	// Test near int16 boundaries (when multiplied by 8)
	// int16 range is -32768 to 32767
	
	// Maximum value before overflow: 32767/8 = 4095.875
	quantized[0] = 32767; quantized[1] = -32768; quantized[2] = 0;
	UnquantizePosition(quantized, result);
	
	// Should handle int16 boundaries correctly
	float expectedX = 32767 * QUANTIZE_UNIT;
	float expectedY = -32768 * QUANTIZE_UNIT;
	
	TEST_ASSERT_FLOAT_WITHIN(0.001f, expectedX, result[0]);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, expectedY, result[1]);
}

void test_QuantizeRoundtrip_FloatBoundaries(void) {
	vec3_t original, result;
	
	// Test large floats (within int16 range when multiplied by 8)
	// int16 range: -32768 to 32767
	// So max position: 32767/8 = 4095.875
	// Min position: -32768/8 = -4096
	
	VectorSet(original, 4000.0f, -4000.0f, 2000.0f);
	QuantizeRoundtrip(original, result);
	
	// Large integers within range should be exact
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 4000.0f, result[0]);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, -4000.0f, result[1]);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 2000.0f, result[2]);
	
	// Test values near int16 limits
	int16 maxInt16 = 32767;
	int16 minInt16 = -32768;
	
	// Should handle int16 boundaries correctly
	float expectedMax = 32767.0f * QUANTIZE_UNIT;
	float expectedMin = -32768.0f * QUANTIZE_UNIT;
	
	// These are the actual limits of the quantization system
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 4095.875f, expectedMax);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, -4096.0f, expectedMin);
}

// ============================================================================
// MAIN
// ============================================================================

int main(void) {
	UNITY_BEGIN();

	// Basic Quantization Roundtrip
	RUN_TEST(test_QuantizeRoundtrip_ExactMultiples);
	RUN_TEST(test_QuantizeRoundtrip_Zero);
	RUN_TEST(test_QuantizeRoundtrip_Integers);
	RUN_TEST(test_QuantizeRoundtrip_NegativeValues);

	// Quantization Precision Loss
	RUN_TEST(test_QuantizeRoundtrip_PrecisionLoss);
	RUN_TEST(test_QuantizeRoundtrip_RoundingBehavior);
	RUN_TEST(test_QuantizeRoundtrip_MaxPrecisionLoss);

	// Velocity Quantization
	RUN_TEST(test_QuantizeVelocity_Roundtrip);
	RUN_TEST(test_QuantizeVelocity_PrecisionLoss);

	// Quantization Edge Cases
	RUN_TEST(test_QuantizeRoundtrip_VerySmallValues);
	RUN_TEST(test_QuantizeRoundtrip_LargeValues);
	RUN_TEST(test_QuantizeRoundtrip_AllAxes);

	// Quantization Error Bounds
	RUN_TEST(test_QuantizeRoundtrip_ErrorBounds);
	RUN_TEST(test_QuantizeRoundtrip_ConsistentRounding);

	// Network Precision Simulation
	RUN_TEST(test_QuantizeRoundtrip_NetworkPrecision);
	RUN_TEST(test_QuantizeRoundtrip_PlayerMovement);

	// Jitter Behavior
	RUN_TEST(test_QuantizeRoundtrip_JitterRange);

	// Roundtrip Consistency
	RUN_TEST(test_QuantizeRoundtrip_MultipleRounds);
	RUN_TEST(test_QuantizeRoundtrip_InverseOperations);

	// Boundary Values
	RUN_TEST(test_QuantizeRoundtrip_Int16Boundaries);
	RUN_TEST(test_QuantizeRoundtrip_FloatBoundaries);

	return UNITY_END();
}
