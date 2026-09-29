/*
 * lvgl_port.c - LVGL display + input registration and tick source.
 *
 * The heavy lifting (SPI bus, ST7789 init sequence, XPT2046 reads) lives in
 * components/lvgl_esp32_drivers. This file only wires it into LVGL.
 */
#include "lvgl_port.h"

#include "esp_timer.h"
#include "lvgl.h"
#include "lvgl_helpers.h"

#define LVGL_TICK_PERIOD_MS  10
#define LVGL_BUF_LINES       20   /* draw buffer height; 320 x 20 px x 2 bytes = 12.5 KiB */

static void lvgl_tick_cb(void *arg)
{
    (void)arg;
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

void lvgl_port_init(void)
{
    lv_init();

    /* SPI bus + display controller + touch controller, configured via Kconfig */
    lvgl_driver_init();

    /* One partial draw buffer. LV_HOR_RES_MAX/LV_VER_RES_MAX are 320x240
     * (landscape), see lvgl_helpers.h. */
    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf[LV_HOR_RES_MAX * LVGL_BUF_LINES];
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, LV_HOR_RES_MAX * LVGL_BUF_LINES);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LV_HOR_RES_MAX;
    disp_drv.ver_res = LV_VER_RES_MAX;
    disp_drv.flush_cb = disp_driver_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

#if CONFIG_LV_TOUCH_CONTROLLER
    /* The UI does not react to touch yet, but the XPT2046 is registered so
     * adding buttons later just works. */
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_driver_read;
    lv_indev_drv_register(&indev_drv);
#endif

    const esp_timer_create_args_t tick_args = {
        .callback = &lvgl_tick_cb,
        .name = "lvgl_tick",
    };
    esp_timer_handle_t tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&tick_args, &tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, LVGL_TICK_PERIOD_MS * 1000));
}
