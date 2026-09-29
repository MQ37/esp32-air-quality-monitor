/*
 * ccs811_driver.c - minimal register-level driver for the ams/ScioSense CCS811.
 *
 * The CCS811 provides the eCO2 value shown on the display. Its nWAKE pin must
 * be held low (tied to GND) for it to answer on I2C.
 */
#include "ccs811_driver.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "i2c_bus.h"

// 0x5A = ADDR pin low. Modules with ADDR pulled high answer at 0x5B.
#define CCS811_I2C_ADDR         0x5A

// Register map

#define CCS811_STATUS           0x00
#define CCS811_MEAS_MODE        0x01
#define CCS811_ALG_RESULT_DATA  0x02
#define CCS811_ENV_DATA         0x05
#define CCS811_HW_ID            0x20
#define CCS811_HW_VERSION       0x21
#define CCS811_FW_APP_VERSION   0x24
#define CCS811_ERROR_ID         0xE0
#define CCS811_APP_START        0xF4
#define CCS811_SW_RESET         0xFF

// Drive modes
#define CCS811_DRIVE_MODE_IDLE  0x00
#define CCS811_DRIVE_MODE_1SEC  0x10
#define CCS811_DRIVE_MODE_10SEC 0x20
#define CCS811_DRIVE_MODE_60SEC 0x30

static const char *TAG = "CCS811";

static int8_t ccs811_i2c_read(uint8_t reg_addr, uint8_t *data, uint16_t len)
{
    return (i2c_bus_read_reg(CCS811_I2C_ADDR, reg_addr, data, len) == ESP_OK) ? 0 : -1;
}

static int8_t ccs811_i2c_write(uint8_t reg_addr, uint8_t *data, uint16_t len)
{
    return (i2c_bus_write_reg(CCS811_I2C_ADDR, reg_addr, data, len) == ESP_OK) ? 0 : -1;
}

// Command without payload (e.g. APP_START)
static int8_t ccs811_i2c_write_cmd(uint8_t reg_addr)
{
    return ccs811_i2c_write(reg_addr, NULL, 0);
}

int8_t ccs811_init_sensor(void)
{
    uint8_t hw_id;
    uint8_t status;

    // Read Hardware ID
    if (ccs811_i2c_read(CCS811_HW_ID, &hw_id, 1) != 0) {
        ESP_LOGE(TAG, "Failed to read HW ID");
        return -1;
    }

    if (hw_id != 0x81) {
        ESP_LOGE(TAG, "Invalid HW ID: 0x%02X (expected 0x81)", hw_id);
        return -1;
    }

    ESP_LOGI(TAG, "CCS811 HW ID verified: 0x%02X", hw_id);

    // Read status register
    if (ccs811_i2c_read(CCS811_STATUS, &status, 1) != 0) {
        ESP_LOGE(TAG, "Failed to read status");
        return -1;
    }

    ESP_LOGI(TAG, "Initial status: 0x%02X", status);

    // Check if app is valid (bit 4)
    if ((status & 0x10) == 0) {
        ESP_LOGE(TAG, "App not valid!");
        return -1;
    }

    // Check if in boot mode (bit 7 = 0 means boot mode)
    if ((status & 0x80) == 0) {
        ESP_LOGI(TAG, "Sensor in boot mode, starting app...");

        // Start application
        if (ccs811_i2c_write_cmd(CCS811_APP_START) != 0) {
            ESP_LOGE(TAG, "Failed to start app");
            return -1;
        }

        vTaskDelay(pdMS_TO_TICKS(100));

        // Re-read status
        if (ccs811_i2c_read(CCS811_STATUS, &status, 1) != 0) {
            ESP_LOGE(TAG, "Failed to read status after app start");
            return -1;
        }

        ESP_LOGI(TAG, "Status after app start: 0x%02X", status);
    }

    // Check for errors (bit 0)
    if (status & 0x01) {
        uint8_t error;
        if (ccs811_i2c_read(CCS811_ERROR_ID, &error, 1) == 0) {
            ESP_LOGE(TAG, "Sensor error: 0x%02X", error);
        }
        return -1;
    }

    // Set drive mode to 1 second measurements
    uint8_t meas_mode = CCS811_DRIVE_MODE_1SEC;
    if (ccs811_i2c_write(CCS811_MEAS_MODE, &meas_mode, 1) != 0) {
        ESP_LOGE(TAG, "Failed to set measurement mode");
        return -1;
    }

    // Wait for sensor to start measurements (first reading takes ~1 second)
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Verify sensor is measuring
    if (ccs811_i2c_read(CCS811_STATUS, &status, 1) == 0) {
        ESP_LOGI(TAG, "Status after mode set: 0x%02X", status);
        if (status & 0x01) {
            uint8_t error;
            if (ccs811_i2c_read(CCS811_ERROR_ID, &error, 1) == 0) {
                ESP_LOGW(TAG, "Sensor has error after init: 0x%02X", error);
            }
        }
    }

    ESP_LOGI(TAG, "CCS811 initialized successfully (1-second mode)");
    ESP_LOGW(TAG, "Sensor warming up - wait 20 minutes for accurate readings");

    return 0;
}

