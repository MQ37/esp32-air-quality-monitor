/*
 * ui_air_quality.c - landscape 320x240 screen with four cards:
 * Temp, Humidity, eCO2, TVOC. Each card shows a value and a short label.
 */
#include "ui_air_quality.h"

#include <stdio.h>

#include "lvgl.h"

static lv_obj_t * label_temp_val;
static lv_obj_t * label_humi_val;
static lv_obj_t * label_eco2_val;
static lv_obj_t * label_tvoc_val;

static lv_obj_t * label_temp_status;
static lv_obj_t * label_humi_status;
static lv_obj_t * label_eco2_status;
static lv_obj_t * label_tvoc_status;

// Label thresholds. These are rough comfort/indoor-air rules of thumb, not
// official standards; tweak them to taste.
static const char* get_temp_status(float temp)
{
    if (temp < 18.0) return "Cold";
    if (temp < 20.0) return "Cool";
    if (temp < 26.0) return "Comfortable";
    if (temp < 28.0) return "Warm";
    return "Hot";
}

static const char* get_humidity_status(float humi)
{
    if (humi < 30.0) return "Dry";
    if (humi < 40.0) return "Low";
    if (humi < 60.0) return "Optimal";
    if (humi < 70.0) return "High";
    return "Humid";
}

static const char* get_eco2_status(uint16_t eco2)
{
    if (eco2 < 600) return "Excellent";
    if (eco2 < 800) return "Good";
    if (eco2 < 1000) return "Fair";
    if (eco2 < 1500) return "Poor";
    return "Very Poor";
}

static const char* get_tvoc_status(uint16_t tvoc)
{
    if (tvoc < 100) return "Excellent";
    if (tvoc < 200) return "Good";
    if (tvoc < 400) return "Fair";
    if (tvoc < 600) return "Poor";
    return "Very Poor";
}

static lv_obj_t * create_card(lv_obj_t * parent, const char * title, const char * default_val,
                              lv_obj_t ** val_label_ptr, lv_obj_t ** status_label_ptr, lv_color_t color)
{
    lv_obj_t * card = lv_obj_create(parent);
    // Four cards in a 2x2 grid on the 320x240 screen
    lv_obj_set_size(card, 145, 95);
    lv_obj_set_style_bg_color(card, color, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_radius(card, 8, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    // Title
    lv_obj_t * title_label = lv_label_create(card);
    lv_label_set_text(title_label, title);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 3);
    lv_obj_set_style_text_color(title_label, lv_color_white(), 0);

    // Value
    *val_label_ptr = lv_label_create(card);
    lv_label_set_text(*val_label_ptr, default_val);
    lv_obj_align(*val_label_ptr, LV_ALIGN_CENTER, 0, -5);
    lv_obj_set_style_text_font(*val_label_ptr, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(*val_label_ptr, lv_color_white(), 0);

    // Status label
    *status_label_ptr = lv_label_create(card);
    lv_label_set_text(*status_label_ptr, "--");
    lv_obj_align(*status_label_ptr, LV_ALIGN_BOTTOM_MID, 0, -3);
    lv_obj_set_style_text_color(*status_label_ptr, lv_color_white(), 0);

    return card;
}

void ui_air_quality_init(void)
{
    lv_obj_t * scr = lv_scr_act();
    lv_obj_clean(scr);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x101010), 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // Grid Container
    lv_obj_t * grid_cont = lv_obj_create(scr);
    lv_obj_set_size(grid_cont, 310, 230);
    lv_obj_align(grid_cont, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(grid_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid_cont, 0, 0);
    lv_obj_set_style_pad_all(grid_cont, 0, 0);
    lv_obj_set_style_pad_gap(grid_cont, 6, 0); // Gap between cards
    lv_obj_clear_flag(grid_cont, LV_OBJ_FLAG_SCROLLABLE);

    // Flex Layout: Row Wrap
    lv_obj_set_flex_flow(grid_cont, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Create Cards
    create_card(grid_cont, "Temp", "-- C", &label_temp_val, &label_temp_status, lv_color_hex(0xE67E22));
    create_card(grid_cont, "Humidity", "-- %", &label_humi_val, &label_humi_status, lv_color_hex(0x3498DB));
    create_card(grid_cont, "eCO2", "-- ppm", &label_eco2_val, &label_eco2_status, lv_color_hex(0x27AE60));
    create_card(grid_cont, "TVOC", "-- ppb", &label_tvoc_val, &label_tvoc_status, lv_color_hex(0x9B59B6));
}

void ui_air_quality_update(float temp, float humi, uint16_t eco2, uint16_t tvoc)
{
    static char buf[32];

    // Update temperature
    if(label_temp_val) {
        snprintf(buf, sizeof(buf), "%.1f C", temp);
        lv_label_set_text(label_temp_val, buf);
    }
    if(label_temp_status) {
        lv_label_set_text(label_temp_status, get_temp_status(temp));
    }

    // Update humidity
    if(label_humi_val) {
        snprintf(buf, sizeof(buf), "%.1f %%", humi);
        lv_label_set_text(label_humi_val, buf);
    }
    if(label_humi_status) {
        lv_label_set_text(label_humi_status, get_humidity_status(humi));
    }

    // Update eCO2 (show "Warming" if no sensor data available)
    if(label_eco2_val) {
        if (eco2 == 0) {  // No sensor ready yet
            lv_label_set_text(label_eco2_val, "Warming");
        } else {
            snprintf(buf, sizeof(buf), "%u ppm", eco2);
            lv_label_set_text(label_eco2_val, buf);
        }
    }
    if(label_eco2_status) {
        if (eco2 == 0) {
            lv_label_set_text(label_eco2_status, "Please wait");
        } else {
            lv_label_set_text(label_eco2_status, get_eco2_status(eco2));
        }
    }

    // Update TVOC (show "Warming" if no sensor data available)
    if(label_tvoc_val) {
        if (tvoc == 0) {  // No sensor ready yet
            lv_label_set_text(label_tvoc_val, "Warming");
        } else {
            snprintf(buf, sizeof(buf), "%u ppb", tvoc);
            lv_label_set_text(label_tvoc_val, buf);
        }
    }
    if(label_tvoc_status) {
        if (tvoc == 0) {
            lv_label_set_text(label_tvoc_status, "Please wait");
        } else {
            lv_label_set_text(label_tvoc_status, get_tvoc_status(tvoc));
        }
    }
}
