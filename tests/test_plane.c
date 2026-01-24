/*
 * test_plane.c - Unit tests for shared/m_plane.c
 *
 * These tests verify the plane math functions.
 * Run with: make test_plane && ./test_plane
 */

#include "../unity/unity.h"

/* Include the actual implementation */
#include "../shared/shared.h"

/*
 * ============================================================================
 * PlaneFromPoints Tests
 * ============================================================================
 */

void test_PlaneFromPoints_basic_triangle(void) {
    vec3_t verts[3] = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f}
    };
    cBspPlane_t plane;
    PlaneFromPoints(verts, &plane);
    // Normal should be (0,1,0) for this triangle
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, plane.normal[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, plane.normal[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, plane.normal[2]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, plane.dist);
}

void test_PlaneFromPoints_slanted_plane(void) {
    vec3_t verts[3] = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 0.0f}
    };
    cBspPlane_t plane;
    PlaneFromPoints(verts, &plane);
    // Normal should be normalized
    float len = Vec3Length(plane.normal);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, len);
    // Dist should be 0 since origin is on plane
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, plane.dist);
}

void test_PlaneFromPoints_normal_direction(void) {
    vec3_t verts[3] = {
        {0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {1.0f, 0.0f, 0.0f}
    };
    cBspPlane_t plane;
    PlaneFromPoints(verts, &plane);
    // Normal should be (0,0,1) for this winding
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, plane.normal[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, plane.normal[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, plane.normal[2]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, plane.dist);
}

void test_PlaneFromPoints_colinear_points(void) {
    vec3_t verts[3] = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 1.0f},
        {2.0f, 2.0f, 2.0f}
    };
    cBspPlane_t plane;
    PlaneFromPoints(verts, &plane);
    // Colinear points should result in zero normal (degenerate)
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, plane.normal[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, plane.normal[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, plane.normal[2]);
}

/*
 * ============================================================================
 * BoxOnPlaneSide Tests
 * ============================================================================
 */

void test_BoxOnPlaneSide_box_in_front(void) {
    vec3_t mins = {0.0f, 0.0f, 1.0f};
    vec3_t maxs = {1.0f, 1.0f, 2.0f};
    cBspPlane_t plane = {{0.0f, 0.0f, 1.0f}, 0.5f, 2, 0}; // Z plane at 0.5, axial
    int side = BoxOnPlaneSide(mins, maxs, &plane);
    TEST_ASSERT_EQUAL(1, side); // In front
}

void test_BoxOnPlaneSide_box_behind(void) {
    vec3_t mins = {0.0f, 0.0f, -1.0f};
    vec3_t maxs = {1.0f, 1.0f, -0.5f};
    cBspPlane_t plane = {{0.0f, 0.0f, 1.0f}, 0.0f, 2, 0}; // Z plane at 0
    int side = BoxOnPlaneSide(mins, maxs, &plane);
    TEST_ASSERT_EQUAL(2, side); // Behind
}

void test_BoxOnPlaneSide_box_crossing(void) {
    vec3_t mins = {0.0f, 0.0f, -0.5f};
    vec3_t maxs = {1.0f, 1.0f, 0.5f};
    cBspPlane_t plane = {{0.0f, 0.0f, 1.0f}, 0.0f, 2, 0}; // Z plane at 0
    int side = BoxOnPlaneSide(mins, maxs, &plane);
    TEST_ASSERT_EQUAL(3, side); // Crossing (1 + 2)
}

void test_BoxOnPlaneSide_axial_plane_fast_path(void) {
    vec3_t mins = {0.0f, 0.0f, 0.0f};
    vec3_t maxs = {1.0f, 1.0f, 1.0f};
    cBspPlane_t plane = {{0.0f, 0.0f, 1.0f}, 0.5f, 2, 0}; // Axial Z plane
    int side = BoxOnPlaneSide(mins, maxs, &plane);
    TEST_ASSERT_EQUAL(3, side); // Crossing
}

void test_BoxOnPlaneSide_non_axial_plane(void) {
    vec3_t mins = {0.0f, 0.0f, 0.0f};
    vec3_t maxs = {1.0f, 1.0f, 1.0f};
    cBspPlane_t plane = {{1.0f, 0.0f, 0.0f}, 0.5f, 3, 0}; // Non-axial X plane, type 3
    int side = BoxOnPlaneSide(mins, maxs, &plane);
    TEST_ASSERT_EQUAL(3, side); // Should be crossing
}

/*
 * ============================================================================
 * Test Runner
 * ============================================================================
 */

int main(void) {
    UNITY_BEGIN();

    /* PlaneFromPoints */
    RUN_TEST(test_PlaneFromPoints_basic_triangle);
    RUN_TEST(test_PlaneFromPoints_slanted_plane);
    RUN_TEST(test_PlaneFromPoints_normal_direction);
    RUN_TEST(test_PlaneFromPoints_colinear_points);

    /* BoxOnPlaneSide */
    RUN_TEST(test_BoxOnPlaneSide_box_in_front);
    RUN_TEST(test_BoxOnPlaneSide_box_behind);
    RUN_TEST(test_BoxOnPlaneSide_box_crossing);
    RUN_TEST(test_BoxOnPlaneSide_axial_plane_fast_path);
    RUN_TEST(test_BoxOnPlaneSide_non_axial_plane);

    return UNITY_END();
}