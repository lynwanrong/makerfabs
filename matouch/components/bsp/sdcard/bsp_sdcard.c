#include "bsp_sdcard.h"
#include <sd_protocol_types.h>

#include <esp_vfs_fat.h>
#include <sdmmc_cmd.h>
#include <dirent.h>
#include <sys/stat.h>


static const char *TAG = "bsp_sdcard";

/* Definition of the handle struct (Opaque to user) */
struct bsp_sdcard_t {
    sdmmc_card_t *card;         /*!< Card instance */
    bsp_sdcard_config_t config; /*!< Configuration */
    SemaphoreHandle_t lock;     /*!< Mutex for thread safety */
};

esp_err_t bsp_sdcard_print_files_path(bsp_sdcard_handle_t handle, const char *relative_path)
{
    DIR *dir = NULL;
    struct dirent *entry;
    struct stat st;

    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "handle is NULL");
    ESP_RETURN_ON_FALSE(relative_path != NULL, ESP_ERR_INVALID_ARG, TAG, "relative_path is NULL");

    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s/%s", handle->config.mount_point, relative_path);

    dir = opendir(full_path);
    ESP_RETURN_ON_FALSE(dir != NULL, ESP_FAIL, TAG, "opendir failed, path=%s", full_path);

    ESP_LOGI(TAG, "Listing files in %s:", full_path);

    while ((entry = readdir(dir)) != NULL) {
        char _path[512];
        int _len = snprintf(_path, sizeof(_path), "%s/%s", full_path, entry->d_name);
        if (_len < 0 || _len >= sizeof(_path)) {
            ESP_LOGW(TAG, "Path too long: %s/%s", full_path, entry->d_name);
            continue;
        }
        if (stat(_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                ESP_LOGI(TAG, "  [DIR]  %s", entry->d_name);
            } else {
                ESP_LOGI(TAG, "  [FILE] %s  (%lld bytes)", entry->d_name, (long long)st.st_size);
            }
        } else {
            ESP_LOGW(TAG, "  [?] %s (stat failed)", entry->d_name);
        }
    }

    closedir(dir);
    return ESP_OK;
}

esp_err_t bsp_sdcard_init(const bsp_sdcard_config_t *config, bsp_sdcard_handle_t *ret_handle)
{
    ESP_RETURN_ON_FALSE(config != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid config");
    ESP_RETURN_ON_FALSE(ret_handle != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid handle");

    esp_err_t ret = ESP_OK;
    bsp_sdcard_handle_t _handle = calloc(1, sizeof(struct bsp_sdcard_t));
    ESP_RETURN_ON_FALSE(_handle != NULL, ESP_ERR_NO_MEM, TAG, "No memory for handle");

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_BSP_SDCARD_FORMAT_ON_MOUNT_FAIL
        .format_if_mount_failed = true, /* If mount failed, format the filesystem */
#else
        .format_if_mount_failed = false,
#endif
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
        .disk_status_check_enable = true
    };

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = config->sd_cs;
    slot_config.host_id = config->host;

    ESP_LOGI(TAG, "Mounting filesystem");
    sdmmc_card_t *_card = NULL;
    ret = esp_vfs_fat_sdspi_mount(config->mount_point, &host, &slot_config, &mount_config, &_card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                     "If you want the card to be formatted, set the CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return ret;
    }

    ESP_GOTO_ON_ERROR(ret, err, TAG, "Failed to mount filesystem");

    _handle->card = _card;
    _handle->config = *config;
    *ret_handle = _handle;

    ESP_LOGI(TAG, "Filesystem mounted");

    sdmmc_card_print_info(stdout, _card);

    // _print_sd_files();

    return ret;

err:
    if (_handle) free(_handle);

    return ret;
}

esp_err_t bsp_sdcard_deinit()
{
    esp_err_t ret = esp_vfs_fat_sdcard_unmount(NULL, NULL);
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to unmount SD card");

    ESP_LOGI(TAG, "SD card unmounted");
    return ESP_OK;
}