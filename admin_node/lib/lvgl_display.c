/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <lvgl_input_device.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>

// #include <lvgl.h>
#include <stdio.h>

#include "mqtt_client.h"

// #include "lv_symbol_def.h"

int run_lvgl_display(void);

#ifndef LV_SYMBOL_LOCK
#define LV_SYMBOL_LOCK "\xEF\x80\xA3" 
#endif
#ifndef LV_SYMBOL_UNLOCK
#define LV_SYMBOL_UNLOCK "\xEF\x82\x13"
#endif
#ifndef LV_SYMBOL_USER
#define LV_SYMBOL_USER "\xEF\x80\x87"
#endif
#ifndef LV_SYMBOL_EYE_OPEN
#define LV_SYMBOL_EYE_OPEN "\xEF\x81\xAE"
#endif
#ifndef LV_SYMBOL_EYE_CLOSE
#define LV_SYMBOL_EYE_CLOSE "\xEF\x81\xB0"
#endif
#ifndef LV_SYMBOL_WARNING
#define LV_SYMBOL_WARNING "\xEF\x81\xB1"
#endif
#ifndef LV_SYMBOL_DROPLET
#define LV_SYMBOL_DROPLET "\xEF\x8B\x83"
#endif

LOG_MODULE_REGISTER(app);

typedef enum {
    SCREEN_SENSORS,
    SCREEN_HOME,
    SCREEN_SECURITY, // Add more screens as needed
    SCREEN_LOG,
} screen_id_t;

static lv_obj_t *screen1;
static lv_obj_t *screen2;
static lv_obj_t *screen3;
static lv_obj_t *screen4;

static lv_obj_t *lbl_door_status;
static lv_obj_t *lbl_door_open_status;

static lv_obj_t *lbl_motion;
static lv_obj_t *lbl_face_id;
static lv_obj_t *lbl_face_status;
static lv_obj_t *lbl_code_status;
static lv_obj_t *lbl_unlock_attempt;

static lv_style_t style_locked;
static lv_style_t style_unlocked;
static lv_style_t style_warning;
static lv_style_t style_valid;
static lv_style_t style_invalid;

// Style declarations (outside of functions, globally)
static lv_style_t style_locked;
static lv_style_t style_unlocked;
static lv_style_t style_warning;
static lv_style_t style_valid;
static lv_style_t style_invalid;

#define MAX_LOG_ENTRIES 5

static lv_obj_t *log_container;
static lv_style_t style_log;
static lv_obj_t *log_entries[MAX_LOG_ENTRIES];
static int log_index = 0;

// Call this once during setup (e.g., in create_security_screen or init function)
void init_styles(void) {
    lv_style_init(&style_locked);
    lv_style_set_text_color(&style_locked, lv_color_hex(0x00FF00)); // Green

    lv_style_init(&style_unlocked);
    lv_style_set_text_color(&style_unlocked, lv_color_hex(0xFF0000)); // Red

    lv_style_init(&style_warning);
    lv_style_set_text_color(&style_warning, lv_color_hex(0xFFA500)); // Orange

    lv_style_init(&style_valid);
    lv_style_set_text_color(&style_valid, lv_color_hex(0x00FFFF)); // Cyan

    lv_style_init(&style_invalid);
    lv_style_set_text_color(&style_invalid, lv_color_hex(0xFF00FF)); // Magenta
}

void add_log_entry(const char *message) {
    // If at max, remove the oldest log entry
    if (log_index >= MAX_LOG_ENTRIES) {
        lv_obj_del(log_entries[0]); // delete the oldest
        memmove(&log_entries[0], &log_entries[1], sizeof(lv_obj_t*) * (MAX_LOG_ENTRIES - 1));
        log_index = MAX_LOG_ENTRIES - 1;
    }

    // Create new label and add to container
    lv_obj_t *log_label = lv_label_create(log_container);
    lv_label_set_text(log_label, message);
    lv_obj_add_style(log_label, &style_log, 0);
    log_entries[log_index++] = log_label;

    // Scroll to bottom
    lv_obj_scroll_to_y(log_container, lv_obj_get_scroll_bottom(log_container), LV_ANIM_ON);
}

void update_door_status(bool is_locked) {
    lv_label_set_text(lbl_door_status, is_locked ? LV_SYMBOL_LOCK " Door: Locked" : LV_SYMBOL_UNLOCK " Door: Unlocked");
    lv_obj_remove_style(lbl_door_status, &style_locked, LV_PART_MAIN);
    lv_obj_remove_style(lbl_door_status, &style_unlocked, LV_PART_MAIN);
    lv_obj_add_style(lbl_door_status, is_locked ? &style_locked : &style_unlocked, 0);
}

