#pragma once
#include "pcf85063a.h"
#include "bsp.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Handle for the BSP PCF85063A instance
 */
typedef struct bsp_pcf85063a_t *bsp_pcf85063a_handle_t;

/**
 * @brief Configuration structure for BSP PCF85063A
 */
typedef struct {
    i2c_master_bus_handle_t bus_handle; /*!< Handle of the I2C master bus */
    uint8_t i2c_addr;                   /*!< I2C address of the sensor (usually 0x51) */
} bsp_pcf85063a_config_t;

/**
 * @brief Initialize the PCF85063A BSP wrapper
 *
 * This function allocates memory, initializes the low-level driver,
 * and creates a mutex for thread safety.
 *
 * @param[in]  config      Configuration structure
 * @param[out] ret_handle  Pointer to return the created handle
 * @return
 * - ESP_OK: Success
 * - ESP_ERR_NO_MEM: Memory allocation failed
 * - ESP_ERR_INVALID_ARG: Invalid arguments
 * - Other errors from low-level driver
 */
esp_err_t bsp_pcf85063a_init(const bsp_pcf85063a_config_t *config, bsp_pcf85063a_handle_t *ret_handle);

/**
 * @brief De-initialize the PCF85063A BSP wrapper
 *
 * Frees memory and deletes the mutex.
 *
 * @param[in] handle Handle to the BSP instance
 * @return
 * - ESP_OK: Success
 * - ESP_ERR_INVALID_ARG: Invalid handle
 */
esp_err_t bsp_pcf85063a_deinit(bsp_pcf85063a_handle_t handle);

/**
 * @brief Get the current time in standard struct tm format
 *
 * This function is thread-safe.
 *
 * @param[in]  handle  Handle to the BSP instance
 * @param[out] time    Pointer to standard C struct tm to fill
 * @return
 * - ESP_OK: Success
 * - ESP_ERR_TIMEOUT: Failed to acquire lock
 * - Other errors from low-level driver
 */
esp_err_t bsp_pcf85063a_get_tm(bsp_pcf85063a_handle_t handle, struct tm *time);

/**
 * @brief Set the current time using standard struct tm format
 *
 * This function is thread-safe.
 *
 * @param[in] handle  Handle to the BSP instance
 * @param[in] time    Pointer to standard C struct tm containing new time
 * @return
 * - ESP_OK: Success
 * - ESP_ERR_TIMEOUT: Failed to acquire lock
 * - Other errors from low-level driver
 */
esp_err_t bsp_pcf85063a_set_tm(bsp_pcf85063a_handle_t handle, const struct tm *time);

#ifdef __cplusplus
}
#endif