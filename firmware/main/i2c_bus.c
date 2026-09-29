/*
 * i2c_bus.c - shared I2C helpers used by the BME680, CCS811 and ENS160 drivers.
 *
 * Uses the ESP-IDF "legacy" I2C driver (driver/i2c.h), which is still shipped
 * with ESP-IDF 5.5.
 */
#include "i2c_bus.h"

#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#include "pins.h"

#define I2C_PORT        I2C_NUM_0
#define I2C_TIMEOUT_MS  1000

static const char *TAG = "I2C";

void i2c_bus_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = PIN_I2C_SDA,
        .scl_io_num = PIN_I2C_SCL,
        /* Internal pull-ups are weak (~45 kOhm); most breakout boards also
         * carry their own pull-up resistors. */
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_BUS_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_PORT, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_PORT, conf.mode, 0, 0, 0));
    ESP_LOGI(TAG, "I2C initialized on SDA=%d, SCL=%d", PIN_I2C_SDA, PIN_I2C_SCL);
}

void i2c_bus_scan(void)
{
    ESP_LOGI(TAG, "Scanning I2C bus...");
    uint8_t device_count = 0;

    for (uint8_t addr = 1; addr < 127; addr++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
        i2c_cmd_link_delete(cmd);

        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Found device at 0x%02X", addr);
            device_count++;
        }
    }

    if (device_count == 0) {
        ESP_LOGW(TAG, "No I2C devices found - check SDA/SCL, 3V3 and GND");
    } else {
        ESP_LOGI(TAG, "Total I2C devices found: %d", device_count);
    }
}

esp_err_t i2c_bus_read_reg(uint8_t dev_addr, uint8_t reg, uint8_t *data, size_t len)
{
    if (len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_READ, true);
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t i2c_bus_write_reg(uint8_t dev_addr, uint8_t reg, const uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    if (len > 0) {
        i2c_master_write(cmd, data, len, true);
    }
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    return ret;
}
