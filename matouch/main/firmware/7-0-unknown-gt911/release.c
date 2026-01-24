#include "template.h"



// static const char *TAG = "RELEASE";

static void release_task(void *arg)
{
    // template_image_switch_lvgl("/sdcard/images");
    // template_lvgl_demos_test();  
    vTaskDelete(NULL);
}


void release_demo(void)
{
    xTaskCreate(release_task,
                "release_task",
                1024 * 10,
                NULL,
                5,
                NULL);
}
