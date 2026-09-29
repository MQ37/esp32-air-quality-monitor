#ifndef UI_AIR_QUALITY_H
#define UI_AIR_QUALITY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Build the four-card air-quality screen
 */
void ui_air_quality_init(void);

/**
 * @brief Update UI with real sensor data
 *
 * @param temp Temperature in °C
 * @param humi Humidity in %
 * @param eco2 Equivalent CO2 in ppm
 * @param tvoc Total VOC in ppb
 */
void ui_air_quality_update(float temp, float humi, uint16_t eco2, uint16_t tvoc);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*UI_AIR_QUALITY_H*/
