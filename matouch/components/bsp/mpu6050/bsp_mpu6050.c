#include "bsp_mpu6050.h"

#include <esp_log.h>
#include <string.h>

static const char *TAG = "bsp_mpu6050";

#define PCF8563_REG_VL_SECONDS 0x02
#define PCF8563_CMD_START      0x02
#define PCF8563_I2C_ADDR       0x51
#define PCF8563_I2C_FREQ_HZ    100000


typedef struct {
    bsp_mpu6050_handle_t handle;
}bsp_mpu6050_local_t;

bsp_mpu6050_local_t bsp_mpu6050_local;
#define local bsp_mpu6050_local


// 私有数据结构（封装实现细节）
typedef struct {
    i2c_master_dev_handle_t i2c_dev;
    SemaphoreHandle_t lock;
} bsp_mpu6050_priv_t;

// ============================================================================
// 辅助函数
// ============================================================================

static inline uint8_t bcd2dec(uint8_t val) {
    return ((val >> 4) * 10) + (val & 0x0f);
}

static inline uint8_t dec2bcd(uint8_t val) {
    return ((val / 10) << 4) + (val % 10);
}

// ============================================================================
// 私有方法实现
// ============================================================================

static esp_err_t _init(bsp_mpu6050_config_t *config)
{
    esp_err_t err = ESP_FAIL;

    mpu6050_config_t dev_cfg          = I2C_MPU6050_CONFIG_DEFAULT;
    dev_cfg.accel_full_scale_range    = MPU6050_ACCEL_FS_RANGE_2G;
    mpu6050_handle_t dev_hdl;

    // init device
    err |= mpu6050_init(config->i2c_handle, &dev_cfg, &dev_hdl);

    uint8_t                                 sample_rate_divider_reg;
    mpu6050_config_register_t               config_reg;
    mpu6050_gyro_config_register_t          gyro_config_reg;
    mpu6050_accel_config_register_t         accel_config_reg;
    mpu6050_interrupt_enable_register_t     irq_enable_reg;
    mpu6050_power_management1_register_t    power_management1_reg;
    mpu6050_power_management2_register_t    power_management2_reg;
    mpu6050_who_am_i_register_t             who_am_i_reg;

    /* attempt to read device sample rate divider register */
    err |= mpu6050_get_sample_rate_divider_register(dev_hdl, &sample_rate_divider_reg);

    /* attempt to read device configuration register */
    err |= mpu6050_get_config_register(dev_hdl, &config_reg);

    /* attempt to read device gyroscope configuration register */
    err |= mpu6050_get_gyro_config_register(dev_hdl, &gyro_config_reg);

    /* attempt to read device accelerometer configuration register */
    err |= mpu6050_get_accel_config_register(dev_hdl, &accel_config_reg);

    /* attempt to read device interrupt enable register */
    err |= mpu6050_get_interrupt_enable_register(dev_hdl, &irq_enable_reg);

    /* attempt to read device power management 1 register */
    err |= mpu6050_get_power_management1_register(dev_hdl, &power_management1_reg);

    /* attempt to read device power management 2 register */
    err |= mpu6050_get_power_management2_register(dev_hdl, &power_management2_reg);

    /* attempt to read device who am i register */
    err |= mpu6050_get_who_am_i_register(dev_hdl, &who_am_i_reg);
}



// ============================================================================
// 公共接口实现
// ============================================================================

esp_err_t bsp_mpu6050_init(bsp_mpu6050_config_t *config)
{
    if (config->i2c_handle == NULL) {
        ESP_LOGE(TAG, "Invalid arguments");
        return ESP_ERR_INVALID_ARG;
    }
    
    bsp_mpu6050_handle_t dev = (bsp_mpu6050_handle_t)calloc(1, sizeof(struct bsp_mpu6050_t));
    if (!dev) {
        ESP_LOGE(TAG, "Failed to allocate memory for handle");
        return ESP_ERR_NO_MEM;
    }
    
    // bsp_mpu6050_priv_t *priv = (bsp_mpu6050_priv_t *)calloc(1, sizeof(bsp_mpu6050_priv_t));
    // if (!priv) {
    //     ESP_LOGE(TAG, "Failed to allocate memory for private data");
    //     free(dev);
    //     return ESP_ERR_NO_MEM;
    // }
    
    // 创建互斥锁
    // priv->lock = xSemaphoreCreateMutex();
    // if (!priv->lock) {
    //     ESP_LOGE(TAG, "Failed to create mutex");
    //     free(priv);
    //     free(dev);
    //     return ESP_ERR_NO_MEM;
    // }



    
    ESP_LOGI(TAG, "PCF8563 RTC initialized successfully");
    return ESP_OK;
}

esp_err_t bsp_mpu6050_destroy(bsp_mpu6050_handle_t handle)
{
    if (!handle || !handle->destroy) {
        return ESP_ERR_INVALID_ARG;
    }
    
    return handle->destroy(handle);
}