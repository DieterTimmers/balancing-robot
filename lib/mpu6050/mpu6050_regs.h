#ifndef MPU6050_REGS_H
#define MPU6050_REGS_H

/* I2C device address (7-bit). AD0 low = 0x68, AD0 high = 0x69. */
#define MPU6050_I2C_ADDR_7BIT        0x68U

/* MPU-6050 register addresses (subset we actually use) */
#define MPU6050_REG_SMPLRT_DIV       0x19U
#define MPU6050_REG_CONFIG           0x1AU
#define MPU6050_REG_GYRO_CONFIG      0x1BU
#define MPU6050_REG_ACCEL_CONFIG     0x1CU
#define MPU6050_REG_ACCEL_XOUT_H     0x3BU   /* start of 14-byte burst */
#define MPU6050_REG_PWR_MGMT_1       0x6BU
#define MPU6050_REG_WHO_AM_I         0x75U

#define MPU6050_WHOAMI_EXPECTED      0x68U

/* PWR_MGMT_1 */
#define MPU6050_PWR1_DEVICE_RESET       (1U << 7)
#define MPU6050_PWR1_SLEEP              (1U << 6)
#define MPU6050_PWR1_CLKSEL_PLL_GYRO_X  0x01U  /* better stability than internal osc */

/* CONFIG (DLPF_CFG, bits 2:0) */
#define MPU6050_DLPF_44HZ            0x03U

/* GYRO_CONFIG FS_SEL << 3 */
#define MPU6050_GYRO_FS_250          (0U << 3)
#define MPU6050_GYRO_FS_500          (1U << 3)
#define MPU6050_GYRO_FS_1000         (2U << 3)
#define MPU6050_GYRO_FS_2000         (3U << 3)

/* ACCEL_CONFIG AFS_SEL << 3 */
#define MPU6050_ACCEL_FS_2G          (0U << 3)
#define MPU6050_ACCEL_FS_4G          (1U << 3)
#define MPU6050_ACCEL_FS_8G          (2U << 3)
#define MPU6050_ACCEL_FS_16G         (3U << 3)

/* LSB-per-unit scale factors, from the datasheet */
#define MPU6050_ACCEL_LSB_PER_G_2G   16384.0f
#define MPU6050_ACCEL_LSB_PER_G_4G    8192.0f
#define MPU6050_ACCEL_LSB_PER_G_8G    4096.0f
#define MPU6050_ACCEL_LSB_PER_G_16G   2048.0f

#define MPU6050_GYRO_LSB_PER_DPS_250   131.0f
#define MPU6050_GYRO_LSB_PER_DPS_500    65.5f
#define MPU6050_GYRO_LSB_PER_DPS_1000   32.8f
#define MPU6050_GYRO_LSB_PER_DPS_2000   16.4f

/* Sample rate divider: Rate = 1 kHz / (1 + div) when DLPF is enabled. */
#define MPU6050_SMPLRT_DIV_FOR_HZ(hz)   ((uint8_t)((1000U / (hz)) - 1U))

#endif /* MPU6050_REGS_H */
