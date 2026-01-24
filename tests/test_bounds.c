/*
 * test_bounds.c - Unit tests for shared/m_bounds.c
 *
 * These tests verify the bounds and collision detection functions.
 * Run with: make test_bounds && ./test_bounds
 */

#include "../unity/unity.h"

/* Include the actual implementation */
#include "../shared/shared.h"

/*
 * ============================================================================
 * ClearBounds Tests
 * ============================================================================
 */

void test_ClearBounds_sets_mins_to_max_float(void) {
    vec3_t mins, maxs;
    ClearBounds(mins, maxs);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 99999.0f, mins[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 99999.0f, mins[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 99999.0f, mins[2]);
}

void test_ClearBounds_sets_maxs_to_min_float(void) {
    vec3_t mins, maxs;
    ClearBounds(mins, maxs);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -99999.0f, maxs[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -99999.0f, maxs[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -99999.0f, maxs[2]);
}

/*
 * ============================================================================
 * AddPointToBounds Tests
 * ============================================================================
 */

void test_AddPointToBounds_expands_empty_bounds(void) {
    vec3_t mins, maxs;
    vec3_t point = {1.0f, 2.0f, 3.0f};
    ClearBounds(mins, maxs);
    AddPointToBounds(point, mins, maxs);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, mins[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, mins[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.0f, mins[2]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, maxs[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, maxs[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.0f, maxs[2]);
}

void test_AddPointToBounds_updates_mins(void) {
    vec3_t mins, maxs;
    vec3_t point1 = {1.0f, 1.0f, 1.0f};
    vec3_t point2 = {0.0f, 0.0f, 0.0f};
    ClearBounds(mins, maxs);
    AddPointToBounds(point1, mins, maxs);
    AddPointToBounds(point2, mins, maxs);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, mins[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, mins[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, mins[2]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, maxs[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, maxs[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, maxs[2]);
}

void test_AddPointToBounds_updates_maxs(void) {
    vec3_t mins, maxs;
    vec3_t point1 = {0.0f, 0.0f, 0.0f};
    vec3_t point2 = {2.0f, 3.0f, 4.0f};
    ClearBounds(mins, maxs);
    AddPointToBounds(point1, mins, maxs);
    AddPointToBounds(point2, mins, maxs);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, mins[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, mins[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, mins[2]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, maxs[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.0f, maxs[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 4.0f, maxs[2]);
}

void test_AddPointToBounds_negative_values(void) {
    vec3_t mins, maxs;
    vec3_t point = {-1.0f, -2.0f, -3.0f};
    ClearBounds(mins, maxs);
    AddPointToBounds(point, mins, maxs);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, mins[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -2.0f, mins[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -3.0f, mins[2]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, maxs[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -2.0f, maxs[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -3.0f, maxs[2]);
}

/*
 * ============================================================================
 * BoundsIntersect Tests
 * ============================================================================
 */

void test_BoundsIntersect_identical_bounds(void) {
    vec3_t mins1 = {0.0f, 0.0f, 0.0f};
    vec3_t maxs1 = {1.0f, 1.0f, 1.0f};
    vec3_t mins2 = {0.0f, 0.0f, 0.0f};
    vec3_t maxs2 = {1.0f, 1.0f, 1.0f};
    TEST_ASSERT_TRUE(BoundsIntersect(mins1, maxs1, mins2, maxs2));
}

void test_BoundsIntersect_overlapping_bounds(void) {
    vec3_t mins1 = {0.0f, 0.0f, 0.0f};
    vec3_t maxs1 = {2.0f, 2.0f, 2.0f};
    vec3_t mins2 = {1.0f, 1.0f, 1.0f};
    vec3_t maxs2 = {3.0f, 3.0f, 3.0f};
    TEST_ASSERT_TRUE(BoundsIntersect(mins1, maxs1, mins2, maxs2));
}

void test_BoundsIntersect_touching_bounds(void) {
    vec3_t mins1 = {0.0f, 0.0f, 0.0f};
    vec3_t maxs1 = {1.0f, 1.0f, 1.0f};
    vec3_t mins2 = {1.0f, 1.0f, 1.0f};
    vec3_t maxs2 = {2.0f, 2.0f, 2.0f};
    TEST_ASSERT_TRUE(BoundsIntersect(mins1, maxs1, mins2, maxs2));
}

void test_BoundsIntersect_separate_bounds(void) {
    vec3_t mins1 = {0.0f, 0.0f, 0.0f};
    vec3_t maxs1 = {1.0f, 1.0f, 1.0f};
    vec3_t mins2 = {2.0f, 2.0f, 2.0f};
    vec3_t maxs2 = {3.0f, 3.0f, 3.0f};
    TEST_ASSERT_FALSE(BoundsIntersect(mins1, maxs1, mins2, maxs2));
}

void test_BoundsIntersect_empty_bounds(void) {
    vec3_t mins1, maxs1;
    vec3_t mins2 = {0.0f, 0.0f, 0.0f};
    vec3_t maxs2 = {1.0f, 1.0f, 1.0f};
    ClearBounds(mins1, maxs1);
    TEST_ASSERT_FALSE(BoundsIntersect(mins1, maxs1, mins2, maxs2));
}

/*
 * ============================================================================
 * BoundsAndSphereIntersect Tests
 * ============================================================================
 */

void test_BoundsAndSphereIntersect_sphere_completely_inside(void) {
    vec3_t mins = {0.0f, 0.0f, 0.0f};
    vec3_t maxs = {2.0f, 2.0f, 2.0f};
    vec3_t centre = {1.0f, 1.0f, 1.0f};
    float radius = 0.5f;
    TEST_ASSERT_TRUE(BoundsAndSphereIntersect(mins, maxs, centre, radius));
}

void test_BoundsAndSphereIntersect_sphere_outside(void) {
    vec3_t mins = {0.0f, 0.0f, 0.0f};
    vec3_t maxs = {1.0f, 1.0f, 1.0f};
    vec3_t centre = {3.0f, 3.0f, 3.0f};
    float radius = 0.5f;
    TEST_ASSERT_FALSE(BoundsAndSphereIntersect(mins, maxs, centre, radius));
}

void test_BoundsAndSphereIntersect_sphere_intersecting(void) {
    vec3_t mins = {0.0f, 0.0f, 0.0f};
    vec3_t maxs = {1.0f, 1.0f, 1.0f};
    vec3_t centre = {1.5f, 1.5f, 1.5f};
    float radius = 1.0f;
    TEST_ASSERT_TRUE(BoundsAndSphereIntersect(mins, maxs, centre, radius));
}

void test_BoundsAndSphereIntersect_sphere_touching(void) {
    vec3_t mins = {0.0f, 0.0f, 0.0f};
    vec3_t maxs = {1.0f, 1.0f, 1.0f};
    vec3_t centre = {2.0f, 1.0f, 1.0f};
    float radius = 1.0f;
    TEST_ASSERT_TRUE(BoundsAndSphereIntersect(mins, maxs, centre, radius));
}

/*
 * ============================================================================
 * RadiusFromBounds Tests
 * ============================================================================
 */

void test_RadiusFromBounds_unit_cube(void) {
    vec3_t mins = {-0.5f, -0.5f, -0.5f};
    vec3_t maxs = {0.5f, 0.5f, 0.5f};
    float radius = RadiusFromBounds(mins, maxs);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.866f, radius);  // sqrt(0.5^2 * 3) ≈ 0.866
}

void test_RadiusFromBounds_origin_bounds(void) {
    vec3_t mins = {-1.0f, -2.0f, -3.0f};
    vec3_t maxs = {1.0f, 2.0f, 3.0f};
    float radius = RadiusFromBounds(mins, maxs);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.741f, radius);  // sqrt(1^2 + 2^2 + 3^2) = sqrt(14) ≈ 3.741
}

void test_RadiusFromBounds_positive_bounds(void) {
    vec3_t mins = {0.0f, 0.0f, 0.0f};
    vec3_t maxs = {1.0f, 2.0f, 3.0f};
    float radius = RadiusFromBounds(mins, maxs);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.741f, radius);  // sqrt(3^2 + 2^2 + 1^2) = sqrt(14) ≈ 3.741
}

/*
 * ============================================================================
 * 2D Bounds Tests
 * ============================================================================
 */

void test_Clear2DBounds_sets_mins_to_max_float(void) {
    vec2_t mins, maxs;
    Clear2DBounds(mins, maxs);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 99999.0f, mins[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 99999.0f, mins[1]);
}

void test_Clear2DBounds_sets_maxs_to_min_float(void) {
    vec2_t mins, maxs;
    Clear2DBounds(mins, maxs);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -99999.0f, maxs[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -99999.0f, maxs[1]);
}

void test_AddPointTo2DBounds_expands_empty_bounds(void) {
    vec2_t mins, maxs;
    vec2_t point = {1.0f, 2.0f};
    Clear2DBounds(mins, maxs);
    AddPointTo2DBounds(point, mins, maxs);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, mins[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, mins[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, maxs[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, maxs[1]);
}

void test_AddPointTo2DBounds_updates_bounds(void) {
    vec2_t mins, maxs;
    vec2_t point1 = {0.0f, 0.0f};
    vec2_t point2 = {2.0f, 3.0f};
    vec2_t point3 = {-1.0f, -1.0f};
    Clear2DBounds(mins, maxs);
    AddPointTo2DBounds(point1, mins, maxs);
    AddPointTo2DBounds(point2, mins, maxs);
    AddPointTo2DBounds(point3, mins, maxs);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, mins[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, mins[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, maxs[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.0f, maxs[1]);
}

/*
 * ============================================================================
 * Test Runner
 * ============================================================================
 */

int main(void) {
    UNITY_BEGIN();

    /* ClearBounds */
    RUN_TEST(test_ClearBounds_sets_mins_to_max_float);
    RUN_TEST(test_ClearBounds_sets_maxs_to_min_float);

    /* AddPointToBounds */
    RUN_TEST(test_AddPointToBounds_expands_empty_bounds);
    RUN_TEST(test_AddPointToBounds_updates_mins);
    RUN_TEST(test_AddPointToBounds_updates_maxs);
    RUN_TEST(test_AddPointToBounds_negative_values);

    /* BoundsIntersect */
    RUN_TEST(test_BoundsIntersect_identical_bounds);
    RUN_TEST(test_BoundsIntersect_overlapping_bounds);
    RUN_TEST(test_BoundsIntersect_touching_bounds);
    RUN_TEST(test_BoundsIntersect_separate_bounds);
    RUN_TEST(test_BoundsIntersect_empty_bounds);

    /* BoundsAndSphereIntersect */
    RUN_TEST(test_BoundsAndSphereIntersect_sphere_completely_inside);
    RUN_TEST(test_BoundsAndSphereIntersect_sphere_outside);
    RUN_TEST(test_BoundsAndSphereIntersect_sphere_intersecting);
    RUN_TEST(test_BoundsAndSphereIntersect_sphere_touching);

    /* RadiusFromBounds */
    RUN_TEST(test_RadiusFromBounds_unit_cube);
    RUN_TEST(test_RadiusFromBounds_origin_bounds);
    RUN_TEST(test_RadiusFromBounds_positive_bounds);

    /* 2D Bounds */
    RUN_TEST(test_Clear2DBounds_sets_mins_to_max_float);
    RUN_TEST(test_Clear2DBounds_sets_maxs_to_min_float);
    RUN_TEST(test_AddPointTo2DBounds_expands_empty_bounds);
    RUN_TEST(test_AddPointTo2DBounds_updates_bounds);

    return UNITY_END();
}