int8_t ccs811_set_environment(float temp_c, float humi_rh)
{
    // Temperature: (temp + 25) * 512
    // Humidity: humidity * 512
    uint16_t temp_conv = (uint16_t)((temp_c + 25.0f) * 512.0f);
    uint16_t humi_conv = (uint16_t)(humi_rh * 512.0f);

    uint8_t env_data[4] = {
        (uint8_t)((humi_conv >> 8) & 0xFF),  // Humidity MSB
        (uint8_t)(humi_conv & 0xFF),         // Humidity LSB
        (uint8_t)((temp_conv >> 8) & 0xFF),  // Temperature MSB
        (uint8_t)(temp_conv & 0xFF)          // Temperature LSB
    };

    // Retry a few times in case of I2C collision
    for (int retry = 0; retry < 3; retry++) {
        if (ccs811_i2c_write(CCS811_ENV_DATA, env_data, 4) == 0) {
            return 0;  // Success
        }
        vTaskDelay(pdMS_TO_TICKS(10));  // Small delay before retry
    }

    // Only log error if all retries failed (suppress occasional glitches)
    ESP_LOGD(TAG, "Failed to set environmental data after retries (not critical)");
    return -1;
}

bool ccs811_data_available(void)
{
    uint8_t status;

    if (ccs811_i2c_read(CCS811_STATUS, &status, 1) != 0) {
        return false;
    }

    // Bit 3: DATA_READY
    return (status & 0x08) != 0;
}

int8_t ccs811_read_data(ccs811_data_t *data)
{
    if (data == NULL) {
        return -1;
    }

    uint8_t status;

    // Read status
    if (ccs811_i2c_read(CCS811_STATUS, &status, 1) != 0) {
        ESP_LOGE(TAG, "Failed to read status");
        data->valid = false;
        return -1;
    }

    // Check for error (bit 0)
    if (status & 0x01) {
        uint8_t error;
        if (ccs811_i2c_read(CCS811_ERROR_ID, &error, 1) == 0) {
            ESP_LOGW(TAG, "Sensor error: 0x%02X", error);
        }
        data->valid = false;
        return -1;
    }

    // Check if data is ready (bit 3)
    if ((status & 0x08) == 0) {
        ESP_LOGD(TAG, "Data not ready");
        data->valid = false;
        return -1;
    }

    // Small delay before reading data (CCS811 quirk)
    vTaskDelay(pdMS_TO_TICKS(10));

    // Read algorithm results (4 bytes minimum for eCO2 and TVOC)
    uint8_t alg_data[4];
    if (ccs811_i2c_read(CCS811_ALG_RESULT_DATA, alg_data, 4) != 0) {
        ESP_LOGE(TAG, "Failed to read algorithm data (status: 0x%02X)", status);
        data->valid = false;
        return -1;
    }

    // Parse data
    data->eco2 = (alg_data[0] << 8) | alg_data[1];
    data->tvoc = (alg_data[2] << 8) | alg_data[3];

    // CCS811 returns 0 during first few minutes - this is NORMAL warm-up
    // Accept the data but mark with a flag for UI handling
    if (data->eco2 == 0) {
        static uint8_t log_counter = 0;
        if (log_counter++ % 10 == 0) {  // Log every 10th attempt
            ESP_LOGI(TAG, "Warming up... eCO2 still at 0 ppm (attempt %d)", log_counter);
        }
        data->valid = false;  // Mark as not ready for display
        return -1;
    }

    // Sanity check: CCS811 normal range is 400-8192 ppm
    // But accept anything > 0 during warm-up period
    if (data->eco2 > 8192) {
        ESP_LOGW(TAG, "eCO2 unusually high: %u ppm", data->eco2);
    }

    data->valid = true;
    ESP_LOGD(TAG, "Valid reading: eCO2=%u ppm, TVOC=%u ppb", data->eco2, data->tvoc);

    return 0;
}
