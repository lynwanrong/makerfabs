#pragma once

#include "qmi8658.h"
#include "bsp_board.h"

#define QMI8658_LOCK_TIMEOUT_MS 2000

typedef struct bsp_qmi8658_t *bsp_qmi8658_handle_t;

typedef struct {
    i2c_master_bus_handle_t bus_handle; /*!< I2C bus handle */
    uint8_t i2c_addr;                   /*!< I2C address of the QMI8658 sensor */
} bsp_qmi8658_config_t;

#ifdef __cpluscplus
extern "C" {
#endif

esp_err_t bsp_qmi8658_init(const bsp_qmi8658_config_t *config, bsp_qmi8658_handle_t *ret_handle);

esp_err_t bsp_qmi8658_deinit(bsp_qmi8658_handle_t handle);

#ifdef __cpluscplus
}
#endif