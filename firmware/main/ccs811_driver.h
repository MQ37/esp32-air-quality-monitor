#ifndef CCS811_DRIVER_H
#define CCS811_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

// CCS811 sensor data structure
typedef struct {
    uint16_t eco2;      // Equivalent CO2 in ppm
    uint16_t tvoc;      // Total VOC in ppb
    bool valid;         // Data validity flag
} ccs811_data_t;

/**
 * @brief Initialize CCS811 sensor
 *
 * @return 0 on success, -1 on failure
 */
int8_t ccs811_init_sensor(void);

/**
 * @brief Read sensor data from CCS811
 *
 * @param data Pointer to data structure to fill
 * @return 0 on success, -1 on failure
 */
int8_t ccs811_read_data(ccs811_data_t *data);

/**
 * @brief Set temperature and humidity compensation
 *
 * @param temp_c Temperature in Celsius
 * @param humi_rh Relative humidity in %
 * @return 0 on success, -1 on failure
 */
int8_t ccs811_set_environment(float temp_c, float humi_rh);

/**
 * @brief Check if new data is available
 *
 * @return true if new data ready, false otherwise
 */
bool ccs811_data_available(void);

#endif // CCS811_DRIVER_H
