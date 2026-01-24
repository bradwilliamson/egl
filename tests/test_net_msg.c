/*
 * test_net_msg.c - Unit tests for common/net_msg.c
 * 
 * Tests network message buffer read/write functions.
 * Critical for network protocol correctness.
 * Run with: make test_net_msg && ./test_net_msg
 */

#include "../unity/unity.h"
#include <float.h>
#include <math.h>

/* Include the actual implementation */
#include "../shared/shared.h"
#include "../common/protocol.h"

/* Stub for Com_Printf and Com_Error since we don't want to link the whole engine */
void Com_Printf(comPrint_t flags, char *fmt, ...) {
    /* Silently ignore for tests */
}

void Com_Error(comError_t code, char *fmt, ...) {
    /* For tests, just fail the test */
    TEST_FAIL_MESSAGE("Com_Error was called");
}

void ProjectPointOnPlane(vec3_t dst, vec3_t p, vec3_t normal) {
    /* Stub for mathlib dependency */
    Vec3Clear(dst);
}

/*
 * ============================================================================
 * MSG_Init / MSG_Clear Tests
 * 
 * Basic buffer initialization and clearing
 * ============================================================================
 */

void test_MSG_Init_creates_valid_buffer(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    
    TEST_ASSERT_EQUAL_PTR(buffer, msg.data);
    TEST_ASSERT_EQUAL(256, msg.maxSize);
    TEST_ASSERT_EQUAL(256, msg.bufferSize);
    TEST_ASSERT_EQUAL(0, msg.curSize);
    TEST_ASSERT_EQUAL(qFalse, msg.overFlowed);
}

void test_MSG_Clear_resets_size(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    msg.curSize = 100;
    msg.overFlowed = qTrue;
    
    MSG_Clear(&msg);
    
    TEST_ASSERT_EQUAL(0, msg.curSize);
    TEST_ASSERT_EQUAL(qFalse, msg.overFlowed);
}

/*
 * ============================================================================
 * MSG_WriteByte Tests
 * 
 * Core function for writing a single byte to network buffer
 * ============================================================================
 */

void test_MSG_WriteByte_writes_zero(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteByte(&msg, 0);
    
    TEST_ASSERT_EQUAL(1, msg.curSize);
    TEST_ASSERT_EQUAL(0, buffer[0]);
}

void test_MSG_WriteByte_writes_max_value(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteByte(&msg, 255);
    
    TEST_ASSERT_EQUAL(1, msg.curSize);
    TEST_ASSERT_EQUAL(255, buffer[0]);
}

void test_MSG_WriteByte_writes_multiple_bytes(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteByte(&msg, 0x12);
    MSG_WriteByte(&msg, 0x34);
    MSG_WriteByte(&msg, 0x56);
    
    TEST_ASSERT_EQUAL(3, msg.curSize);
    TEST_ASSERT_EQUAL(0x12, buffer[0]);
    TEST_ASSERT_EQUAL(0x34, buffer[1]);
    TEST_ASSERT_EQUAL(0x56, buffer[2]);
}

void test_MSG_WriteByte_increments_size(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    
    for (int i = 0; i < 10; i++) {
        MSG_WriteByte(&msg, i);
        TEST_ASSERT_EQUAL(i + 1, msg.curSize);
    }
}

void test_MSG_WriteByte_handles_out_of_range_negative(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteByte(&msg, -1);
    
    /* Should still write, but the value behavior is implementation-defined */
    TEST_ASSERT_EQUAL(1, msg.curSize);
}

void test_MSG_WriteByte_handles_out_of_range_positive(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteByte(&msg, 256);
    
    /* Should still write, but value will be truncated */
    TEST_ASSERT_EQUAL(1, msg.curSize);
    TEST_ASSERT_EQUAL(0, buffer[0]); /* 256 & 0xFF = 0 */
}

/*
 * ============================================================================
 * MSG_WriteChar Tests
 * 
 * Writing signed char values
 * ============================================================================
 */

void test_MSG_WriteChar_writes_negative_value(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteChar(&msg, -1);
    
    TEST_ASSERT_EQUAL(1, msg.curSize);
    TEST_ASSERT_EQUAL(0xFF, buffer[0]);
}

void test_MSG_WriteChar_writes_positive_value(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteChar(&msg, 127);
    
    TEST_ASSERT_EQUAL(1, msg.curSize);
    TEST_ASSERT_EQUAL(127, buffer[0]);
}

/*
 * ============================================================================
 * MSG_WriteShort Tests
 * 
 * Writing 16-bit integers (little-endian)
 * ============================================================================
 */

void test_MSG_WriteShort_writes_zero(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteShort(&msg, 0);
    
    TEST_ASSERT_EQUAL(2, msg.curSize);
    TEST_ASSERT_EQUAL(0, buffer[0]);
    TEST_ASSERT_EQUAL(0, buffer[1]);
}

void test_MSG_WriteShort_writes_little_endian(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteShort(&msg, 0x1234);
    
    TEST_ASSERT_EQUAL(2, msg.curSize);
    /* Little-endian: low byte first */
    TEST_ASSERT_EQUAL(0x34, buffer[0]);
    TEST_ASSERT_EQUAL(0x12, buffer[1]);
}

void test_MSG_WriteShort_handles_negative(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteShort(&msg, -1);
    
    TEST_ASSERT_EQUAL(2, msg.curSize);
    TEST_ASSERT_EQUAL(0xFF, buffer[0]);
    TEST_ASSERT_EQUAL(0xFF, buffer[1]);
}

void test_MSG_WriteShort_max_positive(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteShort(&msg, 32767); /* Max signed short */
    
    MSG_BeginReading(&msg);
    int result = MSG_ReadShort(&msg);
    TEST_ASSERT_EQUAL(32767, result);
}

void test_MSG_WriteShort_max_negative(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteShort(&msg, -32768); /* Min signed short */
    
    MSG_BeginReading(&msg);
    int result = MSG_ReadShort(&msg);
    TEST_ASSERT_EQUAL(-32768, result);
}

void test_MSG_ReadShort_multiple_values(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteShort(&msg, 100);
    MSG_WriteShort(&msg, -200);
    MSG_WriteShort(&msg, 300);
    
    MSG_BeginReading(&msg);
    TEST_ASSERT_EQUAL(100, MSG_ReadShort(&msg));
    TEST_ASSERT_EQUAL(-200, MSG_ReadShort(&msg));
    TEST_ASSERT_EQUAL(300, MSG_ReadShort(&msg));
}

/*
 * ============================================================================
 * MSG_WriteLong Tests
 * 
 * Writing 32-bit integers (little-endian)
 * ============================================================================
 */

void test_MSG_WriteLong_writes_zero(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteLong(&msg, 0);
    
    TEST_ASSERT_EQUAL(4, msg.curSize);
    TEST_ASSERT_EQUAL(0, buffer[0]);
    TEST_ASSERT_EQUAL(0, buffer[1]);
    TEST_ASSERT_EQUAL(0, buffer[2]);
    TEST_ASSERT_EQUAL(0, buffer[3]);
}

void test_MSG_WriteLong_writes_little_endian(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteLong(&msg, 0x12345678);
    
    TEST_ASSERT_EQUAL(4, msg.curSize);
    /* Little-endian: low byte first */
    TEST_ASSERT_EQUAL(0x78, buffer[0]);
    TEST_ASSERT_EQUAL(0x56, buffer[1]);
    TEST_ASSERT_EQUAL(0x34, buffer[2]);
    TEST_ASSERT_EQUAL(0x12, buffer[3]);
}

void test_MSG_WriteLong_handles_negative(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteLong(&msg, -1);
    
    MSG_BeginReading(&msg);
    int result = MSG_ReadLong(&msg);
    TEST_ASSERT_EQUAL(-1, result);
}

void test_MSG_WriteLong_max_positive(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteLong(&msg, 0x7FFFFFFF); /* Max signed int */
    
    MSG_BeginReading(&msg);
    int result = MSG_ReadLong(&msg);
    TEST_ASSERT_EQUAL(0x7FFFFFFF, result);
}

void test_MSG_ReadLong_multiple_values(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteLong(&msg, 1000000);
    MSG_WriteLong(&msg, -2000000);
    MSG_WriteLong(&msg, 0xDEADBEEF);
    
    MSG_BeginReading(&msg);
    TEST_ASSERT_EQUAL(1000000, MSG_ReadLong(&msg));
    TEST_ASSERT_EQUAL(-2000000, MSG_ReadLong(&msg));
    TEST_ASSERT_EQUAL((int)0xDEADBEEF, MSG_ReadLong(&msg));
}

/*
 * ============================================================================
 * MSG_WriteString Tests
 * 
 * Writing null-terminated strings
 * ============================================================================
 */

void test_MSG_WriteString_writes_empty_string(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteString(&msg, "");
    
    TEST_ASSERT_EQUAL(1, msg.curSize);
    TEST_ASSERT_EQUAL(0, buffer[0]); /* Just null terminator */
}

void test_MSG_WriteString_writes_simple_string(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteString(&msg, "Hello");
    
    TEST_ASSERT_EQUAL(6, msg.curSize); /* 5 chars + null */
    TEST_ASSERT_EQUAL_STRING("Hello", (char*)buffer);
}

void test_MSG_WriteString_writes_with_null_terminator(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteString(&msg, "Test");
    
    TEST_ASSERT_EQUAL('T', buffer[0]);
    TEST_ASSERT_EQUAL('e', buffer[1]);
    TEST_ASSERT_EQUAL('s', buffer[2]);
    TEST_ASSERT_EQUAL('t', buffer[3]);
    TEST_ASSERT_EQUAL(0, buffer[4]);
}

/*
 * ============================================================================
 * MSG_WriteFloat Tests
 * 
 * Writing float values (converted to little-endian)
 * ============================================================================
 */

void test_MSG_WriteFloat_writes_zero(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteFloat(&msg, 0.0f);
    
    TEST_ASSERT_EQUAL(4, msg.curSize);
}

void test_MSG_WriteFloat_writes_value(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteFloat(&msg, 123.456f);
    
    TEST_ASSERT_EQUAL(4, msg.curSize);
    
    /* Verify we can read it back */
    float *fptr = (float*)buffer;
    float result = LittleFloat(*fptr);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 123.456f, result);
}

/*
 * ============================================================================
 * MSG_Read* Tests
 * 
 * Reading values back from buffer
 * ============================================================================
 */

void test_MSG_ReadByte_reads_written_value(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteByte(&msg, 0xAB);
    
    MSG_BeginReading(&msg);
    int result = MSG_ReadByte(&msg);
    
    TEST_ASSERT_EQUAL(0xAB, result);
}

