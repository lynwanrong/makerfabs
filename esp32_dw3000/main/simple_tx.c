#include "dw3000_hw.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <esp_log.h>
#include "dw3000_hw.h"
#include "dw3000.h"
#include "dwhw.h"
#include "dwmac.h"
#include "dwphy.h"
#include "dwproto.h"
#include "ranging.h"
#include "dw3000_deca_regs.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define TEST_SIMPLE_TX

#if defined(TEST_SIMPLE_TX)

/* Example application name */
#define APP_NAME "SIMPLE TX v1.0"

extern dwt_txconfig_t txconfig_options;
static dwt_config_t config = {
    5,               /* 信道号. (Channel number.) */
    DWT_PLEN_128,    /* Preamble length. Used in TX only. */
    DWT_PAC8,        /* Preamble acquisition chunk size. Used in RX only. */
    9,               /* Tx前导码. (TX preamble code. Used in TX only.) */
    9,               /* Rx前导码. (RX preamble code. Used in RX only.) */
    1,               /* 帧分隔符模式. (0 to use standard 8 symbol SFD, 1 to use non-standard 8 symbol, 2 for non-standard 16 symbol SFD and 3 for 4z 8 symbol SDF type) */
    DWT_BR_6M8,      /* 数据速率. (Data rate.) */
    DWT_PHRMODE_STD, /* 物理层头模式. (PHY header mode.) */
    DWT_PHRRATE_STD, /* 物理层头速率. (PHY header rate.) */
    (129 + 8 - 8),   /* 帧分隔符超时. SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only. */
    DWT_STS_MODE_OFF,/* STS模式*/
    DWT_STS_LEN_64,  /* STS长度. (STS length, see allowed values in Enum dwt_sts_lengths_e) */
    DWT_PDOA_M0      /* PDOA mode off */
};


/* The frame sent in this example is an 802.15.4e standard blink. It is a 12-byte frame composed of the following fields:
 *     - byte 0: frame type (0xC5 for a blink).
 *     - byte 1: sequence number, incremented for each new frame.
 *     - byte 2 -> 9: device ID, see NOTE 1 below.
 */
static uint8_t tx_msg[] = { 0xC5, 0, 'D', 'E', 'C', 'A', 'W', 'A', 'V', 'E' };
/* Index to access to sequence number of the blink frame in the tx_msg array. */
#define BLINK_FRAME_SN_IDX 1

#define FRAME_LENGTH (sizeof(tx_msg) + FCS_LEN) // The real length that is going to be transmitted

/* Inter-frame delay period, in milliseconds. */
#define TX_DELAY_MS 500

/* Values for the PG_DELAY and TX_POWER registers reflect the bandwidth and power of the spectrum at the current
 * temperature. These values can be calibrated prior to taking reference measurements. See NOTE 2 below. */

void simple_tx()
{
     uint32_t dev_id;
    /* Configure SPI rate, DW3000 supports up to 38 MHz */

    dw3000_hw_init();
    dw3000_hw_reset();

    dw3000_spi_speed_fast();

    if (dwt_probe((struct dwt_probe_s *)&dw3000_probe_interf) != DWT_SUCCESS) {
        ESP_LOGI("main", "DWT Probe Failed!");
        return ;
    }

    // 4. 初始化驱动 (这一步会分配内部内存，并安全地读取 Dev ID)
    if (dwt_initialise(DWT_LOADLDO) != DWT_SUCCESS) {
        ESP_LOGI("main", "DWT Initialise Failed!");
        return ;
    }

    dev_id = dwt_readdevid();
    if (dev_id != (uint32_t)DWT_DW3000_DEV_ID) {
        ESP_LOGI("main", "error dev_id");
        return ;
    }
    while (!dwt_checkidlerc()) {};


    dwt_setleds(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK);

    if (dwt_configure(&config)) {
        ESP_LOGI("main", "DWT Configure Failed!");
        return ;
    }

    /* Configure the TX spectrum parameters (power PG delay and PG Count) */
    dwt_configuretxrf(&txconfig_options);
    
    while (1) {
        dwt_writetxdata(FRAME_LENGTH - FCS_LEN, tx_msg, 0); /* Zero offset in TX buffer. */
        dwt_writetxfctrl(FRAME_LENGTH, 0, 0); /* Zero offset in TX buffer, no ranging. */
        dwt_starttx(DWT_START_TX_IMMEDIATE);
        while (!(dwt_read_reg(SYS_STATUS_ID) & SYS_STATUS_TXFRS_BIT_MASK));
        dwt_write_reg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);
        vTaskDelay(TX_DELAY_MS);
        tx_msg[BLINK_FRAME_SN_IDX]++;
    }
}



#endif  // TEST_SIMPLE_TX