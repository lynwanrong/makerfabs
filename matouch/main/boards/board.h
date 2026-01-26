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

#include "bsp_board.h"

typedef struct board_t* board_handle_t;

struct board_t {
// components/bsp/pcf85063a/bsp_pcf85063a.h
#if CONFIG_PCF85063A_ENABLE
    bsp_pcf85063a_handle_t pcf85063a_handle;
#endif
// components/bsp/pcf8563/bsp_pcf8563.h
#if CONFIG_PCF8563_ENABLE
    bsp_pcf8563_handle_t pcf8563_handle;
#endif
// components/bsp/qmi8658/bsp_qmi8658.h
#if CONFIG_QMI8658_ENABLE
    bsp_qmi8658_handle_t qmi8658_handle;
#endif
// components/bsp/audio/bsp_audio.h
#if CONFIG_AUDIO_ENABLE
    bsp_audio_handle_t audio_handle;
#endif
// components/bsp/sdcard/bsp_sdcard.h
#if CONFIG_SDCARD_ENABLE
    bsp_sdcard_handle_t sdcard_handle;
#endif
// components/bsp/spiffs/bsp_spiffs.h
#if CONFIG_SPIFFS_ENABLE
    bsp_spiffs_handle_t spiffs_handle;
#endif
    int reverse;

};

extern board_handle_t board_handle;


void board_init(void);