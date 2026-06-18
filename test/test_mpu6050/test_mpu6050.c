#include <unity.h>
#include "mpu6050.h"
#include "mpu6050_regs.h"
#include "mock_bus.h"

static mock_bus_t mock;
static mpu6050_t  dev;

static mpu6050_cfg_t default_cfg(void) {
    return (mpu6050_cfg_t){
        .dev_addr       = MPU6050_I2C_ADDR_7BIT,
        .accel_range    = MPU6050_ACCEL_2G,
        .gyro_range     = MPU6050_GYRO_500,
        .dlpf_cfg       = MPU6050_DLPF_44HZ,
        .sample_rate_hz = 100,
    };
}

void setUp(void) {
    mock_bus_reset(&mock);
    mock.regs[MPU6050_REG_WHO_AM_I] = MPU6050_WHOAMI_EXPECTED;
    dev = (mpu6050_t){
        .bus      = mock_bus_interface(&mock),
        .dev_addr = MPU6050_I2C_ADDR_7BIT,
    };
}
void tearDown(void) {}

/* ---- who_am_i ------------------------------------------------------- */

void test_who_am_i_returns_expected_value(void) {
    uint8_t id = 0;
    TEST_ASSERT_EQUAL(MPU6050_OK, mpu6050_who_am_i(&dev, &id));
    TEST_ASSERT_EQUAL_HEX8(0x68, id);
}

void test_who_am_i_propagates_bus_error(void) {
    uint8_t id = 0;
    mock.fail_next_read = 1;
    TEST_ASSERT_EQUAL(MPU6050_ERR_BUS, mpu6050_who_am_i(&dev, &id));
}

/* ---- init ----------------------------------------------------------- */

void test_init_sets_scales_for_2g_500dps(void) {
    mpu6050_bus_t bus = mock_bus_interface(&mock);
    mock.regs[MPU6050_REG_WHO_AM_I] = MPU6050_WHOAMI_EXPECTED;
    mpu6050_cfg_t cfg = default_cfg();
    TEST_ASSERT_EQUAL(MPU6050_OK, mpu6050_init(&dev, &bus, &cfg));
    TEST_ASSERT_EQUAL_FLOAT(16384.0f, dev.accel_lsb_per_g);
    TEST_ASSERT_EQUAL_FLOAT(65.5f,    dev.gyro_lsb_per_dps);
}

void test_init_writes_expected_registers(void) {
    /* mpu6050_init zeroes the device struct, which wipes dev.bus. We need
     * the mock bus to survive init, so keep it in a local cfg and pass it. */
    mpu6050_bus_t bus = mock_bus_interface(&mock);
    mock.regs[MPU6050_REG_WHO_AM_I] = MPU6050_WHOAMI_EXPECTED;

    mpu6050_cfg_t cfg = default_cfg();
    TEST_ASSERT_EQUAL(MPU6050_OK, mpu6050_init(&dev, &bus, &cfg));

    TEST_ASSERT_EQUAL_HEX8(MPU6050_PWR1_CLKSEL_PLL_GYRO_X,
                           mock.regs[MPU6050_REG_PWR_MGMT_1]);
    TEST_ASSERT_EQUAL_HEX8(MPU6050_DLPF_44HZ,
                           mock.regs[MPU6050_REG_CONFIG]);
    TEST_ASSERT_EQUAL_HEX8(MPU6050_ACCEL_FS_2G,
                           mock.regs[MPU6050_REG_ACCEL_CONFIG]);
    TEST_ASSERT_EQUAL_HEX8(MPU6050_GYRO_FS_500,
                           mock.regs[MPU6050_REG_GYRO_CONFIG]);
    TEST_ASSERT_EQUAL_HEX8(9,   /* 1000/100 - 1 */
                           mock.regs[MPU6050_REG_SMPLRT_DIV]);
}

void test_init_rejects_wrong_whoami(void) {
    mpu6050_bus_t bus = mock_bus_interface(&mock);
    mock.regs[MPU6050_REG_WHO_AM_I] = 0x42;
    mpu6050_cfg_t cfg = default_cfg();
    TEST_ASSERT_EQUAL(MPU6050_ERR_WHOAMI, mpu6050_init(&dev, &bus, &cfg));
}

void test_init_rejects_null_args(void) {
    mpu6050_bus_t bus = mock_bus_interface(&mock);
    mpu6050_cfg_t cfg = default_cfg();
    TEST_ASSERT_EQUAL(MPU6050_ERR_ARG, mpu6050_init(NULL, &bus,  &cfg));
    TEST_ASSERT_EQUAL(MPU6050_ERR_ARG, mpu6050_init(&dev, NULL, &cfg));
    TEST_ASSERT_EQUAL(MPU6050_ERR_ARG, mpu6050_init(&dev, &bus,  NULL));
}

/* ---- read_raw ------------------------------------------------------- */

