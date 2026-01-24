#include "file_iterator.h"
#include "template.h"



static char *TAG = "template";



// lvgl demos
static int img_index = 0;
static int img_count = 0;
static lv_obj_t *img_obj = NULL;
static file_iterator_instance_t *file_iterator = NULL;


/* =============================================================================================================
                                                LVGL Demos
    1. image switch                                       
    2. lvgl demos
    ============================================================================================================ */

static char* image_switch_get_path(int index)
{
    const char *image_name = file_iterator_get_name_from_index(file_iterator,index);

    static char image_path[256];
    // file_iterator_get_full_path_from_index(file_iterator,index,image_path,256);
    snprintf(image_path, sizeof(image_path), "A:/images/%s", image_name);

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
    lvgl_port_lock(0);
    lv_img_set_src(img_obj, image_switch_get_path(img_index));
    lvgl_port_unlock();
}
void template_image_switch_lvgl(const char *image_dir_path)
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
    lv_image_set_src(img_obj, image_switch_get_path(img_index));
    lvgl_port_unlock();
}

void template_lvgl_demos_test()
{
    lvgl_port_lock(0);

    lv_demo_widgets();

    lvgl_port_unlock();
}


/* =============================================================================================================
                                                sensor
    1. dht11
    2. mpu6050                                         
    ============================================================================================================ */
// void template_dht11_test()
// {
//     gpio_config_t io_conf = {
//         .intr_type = GPIO_INTR_DISABLE,
//         .mode = GPIO_MODE_INPUT_OUTPUT_OD,
//         .pin_bit_mask = (1ULL << GPIO_NUM_20),
//         .pull_down_en = GPIO_PULLDOWN_DISABLE,
//         .pull_up_en = GPIO_PULLUP_ENABLE,
//     };
//     gpio_config(&io_conf);

//     int16_t humidity = 0;
//     int16_t temperature = 0;

//     while (1) {
//         vTaskDelay(pdMS_TO_TICKS(2000));

//         if (dht_read_data(DHT_TYPE_DHT11, GPIO_NUM_20, &humidity, &temperature) == ESP_OK) {
//         }
//     }

// }


// void template_mpu6050_test()
// {
//     mpu6050_config_t dev_cfg          = I2C_MPU6050_CONFIG_DEFAULT;
//     dev_cfg.accel_full_scale_range    = MPU6050_ACCEL_FS_RANGE_2G;
//     mpu6050_handle_t dev_hdl;
//     //
//     // init device
//     mpu6050_init(_i2c_bus, &dev_cfg, &dev_hdl);

//     uint8_t                                 sample_rate_divider_reg;
//     mpu6050_config_register_t               config_reg;
//     mpu6050_gyro_config_register_t          gyro_config_reg;
//     mpu6050_accel_config_register_t         accel_config_reg;
//     mpu6050_interrupt_enable_register_t     irq_enable_reg;
//     mpu6050_power_management1_register_t    power_management1_reg;
//     mpu6050_power_management2_register_t    power_management2_reg;
//     mpu6050_who_am_i_register_t             who_am_i_reg;

//     /* attempt to read device sample rate divider register */
//     mpu6050_get_sample_rate_divider_register(dev_hdl, &sample_rate_divider_reg);

//     /* attempt to read device configuration register */
//     mpu6050_get_config_register(dev_hdl, &config_reg);

//     /* attempt to read device gyroscope configuration register */
//     mpu6050_get_gyro_config_register(dev_hdl, &gyro_config_reg);

//     /* attempt to read device accelerometer configuration register */
//     mpu6050_get_accel_config_register(dev_hdl, &accel_config_reg);

//     /* attempt to read device interrupt enable register */
//     mpu6050_get_interrupt_enable_register(dev_hdl, &irq_enable_reg);

//     /* attempt to read device power management 1 register */
//     mpu6050_get_power_management1_register(dev_hdl, &power_management1_reg);

//     /* attempt to read device power management 2 register */
//     mpu6050_get_power_management2_register(dev_hdl, &power_management2_reg);

//     /* attempt to read device who am i register */
//     mpu6050_get_who_am_i_register(dev_hdl, &who_am_i_reg);

//         // show registers
//     ESP_LOGI(TAG, "Sample Rate Divider Register:         0x%02x (%s)", sample_rate_divider_reg, uint8_to_binary(sample_rate_divider_reg));
//     ESP_LOGI(TAG, "Configuration Register:               0x%02x (%s)", config_reg.reg, uint8_to_binary(config_reg.reg));
//     ESP_LOGI(TAG, "Gyroscope Configuration Register:     0x%02x (%s)", gyro_config_reg.reg, uint8_to_binary(gyro_config_reg.reg));
//     ESP_LOGI(TAG, "Accelerometer Configuration Register: 0x%02x (%s)", accel_config_reg.reg, uint8_to_binary(accel_config_reg.reg));
//     ESP_LOGI(TAG, "Interrupt Enable Register:            0x%02x (%s)", irq_enable_reg.reg, uint8_to_binary(irq_enable_reg.reg));
//     ESP_LOGI(TAG, "Power Management 1 Register:          0x%02x (%s)", power_management1_reg.reg, uint8_to_binary(power_management1_reg.reg));
//     ESP_LOGI(TAG, "Power Management 2 Register:          0x%02x (%s)", power_management2_reg.reg, uint8_to_binary(power_management2_reg.reg));
//     ESP_LOGI(TAG, "Who am I Register:                    0x%02x (%s)", who_am_i_reg.reg, uint8_to_binary(who_am_i_reg.reg));

//     while (1) {
//         // handle sensor
//         float temperature;
//         mpu6050_gyro_data_axes_t gyro_data;
//         mpu6050_accel_data_axes_t accel_data;
//         esp_err_t result = mpu6050_get_motion(dev_hdl, &gyro_data, &accel_data, &temperature);
//         if(result != ESP_OK) {
//             ESP_LOGE(TAG, "mpu6050 device read failed (%s)", esp_err_to_name(result));
//         } else {
//             /* pitch and roll */
//             float pitch = atanf(accel_data.x_axis / sqrtf(powf(accel_data.y_axis, 2.0f) + powf(accel_data.z_axis, 2.0f)));
//             float roll  = atanf(accel_data.y_axis / sqrtf(powf(accel_data.x_axis, 2.0f) + powf(accel_data.z_axis, 2.0f)));

//             // ESP_LOGI(TAG, "Accelerometer X-Axis: %fg", accel_data.x_axis);
//             // ESP_LOGI(TAG, "Accelerometer Y-Axis: %fg", accel_data.y_axis);
//             // ESP_LOGI(TAG, "Accelerometer Z-Axis: %fg", accel_data.z_axis);
//             // ESP_LOGI(TAG, "Gyroscope X-Axis:     %f°/sec", gyro_data.x_axis);
//             // ESP_LOGI(TAG, "Gyroscope Y-Axis:     %f°/sec", gyro_data.y_axis);
//             // ESP_LOGI(TAG, "Gyroscope Z-Axis:     %f°/sec", gyro_data.z_axis);
//             // ESP_LOGI(TAG, "Temperature:          %f°C", temperature);
//             // ESP_LOGI(TAG, "Pitch Angle:          %f°", pitch);
//             // ESP_LOGI(TAG, "Roll Angle:           %f°", roll);
//         }
//         vTaskDelay(pdMS_TO_TICKS(1000));
//     }
// }