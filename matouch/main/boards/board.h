#pragma once

#include <esp_log.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <driver/spi_common.h>
#include <driver/ledc.h>
#include <esp_log.h>
#include <driver/i2c_master.h>
#include "lcd_display.h"
#include "esp_lvgl_port.h"

#include "bsp.h"

typedef struct board_t* board_handle_t;

struct board_t {

#if CONFIG_PCF85063A_ENABLE
    bsp_pcf85063a_handle_t pcf85063a;
#endif

#if CONFIG_PCF8563_ENABLE
    bsp_pcf8563_handle_t pcf8563;
#endif

#if CONFIG_QMI8658_ENABLE
    qmi8658_handle_t qmi8658;
#endif

#if CONFIG_AUDIO_ENABLE
    audio_handle_t audio;
#endif
    int reverse;

};

extern board_handle_t board_handle;


void board_init(void);