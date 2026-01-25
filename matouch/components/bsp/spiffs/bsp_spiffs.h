#pragma once

#include "bsp_board.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bsp_spiffs_t *bsp_spiffs_handle_t;

typedef struct {
    const char *mount_point;   /*!< Mount point for the filesystem */
    const char *partition_label;   /*!< SPIFFS partition label */
}bsp_spiffs_config_t;

/**
 * @brief Initialize the SPIFFS filesystem
 *
 * @param[in]  mount_point      Mount point for the filesystem
 * @return
 * - ESP_OK: Success
 */
esp_err_t bsp_spiffs_init(const bsp_spiffs_config_t *config, bsp_spiffs_handle_t *ret_handle);


/**
 * @brief Deinitialize the SPIFFS filesystem
 *
 * @param[in]  handle       SPIFFS handle
 * @return
 * - ESP_OK: Success
 */
esp_err_t bsp_spiffs_deinit(bsp_spiffs_handle_t handle);

#ifdef __cplusplus
}   
#endif