void test_MSG_ReadByte_reads_multiple_values(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteByte(&msg, 0x12);
    MSG_WriteByte(&msg, 0x34);
    MSG_WriteByte(&msg, 0x56);
    
    MSG_BeginReading(&msg);
    TEST_ASSERT_EQUAL(0x12, MSG_ReadByte(&msg));
    TEST_ASSERT_EQUAL(0x34, MSG_ReadByte(&msg));
    TEST_ASSERT_EQUAL(0x56, MSG_ReadByte(&msg));
}

void test_MSG_ReadShort_roundtrip(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteShort(&msg, 0x1234);
    
    MSG_BeginReading(&msg);
    int result = MSG_ReadShort(&msg);
    
    TEST_ASSERT_EQUAL(0x1234, result);
}

void test_MSG_ReadLong_roundtrip(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteLong(&msg, 0x12345678);
    
    MSG_BeginReading(&msg);
    int result = MSG_ReadLong(&msg);
    
    TEST_ASSERT_EQUAL(0x12345678, result);
}

void test_MSG_ReadString_roundtrip(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteString(&msg, "TestString");
    
    MSG_BeginReading(&msg);
    char *result = MSG_ReadString(&msg);
    
    TEST_ASSERT_EQUAL_STRING("TestString", result);
}

void test_MSG_ReadFloat_roundtrip(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteFloat(&msg, 456.789f);
    
    MSG_BeginReading(&msg);
    float result = MSG_ReadFloat(&msg);
    
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 456.789f, result);
}

void test_MSG_Float_roundtrip_negative(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteFloat(&msg, -123.456f);
    
    MSG_BeginReading(&msg);
    float result = MSG_ReadFloat(&msg);
    
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -123.456f, result);
}

void test_MSG_Float_roundtrip_negative_one(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteFloat(&msg, -1.0f);
    
    MSG_BeginReading(&msg);
    float result = MSG_ReadFloat(&msg);
    
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, -1.0f, result);
}

void test_MSG_Float_roundtrip_very_large(void) {
    netMsg_t msg;
    byte buffer[256];
    float large_value = 1e30f; /* Very large but not FLT_MAX */
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteFloat(&msg, large_value);
    
    MSG_BeginReading(&msg);
    float result = MSG_ReadFloat(&msg);
    
    TEST_ASSERT_FLOAT_WITHIN(large_value * 0.001f, large_value, result);
}

void test_MSG_Float_roundtrip_very_small_positive(void) {
    netMsg_t msg;
    byte buffer[256];
    float small_value = 1e-30f; /* Very small positive */
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteFloat(&msg, small_value);
    
    MSG_BeginReading(&msg);
    float result = MSG_ReadFloat(&msg);
    
    TEST_ASSERT_FLOAT_WITHIN(small_value * 0.1f, small_value, result);
}

void test_MSG_Float_roundtrip_very_small_negative(void) {
    netMsg_t msg;
    byte buffer[256];
    float small_value = -1e-30f; /* Very small negative */
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteFloat(&msg, small_value);
    
    MSG_BeginReading(&msg);
    float result = MSG_ReadFloat(&msg);
    
    TEST_ASSERT_FLOAT_WITHIN(fabsf(small_value) * 0.1f, small_value, result);
}

void test_MSG_Float_roundtrip_epsilon(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteFloat(&msg, FLT_EPSILON);
    
    MSG_BeginReading(&msg);
    float result = MSG_ReadFloat(&msg);
    
    TEST_ASSERT_FLOAT_WITHIN(FLT_EPSILON * 0.1f, FLT_EPSILON, result);
}

void test_MSG_Float_roundtrip_infinity_positive(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteFloat(&msg, INFINITY);
    
    MSG_BeginReading(&msg);
    float result = MSG_ReadFloat(&msg);
    
    TEST_ASSERT_TRUE(isinf(result) && result > 0);
}

void test_MSG_Float_roundtrip_infinity_negative(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteFloat(&msg, -INFINITY);
    
    MSG_BeginReading(&msg);
    float result = MSG_ReadFloat(&msg);
    
    TEST_ASSERT_TRUE(isinf(result) && result < 0);
}

void test_MSG_Float_roundtrip_nan(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteFloat(&msg, NAN);
    
    MSG_BeginReading(&msg);
    float result = MSG_ReadFloat(&msg);
    
    TEST_ASSERT_TRUE(isnan(result));
}

void test_MSG_Float_roundtrip_multiple_values(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteFloat(&msg, 1.0f);
    MSG_WriteFloat(&msg, -2.5f);
    MSG_WriteFloat(&msg, 0.0f);
    MSG_WriteFloat(&msg, 999.999f);
    
    MSG_BeginReading(&msg);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, MSG_ReadFloat(&msg));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -2.5f, MSG_ReadFloat(&msg));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, MSG_ReadFloat(&msg));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 999.999f, MSG_ReadFloat(&msg));
}

void test_MSG_Float_size_consistency(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteFloat(&msg, 123.456f);
    MSG_WriteFloat(&msg, -789.012f);
    
    /* Each float should be 4 bytes */
    TEST_ASSERT_EQUAL(8, msg.curSize);
}

/*
 * ============================================================================
 * MSG_WriteCoord / MSG_ReadCoord Tests
 * 
 * CRITICAL for R1Q2 server compatibility!
 * Coordinates are stored as: short = (int)(float * 8)
 * This provides 0.125 unit precision (1/8th unit)
 * Used extensively for entity positions in network protocol
 * ============================================================================
 */

void test_MSG_WriteCoord_zero(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteCoord(&msg, 0.0f);
    
    TEST_ASSERT_EQUAL(2, msg.curSize); /* Writes as short */
    
    MSG_BeginReading(&msg);
    float result = MSG_ReadCoord(&msg);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, result);
}

void test_MSG_WriteCoord_positive(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteCoord(&msg, 100.0f);
    
    MSG_BeginReading(&msg);
    float result = MSG_ReadCoord(&msg);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, result);
}

void test_MSG_WriteCoord_negative(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteCoord(&msg, -256.5f);
    
    MSG_BeginReading(&msg);
    float result = MSG_ReadCoord(&msg);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -256.5f, result);
}

void test_MSG_WriteCoord_precision_eighth_unit(void) {
    netMsg_t msg;
    byte buffer[256];
    
    /* Test that 0.125 (1/8) precision is maintained */
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteCoord(&msg, 10.125f); /* Exactly representable */
    
    MSG_BeginReading(&msg);
    float result = MSG_ReadCoord(&msg);
    
    /* Should be exact since 10.125 = 10 + 1/8 */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.125f, result);
}

void test_MSG_WriteCoord_quantization(void) {
    netMsg_t msg;
    byte buffer[256];
    
    /* Test quantization: 10.1 should round to nearest 1/8 */
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteCoord(&msg, 10.1f);
    
    MSG_BeginReading(&msg);
    float result = MSG_ReadCoord(&msg);
    
    /* 10.1 * 8 = 80.8 -> rounds to 81 -> 81 / 8 = 10.125 */
    /* Allow tolerance for quantization */
    TEST_ASSERT_FLOAT_WITHIN(0.13f, 10.1f, result);
}

void test_MSG_WriteCoord_max_range(void) {
    netMsg_t msg;
    byte buffer[256];
    
    /* Max coord value that fits in signed short: 32767 / 8 = 4095.875 */
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteCoord(&msg, 4095.875f);
    
    MSG_BeginReading(&msg);
    float result = MSG_ReadCoord(&msg);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 4095.875f, result);
}

void test_MSG_WriteCoord_min_range(void) {
    netMsg_t msg;
    byte buffer[256];
    
    /* Min coord value that fits in signed short: -32768 / 8 = -4096.0 */
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteCoord(&msg, -4096.0f);
    
    MSG_BeginReading(&msg);
    float result = MSG_ReadCoord(&msg);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -4096.0f, result);
}

void test_MSG_WriteCoord_multiple_coords(void) {
    netMsg_t msg;
    byte buffer[256];
    
    /* Test writing/reading multiple coordinates in sequence */
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteCoord(&msg, 128.625f);
    MSG_WriteCoord(&msg, -64.375f);
    MSG_WriteCoord(&msg, 0.125f);
    
    MSG_BeginReading(&msg);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 128.625f, MSG_ReadCoord(&msg));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -64.375f, MSG_ReadCoord(&msg));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.125f, MSG_ReadCoord(&msg));
}

void test_MSG_WriteCoord_r1q2_typical_values(void) {
    netMsg_t msg;
    byte buffer[256];
    
    /* Test typical Quake 2 map coordinate values */
    float test_coords[] = {512.0f, -384.0f, 1024.5f, 256.125f, -2048.0f};
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    for (int i = 0; i < 5; i++) {
        MSG_WriteCoord(&msg, test_coords[i]);
    }
    
    MSG_BeginReading(&msg);
    for (int i = 0; i < 5; i++) {
        float result = MSG_ReadCoord(&msg);
        TEST_ASSERT_FLOAT_WITHIN(0.001f, test_coords[i], result);
    }
}

/*
 * ============================================================================
 * MSG_WriteAngle / MSG_ReadAngle Tests
 * 
 * CRITICAL for R1Q2 entity rotation encoding!
 * Angles are stored as: byte = (int)(angle * 256 / 360) & 255
 * This provides ~1.406 degree precision (360/256)
 * Used for entity angles (pitch, yaw, roll) in network protocol
 * ============================================================================
 */

void test_MSG_WriteAngle_zero(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteAngle(&msg, 0.0f);
    
    TEST_ASSERT_EQUAL(1, msg.curSize); /* Writes as byte */
    
    MSG_BeginReading(&msg);
    float result = MSG_ReadAngle(&msg);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result);
}

void test_MSG_WriteAngle_90_degrees(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteAngle(&msg, 90.0f);
    
    MSG_BeginReading(&msg);
    float result = MSG_ReadAngle(&msg);
    /* 90 * 256 / 360 = 64, 64 * 360 / 256 = 90 (exact) */
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 90.0f, result);
}

void test_MSG_WriteAngle_180_degrees(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteAngle(&msg, 180.0f);
    
    MSG_BeginReading(&msg);
    float result = MSG_ReadAngle(&msg);
    /* 180 degrees encoded as signed byte returns -180 (equivalent angle) */
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -180.0f, result);
}

void test_MSG_WriteAngle_270_degrees(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteAngle(&msg, 270.0f);
    
    MSG_BeginReading(&msg);
    float result = MSG_ReadAngle(&msg);
    /* 270 degrees encoded as signed byte returns -90 (equivalent angle) */
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -90.0f, result);
}

