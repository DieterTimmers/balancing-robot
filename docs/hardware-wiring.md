# Hardware Wiring

## Boards

| Component | Part | Notes |
|---|---|---|
| MCU board | STM32F407G-DISC1 | Onboard ST-Link v2-A, 168 MHz Cortex-M4F |
| IMU breakout | MPU-6050 (GY-521) | 3-axis accel + 3-axis gyro, I²C only |

## I²C2 wiring (MPU-6050 ↔ F407-DISC1)

| MPU-6050 pin | STM32 pin | Note |
|---|---|---|
| VCC | 3V  | 3.3 V from DISC1 — **not** the 5V pin |
| GND | GND | |
| SCL | PB10 | I2C2_SCL, AF4, needs external 4.7 kΩ pull-up to 3V3 if breakout lacks one |
| SDA | PB11 | I2C2_SDA, AF4, same pull-up rule |
| XDA | — | leave floating (auxiliary I²C master, unused) |
| XCL | — | leave floating |
| AD0 | GND | selects 7-bit I²C address 0x68 (default) |
| INT | — | unused in this milestone |

Most GY-521 breakouts already carry 10 kΩ pull-ups on SCL/SDA — no external resistors needed. Verify with a multimeter between SCL↔3V3 and SDA↔3V3 (expect ~10 kΩ).

## Power

- The MPU-6050 runs at 3.3 V. The GY-521 breakout has an on-board LDO that will also accept 5 V on VCC, but powering it from the DISC1's 3V3 rail is simpler and avoids a level mismatch on the logic lines.
- Current draw is < 4 mA — the DISC1 3V3 LDO handles it easily.

## Orientation convention (for the complementary filter)

Mount the MPU-6050 flat on the robot chassis so that:

- **X axis** → points along the wheel axle (roll axis)
- **Y axis** → points in the direction of travel
- **Z axis** → points up when the robot is upright

Pitch (the angle the robot tips forward/backward) is then:

    pitch = atan2(accel_y, accel_z)

and is integrated from gyro rate on the X axis.

If your physical mounting differs, adjust the axis selection in `main.c` accordingly (see the `imu_filter_update` call and the pitch axis macros in `include/app_config.h`).

## Debug / telemetry

- USB mini cable → CN1 (ST-Link) on the DISC1. This is what powers the board and provides SWO.
- No UART / USB-TTL adapter required for this milestone.

## Why I2C2 and not I2C1?

The DISC1 has an onboard CS43L22 audio codec hard-wired to I2C1 on PB6 (SCL) / PB9 (SDA). Using I2C2 on PB10/PB11 keeps the IMU bus isolated from the codec and avoids accidental contention during development.
