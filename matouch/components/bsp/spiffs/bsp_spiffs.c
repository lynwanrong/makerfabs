#include <esp_spiffs.h>
#include <stdbool.h>

#include "bsp_spiffs.h"

static const char *TAG = "bsp_spiffs";


/* Definition of the handle struct (Opaque to user) */
struct bsp_spiffs_t {
    bsp_spiffs_config_t config; /*!< Configuration */
    SemaphoreHandle_t lock;     /*!< Mutex for thread safety */
};
esp_err_t bsp_spiffs_init(const bsp_spiffs_config_t *config, bsp_spiffs_handle_t *ret_handle)
{
    ESP_RETURN_ON_FALSE(config != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid config");

    esp_err_t ret = ESP_OK;
    bsp_spiffs_handle_t handle = calloc(1, sizeof(struct bsp_spiffs_t));
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_NO_MEM, TAG, "No memory for handle");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = config->mount_point,
        .partition_label = config->partition_label,
        .max_files = 5,
#ifdef CONFIG_BSP_SPIFFS_FORMAT_ON_MOUNT_FAIL
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif
    };

    ret = esp_vfs_spiffs_register(&conf);

    ESP_GOTO_ON_ERROR(ret, err, TAG, "Failed to mount SPIFFS");

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    handle->config = *config;
    *ret_handle = handle;
    return ret;

err:
    if (handle) free(handle);

    return ret;
}


esp_err_t bsp_spiffs_deinit(bsp_spiffs_handle_t handle)
{
    return esp_vfs_spiffs_unregister(handle->config.partition_label);
}