#include <stdio.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <esp_log.h>
#include <esp_event.h>
#include <esp_sntp.h>
#include <time.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "protocol_examples_common.h"
#include "boards/board.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"
// 添加JPEG解码头文件
#include "lvgl/src/libs/tjpgd/lv_tjpgd.h"

static const char *TAG = "main";

void app_main(void)
{
    /* Initialize board */
    board_init();

    /* Initialize LVGL */
    lv_init();
    lvgl_port_init();

    /* Initialize TJPGD for JPEG decoding */
    lv_tjpgd_init();
    
    /* Display welcome text */
    lvgl_port_lock(0);
    lv_obj_t *scr = lv_screen_active();
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text(label, "Hello Makerfabs!");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lvgl_port_unlock();

    /* Run release demo */
    release_demo();
}