void update_door_position(bool is_open) {
    if (is_open) {
        lv_label_set_text(lbl_door_open_status, LV_SYMBOL_LEFT " Door: Open");
        lv_obj_remove_style(lbl_door_open_status, &style_warning, LV_PART_MAIN);
        lv_obj_remove_style(lbl_door_open_status, &style_valid, LV_PART_MAIN);
        lv_obj_add_style(lbl_door_open_status, &style_warning, LV_PART_MAIN);
    } else {
        lv_label_set_text(lbl_door_open_status, LV_SYMBOL_RIGHT " Door: Closed");
        lv_obj_remove_style(lbl_door_open_status, &style_warning, LV_PART_MAIN);
        lv_obj_remove_style(lbl_door_open_status, &style_valid, LV_PART_MAIN);
        lv_obj_add_style(lbl_door_open_status, &style_valid, LV_PART_MAIN);
    }
}

void update_motion_detected(bool motion) {
    lv_label_set_text(lbl_motion, motion ? LV_SYMBOL_EYE_OPEN " Motion: Detected" : LV_SYMBOL_EYE_CLOSE " Motion: None");
    lv_obj_remove_style(lbl_motion, &style_valid, LV_PART_MAIN);
    lv_obj_remove_style(lbl_motion, &style_warning, LV_PART_MAIN);
    lv_obj_add_style(lbl_motion, motion ? &style_warning : &style_valid, 0);
}

void update_face_recognition(const char *name, bool validated) {
    char buf[64];
    snprintf(buf, sizeof(buf), LV_SYMBOL_USER " Person: %s", name);
    lv_label_set_text(lbl_face_id, buf);
    lv_obj_remove_style(lbl_face_id, &style_warning, LV_PART_MAIN);
    lv_obj_remove_style(lbl_face_id, &style_valid, LV_PART_MAIN);
    lv_obj_add_style(lbl_face_id, validated ? &style_valid : &style_warning, 0);

    lv_label_set_text(lbl_face_status, validated ? LV_SYMBOL_OK " Face: Validated" : LV_SYMBOL_CLOSE " Face: Rejected");
    lv_obj_remove_style(lbl_face_status, &style_valid, LV_PART_MAIN);
    lv_obj_remove_style(lbl_face_status, &style_invalid, LV_PART_MAIN);
    lv_obj_add_style(lbl_face_status, validated ? &style_valid : &style_invalid, 0);
}

void update_code_status(bool valid) {
    lv_label_set_text(lbl_code_status, valid ? LV_SYMBOL_OK " Code: Valid" : LV_SYMBOL_CLOSE " Code: Invalid");
    lv_obj_remove_style(lbl_code_status, &style_valid, LV_PART_MAIN);
    lv_obj_remove_style(lbl_code_status, &style_invalid, LV_PART_MAIN);
    lv_obj_add_style(lbl_code_status, valid ? &style_valid : &style_invalid, 0);
}

void update_unlock_attempt(const char *name) {
    char buf[64];
    snprintf(buf, sizeof(buf), LV_SYMBOL_WARNING " Attempt: %s attempting", name);
    lv_label_set_text(lbl_unlock_attempt, buf);
    lv_obj_remove_style(lbl_unlock_attempt, &style_warning, LV_PART_MAIN);
    lv_obj_add_style(lbl_unlock_attempt, &style_warning, 0);
}

void load_screen(screen_id_t id) {
    switch(id) {
        case SCREEN_SENSORS:
            lv_scr_load_anim(screen1, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, false);
            break;
        case SCREEN_HOME:
            lv_scr_load_anim(screen2, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, false);
            break;
        case SCREEN_SECURITY:
            lv_scr_load_anim(screen3, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, false);
            break;
        case SCREEN_LOG:
            lv_scr_load_anim(screen4, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, false);
            break;

        // Add more screens here
    }
}

static void switch_to_home_screen_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        load_screen(SCREEN_HOME);
    }
}

static void switch_to_sensor_screen_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        load_screen(SCREEN_SENSORS);
    }
}

static void switch_to_log_screen_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        load_screen(SCREEN_LOG);
    }
}

static void switch_to_security_screen_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        load_screen(SCREEN_SECURITY);
    }
}

