#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/ringbuf.h>
#include <freertos/event_groups.h>
#include <math.h>
#include <esp_err.h>
#include <esp_log.h>
#include <string.h>
#include <dirent.h>
#include <esp_check.h>
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "lv_demos.h"
#include "driver/gpio.h"
#include "driver/jpeg_decode.h"
#include "driver/pulse_cnt.h"
#include "file_iterator.h"

#include "../../boards/1-28-gc9a01-cst816s/board.h"

static const char *TAG = "RELEASE";


/***************** 旋钮编码器 *****************/

#define EC11_GPIO_A     47
#define EC11_GPIO_B     48
#define EC11_GPIO_KEY   17

static pcnt_unit_handle_t pcnt_unit = NULL;
static lv_indev_t * encoder_indev = NULL;
static lv_group_t * input_group = NULL;
/*******************************************/
#define IMAGES_PATH  "/sdcard/images"

// static int image_count = 0;
// static int count_now = 0;

// static jpeg_decoder_handle_t jpgd_handle;

// // 定义全局的 LVGL 对象和图像描述符
// static lv_obj_t *img_obj = NULL;
// static lv_image_dsc_t img_dsc;
// static uint8_t *current_image_data = NULL; // 用于保存当前显示的图片数据指针
// static file_iterator_instance_t *file_iterator = NULL; // 文件迭代器句柄

// // 前向声明
// static void image_change_display(file_iterator_instance_t *ft, int index);

// /*
//     回调: 处理触摸/手势事件
// */
// void image_change_cb(lv_event_t *e)
// {
//     lv_event_code_t code = lv_event_get_code(e);
    
//     // 我们在屏幕或者容器上监听手势事件
//     if(code == LV_EVENT_GESTURE) {
//         lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        
//         bool switched = false;
//         if(dir == LV_DIR_LEFT) {
//             ESP_LOGI(TAG, "Gesture Left: Next Image");
//             count_now++;
//             if(count_now >= image_count) count_now = 0; // 循环
//             switched = true;
//         } else if(dir == LV_DIR_RIGHT) {
//             ESP_LOGI(TAG, "Gesture Right: Prev Image");
//             count_now--;
//             if(count_now < 0) count_now = image_count - 1; // 循环
//             switched = true;
//         }

//         if(switched && image_count > 0) {
//             // 在事件回调中调用显示函数，注意不要在回调里做太耗时的操作，
//             // 但解码JPEG通常还可以接受，如果太卡可以考虑放到任务里。
//             image_change_display(file_iterator, count_now);
//         }
//     }
//     // 也可以添加点击事件作为简单的切换
//     else if(code == LV_EVENT_CLICKED) {
//          ESP_LOGI(TAG, "Clicked: Next Image");
//          count_now++;
//          if(count_now >= image_count) count_now = 0;
//          if(image_count > 0) {
//              image_change_display(file_iterator, count_now);
//          }
//     }
// }
// /*
//     描述: 显示图片
// */
// static void image_change_display(file_iterator_instance_t *ft,int index)
// {
//     jpeg_decode_picture_info_t image_info;

//     // 1. 获取文件路径
//     char image_path[512];
//     file_iterator_get_full_path_from_index(ft, index, image_path, sizeof(image_path));
//     ESP_LOGI(TAG, "Displaying index %d: %s", index, image_path);

//     // 2. 打开文件
//     FILE *image_fp = fopen(image_path, "rb");
//     if(image_fp == NULL)
//     {
//         ESP_LOGE(TAG, "Failed to open file: %s", image_path);
//         return;
//     }

//     // 3. 获取文件大小并读取到输入 buffer
//     fseek(image_fp, 0, SEEK_END);
//     int image_size_fp = ftell(image_fp);
//     fseek(image_fp, 0, SEEK_SET);

