#include "mpu6050.h"
#include "mpu6050_regs.h"
#include <string.h>

static const float accel_scales[] = {
    MPU6050_ACCEL_LSB_PER_G_2G,  MPU6050_ACCEL_LSB_PER_G_4G,
    MPU6050_ACCEL_LSB_PER_G_8G,  MPU6050_ACCEL_LSB_PER_G_16G,
};
static const float gyro_scales[] = {
    MPU6050_GYRO_LSB_PER_DPS_250,  MPU6050_GYRO_LSB_PER_DPS_500,
    MPU6050_GYRO_LSB_PER_DPS_1000, MPU6050_GYRO_LSB_PER_DPS_2000,
};
static const uint8_t accel_fs_bits[] = {
    MPU6050_ACCEL_FS_2G,  MPU6050_ACCEL_FS_4G,
    MPU6050_ACCEL_FS_8G,  MPU6050_ACCEL_FS_16G,
};
static const uint8_t gyro_fs_bits[] = {
    MPU6050_GYRO_FS_250,  MPU6050_GYRO_FS_500,
    MPU6050_GYRO_FS_1000, MPU6050_GYRO_FS_2000,
};

static int bus_w1(mpu6050_t *d, uint8_t reg, uint8_t val) {
    return d->bus.write(d->bus.ctx, d->dev_addr, reg, &val, 1);
}

static int16_t be16(const uint8_t *p) {
    return (int16_t)(((uint16_t)p[0] << 8) | p[1]);
}

mpu6050_err_t mpu6050_who_am_i(mpu6050_t *dev, uint8_t *out) {
    if (!dev || !out) return MPU6050_ERR_ARG;
    if (dev->bus.read(dev->bus.ctx, dev->dev_addr,
                      MPU6050_REG_WHO_AM_I, out, 1) != 0) {
        return MPU6050_ERR_BUS;
    }
    return MPU6050_OK;
}

mpu6050_err_t mpu6050_init(mpu6050_t *dev, const mpu6050_bus_t *bus,
                           const mpu6050_cfg_t *cfg) {
    if (!dev || !bus || !cfg) return MPU6050_ERR_ARG;
    if (cfg->sample_rate_hz == 0 || cfg->sample_rate_hz > 1000)
        return MPU6050_ERR_ARG;

    memset(dev, 0, sizeof(*dev));
    dev->bus              = *bus;
    dev->dev_addr         = cfg->dev_addr;
    dev->accel_lsb_per_g  = accel_scales[cfg->accel_range];
    dev->gyro_lsb_per_dps = gyro_scales[cfg->gyro_range];

    uint8_t id = 0;
    mpu6050_err_t rc = mpu6050_who_am_i(dev, &id);
    if (rc != MPU6050_OK) return rc;
    if (id != MPU6050_WHOAMI_EXPECTED) return MPU6050_ERR_WHOAMI;

    if (bus_w1(dev, MPU6050_REG_PWR_MGMT_1, MPU6050_PWR1_CLKSEL_PLL_GYRO_X)) return MPU6050_ERR_BUS;
    dev->bus.delay_ms(10);
    if (bus_w1(dev, MPU6050_REG_CONFIG, cfg->dlpf_cfg))                         return MPU6050_ERR_BUS;
    if (bus_w1(dev, MPU6050_REG_GYRO_CONFIG, gyro_fs_bits[cfg->gyro_range]))    return MPU6050_ERR_BUS;
    if (bus_w1(dev, MPU6050_REG_ACCEL_CONFIG, accel_fs_bits[cfg->accel_range])) return MPU6050_ERR_BUS;
    if (bus_w1(dev, MPU6050_REG_SMPLRT_DIV,
               MPU6050_SMPLRT_DIV_FOR_HZ(cfg->sample_rate_hz)))                 return MPU6050_ERR_BUS;

    return MPU6050_OK;
}

mpu6050_err_t mpu6050_read_raw(mpu6050_t *dev, mpu6050_sample_raw_t *out) {
    if (!dev || !out) return MPU6050_ERR_ARG;
    uint8_t buf[14];
    if (dev->bus.read(dev->bus.ctx, dev->dev_addr,
                      MPU6050_REG_ACCEL_XOUT_H, buf, sizeof(buf)) != 0) {
        return MPU6050_ERR_BUS;
    }
    out->accel[0] = be16(&buf[0]);
    out->accel[1] = be16(&buf[2]);
    out->accel[2] = be16(&buf[4]);
    out->temp     = be16(&buf[6]);
    out->gyro[0]  = be16(&buf[8]);
    out->gyro[1]  = be16(&buf[10]);
    out->gyro[2]  = be16(&buf[12]);
    return MPU6050_OK;
}

mpu6050_err_t mpu6050_read_scaled(mpu6050_t *dev, mpu6050_sample_t *out) {
    if (!dev || !out) return MPU6050_ERR_ARG;
    mpu6050_sample_raw_t raw;
    mpu6050_err_t rc = mpu6050_read_raw(dev, &raw);
    if (rc != MPU6050_OK) return rc;

    for (int i = 0; i < 3; ++i) {
        out->accel_g[i]  = (float)raw.accel[i] / dev->accel_lsb_per_g;
        out->gyro_dps[i] = (float)raw.gyro[i]  / dev->gyro_lsb_per_dps
                           - dev->gyro_bias_dps[i];
    }
    /* Datasheet: T_degC = raw / 340 + 36.53 */
    out->temp_c = (float)raw.temp / 340.0f + 36.53f;
    return MPU6050_OK;
}

mpu6050_err_t mpu6050_calibrate_gyro(mpu6050_t *dev, uint16_t n_samples) {
    if (!dev || n_samples == 0) return MPU6050_ERR_ARG;
    /* Save and zero current bias so we don't double-subtract during averaging. */
    float saved[3] = { dev->gyro_bias_dps[0],
                       dev->gyro_bias_dps[1],
                       dev->gyro_bias_dps[2] };
    dev->gyro_bias_dps[0] = 0.0f;
    dev->gyro_bias_dps[1] = 0.0f;
    dev->gyro_bias_dps[2] = 0.0f;

    float acc[3] = { 0.0f, 0.0f, 0.0f };
    for (uint16_t i = 0; i < n_samples; ++i) {
        mpu6050_sample_t s;
        mpu6050_err_t rc = mpu6050_read_scaled(dev, &s);
        if (rc != MPU6050_OK) {
            dev->gyro_bias_dps[0] = saved[0];
            dev->gyro_bias_dps[1] = saved[1];
            dev->gyro_bias_dps[2] = saved[2];
            return rc;
        }
        acc[0] += s.gyro_dps[0];
        acc[1] += s.gyro_dps[1];
        acc[2] += s.gyro_dps[2];
        dev->bus.delay_ms(2);
    }
    dev->gyro_bias_dps[0] = acc[0] / (float)n_samples;
    dev->gyro_bias_dps[1] = acc[1] / (float)n_samples;
    dev->gyro_bias_dps[2] = acc[2] / (float)n_samples;
    return MPU6050_OK;
}
