#include "bsp_pcf85063a.h"
#include "esp_check.h"
#include <stdlib.h>

static const char *TAG = "bsp_pcf85063a";

#define PCF85063A_LOCK_TIMEOUT_MS 1000

/* Definition of the handle struct (Opaque to user) */
struct bsp_pcf85063a_t {
    pcf85063a_dev_t dev;      /*!< Low-level device instance */
    SemaphoreHandle_t lock;   /*!< Mutex for thread safety */
};

/**
 * @brief Helper to convert pcf85063a_datetime_t to struct tm
 */
static void _pcf_time_to_tm(const pcf85063a_datetime_t *src, struct tm *dst)
{
    // struct tm year is years since 1900
    // PCF85063A usually returns full year (e.g. 2025) or offset from 1970 depending on driver implementation
    // Based on your provided driver: time->year = bcdToDec(...) + YEAR_OFFSET (1970)
    dst->tm_year = src->year - 1900; 
    
    // struct tm month is 0-11
    // PCF85063A is 1-12
    dst->tm_mon  = src->month - 1;
    
    dst->tm_mday = src->day;
    dst->tm_hour = src->hour;
    dst->tm_min  = src->min;
    dst->tm_sec  = src->sec;
    
    // struct tm wday is 0-6 (Sunday=0)
    dst->tm_wday = src->dotw; 
    
    // yday and isdst are typically ignored or calculated if needed, setting to 0/-1 is safe
    dst->tm_yday = 0; 
    dst->tm_isdst = -1; 
}

/**
 * @brief Helper to convert struct tm to pcf85063a_datetime_t
 */
static void _tm_to_pcf_time(const struct tm *src, pcf85063a_datetime_t *dst)
{
    dst->year  = src->tm_year + 1900;
    dst->month = src->tm_mon + 1;
    dst->day   = src->tm_mday;
    dst->hour  = src->tm_hour;
    dst->min   = src->tm_min;
    dst->sec   = src->tm_sec;
    dst->dotw  = src->tm_wday;
}

esp_err_t bsp_pcf85063a_init(const bsp_pcf85063a_config_t *config, bsp_pcf85063a_handle_t *ret_handle)
{
    esp_err_t ret = ESP_OK;
    bsp_pcf85063a_handle_t handle = NULL;

    ESP_RETURN_ON_FALSE(config != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid config");
    ESP_RETURN_ON_FALSE(config->bus_handle != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid bus handle");
    ESP_RETURN_ON_FALSE(ret_handle != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid return handle");

    handle = (bsp_pcf85063a_handle_t)calloc(1, sizeof(struct bsp_pcf85063a_t));
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_NO_MEM, TAG, "No memory for handle");

    /* Create Mutex */
    handle->lock = xSemaphoreCreateMutex();
    ESP_GOTO_ON_FALSE(handle->lock != NULL, ESP_ERR_NO_MEM, err, TAG, "Failed to create mutex");

    /* Initialize Low-level Driver */
    // Note: passing the address from config allows flexibility
    ret = pcf85063a_init(&handle->dev, config->bus_handle, config->i2c_addr);
    ESP_GOTO_ON_ERROR(ret, err, TAG, "Low level driver init failed");

    *ret_handle = handle;
    ESP_LOGI(TAG, "Instance initialized");
    return ret;

err:
    if (handle->lock) vSemaphoreDelete(handle->lock);
    if (handle) free(handle);

    return ret;
}

esp_err_t bsp_pcf85063a_deinit(bsp_pcf85063a_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid handle");

    // Ideally, the low-level driver should also have a deinit/remove_device function.
    // Since `pcf85063a_init` calls `i2c_master_bus_add_device`, we should call `i2c_master_bus_rm_device`
    // However, looking at your provided pcf85063a.c, there is no deinit/remove function exposed.
    // We strictly handle BSP level cleanup here.
    
    if (handle->dev.dev_handle) {
        i2c_master_bus_rm_device(handle->dev.dev_handle);
    }

    if (handle->lock) {
        vSemaphoreDelete(handle->lock);
    }

    free(handle);
    return ESP_OK;
}

esp_err_t bsp_pcf85063a_get_tm(bsp_pcf85063a_handle_t handle, struct tm *time)
{
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid handle");
    ESP_RETURN_ON_FALSE(time != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid time pointer");

    esp_err_t ret = ESP_OK;
    pcf85063a_datetime_t raw_time;

    if (xSemaphoreTake(handle->lock, pdMS_TO_TICKS(PCF85063A_LOCK_TIMEOUT_MS)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to acquire lock");
        return ESP_ERR_TIMEOUT;
    }

    ret = pcf85063a_get_time_date(&handle->dev, &raw_time);
    
    xSemaphoreGive(handle->lock);

    if (ret == ESP_OK) {
        _pcf_time_to_tm(&raw_time, time);
    }

    return ret;
}

esp_err_t bsp_pcf85063a_set_tm(bsp_pcf85063a_handle_t handle, const struct tm *time)
{
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid handle");
    ESP_RETURN_ON_FALSE(time != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid time pointer");

    esp_err_t ret = ESP_OK;
    pcf85063a_datetime_t raw_time;

    /* Convert standard struct tm to driver format outside the lock (pure computation) */
    _tm_to_pcf_time(time, &raw_time);

    if (xSemaphoreTake(handle->lock, pdMS_TO_TICKS(PCF85063A_LOCK_TIMEOUT_MS)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to acquire lock");
        return ESP_ERR_TIMEOUT;
    }

    ret = pcf85063a_set_time_date(&handle->dev, raw_time); // Passed by value as per driver API

    xSemaphoreGive(handle->lock);

    return ret;
}