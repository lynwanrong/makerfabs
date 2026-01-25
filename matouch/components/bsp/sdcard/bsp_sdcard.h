#pragma once

#include "bsp_board.h"
#include "hal/spi_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bsp_sdcard_t *bsp_sdcard_handle_t;

typedef struct {
    const char *mount_point;   /*!< Mount point for the filesystem */
    gpio_num_t sd_cs;          /*!< GPIO pin for SD card chip select */
    spi_host_device_t host;    /*!< SPI host to use */
}bsp_sdcard_config_t;

/**
 * @brief Initialize the SD card and mount the FAT filesystem
 *
 * This function allocates memory, initializes the low-level driver,
 * and creates a mutex for thread safety.
 *
 * @param[in]  mount_point      Mount point for the filesystem
 * @param[in]  sd_cs            GPIO pin for SD card chip select
 * @return
 * - ESP_OK: Success
 */
esp_err_t bsp_sdcard_init(const bsp_sdcard_config_t *config, bsp_sdcard_handle_t *ret_handle);

/**
 * @brief De-initialize the SD card and unmount the FAT filesystem
 *
 * Frees memory and deletes the mutex.
 *
 * @param[in] handle Handle to the BSP instance
 * @return
 * - ESP_OK: Success
 */
esp_err_t bsp_sdcard_deinit(void);

/**
 * @brief print all files in a directory
 *
 * @param[in]  handle  Handle to the BSP instance
 * @param[in]  relative_path  Relative path to list files in
 * @return
 * - ESP_OK: Success
 */
esp_err_t bsp_sdcard_print_files_path(bsp_sdcard_handle_t handle, const char *relative_path);

#ifdef __cplusplus
}   
#endif