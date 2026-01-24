#pragma once

#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <driver/i2c_master.h>

#include "mpu6050.h"

#define MPU6050_LOCK_TIMEOUT_MS 2000

typedef struct bsp_mpu6050_t *bsp_mpu6050_handle_t;


typedef struct {
    i2c_master_bus_handle_t i2c_handle;
}bsp_mpu6050_config_t;


struct bsp_mpu6050_t {
    esp_err_t (*get_time)(bsp_mpu6050_handle_t self, struct tm *timeinfo);
    esp_err_t (*set_time)(bsp_mpu6050_handle_t self, const struct tm *timeinfo);
    esp_err_t (*destroy)(bsp_mpu6050_handle_t self);
    
    void *_priv;
}; 

/**
 * @brief init mpu6050
 * 
 * @param handle 输出句柄指针
 * @param bus_handle I2C 总线句柄
 * @return esp_err_t 
 */
esp_err_t bsp_pcf8563_init(bsp_mpu6050_config_t *config)

/**
 * @brief detroy mpu6050
 * 
 * @param handle RTC 句柄
 * @return esp_err_t 
 */
esp_err_t bsp_mpu6050_destroy(bsp_mpu6050_handle_t handle);