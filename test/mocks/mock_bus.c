#include "mock_bus.h"
#include <string.h>

static int mock_read(void *ctx, uint8_t addr, uint8_t reg,
                     uint8_t *buf, size_t len) {
    (void)addr;
    mock_bus_t *m = (mock_bus_t *)ctx;
    m->read_count++;
    if (m->fail_next_read) { m->fail_next_read = 0; return -1; }
    if ((size_t)reg + len > MOCK_BUS_MAX_REG) return -1;
    memcpy(buf, &m->regs[reg], len);
    return 0;
}

static int mock_write(void *ctx, uint8_t addr, uint8_t reg,
                      const uint8_t *buf, size_t len) {
    (void)addr;
    mock_bus_t *m = (mock_bus_t *)ctx;
    m->write_count++;
    if (m->fail_next_write) { m->fail_next_write = 0; return -1; }
    if ((size_t)reg + len > MOCK_BUS_MAX_REG) return -1;
    memcpy(&m->regs[reg], buf, len);
    if (len == 1) { m->last_written_reg = reg; m->last_written_val = buf[0]; }
    return 0;
}

static void mock_delay(uint32_t ms) { (void)ms; }

void mock_bus_reset(mock_bus_t *m) { memset(m, 0, sizeof(*m)); }

mpu6050_bus_t mock_bus_interface(mock_bus_t *m) {
    return (mpu6050_bus_t){
        .read     = mock_read,
        .write    = mock_write,
        .delay_ms = mock_delay,
        .ctx      = m,
    };
}
