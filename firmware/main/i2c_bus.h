/*
 * i2c_bus.h - the shared I2C bus the three sensors sit on.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

/* Configure I2C_NUM_0 as master on PIN_I2C_SDA / PIN_I2C_SCL (see pins.h). */
void i2c_bus_init(void);

/* Probe every 7-bit address and log the ones that answer. Handy when wiring. */
void i2c_bus_scan(void);

/* Write the register address, then read `len` bytes back (repeated start). */
esp_err_t i2c_bus_read_reg(uint8_t dev_addr, uint8_t reg, uint8_t *data, size_t len);

/* Write the register address followed by `len` bytes. `len` may be 0 to send
 * a bare command byte (e.g. CCS811 APP_START). */
esp_err_t i2c_bus_write_reg(uint8_t dev_addr, uint8_t reg, const uint8_t *data, size_t len);
