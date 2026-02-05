#include "dw3000_hw.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

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
#include "dwtime.h"

#define TEST_SIMPLE_TX
#define FRAME_LEN_MAX 127

#if defined(TEST_SIMPLE_TX)

static const char *TAG = "simple_rx";

/* Example application name */
#define APP_NAME "SIMPLE RX v1.0"

/* Buffer to store received frame. See NOTE 1 below. */
static uint8_t rx_buffer[FRAME_LEN_MAX];


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


void simple_rx()
{
    uint32_t status_reg;
    uint16_t frame_len;
    /* Configure SPI rate, DW3000 supports up to 38 MHz */

    dw3000_hw_init();
    dw3000_hw_reset();

    dw3000_spi_speed_fast();

    if (dwt_probe((struct dwt_probe_s *)&dw3000_probe_interf) != DWT_SUCCESS) {
        ESP_LOGI(TAG, "DWT Probe Failed!");
        return ;
    }

    // 4. 初始化驱动 (这一步会分配内部内存，并安全地读取 Dev ID)
    if (dwt_initialise(DWT_LOADLDO) != DWT_SUCCESS) {
        ESP_LOGI(TAG, "DWT Initialise Failed!");
        return ;
    }

    while (!dwt_checkidlerc()) {};

    if (dwt_initialise(DWT_LOADLDO) == DWT_ERROR) {
        ESP_LOGI(TAG, "DWT Initialise Failed!");
        return ;
    }

    dwt_setleds(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK);

    if (dwt_configure(&config)) {
        ESP_LOGI(TAG, "DWT Configure Failed!");
        return ;
    }

    while (1) {
        memset(rx_buffer,0,sizeof(rx_buffer));
        dwt_rxenable(DWT_START_RX_IMMEDIATE);
        while (!((status_reg = dwt_read_reg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_ERR ))) {
            vTaskDelay(1);
        };
        
        if (status_reg & SYS_STATUS_RXFCG_BIT_MASK) {
            frame_len = dwt_read_reg(RX_FINFO_ID) & RX_FINFO_RXFLEN_BIT_MASK;
            if (frame_len <= FRAME_LEN_MAX) {
                dwt_readrxdata(rx_buffer, frame_len - FCS_LEN, 0);
                // 使用espidf自带的hex打印函数
                ESP_LOG_BUFFER_HEX(TAG, rx_buffer, frame_len - FCS_LEN);

                uint64_t rx_time = dw_get_rx_timestamp();

            }
            dwt_write_reg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK);
        } else {
            dwt_write_reg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_ERR);
        }
    }
}



#endif  // TEST_SIMPLE_TX