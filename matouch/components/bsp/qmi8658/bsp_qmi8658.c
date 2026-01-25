#include "bsp_qmi8658.h"
#include "esp_check.h"
#include "esp_err.h"
#include <stdlib.h>


static const char *TAG = "bsp_qmi8658";

struct bsp_qmi8658_t {
    bsp_qmi8658_config_t config;    /*!< Configuration */
    SemaphoreHandle_t lock;         /*!< Mutex for thread safety */
    qmi8658_dev_t dev;              /*!< QMI8658 device handle */
};

static esp_err_t _set_arg(bsp_qmi8658_handle_t handle)
{
    esp_err_t ret = ESP_OK;

    ret |= qmi8658_set_accel_range(&handle->dev, QMI8658_ACCEL_RANGE_8G);
    ret |= qmi8658_set_accel_odr(&handle->dev, QMI8658_ACCEL_ODR_1000HZ);
    ret |= qmi8658_set_gyro_range(&handle->dev, QMI8658_GYRO_RANGE_512DPS);
    ret |= qmi8658_set_gyro_odr(&handle->dev, QMI8658_GYRO_ODR_1000HZ);

    qmi8658_set_accel_unit_mps2(&handle->dev, true);
    qmi8658_set_gyro_unit_rads(&handle->dev, true);
    qmi8658_set_display_precision(&handle->dev, 4);

    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to configure QMI8658 sensor");

    return ret;
}

esp_err_t bsp_qmi8658_enable_sensors(bsp_qmi8658_handle_t handle, uint8_t flags)
{
    if (!handle) return ESP_ERR_INVALID_ARG;
    if (xSemaphoreTake(handle->lock, pdMS_TO_TICKS(QMI8658_LOCK_TIMEOUT_MS)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    esp_err_t ret = qmi8658_enable_sensors(&handle->dev, flags);
    xSemaphoreGive(handle->lock);
    return ret;
}

esp_err_t bsp_qmi8658_read_accel(bsp_qmi8658_handle_t handle, float *x, float *y, float *z)
{
    if (!handle || !x || !y || !z) return ESP_ERR_INVALID_ARG;
    if (xSemaphoreTake(handle->lock, pdMS_TO_TICKS(QMI8658_LOCK_TIMEOUT_MS)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    esp_err_t ret = qmi8658_read_accel(&handle->dev, x, y, z);
    xSemaphoreGive(handle->lock);
    return ret;
}

esp_err_t bsp_qmi8658_read_gyro(bsp_qmi8658_handle_t handle, float *x, float *y, float *z)
{
    if (!handle || !x || !y || !z) return ESP_ERR_INVALID_ARG;
    if (xSemaphoreTake(handle->lock, pdMS_TO_TICKS(QMI8658_LOCK_TIMEOUT_MS)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    esp_err_t err = qmi8658_read_gyro(&handle->dev, x, y, z);
    xSemaphoreGive(handle->lock);
    return err;
}

esp_err_t bsp_qmi8658_read_sensor_data(bsp_qmi8658_handle_t handle, qmi8658_data_t *data)
{
    if (!handle || !data) return ESP_ERR_INVALID_ARG;
    if (xSemaphoreTake(handle->lock, pdMS_TO_TICKS(QMI8658_LOCK_TIMEOUT_MS)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    esp_err_t ret = qmi8658_read_sensor_data(&handle->dev, data);
    xSemaphoreGive(handle->lock);
    return ret;
}
esp_err_t bsp_qmi8658_init(const bsp_qmi8658_config_t *config, bsp_qmi8658_handle_t *ret_handle)
{
    ESP_RETURN_ON_FALSE(config != NULL, ESP_ERR_INVALID_ARG, TAG, "config is NULL");

    esp_err_t ret = ESP_OK;
    bsp_qmi8658_handle_t _handle = calloc(1, sizeof(struct bsp_qmi8658_t));
    ESP_RETURN_ON_FALSE(_handle != NULL, ESP_ERR_NO_MEM, TAG, "Failed to allocate memory for handle");

    ret = qmi8658_init(&_handle->dev, config->bus_handle, config->i2c_addr);
    ESP_GOTO_ON_ERROR(ret, err, TAG, "qmi8658_init failed");

    ret = _set_arg(_handle);
    ESP_GOTO_ON_ERROR(ret, err, TAG, "Failed to set QMI8658 arguments");

    _handle->lock = xSemaphoreCreateMutex();
    ESP_GOTO_ON_FALSE(_handle->lock != NULL, ESP_ERR_NO_MEM, err, TAG, "Failed to create semaphore");
    
    /* publish global handle so instance wrappers can access it */
    _handle->config = *config;
    *ret_handle = _handle;

    return ret;

err:
    if (_handle->lock) vSemaphoreDelete(_handle->lock);
    if (_handle) free(_handle);
    return ret;
}

esp_err_t bsp_qmi8658_deinit(bsp_qmi8658_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");

    if (handle->lock) {
        vSemaphoreDelete(handle->lock);
        handle->lock = NULL;
    }

    free(handle);
    handle = NULL;

    return ESP_OK;
}