static lv_obj_t *create_home_screen(void) {
    
    lv_obj_t *scr = lv_obj_create(NULL);

    // SLARM Title
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "SLARM");
    // lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);  // Large font
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);  // Top center

    // Create a container for the buttons
    // lv_obj_t *btn_container = lv_obj_create(scr);
    // lv_obj_set_size(btn_container, 220, 180);
    // lv_obj_align(btn_container, LV_ALIGN_CENTER, 0, 20);
    // lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_COLUMN);  // Vertical layout
    // lv_obj_set_style_pad_row(btn_container, 10, 0);             // Space between buttons
    // lv_obj_set_scrollbar_mode(btn_container, LV_SCROLLBAR_MODE_OFF);

    // Create buttons with labels
    struct {
        const char *text;
        lv_event_cb_t callback;
    } buttons[] = {
        {"Security", switch_to_security_screen_btn_event_cb},
        {"Sensors", switch_to_sensor_screen_btn_event_cb},
        {"Logs",     switch_to_log_screen_btn_event_cb}
    };

    for (int i = 0; i < 3; i++) {
        lv_obj_t *btn = lv_btn_create(scr);
        lv_obj_set_size(btn, 200, 50);
        lv_obj_add_event_cb(btn, buttons[i].callback, LV_EVENT_CLICKED, NULL);

        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, buttons[i].text);
        lv_obj_center(label);
        lv_obj_align(btn, LV_ALIGN_CENTER, 0, (i - 1) * 60);  // Adjust vertical position
    }

    return scr;
}

static lv_obj_t *create_security_screen(void) {
    
    lv_obj_t *scr = lv_obj_create(NULL);
    init_styles(); // Initialize styles

    // Create screen

    // Door status
    lbl_door_status = lv_label_create(scr);
    lv_label_set_text(lbl_door_status, LV_SYMBOL_LOCK " Door: Locked");
    lv_obj_add_style(lbl_door_status, &style_locked, 0);
    lv_obj_set_pos(lbl_door_status, 10, 20);

    // Motion detection
    lbl_motion = lv_label_create(scr);
    lv_label_set_text(lbl_motion, LV_SYMBOL_EYE_OPEN " Motion: None");
    lv_obj_add_style(lbl_motion, &style_valid, 0);
    lv_obj_set_pos(lbl_motion, 10, 50);

    // Face ID
    lbl_face_id = lv_label_create(scr);
    lv_label_set_text(lbl_face_id, LV_SYMBOL_USER " Person: Unknown");
    lv_obj_add_style(lbl_face_id, &style_warning, 0);
    lv_obj_set_pos(lbl_face_id, 10, 80);

    // Face validation
    lbl_face_status = lv_label_create(scr);
    lv_label_set_text(lbl_face_status, LV_SYMBOL_REFRESH " Face: Not validated");
    lv_obj_add_style(lbl_face_status, &style_invalid, 0);
    lv_obj_set_pos(lbl_face_status, 10, 110);

    // Code status
    lbl_code_status = lv_label_create(scr);
    lv_label_set_text(lbl_code_status, LV_SYMBOL_KEYBOARD " Code: ---");
    lv_obj_add_style(lbl_code_status, &style_warning, 0);
    lv_obj_set_pos(lbl_code_status, 10, 140);

    // Unlock attempt
    lbl_unlock_attempt = lv_label_create(scr);
    lv_label_set_text(lbl_unlock_attempt, LV_SYMBOL_WARNING " Attempt: None");
    lv_obj_add_style(lbl_unlock_attempt, &style_warning, 0);
    lv_obj_set_pos(lbl_unlock_attempt, 10, 170);

    lbl_door_open_status = lv_label_create(scr);
    lv_label_set_text(lbl_door_open_status, LV_SYMBOL_MINUS " Door_Open: Unknown");
    lv_obj_add_style(lbl_door_open_status, &style_warning, 0);
    lv_obj_align(lbl_door_open_status, LV_ALIGN_TOP_LEFT, 10, 200);  // adjust as needed

    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);  // set screen background to black

    lv_obj_t *btn = lv_btn_create(scr);
    lv_obj_center(btn);
    lv_obj_add_event_cb(btn, switch_to_log_screen_btn_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_align(btn, LV_ALIGN_RIGHT_MID, 0, 0);

    lv_obj_t *btn2 = lv_btn_create(scr);
    lv_obj_center(btn2);
    lv_obj_add_event_cb(btn2, switch_to_sensor_screen_btn_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_align(btn2, LV_ALIGN_TOP_RIGHT, 0, 0);


    lv_obj_t *label1 = lv_label_create(scr);
    lv_label_set_text(label1, "Security Screen");

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, "View Logs");
    lv_obj_center(label);

    lv_obj_t *label2 = lv_label_create(btn2);
    lv_label_set_text(label2, "View Sensor Data");
    lv_obj_center(label2);

    return scr;
}

