#include <unity.h>
#include <math.h>
#include "imu_filter.h"

void setUp(void) {}
void tearDown(void) {}

void test_init_sets_alpha_and_pitch(void) {
    imu_filter_t f;
    imu_filter_init(&f, 0.98f);
    TEST_ASSERT_EQUAL_FLOAT(0.98f, f.alpha);
    TEST_ASSERT_EQUAL_FLOAT(0.0f,  f.pitch_deg);
    TEST_ASSERT_FALSE(f.initialised);
}

void test_first_update_seeds_from_accel(void) {
    imu_filter_t f;
    imu_filter_init(&f, 0.98f);
    /* 45 deg tilt: ay = sin(45), az = cos(45) */
    float p = imu_filter_update(&f, 0.70710678f, 0.70710678f, 0.0f, 0.01f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 45.0f, p);
    TEST_ASSERT_TRUE(f.initialised);
}

void test_pure_gyro_integration_over_one_second(void) {
    imu_filter_t f;
    imu_filter_init(&f, 1.0f);           /* alpha = 1 -> ignore accel after seed */
    imu_filter_update(&f, 0.0f, 1.0f, 0.0f, 0.01f);   /* seed at 0 deg */
    for (int i = 0; i < 100; ++i) {
        imu_filter_update(&f, 0.0f, 1.0f, 90.0f, 0.01f); /* +90 deg/s for 1 s */
    }
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 90.0f, f.pitch_deg);
}

void test_accel_pulls_estimate_back(void) {
    imu_filter_t f;
    imu_filter_init(&f, 0.5f);           /* heavily accel-weighted */
    imu_filter_update(&f, 0.0f, 1.0f, 0.0f, 0.01f);  /* seed 0 deg */

    /* Sustained accel-implied 30 deg with zero gyro should converge to ~30 deg. */
    float ay = sinf(30.0f * 3.14159265f / 180.0f);
    float az = cosf(30.0f * 3.14159265f / 180.0f);
    for (int i = 0; i < 200; ++i) {
        imu_filter_update(&f, ay, az, 0.0f, 0.01f);
    }
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 30.0f, f.pitch_deg);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_init_sets_alpha_and_pitch);
    RUN_TEST(test_first_update_seeds_from_accel);
    RUN_TEST(test_pure_gyro_integration_over_one_second);
    RUN_TEST(test_accel_pulls_estimate_back);
    return UNITY_END();
}
