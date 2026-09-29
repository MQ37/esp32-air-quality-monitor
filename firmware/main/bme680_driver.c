/*
 * bme680_driver.c - thin wrapper around the Bosch BME68x Sensor API.
 *
 * The BME680 provides temperature and humidity for the display, and the same
 * values are fed to the CCS811 and ENS160 for their compensation.
 */
#include "bme680_driver.h"
#include "bme68x.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

#include "i2c_bus.h"

// 0x77 = SDO pin high. If your module answers at 0x76 instead, use BME68X_I2C_ADDR_LOW.
#define BME_ADDR            BME68X_I2C_ADDR_HIGH

static const char *TAG = "BME680";
static struct bme68x_dev g_bme;
static struct bme68x_conf g_conf;
static struct bme68x_heatr_conf g_heatr_conf;
static uint32_t g_del_period;

// Bosch API callbacks: plain register read/write on the shared I2C bus
static int8_t user_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    (void)intf_ptr;
    return (i2c_bus_read_reg(BME_ADDR, reg_addr, reg_data, len) == ESP_OK) ? BME68X_OK : BME68X_E_COM_FAIL;
}

static int8_t user_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    (void)intf_ptr;
    return (i2c_bus_write_reg(BME_ADDR, reg_addr, reg_data, len) == ESP_OK) ? BME68X_OK : BME68X_E_COM_FAIL;
}

// User delay function (microseconds)
static void user_delay_us(uint32_t period, void *intf_ptr)
{
    (void)intf_ptr;
    esp_rom_delay_us(period);
}

int8_t bme680_init_sensor(void)
{
    // Setup BME68x device structure
    g_bme.intf = BME68X_I2C_INTF;
    g_bme.intf_ptr = NULL;
    g_bme.read = user_i2c_read;
    g_bme.write = user_i2c_write;
    g_bme.delay_us = user_delay_us;
    g_bme.amb_temp = 25;  // Ambient temperature for calculations

    // Initialize the sensor
    int8_t rslt = bme68x_init(&g_bme);
    if (rslt != BME68X_OK) {
        ESP_LOGE(TAG, "BME680 init failed! Error code: %d", rslt);
        return -1;
    }

    ESP_LOGI(TAG, "BME680 initialized successfully");

    // Configure sensor settings
    g_conf.os_hum = BME68X_OS_2X;      // 2x oversampling for humidity
    g_conf.os_pres = BME68X_OS_4X;     // 4x oversampling for pressure
    g_conf.os_temp = BME68X_OS_8X;     // 8x oversampling for temperature
    g_conf.filter = BME68X_FILTER_SIZE_3;
    g_conf.odr = BME68X_ODR_NONE;

    rslt = bme68x_set_conf(&g_conf, &g_bme);
    if (rslt != BME68X_OK) {
        ESP_LOGE(TAG, "Failed to set config! Error code: %d", rslt);
        return -1;
    }

    // The gas-plate heater is switched off. The BME680 gas reading is not shown
    // on the display (the CCS811 and ENS160 cover air quality), and the heater
    // warms the sensor and skews the temperature and humidity readings. Set
    // .enable to BME68X_ENABLE to use gas resistance (e.g. for your own IAQ index).
    g_heatr_conf.enable = BME68X_DISABLE;
    g_heatr_conf.heatr_temp = 320;     // °C (only used when the heater is enabled)
    g_heatr_conf.heatr_dur = 150;      // ms (only used when the heater is enabled)

    rslt = bme68x_set_heatr_conf(BME68X_FORCED_MODE, &g_heatr_conf, &g_bme);
    if (rslt != BME68X_OK) {
        ESP_LOGE(TAG, "Failed to set heater config! Error code: %d", rslt);
        return -1;
    }

    // Calculate measurement delay
    g_del_period = bme68x_get_meas_dur(BME68X_FORCED_MODE, &g_conf, &g_bme);
    if (g_heatr_conf.enable == BME68X_ENABLE) {
        g_del_period += g_heatr_conf.heatr_dur * 1000;
    }

    ESP_LOGI(TAG, "BME680 configuration complete. Measurement delay: %lu us", g_del_period);

    return 0;
}

int8_t bme680_trigger_measurement(void)
{
    int8_t rslt = bme68x_set_op_mode(BME68X_FORCED_MODE, &g_bme);
    if (rslt != BME68X_OK) {
        ESP_LOGE(TAG, "Failed to set forced mode! Error code: %d", rslt);
        return -1;
    }

    return 0;
}

int8_t bme680_read_data(bme680_data_t *data)
{
    if (data == NULL) {
        return -1;
    }

    // Wait for measurement to complete
    user_delay_us(g_del_period, NULL);

    struct bme68x_data sensor_data;
    uint8_t n_fields;

    int8_t rslt = bme68x_get_data(BME68X_FORCED_MODE, &sensor_data, &n_fields, &g_bme);

    if (rslt != BME68X_OK) {
        ESP_LOGE(TAG, "Failed to get data! Error code: %d", rslt);
        data->valid = false;
        return -1;
    }

    if (n_fields == 0) {
        ESP_LOGW(TAG, "No new data available");
        data->valid = false;
        return -1;
    }

    // Fill output structure
    data->temperature = sensor_data.temperature;
    data->humidity = sensor_data.humidity;
    data->pressure = sensor_data.pressure / 100.0f;  // Convert Pa to hPa
    data->gas_resistance = sensor_data.gas_resistance / 1000.0f;  // Convert Ω to kΩ
    data->valid = true;

    return 0;
}
