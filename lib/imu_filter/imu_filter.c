#include "imu_filter.h"
#include <math.h>

#define RAD_TO_DEG (57.29577951308232f)

void imu_filter_init(imu_filter_t *f, float alpha) {
    f->alpha       = alpha;
    f->pitch_deg   = 0.0f;
    f->initialised = 0;
}

float imu_filter_update(imu_filter_t *f, float accel_num_g, float accel_den_g,
                        float gyro_rate_dps, float dt_s) {
    float accel_pitch = atan2f(accel_num_g, accel_den_g) * RAD_TO_DEG;
    if (!f->initialised) {
        f->pitch_deg   = accel_pitch;
        f->initialised = 1;
        return f->pitch_deg;
    }
    float gyro_pitch = f->pitch_deg + gyro_rate_dps * dt_s;
    f->pitch_deg = f->alpha * gyro_pitch + (1.0f - f->alpha) * accel_pitch;
    return f->pitch_deg;
}
