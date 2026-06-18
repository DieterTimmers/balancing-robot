# balancing-robot

DIY self-balancing robot firmware. Target: STM32F407G-DISC1 + MPU-6050.

**Status:** Milestone 1 — IMU read-out & complementary filter (pre-PID, pre-motor).

## What this repo does today

- Initialises I²C2 to the MPU-6050 at 400 kHz
- Configures the sensor: accel ±2g, gyro ±500°/s, DLPF 44 Hz, 100 Hz sample rate
- Calibrates gyro bias at boot (1000-sample average)
- Reads accel + gyro every 10 ms from a SysTick callback
- Fuses to a pitch angle with a complementary filter (α = 0.98)
- Streams `pitch, ax, ay, az, gx, gy, gz` over SWO/ITM at 10 Hz

## Hardware

See [docs/hardware-wiring.md](docs/hardware-wiring.md).

## Prerequisites

- [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/index.html) — install with `pipx install platformio` or `pip install --user platformio`
- `arm-none-eabi-gcc` toolchain — PlatformIO installs this automatically on first build
- `stlink-tools` or OpenOCD for viewing SWO output (Linux: `apt install stlink-tools`)

## Build & flash

```
pio run                   # build firmware
pio run -t upload         # flash over ST-Link
pio run -t clean          # clean build artefacts
```

## Viewing SWO / `printf` output

With the board plugged in over the mini-USB port (CN1, ST-Link side):

Option A — OpenOCD (shipped inside the PlatformIO toolchain):

```
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
        -c "tpiu config internal :3344 uart off 168000000" \
        -c "itm port 0 on" -c "reset run"
```

Then in a second terminal:

```
nc localhost 3344
```

Option B — `st-trace` from `stlink-tools`:

```
st-trace -c 168000000 -b 2000000
```

## Unit tests (host, no hardware required)

```
pio test -e native
```

Tests cover the portable `mpu6050` driver (mocked I²C) and the `imu_filter` math.

## Project layout

- `src/` — STM32-specific entry point and HAL glue
- `lib/mpu6050/` — portable sensor driver (no HAL includes)
- `lib/imu_filter/` — portable complementary filter
- `lib/i2c_stm32/` — STM32 HAL adapter implementing the driver's bus interface
- `test/` — Unity unit tests, run on host via `pio test -e native`
- `docs/` — hardware notes and design docs

## License

Personal project. No license granted.
