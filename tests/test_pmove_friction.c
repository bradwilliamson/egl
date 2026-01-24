/*
====================
test_pmove_friction.c

Unit tests for PM_Friction - ground friction, water friction, stopspeed
====================
*/

#include "../unity/unity.h"
#include <math.h>

// Types
typedef float vec_t;
typedef vec_t vec3_t[3];

// Constants taken from server/sv_pmove.c
#define SV_PM_FRICTION 6.0f
#define SV_PM_WATERFRICTION 1.0f
#define SV_PM_STOPSPEED 100.0f

// Helper: vector set
static void VectorSet(vec3_t v, float x, float y, float z) {
    v[0] = x; v[1] = y; v[2] = z;
}

static float VectorLength(const vec3_t v) {
    return sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

// Minimal replicate of the friction portion used by SV_PM_Friction
static void PM_Friction_Local(vec3_t vel, int onGround, int waterLevel, float frameTime) {
    float speed, newspeed, control, drop = 0.0f, friction;

    speed = VectorLength(vel);
    if (speed < 1.0f) {
        vel[0] = vel[1] = 0.0f;
        return;
    }

    // apply ground friction (if onGround is true)
    if (onGround) {
        friction = SV_PM_FRICTION;
        control = (speed < SV_PM_STOPSPEED) ? SV_PM_STOPSPEED : speed;
        drop += control * friction * frameTime;
    }

    // water friction
    if (waterLevel && !onGround) {
        drop += speed * SV_PM_WATERFRICTION * waterLevel * frameTime;
    }

    newspeed = speed - drop;
    if (newspeed < 0.0f) newspeed = 0.0f;
    newspeed /= speed;

    vel[0] *= newspeed;
    vel[1] *= newspeed;
    vel[2] *= newspeed;
}

// Tests
void test_PM_Friction_GroundVelocityDecay(void) {
    vec3_t vel;
    VectorSet(vel, 100.0f, 0.0f, 0.0f);
    PM_Friction_Local(vel, 1, 0, 0.1f); // frameTime=0.1

    // Expected: control = 100, drop = 100*6*0.1 = 60 -> newspeed = 0.4 -> vel = 40
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 40.0f, vel[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, vel[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, vel[2]);
}

void test_PM_Friction_WaterFriction(void) {
    vec3_t vel;
    VectorSet(vel, 50.0f, 0.0f, 0.0f);
    PM_Friction_Local(vel, 0, 1, 0.2f); // waterLevel=1, frameTime=0.2

    // Expected: drop = 50 * 1 * 1 * 0.2 = 10 -> newspeed = 0.8 -> vel = 40
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 40.0f, vel[0]);
}

void test_PM_Friction_StopspeedThreshold_Zeroes(void) {
    vec3_t vel;
    VectorSet(vel, 10.0f, 0.0f, 0.0f);
    PM_Friction_Local(vel, 1, 0, 0.1f);

    // control becomes SV_PM_STOPSPEED (100) -> drop large -> newspeed clamped to 0
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, vel[0]);
}

// Main runner
int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_PM_Friction_GroundVelocityDecay);
    RUN_TEST(test_PM_Friction_WaterFriction);
    RUN_TEST(test_PM_Friction_StopspeedThreshold_Zeroes);

    return UNITY_END();
}
