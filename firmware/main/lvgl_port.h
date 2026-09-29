/*
 * lvgl_port.h - connects LVGL to the display/touch drivers of this board.
 */
#pragma once

/* Initialize LVGL, the SPI display (ST7789P3), the touch controller (XPT2046)
 * and the 10 ms LVGL tick timer. Call once, before creating any UI. */
void lvgl_port_init(void);
