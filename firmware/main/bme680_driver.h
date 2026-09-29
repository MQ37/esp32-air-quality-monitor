#ifndef BME680_DRIVER_H
#define BME680_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

// Sensor data structure
typedef struct {
    float temperature;      // °C
    float humidity;         // %
    float pressure;         // hPa
    float gas_resistance;   // kΩ
    bool valid;            // Data validity flag
} bme680_data_t;

/**
 * @brief Initialize BME680 sensor
 *
 * @return 0 on success, -1 on failure
 */
int8_t bme680_init_sensor(void);

/**
 * @brief Read sensor data from BME680
 *
 * @param data Pointer to data structure to fill
 * @return 0 on success, -1 on failure
 */
int8_t bme680_read_data(bme680_data_t *data);

/**
 * @brief Trigger a measurement in forced mode
 *
 * @return 0 on success, -1 on failure
 */
int8_t bme680_trigger_measurement(void);

#endif // BME680_DRIVER_H
