/*! ----------------------------------------------------------------------------
 *  @file    read_dev_id.c
 *  @brief   This example just read DW IC's device ID. It can be used to verify
 *           the SPI comms are working correctly.
 *
 *
 * @author Decawave
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 * 
 */

#include "deca_probe_interface.h"
#include <deca_device_api.h>
#include <example_selection.h>
#include <port.h>
#include <stdio.h>

#if defined(TEST_READING_DEV_ID)

/* Example application name and version to print to the console. */
static const char* LOG_TAG = "READ_DEV_ID";

/**
 * Application entry point.
 */
int read_dev_id(void)
{
    int ret;

    reset_DWIC();

    if (dwt_probe((struct dwt_probe_s *)&dw3000_probe_interf) != DWT_SUCCESS) {
        LOG_INF("DWT Probe Failed!");
        return -1;
    }

    if (dwt_initialise(DWT_READ_OTP_PID | DWT_READ_OTP_LID | DWT_READ_OTP_BAT
						 | DWT_READ_OTP_TMP) != DWT_SUCCESS) {
        LOG_INF("DWT Initialise Failed!");
        return -1;
    }

    int cnt = 1000;
    while (!dwt_checkidlerc() && cnt-- > 0) {
        Sleep(1);
    }
    if (cnt <= 0) {
        LOG_ERR("Init did not leave IDLE state");
    }

    uint32_t dev_id = dwt_readdevid();
    ret = dwt_check_dev_id();
    if (ret != DWT_SUCCESS) {
        LOG_ERR("UNKNOWN DEV ID: %" PRIX32, dev_id);
        return -1;
    } else {
        LOG_INF("DEV ID: %" PRIX32, dev_id);
    }

    port_set_dw_ic_spi_fastrate();

    return 0;
}

#endif
/*****************************************************************************************************************************************************
 * NOTES:
 ****************************************************************************************************************************************************/