//     size_t input_buffer_size_image = 0;
//     jpeg_decode_memory_alloc_cfg_t tx_mem_cfg = {
//         .buffer_direction = JPEG_DEC_ALLOC_INPUT_BUFFER,
//     };
//     // 注意：jpeg_alloc_decoder_mem 会分配内存，后面记得释放
//     uint8_t *input_buf = (uint8_t*)jpeg_alloc_decoder_mem(image_size_fp, &tx_mem_cfg, &input_buffer_size_image);
//     if(input_buf == NULL)
//     {
//         ESP_LOGE(TAG, "alloc input buf failed");
//         fclose(image_fp);
//         return;
//     }
//     fread(input_buf, 1, input_buffer_size_image, image_fp);
//     fclose(image_fp);

//     // 4. 解析 JPEG 头部获取宽高
//     if(jpeg_decoder_get_info(input_buf, input_buffer_size_image, &image_info) != ESP_OK) {
//         ESP_LOGE(TAG, "jpeg get info failed");
//         free(input_buf);
//         return;
//     }
//     ESP_LOGI(TAG, "Size: %lux%lu", image_info.width, image_info.height);

//     // 5. 分配输出 Buffer (RGB565)
//     size_t output_buf_size = 0;
//     jpeg_decode_memory_alloc_cfg_t rx_mem_cfg = {
//         .buffer_direction = JPEG_DEC_ALLOC_OUTPUT_BUFFER,
//     };
//     // RGB565 每个像素 2 字节
//     uint8_t *output_buf = (uint8_t*)jpeg_alloc_decoder_mem(image_info.width * image_info.height * 2, &rx_mem_cfg, &output_buf_size);
//     if(output_buf == NULL)
//     {
//         ESP_LOGE(TAG, "alloc output buf failed");
//         free(input_buf);
//         return;
//     }

//     // 6. 执行硬件解码
//     uint32_t out_size_image = 0;
//     static jpeg_decode_cfg_t decode_cfg_rgb = {
//         .output_format = JPEG_DECODE_OUT_FORMAT_RGB565,
//         .rgb_order = JPEG_DEC_RGB_ELEMENT_ORDER_BGR, // LVGL 通常使用 BGR 顺序 (取决于 LV_COLOR_16_SWAP)
//     };
    
//     esp_err_t ret = jpeg_decoder_process(jpgd_handle, &decode_cfg_rgb, input_buf, input_buffer_size_image, output_buf, output_buf_size, &out_size_image);
    
//     // 输入 buffer 解码完就不需要了
//     free(input_buf);

//     if(ret != ESP_OK) {
//         ESP_LOGE(TAG, "jpeg decode process failed");
//         free(output_buf);
//         return;
//     }

//     // 7. 更新 LVGL 显示
//     lvgl_port_lock(0);

//     // 关键点：内存管理
//     // LVGL 需要在绘制期间保持数据有效。如果直接释放 output_buf，屏幕会花掉。
//     // 我们需要释放上一张图片的内存，并保留当前这张。
//     if (current_image_data != NULL) {
//         free(current_image_data); // 释放旧图片内存
//     }
//     current_image_data = output_buf; // 指向新图片内存

//     // 配置 LVGL 图像描述符
//     img_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
//     img_dsc.header.cf = LV_COLOR_FORMAT_RGB565; // LVGL 9.x 格式
//     img_dsc.header.flags = 0;
//     img_dsc.header.w = image_info.width;
//     img_dsc.header.h = image_info.height;
//     img_dsc.header.stride = image_info.width * 2; // RGB565 stride
//     img_dsc.data_size = out_size_image;
//     img_dsc.data = current_image_data;

//     // 设置图片源
//     if(img_obj) {
//         lv_image_set_src(img_obj, &img_dsc);
//     }

//     lvgl_port_unlock();

// }


#define MAX_IMAGES 10
static lv_obj_t *img_obj;
static char image_paths[MAX_IMAGES][512];
static int image_count = 0;
static int img_index = 0;

