#ifndef ENS160_DRIVER_H
#define ENS160_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

// ENS160 sensor data
typedef struct {
    uint16_t eco2;      // Equivalent CO2 in ppm
    uint16_t tvoc;      // Total VOC in ppb
    uint8_t aqi;        // Air Quality Index (1-5)
    bool valid;         // Data validity flag
} ens160_data_t;

/**
 * @brief Initialize ENS160 sensor (ScioSense driver) and start standard mode
 *
 * @return 0 on success, -1 on failure
 */
int8_t ens160_init_sensor(void);

/**
 * @brief Read sensor data from ENS160
 *
 * @param data Pointer to data structure to fill
 * @return 0 on success, 1 while the sensor is still warming up (values are
 *         filled in but not yet reliable), -1 on failure
 */
int8_t ens160_read_data(ens160_data_t *data);

/**
 * @brief Set temperature and humidity compensation
 *
 * @param temp_c Temperature in Celsius
 * @param humi_rh Relative humidity in %
 * @return 0 on success, -1 on failure
 */
int8_t ens160_set_compensation(float temp_c, float humi_rh);

#endif // ENS160_DRIVER_H
