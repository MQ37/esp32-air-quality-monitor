idf_component_register(
    SRCS "main.c"
         "i2c_bus.c"
         "lvgl_port.c"
         "ui_air_quality.c"
         "bme680_driver.c"
         "ccs811_driver.c"
         "ens160_driver.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES driver esp_timer nvs_flash lvgl lvgl_esp32_drivers bme68x sciosense_ens160)