// Hide the old image after animation and delete the timer
static void gesture_event_cb(lv_event_t *e)
{
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    if(dir != LV_DIR_LEFT && dir != LV_DIR_RIGHT) return; // only handle left/right

    if (image_count <= 0) return;

    int new_index = img_index + (dir == LV_DIR_LEFT ? 1 : -1);
    if(new_index < 0) new_index = image_count - 1;
    if(new_index >= image_count) new_index = 0;

    img_index = new_index;
    
    // Load image from SD card
    lvgl_port_lock(0);
    lv_img_set_src(img_obj, image_paths[img_index]);
    lvgl_port_unlock();
}

static void lv_demo_purecolor_png_switch()
{
    static const char* manual_image_paths[] = {
        "A:/9.jpg",
        "A:/6.jpg", 
        "A:/7.jpg",
        "A:/8.jpg"
    };
    image_count = sizeof(manual_image_paths) / sizeof(manual_image_paths[0]);

    for(int i = 0; i < image_count && i < MAX_IMAGES; i++) {
        strncpy(image_paths[i], manual_image_paths[i], sizeof(image_paths[i]) - 1);
        image_paths[i][sizeof(image_paths[i]) - 1] = '\0';
        ESP_LOGI(TAG, "Added image path: %s", image_paths[i]);
    }
    
    ESP_LOGI(TAG, "Total images added: %d", image_count);
    if (image_count > 0) {
        // create a single image object and switch src immediately on gesture
        img_obj = lv_img_create(lv_scr_act());
        lv_img_set_src(img_obj, image_paths[0]);

        // register gesture handler on the screen
        lv_obj_add_event(lv_scr_act(), gesture_event_cb, LV_EVENT_GESTURE, NULL);
    } else {
        ESP_LOGW(TAG, "No PNG images found on SD card");
        // Display a message
        lvgl_port_lock(0);
        lv_obj_t* label = lv_label_create(lv_scr_act());
        lv_label_set_text(label, "No PNG images found on SD card");
        lv_obj_center(label);
        lvgl_port_unlock();
    }
}

/* =============================================================================================================
                                                    LVGL Demos
    ============================================================================================================ */
static void lvgl_demos_test()
{
    lvgl_port_lock(0);
    lv_demo_widgets();
    lvgl_port_unlock();
}

/* =============================================================================================================
                                                    旋钮编码器
    ============================================================================================================ */

