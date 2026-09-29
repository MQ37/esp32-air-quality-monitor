/*
 * ESP32 air-quality monitor
 *
 * Reads three I2C sensors every 5 seconds and shows a moving average of the
 * last 5 readings on the display:
 *   - BME680: temperature + humidity (also used to compensate the gas sensors)
 *   - CCS811: eCO2
 *   - ENS160: TVOC
 */
#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "nvs_flash.h"

#include "bme680_driver.h"
#include "ccs811_driver.h"
#include "ens160_driver.h"
#include "i2c_bus.h"
#include "lvgl_port.h"
#include "pins.h"
#include "ui_air_quality.h"

#define LOOP_PERIOD_MS     10
#define SENSOR_PERIOD_MS   5000
#define AVG_SAMPLES        5      /* 5 samples x 5 s = 25 s moving average */

static const char *TAG = "MAIN";

static void init_nvs(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

static void init_sensors(void)
{
    i2c_bus_init();
    i2c_bus_scan();

    if (bme680_init_sensor() != 0) {
        ESP_LOGE(TAG, "BME680 initialization failed!");
    } else {
        ESP_LOGI(TAG, "BME680 ready");
    }

    if (ens160_init_sensor() != 0) {
        ESP_LOGE(TAG, "ENS160 initialization failed!");
    } else {
        ESP_LOGI(TAG, "ENS160 ready");
    }

    if (ccs811_init_sensor() != 0) {
        ESP_LOGE(TAG, "CCS811 initialization failed!");
    } else {
        ESP_LOGI(TAG, "CCS811 ready");
    }
}

void app_main(void)
{
    // Switch the backlight on right away so the panel is visibly alive while
    // the sensors initialize (the display driver also drives this pin later).
    gpio_reset_pin(PIN_LCD_BACKLIGHT);
    gpio_set_direction(PIN_LCD_BACKLIGHT, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_LCD_BACKLIGHT, 1);

    init_nvs();
    init_sensors();

    lvgl_port_init();
    ui_air_quality_init();
    ESP_LOGI(TAG, "UI created, entering main loop");

    bme680_data_t bme_data = {0};
    ens160_data_t ens_data = {0};
    ccs811_data_t ccs_data = {0};
    uint32_t loop_count = 0;

    // Moving-average ring buffers
    float temp_history[AVG_SAMPLES] = {0};
    float humi_history[AVG_SAMPLES] = {0};
    uint16_t eco2_history[AVG_SAMPLES] = {0};
    uint16_t tvoc_history[AVG_SAMPLES] = {0};
    uint8_t history_index = 0;
    uint8_t history_count = 0;

    while (1) {
        lv_timer_handler();

        if (loop_count % (SENSOR_PERIOD_MS / LOOP_PERIOD_MS) == 0) {
            bool bme_ok = false;
            bool ens_ok = false;
            bool ccs_ok = false;

            // BME680: temperature & humidity, then pass them on as
            // compensation data to both gas sensors
            if (bme680_trigger_measurement() == 0) {
                if (bme680_read_data(&bme_data) == 0 && bme_data.valid) {
                    bme_ok = true;
                    ens160_set_compensation(bme_data.temperature, bme_data.humidity);
                    ccs811_set_environment(bme_data.temperature, bme_data.humidity);
                }
            }

            // ENS160 -> TVOC. Returns 0 = ok, 1 = warming up, -1 = error.
            int8_t ens_result = ens160_read_data(&ens_data);
            if (ens_result >= 0 && ens_data.valid) {
                ens_ok = true;
            }
            // During warm-up the values are readable but not yet reliable;
            // they are still shown if they look plausible.
            bool ens_warmup = (ens_result == 1);

            // CCS811 -> eCO2
            int8_t ccs_result = ccs811_read_data(&ccs_data);
            if (ccs_result == 0 && ccs_data.valid) {
                ccs_ok = true;
            }

            ESP_LOGD(TAG, "ENS160: result=%d valid=%d eCO2=%u TVOC=%u AQI=%u | CCS811: result=%d valid=%d eCO2=%u TVOC=%u",
                     ens_result, ens_data.valid, ens_data.eco2, ens_data.tvoc, ens_data.aqi,
                     ccs_result, ccs_data.valid, ccs_data.eco2, ccs_data.tvoc);

            if (bme_ok) {
                // A value of 0 tells the UI "no data yet" and it shows "Warming".
                uint16_t eco2_raw = 0;
                uint16_t tvoc_raw = 0;

                if (ccs_ok && ccs_data.eco2 > 0) {
                    eco2_raw = ccs_data.eco2;
                }
                if ((ens_ok || ens_warmup) && ens_data.tvoc > 0 && ens_data.tvoc <= 10000) {
                    tvoc_raw = ens_data.tvoc;
                }

                temp_history[history_index] = bme_data.temperature;
                humi_history[history_index] = bme_data.humidity;
                eco2_history[history_index] = eco2_raw;
                tvoc_history[history_index] = tvoc_raw;
                history_index = (history_index + 1) % AVG_SAMPLES;
                if (history_count < AVG_SAMPLES) {
                    history_count++;
                }

                // eCO2/TVOC slots holding 0 ("no data yet") are left out of
                // their averages, so the first real readings after warm-up
                // aren't pulled down towards 0.
                float temp_avg = 0;
                float humi_avg = 0;
                uint32_t eco2_sum = 0;
                uint32_t tvoc_sum = 0;
                uint8_t eco2_n = 0;
                uint8_t tvoc_n = 0;
                for (uint8_t i = 0; i < history_count; i++) {
                    temp_avg += temp_history[i];
                    humi_avg += humi_history[i];
                    if (eco2_history[i] > 0) {
                        eco2_sum += eco2_history[i];
                        eco2_n++;
                    }
                    if (tvoc_history[i] > 0) {
                        tvoc_sum += tvoc_history[i];
                        tvoc_n++;
                    }
                }
                temp_avg /= history_count;
                humi_avg /= history_count;
                uint16_t eco2_avg = eco2_n ? eco2_sum / eco2_n : 0;
                uint16_t tvoc_avg = tvoc_n ? tvoc_sum / tvoc_n : 0;

                ui_air_quality_update(temp_avg, humi_avg, eco2_avg, tvoc_avg);

                ESP_LOGI(TAG, "Avg over %u samples: %.1f C, %.1f %% | eCO2 %u ppm (CCS811) | TVOC %u ppb (ENS160)",
                         history_count, temp_avg, humi_avg, eco2_avg, tvoc_avg);
                if (eco2_raw == 0) {
                    ESP_LOGW(TAG, "eCO2 warming up (CCS811: %u ppm)", ccs_data.eco2);
                }
                if (tvoc_raw == 0) {
                    ESP_LOGW(TAG, "TVOC warming up (ENS160: %u ppb)", ens_data.tvoc);
                }
            }
        }
        loop_count++;

        vTaskDelay(pdMS_TO_TICKS(LOOP_PERIOD_MS));
    }
}
