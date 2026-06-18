#include "stm32f4xx_hal.h"
#include "core_cm4.h"
#include "app_config.h"
#include "mpu6050.h"
#include "mpu6050_regs.h"
#include "imu_filter.h"
#include "i2c_stm32.h"
#include <stdio.h>

static I2C_HandleTypeDef hi2c2;
static mpu6050_t         imu;
static imu_filter_t      filt;

static volatile uint32_t tick_flag  = 0;
static volatile uint32_t tick_count = 0;

static void SystemClock_Config(void);
static void Error_Loop(const char *msg);

/* Enable ITM stimulus port 0 so printf -> ITM doesn't dead-lock if the
 * debugger hasn't configured the TPIU yet (ITM_SendChar polls FIFO space). */
static void ITM_Enable(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    ITM->LAR = 0xC5ACCE55;
    ITM->TCR |= ITM_TCR_ITMENA_Msk;
    ITM->TER |= 1U;
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    ITM_Enable();

    hi2c2 = (I2C_HandleTypeDef){
        .Instance             = I2C2,
        .Init.ClockSpeed      = 400000,
        .Init.DutyCycle       = I2C_DUTYCYCLE_2,
        .Init.OwnAddress1     = 0,
        .Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT,
        .Init.DualAddressMode = I2C_DUALADDRESS_DISABLE,
        .Init.GeneralCallMode = I2C_GENERALCALL_DISABLE,
        .Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE,
    };
    if (HAL_I2C_Init(&hi2c2) != HAL_OK) Error_Loop("i2c init");

    mpu6050_bus_t bus = i2c_stm32_bus(&hi2c2);
    mpu6050_cfg_t cfg = {
        .dev_addr       = MPU6050_I2C_ADDR_7BIT,
        .accel_range    = MPU6050_ACCEL_2G,
        .gyro_range     = MPU6050_GYRO_500,
        .dlpf_cfg       = MPU6050_DLPF_44HZ,
        .sample_rate_hz = APP_SAMPLE_RATE_HZ,
    };
    if (mpu6050_init(&imu, &bus, &cfg) != MPU6050_OK) Error_Loop("mpu init");

    printf("mpu6050 init ok, calibrating gyro -- keep robot still...\n");
    if (mpu6050_calibrate_gyro(&imu, APP_GYRO_CALIB_SAMPLES) != MPU6050_OK)
        Error_Loop("gyro calib");
    printf("gyro bias: %.3f %.3f %.3f deg/s\n",
           (double)imu.gyro_bias_dps[0],
           (double)imu.gyro_bias_dps[1],
           (double)imu.gyro_bias_dps[2]);

    imu_filter_init(&filt, APP_FILTER_ALPHA);

    const float dt = 1.0f / (float)APP_SAMPLE_RATE_HZ;

    while (1) {
        if (!tick_flag) { continue; }
        tick_flag = 0;

        mpu6050_sample_t s;
        if (mpu6050_read_scaled(&imu, &s) != MPU6050_OK) continue;

        /* Pitch = atan2(ay, az), integrate gx. See docs/hardware-wiring.md
         * for the orientation convention. */
        float pitch = imu_filter_update(&filt, s.accel_g[1], s.accel_g[2],
                                        s.gyro_dps[0], dt);

        if ((tick_count % APP_TELEMETRY_DIVIDER) == 0) {
            printf("pitch=%+6.2f  a=[%+5.2f %+5.2f %+5.2f]g  "
                   "g=[%+7.2f %+7.2f %+7.2f]dps  T=%5.2fC\n",
                   (double)pitch,
                   (double)s.accel_g[0], (double)s.accel_g[1], (double)s.accel_g[2],
                   (double)s.gyro_dps[0], (double)s.gyro_dps[1], (double)s.gyro_dps[2],
                   (double)s.temp_c);
        }
    }
}

/* HAL_Init() configures SysTick at 1 kHz. Divide down to APP_SAMPLE_RATE_HZ. */
void HAL_SYSTICK_Callback(void) {
    static uint32_t prescale = 0;
    if (++prescale >= (1000U / APP_SAMPLE_RATE_HZ)) {
        prescale   = 0;
        tick_flag  = 1;
        tick_count++;
    }
}

/* Override the weak Default_Handler SysTick ISR: tick HAL, then fire
 * HAL_SYSTICK_IRQHandler which invokes HAL_SYSTICK_Callback above. */
void SysTick_Handler(void) {
    HAL_IncTick();
    HAL_SYSTICK_IRQHandler();
}

static void Error_Loop(const char *msg) {
    printf("FATAL: %s\n", msg);
    while (1) { HAL_Delay(500); }
}

/* 168 MHz from 8 MHz HSE via PLL. Standard DISC1 clock tree. */
static void SystemClock_Config(void) {
    RCC_OscInitTypeDef osc = {
        .OscillatorType = RCC_OSCILLATORTYPE_HSE,
        .HSEState       = RCC_HSE_ON,
        .PLL.PLLState   = RCC_PLL_ON,
        .PLL.PLLSource  = RCC_PLLSOURCE_HSE,
        .PLL.PLLM       = 8,
        .PLL.PLLN       = 336,
        .PLL.PLLP       = RCC_PLLP_DIV2,
        .PLL.PLLQ       = 7,
    };
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) Error_Loop("osc");

    RCC_ClkInitTypeDef clk = {
        .ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                        | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2,
        .SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK,
        .AHBCLKDivider  = RCC_SYSCLK_DIV1,
        .APB1CLKDivider = RCC_HCLK_DIV4,
        .APB2CLKDivider = RCC_HCLK_DIV2,
    };
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_5) != HAL_OK) Error_Loop("clk");
}
