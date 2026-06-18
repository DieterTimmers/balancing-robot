#ifndef I2C_STM32_H
#define I2C_STM32_H

#include "stm32f4xx_hal.h"
#include "mpu6050.h"

/* Build an mpu6050_bus_t that uses the given HAL I2C handle.
 * The handle must already be initialised by the caller. */
mpu6050_bus_t i2c_stm32_bus(I2C_HandleTypeDef *hi2c);

#endif /* I2C_STM32_H */