static lv_obj_t *create_log_screen(void) {
    
    // Create a scrollable container for logs
    // Initialize log style
    lv_obj_t *scr = lv_obj_create(NULL);

    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);  // set screen background to black

    lv_style_init(&style_log);
    lv_style_set_text_color(&style_log, lv_color_hex(0xFFFFFF)); // White

    // Create a scrollable container for logs
    log_container = lv_obj_create(scr);
    lv_obj_set_size(log_container, 280, 90);  // Adjust size as needed
    lv_obj_align(log_container, LV_ALIGN_BOTTOM_MID, 0, -5);

    // Enable vertical scrolling
    lv_obj_set_scroll_dir(log_container, LV_DIR_VER);

    // Set background and border styles
    lv_obj_set_style_bg_color(log_container, lv_color_hex(0x202020), 0);
    lv_obj_set_style_border_width(log_container, 1, 0);
    lv_obj_set_style_border_color(log_container, lv_color_hex(0xFFFFFF), 0);

    lv_obj_set_layout(log_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(log_container, LV_FLEX_FLOW_COLUMN);  // Stack items top to bottom
    lv_obj_set_flex_align(log_container,
                        LV_FLEX_ALIGN_START,  // Main axis: start (top)
                        LV_FLEX_ALIGN_START,  // Cross axis: start (left)
                        LV_FLEX_ALIGN_START); // Track alignment

    // Optional: spacing between log entries
    lv_obj_set_style_pad_row(log_container, 4, 0);  // 4px row spacing


    lv_obj_t *btn = lv_btn_create(scr);
    lv_obj_center(btn);
    lv_obj_add_event_cb(btn, switch_to_security_screen_btn_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 0, 20);

    lv_obj_t *label1 = lv_label_create(scr);
    lv_label_set_text(label1, "Log Entries");

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, "Back to Security");
    lv_obj_center(label);

    return scr;
}

// static lv_obj_t *sensor_chart;
// static lv_chart_series_t *temp_series;
// static lv_chart_series_t *humidity_series;
// static lv_chart_series_t *air_quality_series;

static lv_obj_t *temp_box;
static lv_obj_t *humidity_box;
static lv_obj_t *air_box;

lv_obj_t *create_sensor_screen(void) {

    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

    // TITLE
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Environmental Dashboard");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // GRID LAYOUT (2x2)
    static lv_coord_t col_dsc[] = { LV_PCT(50), LV_PCT(50), LV_GRID_TEMPLATE_LAST };
    static lv_coord_t row_dsc[] = { LV_PCT(50), LV_PCT(50), LV_GRID_TEMPLATE_LAST };
    lv_obj_t *grid = lv_obj_create(scr);
    lv_obj_set_size(grid, lv_pct(100), lv_pct(85));
    lv_obj_align(grid, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);  // transparent background

    
    // TEMPERATURE BOX
    temp_box = lv_label_create(grid);
    lv_label_set_text(temp_box, LV_SYMBOL_HOME " Temp: 25°C");
    lv_obj_set_style_text_color(temp_box, lv_color_hex(0xFFA500), 0);
    lv_obj_set_style_text_font(temp_box, &lv_font_montserrat_14, 0);
    lv_obj_set_grid_cell(temp_box, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 0, 1);

    // HUMIDITY BOX
    humidity_box = lv_label_create(grid);
    lv_label_set_text(humidity_box, LV_SYMBOL_DROPLET " Humidity: 60%");
    lv_obj_set_style_text_color(humidity_box, lv_color_hex(0x00BFFF), 0);
    lv_obj_set_style_text_font(humidity_box, &lv_font_montserrat_14, 0);
    lv_obj_set_grid_cell(humidity_box, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_START, 0, 1);

    // AIR QUALITY BOX
    air_box = lv_label_create(grid);
    lv_label_set_text(air_box, LV_SYMBOL_WARNING " Air Quality: Good");
    lv_obj_set_style_text_color(air_box, lv_color_hex(0x90EE90), 0);
    lv_obj_set_style_text_font(air_box, &lv_font_montserrat_14, 0);
    lv_obj_set_grid_cell(air_box, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 1, 1);

    // // CHART (bottom-right)
    // sensor_chart = lv_chart_create(grid);
    // lv_obj_set_size(sensor_chart, 140, 100);
    // lv_chart_set_type(sensor_chart, LV_CHART_TYPE_LINE);
    // lv_chart_set_update_mode(sensor_chart, LV_CHART_UPDATE_MODE_SHIFT);
    // lv_chart_set_range(sensor_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    // lv_chart_set_div_line_count(sensor_chart, 4, 4);
    // lv_obj_set_style_bg_color(sensor_chart, lv_color_hex(0x222222), 0);
    // lv_obj_set_style_border_color(sensor_chart, lv_color_hex(0x888888), 0);
    // lv_obj_set_grid_cell(sensor_chart, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);

    // temp_series = lv_chart_add_series(sensor_chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    // humidity_series = lv_chart_add_series(sensor_chart, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_PRIMARY_Y);
    // air_quality_series = lv_chart_add_series(sensor_chart, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);

    // // Fill with initial values
    // for (int i = 0; i < 10; ++i) {
    //     lv_chart_set_next_value(sensor_chart, temp_series, 24 + (i % 3));
    //     lv_chart_set_next_value(sensor_chart, humidity_series, 58 + (i % 5));
    //     lv_chart_set_next_value(sensor_chart, air_quality_series, 40 + (i % 4));
    // }

    lv_obj_t *btn = lv_btn_create(scr);
    lv_obj_center(btn);
    lv_obj_add_event_cb(btn, switch_to_home_screen_btn_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, -10, -20);


    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, "Home");
    lv_obj_center(label);

    return scr;
}