void test_read_raw_burst_decodes_big_endian(void) {
    uint8_t *r = &mock.regs[MPU6050_REG_ACCEL_XOUT_H];
    uint16_t values[7] = { 0x0100, 0x0200, 0x0300, 0x0400,
                           0x0500, 0x0600, 0x0700 };
    for (int i = 0; i < 7; ++i) {
        r[i*2]   = values[i] >> 8;
        r[i*2+1] = values[i] & 0xFF;
    }

    mpu6050_sample_raw_t s;
    TEST_ASSERT_EQUAL(MPU6050_OK, mpu6050_read_raw(&dev, &s));
    TEST_ASSERT_EQUAL_INT16(0x0100, s.accel[0]);
    TEST_ASSERT_EQUAL_INT16(0x0200, s.accel[1]);
    TEST_ASSERT_EQUAL_INT16(0x0300, s.accel[2]);
    TEST_ASSERT_EQUAL_INT16(0x0400, s.temp);
    TEST_ASSERT_EQUAL_INT16(0x0500, s.gyro[0]);
    TEST_ASSERT_EQUAL_INT16(0x0600, s.gyro[1]);
    TEST_ASSERT_EQUAL_INT16(0x0700, s.gyro[2]);
}

void test_read_raw_propagates_bus_error(void) {
    mpu6050_sample_raw_t s;
    mock.fail_next_read = 1;
    TEST_ASSERT_EQUAL(MPU6050_ERR_BUS, mpu6050_read_raw(&dev, &s));
}

/* ---- read_scaled ---------------------------------------------------- */

static void set_raw_sample_in_mock(int16_t ax, int16_t ay, int16_t az,
                                   int16_t temp, int16_t gx, int16_t gy, int16_t gz) {
    uint8_t *r = &mock.regs[MPU6050_REG_ACCEL_XOUT_H];
    int16_t vals[7] = { ax, ay, az, temp, gx, gy, gz };
    for (int i = 0; i < 7; ++i) {
        r[i*2]   = ((uint16_t)vals[i] >> 8) & 0xFF;
        r[i*2+1] = (uint16_t)vals[i] & 0xFF;
    }
}

void test_read_scaled_applies_scales(void) {
    mpu6050_bus_t bus = mock_bus_interface(&mock);
    mock.regs[MPU6050_REG_WHO_AM_I] = MPU6050_WHOAMI_EXPECTED;
    mpu6050_cfg_t cfg = default_cfg();
    TEST_ASSERT_EQUAL(MPU6050_OK, mpu6050_init(&dev, &bus, &cfg));

    /* 1 g on Z (=16384 at +/-2g), 100 deg/s on X (=6550 at +/-500), temp 0 */
    set_raw_sample_in_mock(0, 0, 16384, 0, 6550, 0, 0);

    mpu6050_sample_t s;
    TEST_ASSERT_EQUAL(MPU6050_OK, mpu6050_read_scaled(&dev, &s));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f,   s.accel_g[2]);
    TEST_ASSERT_FLOAT_WITHIN(0.1f,   100.0f, s.gyro_dps[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.1f,   36.53f, s.temp_c);
}

void test_read_scaled_subtracts_gyro_bias(void) {
    mpu6050_bus_t bus = mock_bus_interface(&mock);
    mock.regs[MPU6050_REG_WHO_AM_I] = MPU6050_WHOAMI_EXPECTED;
    mpu6050_cfg_t cfg = default_cfg();
    TEST_ASSERT_EQUAL(MPU6050_OK, mpu6050_init(&dev, &bus, &cfg));

    dev.gyro_bias_dps[0] = 10.0f;
    set_raw_sample_in_mock(0, 0, 0, 0, 6550, 0, 0);  /* 100 deg/s raw */

    mpu6050_sample_t s;
    TEST_ASSERT_EQUAL(MPU6050_OK, mpu6050_read_scaled(&dev, &s));
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 90.0f, s.gyro_dps[0]);
}

/* ---- calibrate_gyro ------------------------------------------------- */

void test_calibrate_gyro_averages_raw_reads(void) {
    mpu6050_bus_t bus = mock_bus_interface(&mock);
    mock.regs[MPU6050_REG_WHO_AM_I] = MPU6050_WHOAMI_EXPECTED;
    mpu6050_cfg_t cfg = default_cfg();
    TEST_ASSERT_EQUAL(MPU6050_OK, mpu6050_init(&dev, &bus, &cfg));

    /* raw_gx = 655 -> 10 deg/s at +/-500 */
    set_raw_sample_in_mock(0, 0, 0, 0, 655, 0, 0);

    TEST_ASSERT_EQUAL(MPU6050_OK, mpu6050_calibrate_gyro(&dev, 100));
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 10.0f, dev.gyro_bias_dps[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f,  dev.gyro_bias_dps[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f,  dev.gyro_bias_dps[2]);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_who_am_i_returns_expected_value);
    RUN_TEST(test_who_am_i_propagates_bus_error);
    RUN_TEST(test_init_sets_scales_for_2g_500dps);
    RUN_TEST(test_init_writes_expected_registers);
    RUN_TEST(test_init_rejects_wrong_whoami);
    RUN_TEST(test_init_rejects_null_args);
    RUN_TEST(test_read_raw_burst_decodes_big_endian);
    RUN_TEST(test_read_raw_propagates_bus_error);
    RUN_TEST(test_read_scaled_applies_scales);
    RUN_TEST(test_read_scaled_subtracts_gyro_bias);
    RUN_TEST(test_calibrate_gyro_averages_raw_reads);
    return UNITY_END();
}
