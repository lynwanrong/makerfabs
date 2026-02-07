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
#include "dw3000.h"

#if defined(TEST_READING_DEV_ID)

/* Example application name and version to print to the console. */
static const char* LOG_TAG = "READ_DEV_ID";

/**
 * Application entry point.
 */
int read_dev_id(void)
{
    reset_DWIC();

    Sleep(2);

    if (dwt_probe((struct dwt_probe_s *)&dw3000_probe_interf) != DWT_SUCCESS) {
        LOG_ERR("DWT Probe Failed!");
        return -1;
    }
    
    if (dwt_initialise(DWT_READ_OTP_PID | DWT_READ_OTP_LID | DWT_READ_OTP_BAT
						 | DWT_READ_OTP_TMP) != DWT_SUCCESS) {
        LOG_ERR("DWT Initialise Failed!");
        return -1;
    }

    uint32_t dev_id = dwt_readdevid();
    LOG_INF("DEV ID: %" PRIX32, dev_id);

    return 0;
}

#endif
/*****************************************************************************************************************************************************
 * NOTES:
 ****************************************************************************************************************************************************/
