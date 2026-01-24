#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/ringbuf.h>
#include <freertos/event_groups.h>
#include <esp_err.h>
#include <esp_log.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <esp_check.h>
#include <driver/gpio.h>

#include "esp_lvgl_port.h"
#include "lv_demos.h"
#include "link_list_menu.h"

#ifdef __cplusplus
extern "C" {
#endif

// lvgl demo functions
void template_lvgl_demos_test(void);

// image switcher from SD card
void template_image_switch_lvgl(const char *image_dir_path);

#ifdef __cplusplus
}
#endif