void test_MSG_WriteAngle_negative(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteAngle(&msg, -45.0f);
    
    MSG_BeginReading(&msg);
    float result = MSG_ReadAngle(&msg);
    /* -45 encoded and decoded stays as -45 (signed representation) */
    TEST_ASSERT_FLOAT_WITHIN(2.0f, -45.0f, result);
}

void test_MSG_WriteAngle_wrap_around(void) {
    netMsg_t msg;
    byte buffer[256];
    
    /* Test that 360 degrees wraps to 0 */
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteAngle(&msg, 360.0f);
    
    MSG_BeginReading(&msg);
    float result = MSG_ReadAngle(&msg);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result);
}

void test_MSG_WriteAngle_precision(void) {
    netMsg_t msg;
    byte buffer[256];
    
    /* Test quantization: angle precision is 360/256 = 1.40625 degrees */
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteAngle(&msg, 45.5f);
    
    MSG_BeginReading(&msg);
    float result = MSG_ReadAngle(&msg);
    
    /* 45.5 should round to nearest 1.40625 step */
    /* Allow 1.5 degree tolerance for quantization */
    TEST_ASSERT_FLOAT_WITHIN(1.5f, 45.5f, result);
}

void test_MSG_WriteAngle_multiple_angles(void) {
    netMsg_t msg;
    byte buffer[256];
    
    /* Test pitch, yaw, roll sequence */
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteAngle(&msg, 0.0f);    /* pitch */
    MSG_WriteAngle(&msg, 90.0f);   /* yaw */
    MSG_WriteAngle(&msg, 0.0f);    /* roll */
    
    MSG_BeginReading(&msg);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, MSG_ReadAngle(&msg));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 90.0f, MSG_ReadAngle(&msg));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, MSG_ReadAngle(&msg));
}

/*
 * ============================================================================
 * MSG_WriteAngle16 / MSG_ReadAngle16 Tests
 * 
 * High-precision angle encoding for R1Q2
 * Angles stored as: short = (int)(angle * 65536 / 360) & 65535
 * This provides ~0.0055 degree precision (360/65536)
 * Used for more precise entity rotation when needed
 * ============================================================================
 */

void test_MSG_WriteAngle16_zero(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteAngle16(&msg, 0.0f);
    
    TEST_ASSERT_EQUAL(2, msg.curSize); /* Writes as short */
    
    MSG_BeginReading(&msg);
    float result = MSG_ReadAngle16(&msg);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, result);
}

void test_MSG_WriteAngle16_90_degrees(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteAngle16(&msg, 90.0f);
    
    MSG_BeginReading(&msg);
    float result = MSG_ReadAngle16(&msg);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 90.0f, result);
}

void test_MSG_WriteAngle16_180_degrees(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteAngle16(&msg, 180.0f);
    
    MSG_BeginReading(&msg);
    float result = MSG_ReadAngle16(&msg);
    /* 180 degrees encoded as signed short returns -180 (equivalent angle) */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -180.0f, result);
}

void test_MSG_WriteAngle16_precise_value(void) {
    netMsg_t msg;
    byte buffer[256];
    
    /* Test a precise fractional angle */
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteAngle16(&msg, 45.678f);
    
    MSG_BeginReading(&msg);
    float result = MSG_ReadAngle16(&msg);
    
    /* Should be very close with 16-bit precision */
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 45.678f, result);
}

void test_MSG_WriteAngle16_negative(void) {
    netMsg_t msg;
    byte buffer[256];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteAngle16(&msg, -45.0f);
    
    MSG_BeginReading(&msg);
    float result = MSG_ReadAngle16(&msg);
    
    /* -45 stays as -45 (signed representation) */
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -45.0f, result);
}

void test_MSG_WriteAngle16_full_rotation(void) {
    netMsg_t msg;
    byte buffer[256];
    
    /* Test 360 degree wrap */
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteAngle16(&msg, 360.0f);
    
    MSG_BeginReading(&msg);
    float result = MSG_ReadAngle16(&msg);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, result);
}

void test_MSG_WriteAngle16_high_precision(void) {
    netMsg_t msg;
    byte buffer[256];
    
    /* Test that 16-bit provides much better precision than 8-bit */
    /* Precision is 360/65536 = 0.00549 degrees */
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteAngle16(&msg, 123.456f);
    
    MSG_BeginReading(&msg);
    float result = MSG_ReadAngle16(&msg);
    
    /* Should be within 0.01 degrees */
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 123.456f, result);
}

void test_MSG_WriteAngle16_multiple_precise_angles(void) {
    netMsg_t msg;
    byte buffer[256];
    
    /* Test multiple precise angles in -180 to 180 range (signed representation) */
    float angles[] = {12.345f, 67.890f, -125.433f, -14.322f};
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    for (int i = 0; i < 4; i++) {
        MSG_WriteAngle16(&msg, angles[i]);
    }
    
    MSG_BeginReading(&msg);
    for (int i = 0; i < 4; i++) {
        float result = MSG_ReadAngle16(&msg);
        TEST_ASSERT_FLOAT_WITHIN(0.01f, angles[i], result);
    }
}

void test_MSG_WriteAngle16_vs_WriteAngle_precision(void) {
    netMsg_t msg1, msg2;
    byte buffer1[256], buffer2[256];
    
    /* Compare precision between 8-bit and 16-bit angle encoding */
    float test_angle = 45.678f;
    
    /* 8-bit angle */
    MSG_Init(&msg1, buffer1, sizeof(buffer1));
    MSG_WriteAngle(&msg1, test_angle);
    MSG_BeginReading(&msg1);
    float result8 = MSG_ReadAngle(&msg1);
    
    /* 16-bit angle */
    MSG_Init(&msg2, buffer2, sizeof(buffer2));
    MSG_WriteAngle16(&msg2, test_angle);
    MSG_BeginReading(&msg2);
    float result16 = MSG_ReadAngle16(&msg2);
    
    /* 16-bit should be more precise */
    float error8 = result8 - test_angle;
    if (error8 < 0.0f) error8 = -error8;
    float error16 = result16 - test_angle;
    if (error16 < 0.0f) error16 = -error16;
    
    /* Verify 16-bit has less error (allow for edge cases) */
    TEST_ASSERT_TRUE(error16 <= error8 + 0.1f);
    
    /* 16-bit should be within 0.01 degrees */
    TEST_ASSERT_FLOAT_WITHIN(0.01f, test_angle, result16);
}

/*
 * ============================================================================
 * MSG_WritePos / MSG_ReadPos Tests
 * 
 * CRITICAL for R1Q2 entity position synchronization!
 * Writes/reads 3D position vector as three MSG_WriteCoord calls
 * Essential for entity spawn, movement, and interpolation
 * ============================================================================
 */

