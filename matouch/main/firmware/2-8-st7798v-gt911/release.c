#include "lvgl.h"
#include "esp_lvgl_port.h"

#include "esp_log.h"
#include <freertos/task.h>
#include <freertos/FreeRTOS.h>
#include "driver/jpeg_decode.h"
#include <stdlib.h>

#include "boards/2-8-st7798v-gt911/board.h"

static const char *TAG = "release";

const static char jpg_file_1080[] = "/spiffs/111.jpg";
static lv_obj_t *img_obj = NULL;
static uint8_t *decoded_img_data = NULL;
static int img_width = 0;
static int img_height = 0;

static void image_switch_test()
{
    jpeg_decoder_handle_t jpgd_handle;

    jpeg_decode_engine_cfg_t decode_eng_cfg = {
        .timeout_ms = 40,
    };

    ESP_ERROR_CHECK(jpeg_new_decoder_engine(&decode_eng_cfg, &jpgd_handle));

    jpeg_decode_cfg_t decode_cfg_rgb = {
        .output_format = JPEG_DECODE_OUT_FORMAT_RGB888,
        .rgb_order = JPEG_DEC_RGB_ELEMENT_ORDER_BGR,
    };

    jpeg_decode_memory_alloc_cfg_t rx_mem_cfg = {
        .buffer_direction = JPEG_DEC_ALLOC_OUTPUT_BUFFER,
    };

    jpeg_decode_memory_alloc_cfg_t tx_mem_cfg = {
        .buffer_direction = JPEG_DEC_ALLOC_INPUT_BUFFER,
    };

    FILE *file_jpg_1080p = fopen(jpg_file_1080, "rb");
    ESP_LOGI(TAG, "jpg_file_1080:%s", jpg_file_1080);
    if (file_jpg_1080p == NULL) {
        ESP_LOGE(TAG, "fopen file_jpg_1080p error");
        return;
    }

    fseek(file_jpg_1080p, 0, SEEK_END);
    int jpeg_size_1080p = ftell(file_jpg_1080p);
    fseek(file_jpg_1080p, 0, SEEK_SET);
    size_t tx_buffer_size_1080p = 0;
    uint8_t *tx_buf_1080p = (uint8_t*)jpeg_alloc_decoder_mem(jpeg_size_1080p, &tx_mem_cfg, &tx_buffer_size_1080p);
    if (tx_buf_1080p == NULL) {
        ESP_LOGE(TAG, "alloc 1080p tx buffer error");
        return;
    }
    fread(tx_buf_1080p, 1, jpeg_size_1080p, file_jpg_1080p);
    fclose(file_jpg_1080p);

    // Get the jpg header information (This step is optional)
    jpeg_decode_picture_info_t header_info;
    ESP_ERROR_CHECK(jpeg_decoder_get_info(tx_buf_1080p, jpeg_size_1080p, &header_info));
    ESP_LOGI(TAG, "header parsed, width is %" PRId32 ", height is %" PRId32, header_info.width, header_info.height);

    // Save image dimensions
    img_width = header_info.width;
    img_height = header_info.height;

    size_t rx_buffer_size_1080p = 0;
    uint8_t *rx_buf_1080p = (uint8_t*)jpeg_alloc_decoder_mem(img_width * img_height * 3, &rx_mem_cfg, &rx_buffer_size_1080p);
    if (rx_buf_1080p == NULL) {
        ESP_LOGE(TAG, "alloc 1080p rx buffer error");
        return;
    }

    uint32_t out_size_1080p = 0;
    ESP_ERROR_CHECK(jpeg_decoder_process(jpgd_handle, &decode_cfg_rgb, tx_buf_1080p, jpeg_size_1080p, rx_buf_1080p, rx_buffer_size_1080p, &out_size_1080p));

    // Convert RGB888 to RGB565 for LVGL
    if (decoded_img_data) {
        free(decoded_img_data);
    }
    decoded_img_data = malloc(img_width * img_height * 2); // RGB565
    
    if (decoded_img_data == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for RGB565 image");
        return;
    }

    // Convert RGB888 to RGB565
    for (int i = 0; i < img_width * img_height; i++) {
        uint8_t r = rx_buf_1080p[i * 3];
        uint8_t g = rx_buf_1080p[i * 3 + 1];
        uint8_t b = rx_buf_1080p[i * 3 + 2];
        
        uint16_t rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        decoded_img_data[i * 2] = rgb565 & 0xFF;
        decoded_img_data[i * 2 + 1] = (rgb565 >> 8) & 0xFF;
    }

    // Create or update image object on the screen
    lvgl_port_lock(0);
    if (img_obj == NULL) {
        img_obj = lv_image_create(lv_screen_active());
    }
    
    // Create image descriptor for LVGL
    lv_image_dsc_t *img_dsc = malloc(sizeof(lv_image_dsc_t));
    if (img_dsc) {
        img_dsc->data = decoded_img_data;
        img_dsc->data_size = img_width * img_height * 2;
        img_dsc->header.w = img_width;
        img_dsc->header.h = img_height;
        img_dsc->header.cf = LV_COLOR_FORMAT_RGB565;
        img_dsc->header.flags = 0;
        
        lv_image_set_src(img_obj, img_dsc);
    }
    lvgl_port_unlock();

    // Clean up
    free(tx_buf_1080p);
    free(rx_buf_1080p);
}

static void release_task(void *arg)
{
    // Call the image display function
    image_switch_test();
    
    vTaskDelete(NULL);
}

void release_demo()
{
    xTaskCreate(release_task,
                "release_task",
                1024 * 10,
                NULL,
                5,
                NULL);
    
}