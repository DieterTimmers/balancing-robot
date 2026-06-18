#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* ---- MPU-6050 ------------------------------------------------------- */
/* Sensor ranges (encoded register values live in mpu6050_regs.h) */
#define APP_ACCEL_RANGE_G            2       /* +/-2 g  */
#define APP_GYRO_RANGE_DPS           500     /* +/-500 deg/s */
#define APP_DLPF_HZ                  44      /* digital low-pass filter cutoff */

/* ---- Timing --------------------------------------------------------- */
#define APP_SAMPLE_RATE_HZ           100U
#define APP_SAMPLE_PERIOD_MS         (1000U / APP_SAMPLE_RATE_HZ)
#define APP_TELEMETRY_DIVIDER        10U     /* print every Nth sample (10 Hz) */

/* ---- Calibration ---------------------------------------------------- */
#define APP_GYRO_CALIB_SAMPLES       1000U

/* ---- Complementary filter ------------------------------------------ */
#define APP_FILTER_ALPHA             0.98f

/* Orientation: pitch = atan2(accel_y, accel_z), integrated from gyro X.
 * If the IMU is mounted differently, change the indices in main.c. */

#endif /* APP_CONFIG_H */