static void encoder_read_cb(lv_indev_t * indev, lv_indev_data_t * data)
{
    int count = 0;
    pcnt_unit_get_count(pcnt_unit, &count);
    pcnt_unit_clear_count(pcnt_unit);

    // PCNT x4模式比较灵敏，除以2可以让手感更好（2格一跳），若想1格一跳可除以4
    data->enc_diff = count / 2; 

    // 读取按键
    if (gpio_get_level(EC11_GPIO_KEY) == 0) {
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}
static void xuanniu_test()
{
    // 初始化 pcnt
    pcnt_unit_config_t unit_config = {
        .high_limit = 32000, 
        .low_limit = -32000,
        .flags.accum_count = false,
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

    // 滤波设置，消除抖动
    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = 1000,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config));

    // 通道 A 设置
    pcnt_chan_config_t chan_a_config = { .edge_gpio_num = EC11_GPIO_A, .level_gpio_num = EC11_GPIO_B };
    pcnt_channel_handle_t pcnt_chan_a = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_a_config, &pcnt_chan_a));

    // 通道 B 设置
    pcnt_chan_config_t chan_b_config = { .edge_gpio_num = EC11_GPIO_B, .level_gpio_num = EC11_GPIO_A };
    pcnt_channel_handle_t pcnt_chan_b = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_b_config, &pcnt_chan_b));

    // 完整的正交解码逻辑 (x4 模式)
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_a, 
                        PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_a, 
                        PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_b, 
                        PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_b, 
                        PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    // 启动单元
    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));

    // 按键 GPIO
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << EC11_GPIO_KEY),
        .pull_down_en = 0,
        .pull_up_en = 1,
    };
    gpio_config(&io_conf);


    // 注册 LVGL 输入设备
    lvgl_port_lock(0);
    encoder_indev = lv_indev_create();
    lv_indev_set_type(encoder_indev, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_read_cb(encoder_indev, encoder_read_cb);
    lvgl_port_unlock();

    // 初始化 UI
    lvgl_port_lock(0);

    lv_obj_t * scr = lv_scr_act();
    // 黑色背景，适合圆屏
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

    // 创建输入组 (用于编码器或按键导航)
    lv_group_t * input_group = lv_group_create();
    lv_group_set_default(input_group); 
    if (encoder_indev) {
        lv_indev_set_group(encoder_indev, input_group);
    }

    // 布局：使用 Flex 纵向居中
    lv_obj_set_layout(scr, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(scr, 0, 0); 
    lv_obj_set_style_pad_row(scr, 25, 0);

    lv_obj_t * slider1 = lv_slider_create(scr);
    lv_obj_set_width(slider1, 160);       
    lv_obj_set_height(slider1, 12);
    lv_slider_set_range(slider1, 0, 100);
    lv_slider_set_value(slider1, 40, LV_ANIM_OFF);
    
    // 1. 设置主体颜色
    lv_obj_set_style_bg_color(slider1, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);      // 槽背景深灰
    lv_obj_set_style_bg_color(slider1, lv_palette_main(LV_PALETTE_BLUE), LV_PART_INDICATOR); // 指示器蓝色
    
    // 2. 设置焦点视觉
    // 正常状态：无边框
    lv_obj_set_style_border_width(slider1, 0, LV_PART_MAIN);
    // 焦点状态：边框变宽 (3px)，颜色醒目 (橙色)
    lv_obj_set_style_border_width(slider1, 3, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(slider1, lv_palette_main(LV_PALETTE_ORANGE), LV_PART_MAIN | LV_STATE_FOCUSED);
    // 编辑状态 (按下旋转时)：颜色加深
    lv_obj_set_style_border_color(slider1, lv_palette_darken(LV_PALETTE_ORANGE, 2), LV_PART_MAIN | LV_STATE_EDITED);
    // 增加内边距
    lv_obj_set_style_pad_all(slider1, 2, LV_PART_MAIN);

    lv_group_add_obj(input_group, slider1);

    lv_obj_t * slider2 = lv_slider_create(scr);
    lv_obj_set_width(slider2, 160);       
    lv_obj_set_height(slider2, 12);       
    lv_slider_set_range(slider2, 0, 100);
    lv_slider_set_value(slider2, 75, LV_ANIM_OFF);

    // 样式复用 Slider 1 的逻辑
    lv_obj_set_style_bg_color(slider2, lv_palette_darken(LV_PALETTE_GREY, 2), LV_PART_MAIN); 
    lv_obj_set_style_bg_color(slider2, lv_palette_main(LV_PALETTE_GREEN), LV_PART_INDICATOR);
    
    // 焦点视觉
    lv_obj_set_style_border_width(slider2, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(slider2, 3, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(slider2, lv_palette_main(LV_PALETTE_ORANGE), LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(slider2, lv_palette_darken(LV_PALETTE_ORANGE, 2), LV_PART_MAIN | LV_STATE_EDITED);
    lv_obj_set_style_pad_all(slider2, 2, LV_PART_MAIN);

    lv_group_add_obj(input_group, slider2);

    lv_group_focus_obj(slider1);

    lvgl_port_unlock();
}


static void release_task(void *arg)
{
    ESP_LOGI(TAG, "Starting Release Task");

    // lv_demo_purecolor_png_switch();
    // lvgl_demos_test();
    xuanniu_test();

    vTaskDelete(NULL); 
}

void release_demo(void)
{
    xTaskCreate(release_task,
                "release_task",
                10 * 1024,
                NULL,
                5,
                NULL);
}