void test_MSG_WritePos_origin(void) {
    netMsg_t msg;
    byte buffer[256];
    vec3_t pos = {0.0f, 0.0f, 0.0f};
    vec3_t result;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WritePos(&msg, pos);
    
    TEST_ASSERT_EQUAL(6, msg.curSize); /* 3 shorts = 6 bytes */
    
    MSG_BeginReading(&msg);
    MSG_ReadPos(&msg, result);
    
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, result[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, result[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, result[2]);
}

void test_MSG_WritePos_positive_coords(void) {
    netMsg_t msg;
    byte buffer[256];
    vec3_t pos = {100.5f, 200.25f, 300.125f};
    vec3_t result;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WritePos(&msg, pos);
    
    MSG_BeginReading(&msg);
    MSG_ReadPos(&msg, result);
    
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.5f, result[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 200.25f, result[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 300.125f, result[2]);
}

void test_MSG_WritePos_negative_coords(void) {
    netMsg_t msg;
    byte buffer[256];
    vec3_t pos = {-256.0f, -512.5f, -128.25f};
    vec3_t result;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WritePos(&msg, pos);
    
    MSG_BeginReading(&msg);
    MSG_ReadPos(&msg, result);
    
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -256.0f, result[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -512.5f, result[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -128.25f, result[2]);
}

void test_MSG_WritePos_mixed_signs(void) {
    netMsg_t msg;
    byte buffer[256];
    vec3_t pos = {512.0f, -384.0f, 1024.125f};
    vec3_t result;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WritePos(&msg, pos);
    
    MSG_BeginReading(&msg);
    MSG_ReadPos(&msg, result);
    
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 512.0f, result[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -384.0f, result[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1024.125f, result[2]);
}

void test_MSG_WritePos_precise_eighth_units(void) {
    netMsg_t msg;
    byte buffer[256];
    /* Test positions with exact 1/8 unit precision */
    vec3_t pos = {64.125f, 128.250f, 256.375f};
    vec3_t result;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WritePos(&msg, pos);
    
    MSG_BeginReading(&msg);
    MSG_ReadPos(&msg, result);
    
    /* Should be exact since all values are multiples of 0.125 */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 64.125f, result[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 128.250f, result[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 256.375f, result[2]);
}

void test_MSG_WritePos_max_range(void) {
    netMsg_t msg;
    byte buffer[256];
    /* Test near maximum coordinate range */
    vec3_t pos = {4095.0f, -4096.0f, 2048.5f};
    vec3_t result;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WritePos(&msg, pos);
    
    MSG_BeginReading(&msg);
    MSG_ReadPos(&msg, result);
    
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 4095.0f, result[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -4096.0f, result[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2048.5f, result[2]);
}

void test_MSG_WritePos_r1q2_spawn_position(void) {
    netMsg_t msg;
    byte buffer[256];
    /* Test typical player spawn position in Quake 2 */
    vec3_t spawn_pos = {512.0f, -256.0f, 24.125f};
    vec3_t result;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WritePos(&msg, spawn_pos);
    
    MSG_BeginReading(&msg);
    MSG_ReadPos(&msg, result);
    
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 512.0f, result[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -256.0f, result[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 24.125f, result[2]);
}

void test_MSG_WritePos_multiple_positions(void) {
    netMsg_t msg;
    byte buffer[256];
    vec3_t positions[] = {
        {100.0f, 200.0f, 300.0f},
        {-50.5f, 75.25f, -125.125f},
        {1024.0f, -2048.0f, 512.5f}
    };
    vec3_t results[3];
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    for (int i = 0; i < 3; i++) {
        MSG_WritePos(&msg, positions[i]);
    }
    
    TEST_ASSERT_EQUAL(18, msg.curSize); /* 3 positions * 6 bytes each */
    
    MSG_BeginReading(&msg);
    for (int i = 0; i < 3; i++) {
        MSG_ReadPos(&msg, results[i]);
        TEST_ASSERT_FLOAT_WITHIN(0.001f, positions[i][0], results[i][0]);
        TEST_ASSERT_FLOAT_WITHIN(0.001f, positions[i][1], results[i][1]);
        TEST_ASSERT_FLOAT_WITHIN(0.001f, positions[i][2], results[i][2]);
    }
}

void test_MSG_WritePos_quantization(void) {
    netMsg_t msg;
    byte buffer[256];
    /* Test position with values that require quantization */
    vec3_t pos = {123.456f, -789.012f, 345.678f};
    vec3_t result;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WritePos(&msg, pos);
    
    MSG_BeginReading(&msg);
    MSG_ReadPos(&msg, result);
    
    /* Allow 0.13 unit tolerance for 1/8 quantization */
    TEST_ASSERT_FLOAT_WITHIN(0.13f, 123.456f, result[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.13f, -789.012f, result[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.13f, 345.678f, result[2]);
}

void test_MSG_WritePos_interleaved_with_other_data(void) {
    netMsg_t msg;
    byte buffer[256];
    vec3_t pos1 = {100.0f, 200.0f, 300.0f};
    vec3_t pos2 = {-50.0f, -100.0f, -150.0f};
    vec3_t result1, result2;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    
    /* Write position, then some other data, then another position */
    MSG_WritePos(&msg, pos1);
    MSG_WriteByte(&msg, 0xFF);
    MSG_WriteShort(&msg, 1234);
    MSG_WritePos(&msg, pos2);
    
    MSG_BeginReading(&msg);
    MSG_ReadPos(&msg, result1);
    int byte_val = MSG_ReadByte(&msg);
    int short_val = MSG_ReadShort(&msg);
    MSG_ReadPos(&msg, result2);
    
    /* Verify positions are correct */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, result1[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 200.0f, result1[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 300.0f, result1[2]);
    
    TEST_ASSERT_EQUAL(0xFF, byte_val);
    TEST_ASSERT_EQUAL(1234, short_val);
    
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -50.0f, result2[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -100.0f, result2[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -150.0f, result2[2]);
}

/*
 * ============================================================================
 * MSG_WriteDeltaEntity Tests
 * 
 * CRITICAL for R1Q2 entity updates!
 * Tests delta compression of entity states using U_* flags
 * Essential for efficient entity updates in multiplayer
 * ============================================================================
 */

void test_MSG_WriteDeltaEntity_no_change(void) {
    netMsg_t msg;
    byte buffer[256];
    entityStateOld_t from, to;
    
    memset(&from, 0, sizeof(from));
    memset(&to, 0, sizeof(to));
    
    from.number = to.number = 10;
    from.origin[0] = to.origin[0] = 100.0f;
    from.origin[1] = to.origin[1] = 200.0f;
    from.origin[2] = to.origin[2] = 300.0f;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDeltaEntity(&msg, &from, &to, qFalse, qFalse);
    
    /* No changes, nothing should be written */
    TEST_ASSERT_EQUAL(0, msg.curSize);
}

void test_MSG_WriteDeltaEntity_force_write_no_change(void) {
    netMsg_t msg;
    byte buffer[256];
    entityStateOld_t from, to;
    
    memset(&from, 0, sizeof(from));
    memset(&to, 0, sizeof(to));
    
    from.number = to.number = 10;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDeltaEntity(&msg, &from, &to, qTrue, qFalse);
    
    /* Force flag should cause write even with no changes */
    TEST_ASSERT_GREATER_THAN(0, msg.curSize);
}

void test_MSG_WriteDeltaEntity_remove(void) {
    netMsg_t msg;
    byte buffer[256];
    entityStateOld_t from;
    
    memset(&from, 0, sizeof(from));
    from.number = 10;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDeltaEntity(&msg, &from, NULL, qFalse, qFalse);
    
    /* Should write U_REMOVE flag and entity number */
    TEST_ASSERT_GREATER_THAN(0, msg.curSize);
    
    /* Check for U_REMOVE bit (bit 6) in first byte */
    TEST_ASSERT_TRUE(buffer[0] & (1 << 6));
}

void test_MSG_WriteDeltaEntity_origin_change(void) {
    netMsg_t msg;
    byte buffer[256];
    entityStateOld_t from, to;
    
    memset(&from, 0, sizeof(from));
    memset(&to, 0, sizeof(to));
    
    from.number = to.number = 5;
    from.origin[0] = 100.0f;
    to.origin[0] = 200.0f;  /* Changed X */
    from.origin[1] = to.origin[1] = 150.0f;
    from.origin[2] = to.origin[2] = 75.0f;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    size_t start = msg.curSize;
    MSG_WriteDeltaEntity(&msg, &from, &to, qFalse, qFalse);
    
    /* Should write bits + number + origin X coordinate */
    TEST_ASSERT_GREATER_THAN(0, msg.curSize - start);
}

void test_MSG_WriteDeltaEntity_all_origins_change(void) {
    netMsg_t msg;
    byte buffer[256];
    entityStateOld_t from, to;
    
    memset(&from, 0, sizeof(from));
    memset(&to, 0, sizeof(to));
    
    from.number = to.number = 7;
    from.origin[0] = 100.0f;
    from.origin[1] = 200.0f;
    from.origin[2] = 300.0f;
    
    to.origin[0] = 150.0f;
    to.origin[1] = 250.0f;
    to.origin[2] = 350.0f;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDeltaEntity(&msg, &from, &to, qFalse, qFalse);
    
    /* Should have U_ORIGIN1, U_ORIGIN2, U_ORIGIN3 flags set */
    /* Each origin is 2 bytes (short), plus header bytes */
    TEST_ASSERT_GREATER_THAN(6, msg.curSize);
}

void test_MSG_WriteDeltaEntity_angles_change(void) {
    netMsg_t msg;
    byte buffer[256];
    entityStateOld_t from, to;
    
    memset(&from, 0, sizeof(from));
    memset(&to, 0, sizeof(to));
    
    from.number = to.number = 8;
    from.angles[0] = 0.0f;
    to.angles[0] = 90.0f;  /* Pitch changed */
    from.angles[1] = to.angles[1] = 45.0f;
    from.angles[2] = to.angles[2] = 0.0f;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDeltaEntity(&msg, &from, &to, qFalse, qFalse);
    
    /* Should write with U_ANGLE1 flag */
    TEST_ASSERT_GREATER_THAN(0, msg.curSize);
}

void test_MSG_WriteDeltaEntity_all_angles_change(void) {
    netMsg_t msg;
    byte buffer[256];
    entityStateOld_t from, to;
    
    memset(&from, 0, sizeof(from));
    memset(&to, 0, sizeof(to));
    
    from.number = to.number = 9;
    from.angles[0] = 0.0f;
    from.angles[1] = 0.0f;
    from.angles[2] = 0.0f;
    
    to.angles[0] = 45.0f;  /* Pitch */
    to.angles[1] = 90.0f;  /* Yaw */
    to.angles[2] = 180.0f; /* Roll */
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDeltaEntity(&msg, &from, &to, qFalse, qFalse);
    
    /* Should have U_ANGLE1, U_ANGLE2, U_ANGLE3 flags */
    TEST_ASSERT_GREATER_THAN(3, msg.curSize);
}

void test_MSG_WriteDeltaEntity_model_change(void) {
    netMsg_t msg;
    byte buffer[256];
    entityStateOld_t from, to;
    
    memset(&from, 0, sizeof(from));
    memset(&to, 0, sizeof(to));
    
    from.number = to.number = 12;
    from.modelIndex = 1;
    to.modelIndex = 5;  /* Model changed */
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDeltaEntity(&msg, &from, &to, qFalse, qFalse);
    
    /* Should have U_MODEL flag */
    TEST_ASSERT_GREATER_THAN(0, msg.curSize);
}

void test_MSG_WriteDeltaEntity_all_models_change(void) {
    netMsg_t msg;
    byte buffer[256];
    entityStateOld_t from, to;
    
    memset(&from, 0, sizeof(from));
    memset(&to, 0, sizeof(to));
    
    from.number = to.number = 15;
    from.modelIndex = 1;
    from.modelIndex2 = 2;
    from.modelIndex3 = 3;
    from.modelIndex4 = 4;
    
    to.modelIndex = 10;
    to.modelIndex2 = 20;
    to.modelIndex3 = 30;
    to.modelIndex4 = 40;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDeltaEntity(&msg, &from, &to, qFalse, qFalse);
    
    /* Should have U_MODEL, U_MODEL2, U_MODEL3, U_MODEL4 flags */
    TEST_ASSERT_GREATER_THAN(4, msg.curSize);
}

void test_MSG_WriteDeltaEntity_frame_8bit(void) {
    netMsg_t msg;
    byte buffer[256];
    entityStateOld_t from, to;
    
    memset(&from, 0, sizeof(from));
    memset(&to, 0, sizeof(to));
    
    from.number = to.number = 20;
    from.frame = 0;
    to.frame = 100;  /* Frame < 256, uses U_FRAME8 */
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDeltaEntity(&msg, &from, &to, qFalse, qFalse);
    
    TEST_ASSERT_GREATER_THAN(0, msg.curSize);
}

void test_MSG_WriteDeltaEntity_frame_16bit(void) {
    netMsg_t msg;
    byte buffer[256];
    entityStateOld_t from, to;
    
    memset(&from, 0, sizeof(from));
    memset(&to, 0, sizeof(to));
    
    from.number = to.number = 21;
    from.frame = 0;
    to.frame = 500;  /* Frame >= 256, uses U_FRAME16 */
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDeltaEntity(&msg, &from, &to, qFalse, qFalse);
    
    /* Should use U_FRAME16 (2 bytes for frame) */
    TEST_ASSERT_GREATER_THAN(0, msg.curSize);
}

void test_MSG_WriteDeltaEntity_skin_8bit(void) {
    netMsg_t msg;
    byte buffer[256];
    entityStateOld_t from, to;
    
    memset(&from, 0, sizeof(from));
    memset(&to, 0, sizeof(to));
    
    from.number = to.number = 25;
    from.skinNum = 0;
    to.skinNum = 50;  /* Skin < 256 */
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDeltaEntity(&msg, &from, &to, qFalse, qFalse);
    
    TEST_ASSERT_GREATER_THAN(0, msg.curSize);
}

void test_MSG_WriteDeltaEntity_effects_change(void) {
    netMsg_t msg;
    byte buffer[256];
    entityStateOld_t from, to;
    
    memset(&from, 0, sizeof(from));
    memset(&to, 0, sizeof(to));
    
    from.number = to.number = 30;
    from.effects = 0;
    to.effects = 128;  /* Some effect active */
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDeltaEntity(&msg, &from, &to, qFalse, qFalse);
    
    TEST_ASSERT_GREATER_THAN(0, msg.curSize);
}

void test_MSG_WriteDeltaEntity_renderfx_change(void) {
    netMsg_t msg;
    byte buffer[256];
    entityStateOld_t from, to;
    
    memset(&from, 0, sizeof(from));
    memset(&to, 0, sizeof(to));
    
    from.number = to.number = 35;
    from.renderFx = 0;
    to.renderFx = 64;  /* Some render effect */
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDeltaEntity(&msg, &from, &to, qFalse, qFalse);
    
    TEST_ASSERT_GREATER_THAN(0, msg.curSize);
}

void test_MSG_WriteDeltaEntity_event(void) {
    netMsg_t msg;
    byte buffer[256];
    entityStateOld_t from, to;
    
    memset(&from, 0, sizeof(from));
    memset(&to, 0, sizeof(to));
    
    from.number = to.number = 40;
    from.event = 0;
    to.event = 5;  /* Event is not delta compressed, just 0 compressed */
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDeltaEntity(&msg, &from, &to, qFalse, qFalse);
    
    /* Should have U_EVENT flag */
    TEST_ASSERT_GREATER_THAN(0, msg.curSize);
}

void test_MSG_WriteDeltaEntity_solid_change(void) {
    netMsg_t msg;
    byte buffer[256];
    entityStateOld_t from, to;
    
    memset(&from, 0, sizeof(from));
    memset(&to, 0, sizeof(to));
    
    from.number = to.number = 45;
    from.solid = 0;
    to.solid = 255;  /* Solid bounding box info */
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDeltaEntity(&msg, &from, &to, qFalse, qFalse);
    
    /* Should have U_SOLID flag */
    TEST_ASSERT_GREATER_THAN(0, msg.curSize);
}

void test_MSG_WriteDeltaEntity_sound_change(void) {
    netMsg_t msg;
    byte buffer[256];
    entityStateOld_t from, to;
    
    memset(&from, 0, sizeof(from));
    memset(&to, 0, sizeof(to));
    
    from.number = to.number = 50;
    from.sound = 0;
    to.sound = 10;  /* Looping sound */
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDeltaEntity(&msg, &from, &to, qFalse, qFalse);
    
    /* Should have U_SOUND flag */
    TEST_ASSERT_GREATER_THAN(0, msg.curSize);
}

void test_MSG_WriteDeltaEntity_number16(void) {
    netMsg_t msg;
    byte buffer[256];
    entityStateOld_t from, to;
    
    memset(&from, 0, sizeof(from));
    memset(&to, 0, sizeof(to));
    
    from.number = to.number = 300;  /* Number >= 256, requires U_NUMBER16 */
    from.origin[0] = 100.0f;
    to.origin[0] = 200.0f;  /* Make a change to trigger write */
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDeltaEntity(&msg, &from, &to, qFalse, qFalse);
    
    /* Should have U_NUMBER16 flag and write 2-byte entity number */
    TEST_ASSERT_GREATER_THAN(0, msg.curSize);
}

void test_MSG_WriteDeltaEntity_complex_update(void) {
    netMsg_t msg;
    byte buffer[256];
    entityStateOld_t from, to;
    
    memset(&from, 0, sizeof(from));
    memset(&to, 0, sizeof(to));
    
    /* Typical player entity update with multiple changes */
    from.number = to.number = 1;
    
    from.origin[0] = 512.0f;
    from.origin[1] = -256.0f;
    from.origin[2] = 24.0f;
    
    to.origin[0] = 520.0f;  /* Moved slightly */
    to.origin[1] = -250.0f;
    to.origin[2] = 24.0f;
    
    from.angles[1] = 90.0f;
    to.angles[1] = 95.0f;  /* Turned slightly */
    
    from.frame = 10;
    to.frame = 11;  /* Animation advanced */
    
    from.modelIndex = 5;
    to.modelIndex = 5;  /* Same model */
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDeltaEntity(&msg, &from, &to, qFalse, qFalse);
    
    /* Should write multiple fields efficiently */
    TEST_ASSERT_GREATER_THAN(0, msg.curSize);
}

/*
 * ============================================================================
 * MSG_WriteDeltaUsercmd / MSG_ReadDeltaUsercmd Tests
 * 
 * CRITICAL for player input!
 * Tests delta compression of player commands (movement, angles, buttons)
 * Essential for client-server input synchronization
 * ============================================================================
 */

void test_MSG_DeltaUsercmd_no_change(void) {
    netMsg_t msg;
    byte buffer[256];
    userCmd_t from, to, result;
    
    memset(&from, 0, sizeof(from));
    memset(&to, 0, sizeof(to));
    
    from.msec = to.msec = 16;
    from.lightLevel = to.lightLevel = 128;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDeltaUsercmd(&msg, &from, &to, 0);
    
    /* Delta with no changes should write minimal data (bits byte, msec, lightLevel) */
    TEST_ASSERT_EQUAL(3, msg.curSize);
    
    /* Verify roundtrip */
    MSG_BeginReading(&msg);
    MSG_ReadDeltaUsercmd(&msg, &from, &result);
    
    TEST_ASSERT_EQUAL(to.msec, result.msec);
    TEST_ASSERT_EQUAL(to.lightLevel, result.lightLevel);
}

void test_MSG_DeltaUsercmd_angles_change(void) {
    netMsg_t msg;
    byte buffer[256];
    userCmd_t from, to, result;
    
    memset(&from, 0, sizeof(from));
    memset(&to, 0, sizeof(to));
    
    from.angles[0] = 0;
    from.angles[1] = 0;
    from.angles[2] = 0;
    
    to.angles[0] = 100;   /* Pitch */
    to.angles[1] = 2000;  /* Yaw */
    to.angles[2] = -50;   /* Roll */
    
    to.msec = 16;
    to.lightLevel = 128;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDeltaUsercmd(&msg, &from, &to, 0);
    
    /* Should write bits + 3 angles (2 bytes each) + msec + lightLevel */
    TEST_ASSERT_GREATER_THAN(5, msg.curSize);
    
    /* Verify roundtrip */
    MSG_BeginReading(&msg);
    MSG_ReadDeltaUsercmd(&msg, &from, &result);
    
    TEST_ASSERT_EQUAL(to.angles[0], result.angles[0]);
    TEST_ASSERT_EQUAL(to.angles[1], result.angles[1]);
    TEST_ASSERT_EQUAL(to.angles[2], result.angles[2]);
}

void test_MSG_DeltaUsercmd_single_angle_change(void) {
    netMsg_t msg;
    byte buffer[256];
    userCmd_t from, to, result;
    
    memset(&from, 0, sizeof(from));
    memset(&to, 0, sizeof(to));
    
    from.angles[0] = 500;
    from.angles[1] = 1000;
    from.angles[2] = 0;
    
    to.angles[0] = 500;
    to.angles[1] = 1200;  /* Only yaw changed */
    to.angles[2] = 0;
    
    to.msec = 16;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDeltaUsercmd(&msg, &from, &to, 0);
    
    /* Verify roundtrip */
    MSG_BeginReading(&msg);
    MSG_ReadDeltaUsercmd(&msg, &from, &result);
    
    TEST_ASSERT_EQUAL(to.angles[0], result.angles[0]);
    TEST_ASSERT_EQUAL(to.angles[1], result.angles[1]);
    TEST_ASSERT_EQUAL(to.angles[2], result.angles[2]);
}

void test_MSG_DeltaUsercmd_forward_movement(void) {
    netMsg_t msg;
    byte buffer[256];
    userCmd_t from, to, result;
    
    memset(&from, 0, sizeof(from));
    memset(&to, 0, sizeof(to));
    
    from.forwardMove = 0;
    to.forwardMove = 400;  /* Running forward */
    to.msec = 16;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDeltaUsercmd(&msg, &from, &to, 0);
    
    /* Verify roundtrip */
    MSG_BeginReading(&msg);
    MSG_ReadDeltaUsercmd(&msg, &from, &result);
    
    TEST_ASSERT_EQUAL(400, result.forwardMove);
    TEST_ASSERT_EQUAL(0, result.sideMove);
    TEST_ASSERT_EQUAL(0, result.upMove);
}

void test_MSG_DeltaUsercmd_side_movement(void) {
    netMsg_t msg;
    byte buffer[256];
    userCmd_t from, to, result;
    
    memset(&from, 0, sizeof(from));
    memset(&to, 0, sizeof(to));
    
    from.sideMove = 0;
    to.sideMove = -300;  /* Strafing left */
    to.msec = 16;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDeltaUsercmd(&msg, &from, &to, 0);
    
    /* Verify roundtrip */
    MSG_BeginReading(&msg);
    MSG_ReadDeltaUsercmd(&msg, &from, &result);
    
    TEST_ASSERT_EQUAL(0, result.forwardMove);
    TEST_ASSERT_EQUAL(-300, result.sideMove);
    TEST_ASSERT_EQUAL(0, result.upMove);
}

void test_MSG_DeltaUsercmd_up_movement(void) {
    netMsg_t msg;
    byte buffer[256];
    userCmd_t from, to, result;
    
    memset(&from, 0, sizeof(from));
    memset(&to, 0, sizeof(to));
    
    from.upMove = 0;
    to.upMove = 200;  /* Jumping */
    to.msec = 16;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDeltaUsercmd(&msg, &from, &to, 0);
    
    /* Verify roundtrip */
    MSG_BeginReading(&msg);
    MSG_ReadDeltaUsercmd(&msg, &from, &result);
    
    TEST_ASSERT_EQUAL(0, result.forwardMove);
    TEST_ASSERT_EQUAL(0, result.sideMove);
    TEST_ASSERT_EQUAL(200, result.upMove);
}

void test_MSG_DeltaUsercmd_all_movement(void) {
    netMsg_t msg;
    byte buffer[256];
    userCmd_t from, to, result;
    
    memset(&from, 0, sizeof(from));
    memset(&to, 0, sizeof(to));
    
    to.forwardMove = 400;
    to.sideMove = 200;
    to.upMove = 100;
    to.msec = 16;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDeltaUsercmd(&msg, &from, &to, 0);
    
    /* Verify roundtrip */
    MSG_BeginReading(&msg);
    MSG_ReadDeltaUsercmd(&msg, &from, &result);
    
    TEST_ASSERT_EQUAL(400, result.forwardMove);
    TEST_ASSERT_EQUAL(200, result.sideMove);
    TEST_ASSERT_EQUAL(100, result.upMove);
}

void test_MSG_DeltaUsercmd_buttons(void) {
    netMsg_t msg;
    byte buffer[256];
    userCmd_t from, to, result;
    
    memset(&from, 0, sizeof(from));
    memset(&to, 0, sizeof(to));
    
    from.buttons = 0;
    to.buttons = 1;  /* BUTTON_ATTACK */
    to.msec = 16;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDeltaUsercmd(&msg, &from, &to, 0);
    
    /* Verify roundtrip */
    MSG_BeginReading(&msg);
    MSG_ReadDeltaUsercmd(&msg, &from, &result);
    
    TEST_ASSERT_EQUAL(1, result.buttons);
}

void test_MSG_DeltaUsercmd_multiple_buttons(void) {
    netMsg_t msg;
    byte buffer[256];
    userCmd_t from, to, result;
    
    memset(&from, 0, sizeof(from));
    memset(&to, 0, sizeof(to));
    
    from.buttons = 0;
    to.buttons = 3;  /* BUTTON_ATTACK | BUTTON_USE */
    to.msec = 16;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDeltaUsercmd(&msg, &from, &to, 0);
    
    /* Verify roundtrip */
    MSG_BeginReading(&msg);
    MSG_ReadDeltaUsercmd(&msg, &from, &result);
    
    TEST_ASSERT_EQUAL(3, result.buttons);
}

void test_MSG_DeltaUsercmd_impulse(void) {
    netMsg_t msg;
    byte buffer[256];
    userCmd_t from, to, result;
    
    memset(&from, 0, sizeof(from));
    memset(&to, 0, sizeof(to));
    
    from.impulse = 0;
    to.impulse = 100;  /* Impulse command */
    to.msec = 16;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDeltaUsercmd(&msg, &from, &to, 0);
    
    /* Verify roundtrip */
    MSG_BeginReading(&msg);
    MSG_ReadDeltaUsercmd(&msg, &from, &result);
    
    TEST_ASSERT_EQUAL(100, result.impulse);
}

void test_MSG_DeltaUsercmd_msec_variations(void) {
    netMsg_t msg;
    byte buffer[256];
    userCmd_t from, to, result;
    
    memset(&from, 0, sizeof(from));
    memset(&to, 0, sizeof(to));
    
    to.msec = 33;  /* Slower frame */
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDeltaUsercmd(&msg, &from, &to, 0);
    
    MSG_BeginReading(&msg);
    MSG_ReadDeltaUsercmd(&msg, &from, &result);
    
    TEST_ASSERT_EQUAL(33, result.msec);
}

void test_MSG_DeltaUsercmd_light_level(void) {
    netMsg_t msg;
    byte buffer[256];
    userCmd_t from, to, result;
    
    memset(&from, 0, sizeof(from));
    memset(&to, 0, sizeof(to));
    
    from.lightLevel = 0;
    to.lightLevel = 255;  /* Bright light */
    to.msec = 16;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDeltaUsercmd(&msg, &from, &to, 0);
    
    MSG_BeginReading(&msg);
    MSG_ReadDeltaUsercmd(&msg, &from, &result);
    
    TEST_ASSERT_EQUAL(255, result.lightLevel);
}

void test_MSG_DeltaUsercmd_complex_input(void) {
    netMsg_t msg;
    byte buffer[256];
    userCmd_t from, to, result;
    
    memset(&from, 0, sizeof(from));
    memset(&to, 0, sizeof(to));
    
    /* Typical player input: running forward while turning and shooting */
    to.angles[0] = 50;    /* Looking slightly down */
    to.angles[1] = 1500;  /* Turning */
    to.angles[2] = 0;
    
    to.forwardMove = 400;
    to.sideMove = 100;
    to.upMove = 0;
    
    to.buttons = 1;  /* Firing */
    to.impulse = 0;
    to.msec = 16;
    to.lightLevel = 128;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDeltaUsercmd(&msg, &from, &to, 0);
    
    /* Verify complete roundtrip */
    MSG_BeginReading(&msg);
    MSG_ReadDeltaUsercmd(&msg, &from, &result);
    
    TEST_ASSERT_EQUAL(to.angles[0], result.angles[0]);
    TEST_ASSERT_EQUAL(to.angles[1], result.angles[1]);
    TEST_ASSERT_EQUAL(to.angles[2], result.angles[2]);
    TEST_ASSERT_EQUAL(to.forwardMove, result.forwardMove);
    TEST_ASSERT_EQUAL(to.sideMove, result.sideMove);
    TEST_ASSERT_EQUAL(to.upMove, result.upMove);
    TEST_ASSERT_EQUAL(to.buttons, result.buttons);
    TEST_ASSERT_EQUAL(to.impulse, result.impulse);
    TEST_ASSERT_EQUAL(to.msec, result.msec);
    TEST_ASSERT_EQUAL(to.lightLevel, result.lightLevel);
}

void test_MSG_DeltaUsercmd_consecutive_commands(void) {
    netMsg_t msg;
    byte buffer[256];
    userCmd_t cmd1, cmd2, cmd3, result2, result3;
    
    /* Simulate sequence of player commands */
    memset(&cmd1, 0, sizeof(cmd1));
    cmd1.msec = 16;
    cmd1.lightLevel = 128;
    
    /* Second command: start moving forward */
    memcpy(&cmd2, &cmd1, sizeof(cmd2));
    cmd2.forwardMove = 400;
    cmd2.angles[1] = 100;
    
    /* Third command: continue moving, turn more */
    memcpy(&cmd3, &cmd2, sizeof(cmd3));
    cmd3.angles[1] = 200;
    cmd3.buttons = 1;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    
    /* Write delta from cmd1 to cmd2 */
    size_t start = msg.curSize;
    MSG_WriteDeltaUsercmd(&msg, &cmd1, &cmd2, 0);
    size_t delta1_size = msg.curSize - start;
    
    /* Write delta from cmd2 to cmd3 */
    start = msg.curSize;
    MSG_WriteDeltaUsercmd(&msg, &cmd2, &cmd3, 0);
    size_t delta2_size = msg.curSize - start;
    
    /* Delta compression should work - changes are minimal */
    TEST_ASSERT_GREATER_THAN(0, delta1_size);
    TEST_ASSERT_GREATER_THAN(0, delta2_size);
    
    /* Read back and verify */
    MSG_BeginReading(&msg);
    MSG_ReadDeltaUsercmd(&msg, &cmd1, &result2);
    MSG_ReadDeltaUsercmd(&msg, &cmd2, &result3);
    
    TEST_ASSERT_EQUAL(cmd2.forwardMove, result2.forwardMove);
    TEST_ASSERT_EQUAL(cmd2.angles[1], result2.angles[1]);
    
    TEST_ASSERT_EQUAL(cmd3.angles[1], result3.angles[1]);
    TEST_ASSERT_EQUAL(cmd3.buttons, result3.buttons);
}

void test_MSG_DeltaUsercmd_zero_to_nonzero(void) {
    netMsg_t msg;
    byte buffer[256];
    userCmd_t from, to, result;
    
    /* Simpler test: only change a few fields to isolate the issue */
    memset(&from, 0, sizeof(from));
    memset(&to, 0, sizeof(to));
    
    /* Only change movement - no buttons */
    to.forwardMove = 400;
    to.msec = 16;
    to.lightLevel = 200;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDeltaUsercmd(&msg, &from, &to, 0);
    
    /* Reset and read back */
    memset(&result, 0, sizeof(result));
    MSG_BeginReading(&msg);
    MSG_ReadDeltaUsercmd(&msg, &from, &result);
    
    /* Verify fields */
    TEST_ASSERT_EQUAL(to.forwardMove, result.forwardMove);
    TEST_ASSERT_EQUAL(to.msec, result.msec);
    TEST_ASSERT_EQUAL(to.lightLevel, result.lightLevel);
}

void test_MSG_DeltaUsercmd_negative_movement(void) {
    netMsg_t msg;
    byte buffer[256];
    userCmd_t from, to, result;
    
    memset(&from, 0, sizeof(from));
    memset(&to, 0, sizeof(to));
    
    /* Walking backward and left */
    to.forwardMove = -400;
    to.sideMove = -300;
    to.msec = 16;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDeltaUsercmd(&msg, &from, &to, 0);
    
    MSG_BeginReading(&msg);
    MSG_ReadDeltaUsercmd(&msg, &from, &result);
    
    TEST_ASSERT_EQUAL(-400, result.forwardMove);
    TEST_ASSERT_EQUAL(-300, result.sideMove);
}

void test_MSG_DeltaUsercmd_button_release(void) {
    netMsg_t msg;
    byte buffer[256];
    userCmd_t from, to, result;
    
    memset(&from, 0, sizeof(from));
    memset(&to, 0, sizeof(to));
    
    /* Button was pressed, now released */
    from.buttons = 1;
    to.buttons = 0;
    to.msec = 16;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDeltaUsercmd(&msg, &from, &to, 0);
    
    MSG_BeginReading(&msg);
    MSG_ReadDeltaUsercmd(&msg, &from, &result);
    
    TEST_ASSERT_EQUAL(0, result.buttons);
}

void test_MSG_DeltaUsercmd_impulse_clear(void) {
    netMsg_t msg;
    byte buffer[256];
    userCmd_t from, to, result;
    
    memset(&from, 0, sizeof(from));
    memset(&to, 0, sizeof(to));
    
    /* Impulse executed, now cleared */
    from.impulse = 100;
    to.impulse = 0;
    to.msec = 16;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDeltaUsercmd(&msg, &from, &to, 0);
    
    MSG_BeginReading(&msg);
    MSG_ReadDeltaUsercmd(&msg, &from, &result);
    
    TEST_ASSERT_EQUAL(0, result.impulse);
}

/*
 * ============================================================================
 * MSG_WriteDir / MSG_ReadDir Tests
 * 
 * Direction vector compression to single byte (162 pre-calculated normals)
 * Used for particle effects, projectile directions, surface normals
 * Tests roundtrip accuracy for cardinal and diagonal directions
 * ============================================================================
 */

void test_MSG_WriteDir_up(void) {
    netMsg_t msg;
    byte buffer[256];
    vec3_t dir_in = {0.0f, 0.0f, 1.0f};  /* Straight up */
    vec3_t dir_out;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDir(&msg, dir_in);
    
    /* Should write exactly 1 byte */
    TEST_ASSERT_EQUAL(1, msg.curSize);
    
    /* Roundtrip */
    MSG_BeginReading(&msg);
    MSG_ReadDir(&msg, dir_out);
    
    /* Should be very close to original (uses nearest normal from 162 options) */
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, dir_out[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, dir_out[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, dir_out[2]);
}

void test_MSG_WriteDir_down(void) {
    netMsg_t msg;
    byte buffer[256];
    vec3_t dir_in = {0.0f, 0.0f, -1.0f};  /* Straight down */
    vec3_t dir_out;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDir(&msg, dir_in);
    
    MSG_BeginReading(&msg);
    MSG_ReadDir(&msg, dir_out);
    
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, dir_out[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, dir_out[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -1.0f, dir_out[2]);
}

void test_MSG_WriteDir_north(void) {
    netMsg_t msg;
    byte buffer[256];
    vec3_t dir_in = {1.0f, 0.0f, 0.0f};  /* North (positive X) */
    vec3_t dir_out;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDir(&msg, dir_in);
    
    MSG_BeginReading(&msg);
    MSG_ReadDir(&msg, dir_out);
    
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, dir_out[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, dir_out[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, dir_out[2]);
}

void test_MSG_WriteDir_south(void) {
    netMsg_t msg;
    byte buffer[256];
    vec3_t dir_in = {-1.0f, 0.0f, 0.0f};  /* South (negative X) */
    vec3_t dir_out;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDir(&msg, dir_in);
    
    MSG_BeginReading(&msg);
    MSG_ReadDir(&msg, dir_out);
    
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -1.0f, dir_out[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, dir_out[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, dir_out[2]);
}

void test_MSG_WriteDir_east(void) {
    netMsg_t msg;
    byte buffer[256];
    vec3_t dir_in = {0.0f, 1.0f, 0.0f};  /* East (positive Y) */
    vec3_t dir_out;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDir(&msg, dir_in);
    
    MSG_BeginReading(&msg);
    MSG_ReadDir(&msg, dir_out);
    
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, dir_out[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, dir_out[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, dir_out[2]);
}

void test_MSG_WriteDir_west(void) {
    netMsg_t msg;
    byte buffer[256];
    vec3_t dir_in = {0.0f, -1.0f, 0.0f};  /* West (negative Y) */
    vec3_t dir_out;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDir(&msg, dir_in);
    
    MSG_BeginReading(&msg);
    MSG_ReadDir(&msg, dir_out);
    
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, dir_out[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -1.0f, dir_out[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, dir_out[2]);
}

void test_MSG_WriteDir_diagonal_xy(void) {
    netMsg_t msg;
    byte buffer[256];
    vec3_t dir_in = {0.707107f, 0.707107f, 0.0f};  /* NE diagonal (45 degrees) */
    vec3_t dir_out;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDir(&msg, dir_in);
    
    MSG_BeginReading(&msg);
    MSG_ReadDir(&msg, dir_out);
    
    /* Should find a close normal - verify general direction */
    TEST_ASSERT_GREATER_THAN(0.5f, dir_out[0]);  /* Positive X component */
    TEST_ASSERT_GREATER_THAN(0.5f, dir_out[1]);  /* Positive Y component */
    TEST_ASSERT_FLOAT_WITHIN(0.2f, 0.0f, dir_out[2]);  /* Near zero Z */
}

void test_MSG_WriteDir_diagonal_xyz(void) {
    netMsg_t msg;
    byte buffer[256];
    vec3_t dir_in = {0.577350f, 0.577350f, 0.577350f};  /* Perfect 3D diagonal */
    vec3_t dir_out;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDir(&msg, dir_in);
    
    MSG_BeginReading(&msg);
    MSG_ReadDir(&msg, dir_out);
    
    /* All components should be roughly equal and positive */
    TEST_ASSERT_GREATER_THAN(0.4f, dir_out[0]);
    TEST_ASSERT_GREATER_THAN(0.4f, dir_out[1]);
    TEST_ASSERT_GREATER_THAN(0.4f, dir_out[2]);
}

void test_MSG_WriteDir_diagonal_up_forward(void) {
    netMsg_t msg;
    byte buffer[256];
    vec3_t dir_in = {0.707107f, 0.0f, 0.707107f};  /* 45 degrees up and forward */
    vec3_t dir_out;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDir(&msg, dir_in);
    
    MSG_BeginReading(&msg);
    MSG_ReadDir(&msg, dir_out);
    
    TEST_ASSERT_GREATER_THAN(0.5f, dir_out[0]);  /* Positive X */
    TEST_ASSERT_FLOAT_WITHIN(0.2f, 0.0f, dir_out[1]);  /* Near zero Y */
    TEST_ASSERT_GREATER_THAN(0.5f, dir_out[2]);  /* Positive Z */
}

void test_MSG_WriteDir_multiple_directions(void) {
    netMsg_t msg;
    byte buffer[256];
    vec3_t dir1 = {1.0f, 0.0f, 0.0f};
    vec3_t dir2 = {0.0f, 1.0f, 0.0f};
    vec3_t dir3 = {0.0f, 0.0f, 1.0f};
    vec3_t result1, result2, result3;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    
    /* Write three directions */
    MSG_WriteDir(&msg, dir1);
    MSG_WriteDir(&msg, dir2);
    MSG_WriteDir(&msg, dir3);
    
    /* Should be 3 bytes */
    TEST_ASSERT_EQUAL(3, msg.curSize);
    
    /* Read them back in order */
    MSG_BeginReading(&msg);
    MSG_ReadDir(&msg, result1);
    MSG_ReadDir(&msg, result2);
    MSG_ReadDir(&msg, result3);
    
    /* Verify each direction */
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, result1[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, result2[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, result3[2]);
}

void test_MSG_WriteDir_unit_vector_required(void) {
    netMsg_t msg;
    byte buffer[256];
    vec3_t dir_in = {2.0f, 0.0f, 0.0f};  /* Not normalized */
    vec3_t dir_out;
    
    /* DirToByte uses dot product to find best match, so non-unit vectors work
     * but results may be less accurate. Still, test the behavior. */
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDir(&msg, dir_in);
    
    MSG_BeginReading(&msg);
    MSG_ReadDir(&msg, dir_out);
    
    /* Result should still be normalized (from lookup table) */
    float length = sqrtf(dir_out[0]*dir_out[0] + dir_out[1]*dir_out[1] + dir_out[2]*dir_out[2]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, length);
    
    /* Direction should still be primarily in X */
    TEST_ASSERT_GREATER_THAN(0.9f, dir_out[0]);
}

void test_MSG_WriteDir_negative_diagonal(void) {
    netMsg_t msg;
    byte buffer[256];
    vec3_t dir_in = {-0.577350f, -0.577350f, -0.577350f};  /* Negative 3D diagonal */
    vec3_t dir_out;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDir(&msg, dir_in);
    
    MSG_BeginReading(&msg);
    MSG_ReadDir(&msg, dir_out);
    
    /* All components should be negative and roughly equal */
    TEST_ASSERT_LESS_THAN(-0.4f, dir_out[0]);
    TEST_ASSERT_LESS_THAN(-0.4f, dir_out[1]);
    TEST_ASSERT_LESS_THAN(-0.4f, dir_out[2]);
}

void test_MSG_WriteDir_accuracy_check(void) {
    netMsg_t msg;
    byte buffer[256];
    vec3_t dir_in = {0.0f, 0.0f, 1.0f};  /* Pure up vector */
    vec3_t dir_out;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDir(&msg, dir_in);
    
    MSG_BeginReading(&msg);
    MSG_ReadDir(&msg, dir_out);
    
    /* Calculate dot product to verify accuracy */
    float dot = dir_in[0]*dir_out[0] + dir_in[1]*dir_out[1] + dir_in[2]*dir_out[2];
    
    /* Dot product should be very close to 1.0 (perfect match) */
    TEST_ASSERT_GREATER_THAN(0.99f, dot);
}

void test_MSG_WriteDir_interleaved_with_other_data(void) {
    netMsg_t msg;
    byte buffer[256];
    vec3_t dir1 = {1.0f, 0.0f, 0.0f};
    vec3_t dir2 = {0.0f, 0.0f, -1.0f};
    vec3_t result1, result2;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    
    /* Interleave directions with other data */
    MSG_WriteDir(&msg, dir1);
    MSG_WriteByte(&msg, 42);
    MSG_WriteShort(&msg, 1000);
    MSG_WriteDir(&msg, dir2);
    MSG_WriteLong(&msg, 99999);
    
    /* Read back in same order */
    MSG_BeginReading(&msg);
    MSG_ReadDir(&msg, result1);
    int byte_val = MSG_ReadByte(&msg);
    int short_val = MSG_ReadShort(&msg);
    MSG_ReadDir(&msg, result2);
    int long_val = MSG_ReadLong(&msg);
    
    /* Verify directions survived */
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, result1[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -1.0f, result2[2]);
    
    /* Verify other data too */
    TEST_ASSERT_EQUAL(42, byte_val);
    TEST_ASSERT_EQUAL(1000, short_val);
    TEST_ASSERT_EQUAL(99999, long_val);
}

void test_MSG_WriteDir_angled_shot(void) {
    netMsg_t msg;
    byte buffer[256];
    vec3_t dir_in = {0.866025f, 0.0f, 0.5f};  /* 30 degrees up from horizontal */
    vec3_t dir_out;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDir(&msg, dir_in);
    
    MSG_BeginReading(&msg);
    MSG_ReadDir(&msg, dir_out);
    
    /* Verify general direction preserved */
    TEST_ASSERT_GREATER_THAN(0.7f, dir_out[0]);  /* Strong forward component */
    TEST_ASSERT_FLOAT_WITHIN(0.2f, 0.0f, dir_out[1]);  /* Minimal side component */
    TEST_ASSERT_GREATER_THAN(0.3f, dir_out[2]);  /* Some upward component */
}

void test_MSG_WriteDir_roundtrip_preserves_normalization(void) {
    netMsg_t msg;
    byte buffer[256];
    vec3_t dir_in = {0.6f, 0.8f, 0.0f};  /* Normalized XY plane vector */
    vec3_t dir_out;
    
    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_WriteDir(&msg, dir_in);
    
    MSG_BeginReading(&msg);
    MSG_ReadDir(&msg, dir_out);
    
    /* Output should be unit length (from lookup table) */
    float length = sqrtf(dir_out[0]*dir_out[0] + dir_out[1]*dir_out[1] + dir_out[2]*dir_out[2]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, length);
}

/*
 * ============================================================================
 * Main Test Runner
 * ============================================================================
 */

int main(void) {
    UNITY_BEGIN();
    
    /* Init/Clear tests */
    RUN_TEST(test_MSG_Init_creates_valid_buffer);
    RUN_TEST(test_MSG_Clear_resets_size);
    
    /* WriteByte tests */
    RUN_TEST(test_MSG_WriteByte_writes_zero);
    RUN_TEST(test_MSG_WriteByte_writes_max_value);
    RUN_TEST(test_MSG_WriteByte_writes_multiple_bytes);
    RUN_TEST(test_MSG_WriteByte_increments_size);
    RUN_TEST(test_MSG_WriteByte_handles_out_of_range_negative);
    RUN_TEST(test_MSG_WriteByte_handles_out_of_range_positive);
    
    /* WriteChar tests */
    RUN_TEST(test_MSG_WriteChar_writes_negative_value);
    RUN_TEST(test_MSG_WriteChar_writes_positive_value);
    
    /* WriteShort tests */
    RUN_TEST(test_MSG_WriteShort_writes_zero);
    RUN_TEST(test_MSG_WriteShort_writes_little_endian);
    RUN_TEST(test_MSG_WriteShort_handles_negative);
    RUN_TEST(test_MSG_WriteShort_max_positive);
    RUN_TEST(test_MSG_WriteShort_max_negative);
    RUN_TEST(test_MSG_ReadShort_multiple_values);
    
    /* WriteLong tests */
    RUN_TEST(test_MSG_WriteLong_writes_zero);
    RUN_TEST(test_MSG_WriteLong_writes_little_endian);
    RUN_TEST(test_MSG_WriteLong_handles_negative);
    RUN_TEST(test_MSG_WriteLong_max_positive);
    RUN_TEST(test_MSG_ReadLong_multiple_values);
    
    /* WriteString tests */
    RUN_TEST(test_MSG_WriteString_writes_empty_string);
    RUN_TEST(test_MSG_WriteString_writes_simple_string);
    RUN_TEST(test_MSG_WriteString_writes_with_null_terminator);
    
    /* WriteFloat tests */
    RUN_TEST(test_MSG_WriteFloat_writes_zero);
    RUN_TEST(test_MSG_WriteFloat_writes_value);
    
    /* Read/Write roundtrip tests */
    RUN_TEST(test_MSG_ReadByte_reads_written_value);
    RUN_TEST(test_MSG_ReadByte_reads_multiple_values);
    RUN_TEST(test_MSG_ReadShort_roundtrip);
    RUN_TEST(test_MSG_ReadLong_roundtrip);
    RUN_TEST(test_MSG_ReadString_roundtrip);
    RUN_TEST(test_MSG_ReadFloat_roundtrip);
    
    /* Float edge case tests */
    RUN_TEST(test_MSG_Float_roundtrip_negative);
    RUN_TEST(test_MSG_Float_roundtrip_negative_one);
    RUN_TEST(test_MSG_Float_roundtrip_very_large);
    RUN_TEST(test_MSG_Float_roundtrip_very_small_positive);
    RUN_TEST(test_MSG_Float_roundtrip_very_small_negative);
    RUN_TEST(test_MSG_Float_roundtrip_epsilon);
    RUN_TEST(test_MSG_Float_roundtrip_infinity_positive);
    RUN_TEST(test_MSG_Float_roundtrip_infinity_negative);
    RUN_TEST(test_MSG_Float_roundtrip_nan);
    RUN_TEST(test_MSG_Float_roundtrip_multiple_values);
    RUN_TEST(test_MSG_Float_size_consistency);
    
    /* WriteCoord/ReadCoord tests - CRITICAL for R1Q2 compatibility */
    RUN_TEST(test_MSG_WriteCoord_zero);
    RUN_TEST(test_MSG_WriteCoord_positive);
    RUN_TEST(test_MSG_WriteCoord_negative);
    RUN_TEST(test_MSG_WriteCoord_precision_eighth_unit);
    RUN_TEST(test_MSG_WriteCoord_quantization);
    RUN_TEST(test_MSG_WriteCoord_max_range);
    RUN_TEST(test_MSG_WriteCoord_min_range);
    RUN_TEST(test_MSG_WriteCoord_multiple_coords);
    RUN_TEST(test_MSG_WriteCoord_r1q2_typical_values);
    
    /* WriteAngle/ReadAngle tests - CRITICAL for entity rotation */
    RUN_TEST(test_MSG_WriteAngle_zero);
    RUN_TEST(test_MSG_WriteAngle_90_degrees);
    RUN_TEST(test_MSG_WriteAngle_180_degrees);
    RUN_TEST(test_MSG_WriteAngle_270_degrees);
    RUN_TEST(test_MSG_WriteAngle_negative);
    RUN_TEST(test_MSG_WriteAngle_wrap_around);
    RUN_TEST(test_MSG_WriteAngle_precision);
    RUN_TEST(test_MSG_WriteAngle_multiple_angles);
    
    /* WriteAngle16/ReadAngle16 tests - High precision rotation */
    RUN_TEST(test_MSG_WriteAngle16_zero);
    RUN_TEST(test_MSG_WriteAngle16_90_degrees);
    RUN_TEST(test_MSG_WriteAngle16_180_degrees);
    RUN_TEST(test_MSG_WriteAngle16_precise_value);
    RUN_TEST(test_MSG_WriteAngle16_negative);
    RUN_TEST(test_MSG_WriteAngle16_full_rotation);
    RUN_TEST(test_MSG_WriteAngle16_high_precision);
    RUN_TEST(test_MSG_WriteAngle16_multiple_precise_angles);
    RUN_TEST(test_MSG_WriteAngle16_vs_WriteAngle_precision);
    
    /* WritePos/ReadPos tests - CRITICAL for entity position sync */
    RUN_TEST(test_MSG_WritePos_origin);
    RUN_TEST(test_MSG_WritePos_positive_coords);
    RUN_TEST(test_MSG_WritePos_negative_coords);
    RUN_TEST(test_MSG_WritePos_mixed_signs);
    RUN_TEST(test_MSG_WritePos_precise_eighth_units);
    RUN_TEST(test_MSG_WritePos_max_range);
    RUN_TEST(test_MSG_WritePos_r1q2_spawn_position);
    RUN_TEST(test_MSG_WritePos_multiple_positions);
    RUN_TEST(test_MSG_WritePos_quantization);
    RUN_TEST(test_MSG_WritePos_interleaved_with_other_data);
    
    /* MSG_WriteDeltaEntity tests */
    RUN_TEST(test_MSG_WriteDeltaEntity_no_change);
    RUN_TEST(test_MSG_WriteDeltaEntity_force_write_no_change);
    RUN_TEST(test_MSG_WriteDeltaEntity_remove);
    RUN_TEST(test_MSG_WriteDeltaEntity_origin_change);
    RUN_TEST(test_MSG_WriteDeltaEntity_all_origins_change);
    RUN_TEST(test_MSG_WriteDeltaEntity_angles_change);
    RUN_TEST(test_MSG_WriteDeltaEntity_all_angles_change);
    RUN_TEST(test_MSG_WriteDeltaEntity_model_change);
    RUN_TEST(test_MSG_WriteDeltaEntity_all_models_change);
    RUN_TEST(test_MSG_WriteDeltaEntity_frame_8bit);
    RUN_TEST(test_MSG_WriteDeltaEntity_frame_16bit);
    RUN_TEST(test_MSG_WriteDeltaEntity_skin_8bit);
    RUN_TEST(test_MSG_WriteDeltaEntity_effects_change);
    RUN_TEST(test_MSG_WriteDeltaEntity_renderfx_change);
    RUN_TEST(test_MSG_WriteDeltaEntity_event);
    RUN_TEST(test_MSG_WriteDeltaEntity_solid_change);
    RUN_TEST(test_MSG_WriteDeltaEntity_sound_change);
    RUN_TEST(test_MSG_WriteDeltaEntity_number16);
    RUN_TEST(test_MSG_WriteDeltaEntity_complex_update);
    
    /* MSG_WriteDeltaUsercmd / MSG_ReadDeltaUsercmd tests */
    RUN_TEST(test_MSG_DeltaUsercmd_no_change);
    RUN_TEST(test_MSG_DeltaUsercmd_angles_change);
    RUN_TEST(test_MSG_DeltaUsercmd_single_angle_change);
    RUN_TEST(test_MSG_DeltaUsercmd_forward_movement);
    RUN_TEST(test_MSG_DeltaUsercmd_side_movement);
    RUN_TEST(test_MSG_DeltaUsercmd_up_movement);
    RUN_TEST(test_MSG_DeltaUsercmd_all_movement);
    RUN_TEST(test_MSG_DeltaUsercmd_buttons);
    RUN_TEST(test_MSG_DeltaUsercmd_multiple_buttons);
    RUN_TEST(test_MSG_DeltaUsercmd_impulse);
    RUN_TEST(test_MSG_DeltaUsercmd_msec_variations);
    RUN_TEST(test_MSG_DeltaUsercmd_light_level);
    RUN_TEST(test_MSG_DeltaUsercmd_complex_input);
    RUN_TEST(test_MSG_DeltaUsercmd_consecutive_commands);
    RUN_TEST(test_MSG_DeltaUsercmd_zero_to_nonzero);
    RUN_TEST(test_MSG_DeltaUsercmd_negative_movement);
    RUN_TEST(test_MSG_DeltaUsercmd_button_release);
    RUN_TEST(test_MSG_DeltaUsercmd_impulse_clear);
    
    /* MSG_WriteDir / MSG_ReadDir tests */
    RUN_TEST(test_MSG_WriteDir_up);
    RUN_TEST(test_MSG_WriteDir_down);
    RUN_TEST(test_MSG_WriteDir_north);
    RUN_TEST(test_MSG_WriteDir_south);
    RUN_TEST(test_MSG_WriteDir_east);
    RUN_TEST(test_MSG_WriteDir_west);
    RUN_TEST(test_MSG_WriteDir_diagonal_xy);
    RUN_TEST(test_MSG_WriteDir_diagonal_xyz);
    RUN_TEST(test_MSG_WriteDir_diagonal_up_forward);
    RUN_TEST(test_MSG_WriteDir_multiple_directions);
    RUN_TEST(test_MSG_WriteDir_unit_vector_required);
    RUN_TEST(test_MSG_WriteDir_negative_diagonal);
    RUN_TEST(test_MSG_WriteDir_accuracy_check);
    RUN_TEST(test_MSG_WriteDir_interleaved_with_other_data);
    RUN_TEST(test_MSG_WriteDir_angled_shot);
    RUN_TEST(test_MSG_WriteDir_roundtrip_preserves_normalization);
    
    return UNITY_END();
}
