#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/jpeg_decode.h"


#include "file_iterator.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"

static char *TAG = "template";



/* ========================= image switch ========================= */
static lv_obj_t *img_obj = NULL;
static lv_image_dsc_t img_dsc;
static uint8_t *current_img_data = NULL; 
static file_iterator_instance_t *file_iterator = NULL; // 文件迭代器句柄
static int img_index = 0;
static int img_count = 0;

static jpeg_decoder_handle_t jpgd_handle;



/* =============================================================================================================
                                                image switch
    ============================================================================================================ */

static char* image_switch_get_path(int index)
{
    const char *image_name = file_iterator_get_name_from_index(file_iterator,index);

    static char image_path[256];
    file_iterator_get_full_path_from_index(file_iterator,index,image_path,256);

    return image_path;
}

static void image_switch_rb()
{
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    if(dir != LV_DIR_LEFT && dir != LV_DIR_RIGHT) return; // only handle left/right

    int new_index = img_index + (dir == LV_DIR_LEFT ? 1 : -1);
    if(new_index < 0) new_index = img_count - 1;
    if(new_index >= img_count) new_index = 0;

    img_index = new_index;

    ESP_LOGI(TAG, "%s", image_switch_get_path(img_index));

    // Load image from SD card
    // lvgl_port_lock(0);
    // lv_img_set_src(img_obj, image_switch_get_path(img_index));
    // lvgl_port_unlock();
}

// 使用 LVGL 自带的编解码
void image_switch_lvgl(const char *image_dir_path)
{
    file_iterator = file_iterator_new(image_dir_path);
    assert(file_iterator);

    img_count = file_iterator_get_count(file_iterator);
    if (img_count) 
        ESP_LOGI(TAG, "image count: %d", img_count);
    else {
        ESP_LOGW(TAG, "no image found");
        return ;
    }

    lv_obj_add_event(lv_scr_act(), image_switch_rb, LV_EVENT_GESTURE, NULL);

    for(int i = 0; i < img_count; i++) {
        ESP_LOGI(TAG, "%s", image_switch_get_path(i));
    }

    lvgl_port_lock(0);
    img_obj = lv_img_create(lv_scr_act());
    lv_obj_center(img_obj);
    lv_image_set_src(img_obj, "A:images/1.jpg");
    lvgl_port_unlock();
}

static void image_switch_handle()
{
    jpeg_decode_picture_info_t image_info;
    jpeg_decode_memory_alloc_cfg_t rx_mem_cfg = {
        .buffer_direction = JPEG_DEC_ALLOC_OUTPUT_BUFFER,
    };

    jpeg_decode_memory_alloc_cfg_t tx_mem_cfg = {
        .buffer_direction = JPEG_DEC_ALLOC_INPUT_BUFFER,
    };
    
    jpeg_decode_cfg_t decode_cfg_rgb = {
        .output_format = JPEG_DECODE_OUT_FORMAT_RGB565,
        .rgb_order = JPEG_DEC_RGB_ELEMENT_ORDER_BGR,
    };

    jpeg_decode_cfg_t decode_cfg_gray = {
        .output_format = JPEG_DECODE_OUT_FORMAT_GRAY,
    };

    FILE *f = fopen(image_switch_get_path(img_index), "rb");
    if (f == NULL) {
        ESP_LOGE(TAG, "open file failed");
        return ;
    }

    fseek(f, 0, SEEK_END);
    int image_size_fp = ftell(f);
    fseek(f, 0, SEEK_SET);

    size_t input_buffer_size_image = 0;
    uint8_t *input_buf =  (uint8_t*)jpeg_alloc_decoder_mem(image_size_fp, &tx_mem_cfg, &input_buffer_size_image);
    if(input_buf == NULL)
    {
        ESP_LOGE(TAG,"alloc input buf failed");
        return;
    }
    fread(input_buf,1,input_buffer_size_image,f);
    fclose(f);
    ESP_ERROR_CHECK(jpeg_decoder_get_info(input_buf,input_buffer_size_image,&image_info));

    size_t output_buf_size = 0;
    uint8_t *output_buf = (uint8_t*)jpeg_alloc_decoder_mem(image_info.width * image_info.height * 2, &rx_mem_cfg, &output_buf_size);
    if(output_buf == NULL)
    {
        ESP_LOGE(TAG,"alloc output buf failed");
        return;
    }
    uint32_t out_size_image = 0;
    
    ESP_ERROR_CHECK(jpeg_decoder_process(jpgd_handle, &decode_cfg_rgb, input_buf, input_buffer_size_image, output_buf, output_buf_size, &out_size_image));

    lvgl_port_lock(0);

    // 配置 LVGL 图像描述符
    img_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    img_dsc.header.cf = LV_COLOR_FORMAT_RGB565;
    img_dsc.header.flags = 0;
    img_dsc.header.w = image_info.width;
    img_dsc.header.h = image_info.height;
    img_dsc.header.stride = image_info.width * 2;
    img_dsc.data_size = out_size_image;
    img_dsc.data = output_buf;

    // 设置图片源
    if(img_obj) {
        lv_image_set_src(img_obj, &img_dsc);
    }

    lvgl_port_unlock();

    free(input_buf);
    free(output_buf);
}


void image_switch_idf(const char *image_dir_path)
{
    file_iterator = file_iterator_new(image_dir_path);
    assert(file_iterator);

    img_count = file_iterator_get_count(file_iterator);
    if (img_count) 
        ESP_LOGI(TAG, "image count: %d", img_count);
    else {
        ESP_LOGW(TAG, "no image found");
        return ;
    }

    lv_obj_add_event(lv_scr_act(), image_switch_rb, LV_EVENT_GESTURE, NULL);

    for(int i = 0; i < img_count; i++) {
        ESP_LOGI(TAG, "%s", image_switch_get_path(i));
    }

    jpeg_decode_engine_cfg_t decode_eng_cfg = {
        .timeout_ms = 40,
    };
    ESP_ERROR_CHECK(jpeg_new_decoder_engine(&decode_eng_cfg, &jpgd_handle));

    image_switch_handle();
}