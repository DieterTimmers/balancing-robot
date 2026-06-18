#ifndef MOCK_BUS_H
#define MOCK_BUS_H

#include "mpu6050.h"

#define MOCK_BUS_MAX_REG 0x80

typedef struct {
    uint8_t  regs[MOCK_BUS_MAX_REG];   /* simulated register file */
    int      fail_next_read;           /* if non-zero, next read returns error */
    int      fail_next_write;
    uint32_t write_count;
    uint32_t read_count;
    uint8_t  last_written_reg;
    uint8_t  last_written_val;
} mock_bus_t;

void          mock_bus_reset(mock_bus_t *m);
mpu6050_bus_t mock_bus_interface(mock_bus_t *m);

#endif /* MOCK_BUS_H */
