#include "i2c_stm32.h"

#define HAL_I2C_TIMEOUT_MS 100U

static int stm32_read(void *ctx, uint8_t addr, uint8_t reg,
                      uint8_t *buf, size_t len) {
    I2C_HandleTypeDef *h = (I2C_HandleTypeDef *)ctx;
    HAL_StatusTypeDef s = HAL_I2C_Mem_Read(
        h, (uint16_t)(addr << 1), reg, I2C_MEMADD_SIZE_8BIT,
        buf, (uint16_t)len, HAL_I2C_TIMEOUT_MS);
    return (s == HAL_OK) ? 0 : -1;
}

static int stm32_write(void *ctx, uint8_t addr, uint8_t reg,
                       const uint8_t *buf, size_t len) {
    I2C_HandleTypeDef *h = (I2C_HandleTypeDef *)ctx;
    HAL_StatusTypeDef s = HAL_I2C_Mem_Write(
        h, (uint16_t)(addr << 1), reg, I2C_MEMADD_SIZE_8BIT,
        (uint8_t *)buf, (uint16_t)len, HAL_I2C_TIMEOUT_MS);
    return (s == HAL_OK) ? 0 : -1;
}

static void stm32_delay_ms(uint32_t ms) { HAL_Delay(ms); }

mpu6050_bus_t i2c_stm32_bus(I2C_HandleTypeDef *hi2c) {
    return (mpu6050_bus_t){
        .read     = stm32_read,
        .write    = stm32_write,
        .delay_ms = stm32_delay_ms,
        .ctx      = hi2c,
    };
}
