#include "template.h"

// static const char *TAG = "RELEASE";

lv_obj_t *menu_screen = NULL;


ScrollingMenuItem *create_main_menu_items(void)
{
    ScrollingMenuItem *menu_head = gdlb_create_item(LV_SYMBOL_AUDIO, NULL, 1, NULL);     // 串口收发
    menu_head->next = gdlb_create_item(LV_SYMBOL_SD_CARD, NULL, 1, NULL);                // 读取挂载sd卡
    menu_head->next->next = gdlb_create_item(LV_SYMBOL_IMAGE, NULL, 1, NULL);            // 摄像头页面
    menu_head->next->next->next = gdlb_create_item(LV_SYMBOL_SETTINGS, NULL, 1, NULL);      // 设置页面
    menu_head->next->next->next->next = gdlb_create_item(LV_SYMBOL_STOP, NULL, 1, NULL); // 停止页面

    menu_head->next->next->next->sub = gdlb_create_item(LV_SYMBOL_CHARGE, NULL, 2, menu_head);              // 设置亮度页面
    menu_head->next->next->next->sub->next = gdlb_create_item(LV_SYMBOL_EYE_OPEN, NULL, 2, menu_head);      // 开发信息页面
    menu_head->next->next->next->sub->next->next = gdlb_create_item(LV_SYMBOL_SHUFFLE, NULL, 2, menu_head); // 设置屏幕反向页面

    return menu_head;
}
void testttt()
{
    if (lvgl_port_lock(100)) // 获取LVGL互斥锁
    {
        menu_screen = lv_obj_create(NULL); // 创建菜单屏幕对象
        if (menu_screen != NULL)
        {
            // 切换到菜单屏幕
            lv_scr_load(menu_screen);
            // 在菜单屏幕上创建菜单
            menu_head = create_main_menu_items(); // 调用main.c中定义的函数创建菜单项

            gdlb_create_menu(menu_head, menu_screen);      // 创建菜单容器和基础结构
            gdlb_operate_menu(menu_head);     // 操作菜单，创建菜单项UI对象
        }
        lvgl_port_unlock(); // 释放LVGL互斥锁
    }
}

static void release_task(void *arg)
{
    // template_image_switch_lvgl("/sdcard/images");
    // template_lvgl_demos_test();
    testttt();
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