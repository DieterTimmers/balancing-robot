#ifndef IMU_FILTER_H
#define IMU_FILTER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float alpha;         /* weight on gyro integration (0..1); 0.98 typical */
    float pitch_deg;     /* current angle estimate */
    int   initialised;   /* seeded from accel on first update */
} imu_filter_t;

void  imu_filter_init(imu_filter_t *f, float alpha);

/* accel_num/den in g, gyro_rate in deg/s, dt in seconds.
 * pitch = atan2(accel_num, accel_den).
 * Returns the new pitch estimate. */
float imu_filter_update(imu_filter_t *f, float accel_num_g, float accel_den_g,
                        float gyro_rate_dps, float dt_s);

#ifdef __cplusplus
}
#endif
#endif /* IMU_FILTER_H */
