#ifndef MPU6050_H
#define MPU6050_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Errors --------------------------------------------------------- */
typedef enum {
    MPU6050_OK            =  0,
    MPU6050_ERR_BUS       = -1,  /* underlying I2C read/write failed */
    MPU6050_ERR_WHOAMI    = -2,  /* sensor ID mismatch */
    MPU6050_ERR_ARG       = -3,  /* invalid argument */
} mpu6050_err_t;

/* ---- Injected bus interface ---------------------------------------- *
 * Return 0 on success, non-zero on failure. The driver treats any
 * non-zero value as MPU6050_ERR_BUS.
 */
typedef struct {
    int  (*read)(void *ctx, uint8_t dev_addr, uint8_t reg,
                 uint8_t *buf, size_t len);
    int  (*write)(void *ctx, uint8_t dev_addr, uint8_t reg,
                  const uint8_t *buf, size_t len);
    void (*delay_ms)(uint32_t ms);
    void *ctx;
} mpu6050_bus_t;

/* ---- Config & state ------------------------------------------------- */
typedef enum {
    MPU6050_ACCEL_2G, MPU6050_ACCEL_4G, MPU6050_ACCEL_8G, MPU6050_ACCEL_16G,
} mpu6050_accel_range_t;

typedef enum {
    MPU6050_GYRO_250, MPU6050_GYRO_500, MPU6050_GYRO_1000, MPU6050_GYRO_2000,
} mpu6050_gyro_range_t;

typedef struct {
    uint8_t                dev_addr;      /* 7-bit, usually 0x68 */
    mpu6050_accel_range_t  accel_range;
    mpu6050_gyro_range_t   gyro_range;
    uint8_t                dlpf_cfg;      /* 0..6, see datasheet */
    uint16_t               sample_rate_hz;
} mpu6050_cfg_t;

typedef struct {
    mpu6050_bus_t bus;
    uint8_t       dev_addr;
    float         accel_lsb_per_g;
    float         gyro_lsb_per_dps;
    float         gyro_bias_dps[3];       /* populated by mpu6050_calibrate_gyro */
} mpu6050_t;

/* ---- Samples -------------------------------------------------------- */
typedef struct {
    int16_t accel[3];  /* x, y, z */
    int16_t temp;
    int16_t gyro[3];
} mpu6050_sample_raw_t;

typedef struct {
    float accel_g[3];   /* g */
    float temp_c;       /* degC */
    float gyro_dps[3];  /* deg/s, bias-corrected */
} mpu6050_sample_t;

/* ---- API ------------------------------------------------------------ */
mpu6050_err_t mpu6050_init(mpu6050_t *dev, const mpu6050_bus_t *bus,
                           const mpu6050_cfg_t *cfg);

mpu6050_err_t mpu6050_who_am_i(mpu6050_t *dev, uint8_t *out);

mpu6050_err_t mpu6050_read_raw(mpu6050_t *dev, mpu6050_sample_raw_t *out);

mpu6050_err_t mpu6050_read_scaled(mpu6050_t *dev, mpu6050_sample_t *out);

/* Call with the robot held still. Averages n_samples gyro reads into
 * dev->gyro_bias_dps. Subsequent mpu6050_read_scaled calls subtract bias. */
mpu6050_err_t mpu6050_calibrate_gyro(mpu6050_t *dev, uint16_t n_samples);

#ifdef __cplusplus
}
#endif
#endif /* MPU6050_H */
