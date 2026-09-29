/*
 * ens160_driver.c - glue between ESP-IDF I2C and the official ScioSense ENS160
 * driver (components/sciosense_ens160).
 *
 * The ENS160 provides the TVOC value shown on the display.
 */
#include "ens160_driver.h"
#include "ScioSense_Ens160.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "i2c_bus.h"

// 0x53 = ADD pin high. Modules with ADD pulled low answer at 0x52.
#define ENS160_I2C_ADDR         0x53

static const char *TAG = "ENS160";

// Driver instance
static ScioSense_Ens16x g_ens160 = {0};

// Passed back to the I/O callbacks by the ScioSense driver
typedef struct {
    uint8_t address;
} ens160_i2c_config_t;

static ens160_i2c_config_t g_i2c_config = {
    .address = ENS160_I2C_ADDR
};

// ============================================================================
// ESP-IDF I2C Callback Implementations
// ============================================================================

/**
 * @brief I2C read callback for the ScioSense driver
 */
static Result ens160_i2c_read_cb(void *config, const uint16_t address, uint8_t *data, const size_t size)
{
    const ens160_i2c_config_t *cfg = (const ens160_i2c_config_t *)config;
    return (i2c_bus_read_reg(cfg->address, (uint8_t)address, data, size) == ESP_OK) ? RESULT_OK : RESULT_IO_ERROR;
}

/**
 * @brief I2C write callback for the ScioSense driver
 */
static Result ens160_i2c_write_cb(void *config, const uint16_t address, uint8_t *data, const size_t size)
{
    const ens160_i2c_config_t *cfg = (const ens160_i2c_config_t *)config;
    return (i2c_bus_write_reg(cfg->address, (uint8_t)address, data, size) == ESP_OK) ? RESULT_OK : RESULT_IO_ERROR;
}

/**
 * @brief Delay callback for the ScioSense driver
 */
static void ens160_wait_cb(const uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

// ============================================================================
// Public API Implementation
// ============================================================================

int8_t ens160_init_sensor(void)
{
    // Hook the ScioSense driver up to our I2C bus
    g_ens160.io.read = ens160_i2c_read_cb;
    g_ens160.io.write = ens160_i2c_write_cb;
    g_ens160.io.wait = ens160_wait_cb;
    g_ens160.io.config = &g_i2c_config;

    // Reset to IDLE and read part ID + firmware version
    Result result = Ens16x_Init(&g_ens160);

    if (result != RESULT_OK) {
        ESP_LOGE(TAG, "ENS160 initialization failed with result: %d", result);
        return -1;
    }

    // Verify Part ID
    if (!Ens160_IsConnected(&g_ens160)) {
        ESP_LOGE(TAG, "ENS160 Part ID verification failed (0x%04X)", g_ens160.partId);
        return -1;
    }

    ESP_LOGI(TAG, "ENS160 Part ID verified: 0x%04X", g_ens160.partId);
    ESP_LOGI(TAG, "Firmware Version: %d.%d.%d",
             g_ens160.firmwareVersion[0],
             g_ens160.firmwareVersion[1],
             g_ens160.firmwareVersion[2]);

    // Start standard measurement mode
    result = Ens16x_StartStandardMeasure(&g_ens160);
    if (result != RESULT_OK) {
        ESP_LOGE(TAG, "Failed to start standard measurement mode");
        return -1;
    }

    ESP_LOGI(TAG, "ENS160 initialized successfully (Standard Mode)");
    ESP_LOGW(TAG, "Sensor warming up - wait ~1 hour for full accuracy");

    return 0;
}

int8_t ens160_read_data(ens160_data_t *data)
{
    if (data == NULL) {
        return -1;
    }

    // Initialize data
    data->eco2 = 0;
    data->tvoc = 0;
    data->aqi = 0;
    data->valid = false;

    // Update sensor (read status and data if available)
    Result result = Ens16x_Update(&g_ens160);

    // Extract data from internal buffers
    data->eco2 = Ens16x_GetEco2(&g_ens160);
    data->tvoc = Ens16x_GetTvoc(&g_ens160);
    data->aqi = Ens16x_GetAirQualityIndex_UBA(&g_ens160);

    // Check device status for data validity
    Ens16x_DeviceStatus status = Ens16x_GetDeviceStatus(&g_ens160);

    // Log status for debugging
    uint8_t validity = (status >> 2) & 0x03;
    bool has_new_data = (status & ENS16X_DEVICE_STATUS_NEW_DATA) != 0;
    bool has_error = (status & ENS16X_DEVICE_STATUS_ERROR) != 0;

    ESP_LOGD(TAG, "Status: 0x%02X (validity=%d, new_data=%d, error=%d, result=%d)",
             status, validity, has_new_data, has_error, result);

    // Determine validity based on status register
    if (has_error) {
        ESP_LOGW(TAG, "Sensor error detected (status: 0x%02X)", status);
        return -1;
    }

    // Mark data as valid if we have new data or if result was OK
    if (has_new_data || result == RESULT_OK) {
        data->valid = true;
    }

    // Validity flag 1 = warm-up, 2 = initial start-up: data is readable but not
    // yet reliable. Both are reported as 1.
    if (validity == 0x01 || validity == 0x02) {
        data->valid = false;
        return 1;  // Warming up
    }

    return (result == RESULT_OK) ? 0 : -1;
}

int8_t ens160_set_compensation(float temp_c, float humi_rh)
{
    uint16_t temp_raw = Ens16x_CalcTempInFromCelsius(temp_c);
    uint16_t humi_raw = Ens16x_CalcRhIn(humi_rh);

    Result result = Ens16x_WriteCompensation(&g_ens160, temp_raw, humi_raw);

    return (result == RESULT_OK) ? 0 : -1;
}
