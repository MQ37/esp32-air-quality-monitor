# Third-party code and licenses

The code written for this project is MIT-licensed (see [LICENSE](LICENSE)); the enclosure design files are CC BY 4.0 (see [enclosure/LICENSE](enclosure/LICENSE)). The firmware also uses the following third-party code.

| Component | Where | Version | License | Modified? |
| --- | --- | --- | --- | --- |
| [LVGL](https://github.com/lvgl/lvgl) | downloaded at build time by the ESP-IDF Component Manager ([`lvgl/lvgl`](https://components.espressif.com/components/lvgl/lvgl)), pinned in `firmware/dependencies.lock` | 8.3.11 | MIT | No |
| [lvgl_esp32_drivers](https://github.com/lvgl/lvgl_esp32_drivers) | `firmware/components/lvgl_esp32_drivers/` | `master` at commit `26fe6e7` (upstream is archived) | MIT ([LICENSE](firmware/components/lvgl_esp32_drivers/LICENSE)) | **Yes**, see below |
| [Bosch BME68x Sensor API](https://github.com/boschsensortec/BME68x_SensorAPI) | `firmware/components/bme68x/` | v4.4.8 | BSD-3-Clause ([LICENSE](firmware/components/bme68x/LICENSE)) | No (files are byte-identical to the v4.4.8 tag) |
| [ScioSense ENS16x Arduino library](https://github.com/sciosense/ens16x-arduino), the plain-C part in `src/lib/ens16x/` | `firmware/components/sciosense_ens160/` | 2.0.5 | MIT ([LICENSE](firmware/components/sciosense_ens160/LICENSE)) | No (byte-identical `ScioSense_Ens160*.h`) |
| [ESP-IDF](https://github.com/espressif/esp-idf) | not included; you install it or use the Docker image | 5.5.2 | Apache-2.0 | – |

Only the Bosch **BME68x Sensor API** (BSD-3-Clause) is used for the BME680. Bosch's separate, proprietary **BSEC** library is *not* used or included.

The `CMakeLists.txt` files in `components/bme68x/` and `components/sciosense_ens160/` were added by this project to make them ESP-IDF components.

## Changes to lvgl_esp32_drivers

The upstream component predates ESP-IDF 5 and has been archived in favour of [lvgl/lv_esp_idf](https://github.com/lvgl/lv_esp_idf). The copy here is based on upstream `master` (commit `26fe6e7`, "clarify the 52/53 px offset oddity") with these changes:

**Removed (not needed to build):** `.github/`, `.editorconfig`, `.gitignore`, `component.mk` (legacy GNU Make build), `README.md`, `lvgl_i2c/README.md`, `CONTRIBUTE_CONTROLLER_SUPPORT.md`. All driver sources were kept, so the Kconfig menus still work for other panels.

**ESP-IDF 5.x compatibility:**
- `gpio_pad_select_gpio()` → `esp_rom_gpio_pad_select_gpio()` in `lvgl_tft/st7789.c`, `lvgl_tft/ili9341.c`, `lvgl_tft/st7796s.c` and `lvgl_tft/esp_lcd_backlight.c`.
- `portTICK_RATE_MS` → `portTICK_PERIOD_MS` in `lvgl_tft/st7789.c`, `lvgl_tft/ili9341.c` and `lvgl_tft/st7796s.c`.
- `lvgl_tft/esp_lcd_backlight.c`: LEDC `bit_num` → `duty_resolution`; include `rom/gpio.h` (instead of `driver/gpio.h`) and `soc/gpio_sig_map.h`.
- `lvgl_helpers.c` (`lvgl_spi_driver_init`): per-target SPI host checks/names and `SPI_DMA_CH_AUTO`.
- `CMakeLists.txt`: add `driver` to `REQUIRES`.
- `lvgl_helpers.h`: define `LV_HOR_RES_MAX` / `LV_VER_RES_MAX` (320 x 240), which LVGL 8 no longer provides.
- `lvgl_tft/disp_spi.c`: drop the duplicate `IRAM_ATTR` on the `spi_ready` forward declaration (silences a GCC warning; the definition keeps it).

**Changes for the ESP32-32E / ST7789P3 panel** (details in [docs/display-init.md](docs/display-init.md)):
- `lvgl_spi_conf.h`: ST7789 SPI mode 2 → **0**, SPI clock 20 MHz → **5 MHz**.
- `lvgl_tft/st7789.c`: init table replaced with the panel vendor's register values (written as a commented table, no vendor code copied); software reset now waits 150 ms; MADCTL values for the two portrait orientations swapped; log via `ESP_LOGI` instead of `printf`.

## LVGL port

`firmware/main/lvgl_port.c` started from LVGL's porting templates (`lv_port_disp_templ.c` / `lv_port_indev_templ.c`, MIT) and was reduced to the few lines this board needs.