void update_sensor_data(char* temp, char* humidity, char* air_quality) {

    char buf[32];
    
    // Update temperature
    snprintf(buf, sizeof(buf), LV_SYMBOL_HOME " Temp: %s°C", temp);
    lv_label_set_text(temp_box, buf);

    // Update humidity
    snprintf(buf, sizeof(buf),  LV_SYMBOL_DROPLET " Humidity: %s %%", humidity);
    lv_label_set_text(humidity_box, buf);

    // // Update air quality
    // if (air_quality <= 50) {
    //     lv_label_set_text(air_box, "Air Quality: Good \U0001F60A");
    // } else if (air_quality <= 100) {
    //     lv_label_set_text(air_box, "Air Quality: Moderate \U0001F610");
    // } else {
    //     lv_label_set_text(air_box, "Air Quality: Poor \U0001F637");
    // }
    snprintf(buf, sizeof(buf), LV_SYMBOL_WARNING " Air Quality: %s", air_quality);
    lv_label_set_text(air_box, buf);

    // lv_chart_set_next_value(sensor_chart, temp_series, temp);
    // lv_chart_set_next_value(sensor_chart, humidity_series, humidity);
    // lv_chart_set_next_value(sensor_chart, air_quality_series, air_quality);
}

int run_lvgl_display(void) {

	const struct device *display_dev;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
		return 0;
	}

    screen1 = create_sensor_screen();
    screen2 = create_home_screen();
    screen3 = create_security_screen();  // If SCREEN_SECURITY = 2, for example
    screen4 = create_log_screen();  // If SCREEN_LOG = 3, for example

    load_screen(SCREEN_HOME);

    lv_timer_handler();
	display_blanking_off(display_dev);

    // Main loop
	while (1) {

        mqtt_lvgl_data_t *data = k_fifo_get(&mqtt_lvgl_fifo, K_NO_WAIT);
        if (data) {
            // Use the data
            update_door_status(data->locked);
            update_door_position(data->open);
            update_motion_detected(data->motion_detected);
            update_face_recognition(data->face_name, data->face_validated);
            update_code_status(data->pin_validated);
            update_unlock_attempt(data->face_name);

            if (data->new_attempt) {

                char log_buf[64];  // Adjust size if needed
                char log_buf2[64];  // Adjust size if needed


                snprintf(log_buf, sizeof(log_buf), "%s: %s, %s",
                    data->face_name,
                    data->face_validated ? "Face Validated" : "Face Rejected",
                    data->pin_validated ? "PIN Validated" : "PIN Invalid"
                );
                add_log_entry(log_buf);

                snprintf(log_buf2, sizeof(log_buf2),
                    "Door %s", data->open ? "Opened" : "Closed"
                );
                add_log_entry(log_buf2);
            }

            update_sensor_data(data->temperature, data->humidity, data->air_quality);
            
            // Update UI accordingly

            // Free memory after processing
            k_free(data);
        
        }

		lv_timer_handler();
		k_sleep(K_MSEC(10));
	}
    return 0;
    
}