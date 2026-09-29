# Display bring-up notes (ST7789P3)

The ESP32-32E ("E32R32P") has a 3.2" 240x320 IPS panel driven by an ST7789P3 controller over SPI. The generic ST7789 driver in `lvgl_esp32_drivers` did not work on it out of the box. This page lists what had to change and why. The code is in [`firmware/components/lvgl_esp32_drivers/lvgl_tft/st7789.c`](../firmware/components/lvgl_esp32_drivers/lvgl_tft/st7789.c) and [`lvgl_spi_conf.h`](../firmware/components/lvgl_esp32_drivers/lvgl_spi_conf.h).

## The quirks

| Symptom | Cause | Fix in this repo |
| --- | --- | --- |
| Backlight on, screen stays black | Upstream uses SPI mode 2 for ST7789; this panel needs **mode 0** | `SPI_TFT_SPI_MODE (0)` in `lvgl_spi_conf.h` |
| Colours look like a photo negative | IPS panels need display inversion | `CONFIG_LV_INVERT_COLORS=y` → the driver sends `INVON` (0x21) |
| No reset GPIO to toggle | The panel's reset line is wired to the ESP32 **EN** pin | `CONFIG_LV_DISP_USE_RST=n`; the driver sends a software reset (0x01) and waits 150 ms |
| Unreliable picture during bring-up | SPI clock too fast | SPI clock lowered to **5 MHz** (upstream: 20 MHz). Faster may work; I haven't tested it |
| Upstream init table is for other ST7789 panels | Voltages, gamma and timing differ per panel | Replaced with the panel vendor's register values (below). VCOMS is the vendor's 0x13; an earlier 0x23 was less stable |

Also needed: `CONFIG_LV_COLOR_16_SWAP=y` (LVGL's RGB565 bytes must be swapped for SPI), and 320x240 landscape (`CONFIG_LV_DISPLAY_ORIENTATION_LANDSCAPE=y`, `LV_HOR_RES_MAX`/`LV_VER_RES_MAX` in `lvgl_helpers.h`).

## Init sequence

Sent once at start-up, after the software reset. Values come from the panel vendor's example code; the descriptions are mine, based on the ST7789 command set.

| Cmd | Name | Data | What it does |
| --- | --- | --- | --- |
| `0x36` | MADCTL | `00` | Memory access order. Overwritten right after the table by the orientation setting (`0x60` for landscape). |
| `0x3A` | COLMOD | `05` | 16 bits per pixel (RGB565). |
| `0xB2` | PORCTRL | `0C 0C 00 33 33` | Porch (blanking) timing. |
| `0xB7` | GCTRL | `74` | Gate driver voltages (VGH / VGL). |
| `0xBB` | VCOMS | `13` | VCOM voltage. |
| `0xC0` | LCMCTRL | `2C` | LCM control. |
| `0xC2` | VDVVRHEN | `01` | Take VDV and VRH from the next two commands. |
| `0xC3` | VRHS | `10` | VRH: gamma reference voltage. |
| `0xC4` | VDVSET | `20` | VDV: voltage offset. |
| `0xC6` | FRCTRL2 | `0F` | Frame rate in normal mode (0x0F is 60 Hz). |
| `0xD0` | PWCTRL1 | `A4 A1` | Power control 1 (AVDD / AVCL / VDS). |
| `0xD6` | – | `A1` | Vendor-specific; kept as the vendor had it. |
| `0xE0` | PVGAMCTRL | `D0 07 0E 0B 0A 14 38 33 4F 37 16 16 2A 2E` | Positive gamma curve. |
| `0xE1` | NVGAMCTRL | `D0 0B 10 08 08 06 35 54 4D 0A 14 14 2C 2F` | Negative gamma curve. |
| `0xE9` | EQCTRL | `11 11 03` | Equalize time control. |
| `0x21` | INVON | – | Display inversion on (IPS). |
| `0x11` | SLPOUT | – | Leave sleep mode, then wait 100 ms. |
| `0x29` | DISPON | – | Display on, then wait 100 ms. |

After that, every LVGL flush sets the window with `CASET` (0x2A) / `RASET` (0x2B) and streams pixels with `RAMWR` (0x2C).

## Orientation

`st7789_set_orientation()` picks a MADCTL value per orientation. This board uses **landscape (0x60)**. The two portrait entries are swapped compared to upstream (`0x00` / `0xC0` instead of `0xC0` / `0x00`) to suit this panel; landscape is the same as upstream.

## Touch

The XPT2046 touch controller shares the SPI bus (MOSI 13, MISO 12, CLK 14) with its own chip select (GPIO33) and an IRQ line on GPIO36. Calibration values are in `firmware/sdkconfig.defaults` (`CONFIG_LV_TOUCH_X_MIN` ... `CONFIG_LV_TOUCH_INVERT_Y`). The current UI does not use touch, but the input device is registered with LVGL.
