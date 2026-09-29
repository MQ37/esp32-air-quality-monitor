/*
 * pins.h - every GPIO this firmware uses, in one place.
 *
 * Board: ESP32-32E (ESP32-WROOM-32) with a 3.2" 240x320 ST7789P3 IPS display
 * and an XPT2046 resistive touch controller (sold as "E32R32P").
 *
 * The I2C pins are used directly by this firmware.
 * The display and touch pins are consumed by the lvgl_esp32_drivers component,
 * which reads them from Kconfig (see sdkconfig.defaults). They are repeated here
 * so the whole pin map is in one file, and the static asserts below fail the
 * build if the two ever disagree.
 */
#pragma once

#include "sdkconfig.h"

/* ---- I2C sensor bus: BME680 (0x77), CCS811 (0x5A), ENS160 (0x53) ---- */
#define PIN_I2C_SDA          32
#define PIN_I2C_SCL          25
#define I2C_BUS_FREQ_HZ      100000

/* ---- Display: ST7789P3 on SPI2 (HSPI) ---- */
#define PIN_SPI_MOSI         13
#define PIN_SPI_MISO         12   /* only the touch controller talks back */
#define PIN_SPI_CLK          14
#define PIN_LCD_CS           15
#define PIN_LCD_DC            2
#define PIN_LCD_BACKLIGHT    27   /* active high */
/* The panel's reset line is wired to the ESP32 EN pin, so there is no reset
 * GPIO. The driver sends a software reset (command 0x01) instead. */

/* ---- Touch: XPT2046, shares the SPI2 bus with the display ---- */
#define PIN_TOUCH_CS         33
#define PIN_TOUCH_IRQ        36   /* input-only pin, low while touched */

/* ---- Keep this file and the Kconfig values in sync ---- */
_Static_assert(PIN_SPI_MOSI == CONFIG_LV_DISP_SPI_MOSI, "display MOSI differs from sdkconfig");
_Static_assert(PIN_SPI_MISO == CONFIG_LV_DISP_SPI_MISO, "display MISO differs from sdkconfig");
_Static_assert(PIN_SPI_CLK == CONFIG_LV_DISP_SPI_CLK, "display CLK differs from sdkconfig");
_Static_assert(PIN_LCD_CS == CONFIG_LV_DISP_SPI_CS, "display CS differs from sdkconfig");
_Static_assert(PIN_LCD_DC == CONFIG_LV_DISP_PIN_DC, "display DC differs from sdkconfig");
_Static_assert(PIN_LCD_BACKLIGHT == CONFIG_LV_DISP_PIN_BCKL, "backlight pin differs from sdkconfig");
_Static_assert(PIN_SPI_MOSI == CONFIG_LV_TOUCH_SPI_MOSI, "touch MOSI differs from sdkconfig");
_Static_assert(PIN_SPI_MISO == CONFIG_LV_TOUCH_SPI_MISO, "touch MISO differs from sdkconfig");
_Static_assert(PIN_SPI_CLK == CONFIG_LV_TOUCH_SPI_CLK, "touch CLK differs from sdkconfig");
_Static_assert(PIN_TOUCH_CS == CONFIG_LV_TOUCH_SPI_CS, "touch CS differs from sdkconfig");
_Static_assert(PIN_TOUCH_IRQ == CONFIG_LV_TOUCH_PIN_IRQ, "touch IRQ differs from sdkconfig");
