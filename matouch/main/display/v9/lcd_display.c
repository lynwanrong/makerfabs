#include <esp_log.h>


#include "lcd_display.h"
#include "esp_lvgl_port.h"

static const char *TAG = "lcd_display";

lv_display_t *spi_lcd_display(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel,
                           int width, int height, int offset_x, int offset_y,
                           bool mirror_x, bool mirror_y, bool swap_xy)
{
    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();

    ESP_LOGI(TAG, "Initialize LVGL port");
    lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    port_cfg.task_priority = 1;
#if CONFIG_SOC_CPU_CORES_NUM > 1
    port_cfg.task_affinity = 1;
#endif
    lvgl_port_init(&port_cfg);

    ESP_LOGI(TAG, "Add display screen");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = panel_io,
        .panel_handle = panel,
        .control_handle = NULL,
        .buffer_size = height / 10 * width, // Buffer for 10 lines
        .double_buffer = 1,
        .hres = (uint32_t)width,
        .vres = (uint32_t)height,
        .monochrome = false,
        /* Rotation values must be same as used in esp_lcd for initial settings of the screen */
        .rotation = {
            .swap_xy = swap_xy,
            .mirror_x = mirror_x,
            .mirror_y = mirror_y,
        },
        .color_format = LV_COLOR_FORMAT_RGB565,
        .flags = {
            .buff_dma = 1,
            .buff_spiram = 0,
            .sw_rotate = 0,
#if CONFIG_BOARD_TYPE_3_5_ili9488_8080 || CONFIG_BOARD_TYPE_3_5_spi_s3
            .swap_bytes = 0,
#else
            .swap_bytes = 1,
#endif
            .full_refresh = 0,
            .direct_mode = 0,
        }
    };
    lv_display_t *display = lvgl_port_add_disp(&disp_cfg);

    if(offset_x != 0 || offset_y != 0) {
        lv_display_set_offset(display, offset_x, offset_y);
    }

    return display;
}


lv_display_t *rgb_lcd_display(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel,
                           int width, int height, int offset_x, int offset_y,
                           bool mirror_x, bool mirror_y, bool swap_xy)
{
    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();

    ESP_LOGI(TAG, "Initialize LVGL port");
    lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    port_cfg.task_priority = 1;
    port_cfg.timer_period_ms = 50;
    lvgl_port_init(&port_cfg);

    ESP_LOGI(TAG, "Add display screen");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = panel_io,
        .panel_handle = panel,
        .control_handle = NULL,
        .buffer_size = 20 * width,
        .double_buffer = 1,
        .hres = (uint32_t)width,
        .vres = (uint32_t)height,
        .monochrome = false,
        /* Rotation values must be same as used in esp_lcd for initial settings of the screen */
        .rotation = {
            .swap_xy = swap_xy,
            .mirror_x = mirror_x,
            .mirror_y = mirror_y,
        },
        .color_format = LV_COLOR_FORMAT_RGB565,
        .flags = {
            .buff_dma = 1,
            .buff_spiram = 0,
            .sw_rotate = 0,
            .swap_bytes = 0,
            .full_refresh = 1,
            .direct_mode = 1,
        }
    };

    const lvgl_port_display_rgb_cfg_t rgb_cfg = {
        .flags = {
            .bb_mode = true,
            .avoid_tearing = true,
        }
    };
    lv_display_t *display = lvgl_port_add_disp_rgb(&disp_cfg, &rgb_cfg);

    if(offset_x != 0 || offset_y != 0) {
        lv_display_set_offset(display, offset_x, offset_y);
    }

    return display;
}
